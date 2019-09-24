#include "PacketFrag.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

using lf = LiteFormat;

const uint16 FRAGMENTED_PACKET = 1;
const uint16 NON_FRAGMENTED_PACKET = 2;

double TestStats::test_recent_recv_speed = 0;
double TestStats::test_recent_send_speed_at_receiver_side = 0;

//
// FragHeader
//

void FragHeader::Write(FragmentedBuffer& output) {
  const uint16 packet_length_class =
      (packet_length > 65535 ? 3 : (packet_length > 255 ? 1 : 0));
  const uint16 pakcet_id_class =
      (packet_id > 65535 ? 3 : (packet_id > 255 ? 1 : 0));
  const uint16 frag_id_class = (frag_id > 65535 ? 3 : (frag_id > 255 ? 1 : 0));

  uint8* ptr = &tx_buffer[0];

  WriteUInt16ToBufferAndAdvance(ptr, splitter | (packet_length_class << 12) |
                                         (pakcet_id_class << 10) |
                                         (frag_id_class << 8));

  WriteOptimalUInt32AndAdvance(ptr, packet_length, packet_length_class);
  WriteOptimalUInt32AndAdvance(ptr, packet_id, pakcet_id_class);
  WriteOptimalUInt32AndAdvance(ptr, frag_id, frag_id_class);

  const int32 written_length = int32(ptr - tx_buffer);
  fun_check(written_length <= sizeof(tx_buffer));

  output.Add(tx_buffer, written_length);
}

bool FragHeader::Read(IMessageIn& input) {
  FUN_DO_CHECKED(lf::Read(input, splitter));

  // 호출부 쪽에서 체크를 할 것이므로, 여기서 체크하지 않는다.  호출부쪽에서
  // 구체적인 에러 내용을 확인하기 위해서..
  // const uint8 frag_type = uint8(splitter >> 14) & 3;
  // if (!(frag_type == FRAGMENTED_PACKET || frag_type ==
  // NON_FRAGMENTED_PACKET)) {
  //  LOG(LogNetEngine,Error,"unknown frag_type: %u", frag_type);
  //  return false;
  //}

  const uint8 packet_length_class = uint8(splitter >> 12) & 3;
  FUN_DO_CHECKED(ReadOptimalUInt32(input, packet_length, packet_length_class));

  const uint8 pakcet_id_class = uint8(splitter >> 10) & 3;
  FUN_DO_CHECKED(ReadOptimalUInt32(input, packet_id, pakcet_id_class));

  const uint8 frag_id_class = uint8(splitter >> 8) & 3;
  FUN_DO_CHECKED(ReadOptimalUInt32(input, frag_id, frag_id_class));

  return true;
}

bool FragHeader::ReadOptimalUInt32(IMessageIn& input, uint32& out_value,
                                   const uint8 length_class) {
  switch (length_class) {
    case 0: {
      uint8 tmp;
      FUN_DO_CHECKED(lf::Read(input, tmp));
      out_value = (uint32)tmp;
      break;
    }
    case 1: {
      uint16 tmp;
      FUN_DO_CHECKED(lf::Read(input, tmp));
      out_value = (uint32)tmp;
      break;
    }
    case 3:
      FUN_DO_CHECKED(lf::Read(input, out_value));
      break;
    default:
      fun_unexpected();
      return false;
  }
  return true;
}

//
// UdpPacketFragger
//

/*
송신큐가 채워져 있는 아무 항목을 얻는다.

최대한 균등하게 기회를 줄 수 있어야 한다.

Pop을 한 후에는 Pop의 대상이 되는 PacketQueue 항목을 맨 뒤로 옮긴다.




[FIXME]

현재, 이 함수에서 메모리 침범 오류가 있는듯 싶다.
참조 관리의 실패인지 아니면, 다른 기반 코어 라이브러리에서 로컬변수로 선언된곳의
메모리 영역을 침범하고 있는건지 확인이 필요하다.

이 함수가 호출된 이후에 output.OwningPackets가 깨지는 경우가 자주 있음.
이 부분을 집중적으로 확인해보는게 여러모로 좋을듯 싶은데...
*/
bool UdpPacketFragger::PopAnySendQueueFilledOneWithCoalesce(
    UdpPacketFraggerOutput& output, double absolute_time) {
  auto head = outstandings_.Front();
  if (head == nullptr) {
    return false;
  }

  // 전송할 대상을 뽑아냈는데, 내부가 비어있다?
  fun_check(head->GetTotalCount() > 0);
  fun_check(head->remote_addr_.IsUnicast());

  // head->PopFragmentOrFullPacket(delegate_->GetAbsoluteTime(), output);
  head->PopFragmentOrFullPacket(absolute_time, output);
  fun_check(output.owning_packets.IsEmpty() == false);
  fun_check(output.send_frag_frag.Buffer.IsEmpty() == false);

  // 만약 queue가 완전히 비게 됐으면 아예 queue 객체 자체를 제거한다.
  if (head->IsOutstanding(absolute_time) == false &&
      head->GetListOwner() != nullptr) {
    head->UnlinkSelf();
  } else {
    // pop을 한 후에는 pop의 대상이 되는 PacketQueue 항목을 맨 뒤로 옮긴다.
    head->UnlinkSelf();
    outstandings_.Append(head);
  }

  head->last_accessed_time_ = absolute_time;

  AssertConsistency();
  return true;
}

// 이건 클라에서나 호출함. 서버는 send brake를 안쓰니까.
void UdpPacketFragger::ShortTick(double absolute_time) {
  // PacketId가 리셋되어도 충분히 문제없는 시간이어야 하니까
  fun_check(NetConfig::remove_too_old_udp_send_packet_queue_timeout_sec >
            NetConfig::assemble_fragged_packet_timeout_sec * 10);

  const int32 old_count = address_to_queue_map_.map.Count();
  // for (auto pair : address_to_queue_map_.map) {
  //  auto queue = pair.value;
  for (auto it = address_to_queue_map_.map.CreateIterator(); it; ++it) {
    auto queue = it->value;

    // 큐에 든게 있지만 앞서 큐에 넣던 순간 과량 송신 상태라 existants에
    // 미등록됐다면 여기서 필요시 재등록
    ConditionalAddToOutstandings(queue, absolute_time);

    // ranged-for를 수행하고 있으므로, 내부에서 collection이 변경되면 안됨.
    fun_check(old_count == address_to_queue_map_.map.Count());
  }
}

void UdpPacketFragger::LongTick(double absolute_time) {
  // PacketId가 리셋되어도 충분히 문제없는 시간이어야 하니까
  fun_check(NetConfig::remove_too_old_udp_send_packet_queue_timeout_sec >
            NetConfig::assemble_fragged_packet_timeout_sec * 10);

  for (auto it = address_to_queue_map_.map.CreateIterator(); it; ++it) {
    auto queue = it->value;

    queue->send_brake_.LongTick(absolute_time);
    queue->send_speed_.LongTick(absolute_time);
    queue->recent_receive_speed_at_receiver_side_.LongTick(absolute_time);

    // 과량 송신중이면 수신측 속도 요청을 한다.
    // 이 함수 자체가 매 일정시간 호출되므로 무조건 즉시.
    if (queue->send_speed_.GetRecentSpeed() >
        delegate_->GetOverSendSuspectingThresholdInByte()) {
      delegate_->RequestReceiveSpeedAtReceiverSide_NoRelay(it->key);
    }

    // 송신속도, 수신속도를 고려해서 최대송신가능속도를 결정
    queue->allowed_max_send_speed_.CalcNewValue(
        queue->send_speed_.GetRecentSpeed(),
        queue->recent_receive_speed_at_receiver_side_.GetValue());

    // 사용 안된지 너무 오래됐으면 추방 또는 iter to next
    // (주의: 여기서는 유령을 없애기 위한 목적이지,
    // 안쓰이는 것을 없애기 위한 목적이 아니다. overlapped I/O가 아직
    // 진행중인 것일 수 있으므로 충분한 기간 값을 넣어야만 의미가 있다!)
    if ((absolute_time - queue->last_accessed_time_) >
        NetConfig::remove_too_old_udp_send_packet_queue_timeout_sec) {
      queue->UnlinkSelf();
      delete queue;
      it.RemoveCurrent();
    }
  }
}

/*
항목을 제거하되, current selection과 같은 것이면 current selection도 이동한다.
*/
void UdpPacketFragger::Remove(const InetAddress& key) {
  // TODO optimize: 두번 검색을 해야하는 문제가 있음...
  if (auto queue = address_to_queue_map_.map.FindRef(key)) {
    queue->UnlinkSelf();
    delete queue;
    address_to_queue_map_.map.Remove(key);
  }
}

void UdpPacketFragger::Clear() {
  // for (auto pair : address_to_queue_map_.map) {
  //  auto queue = pair.value;
  for (auto it = address_to_queue_map_.map.CreateIterator(); it; ++it) {
    auto queue = it->value;
    queue->UnlinkSelf();
    delete queue;
  }

  address_to_queue_map_.map.Clear();
}

void UdpPacketFragger::InitHashTableForClient() {
  // TODO 클라에서는 hash table 기본값 17은 오히려 성능에 나쁘다.  따라서 이렇게
  // 조절해둔다. address_to_queue_map_.InitHashTable(3);
}

int32 UdpPacketFragger::GetTotalPacketCountOfAddr(const InetAddress& addr) {
  // cache for frequent case.
  if (address_to_queue_map_.map.IsEmpty()) {
    return 0;
  }

  auto queue = address_to_queue_map_.map.FindRef(addr);
  return queue ? queue->GetTotalCount() : 0;
}

/*
송신할 패킷을 추가한다.

final_dest_id: 패킷의 최종 수신 대상. relay or route 대상인 경우 실제 수신
대상과 다르므로 None을 넣어야 한다. None을 넣는 경우 UniqueID 비교가 무시된다.

filter_tag: UDP는 제3자가 송신자 주소를 속여서 보낼 수 있다.
(버그있는 2중 NAT 장치에서 속일 의도가 없어도 같은 현상이 제보된 바 있음.)
따라서 filter_tag 값으로 비정상 송신자를 걸러내는 데 쓴다.
*/
void UdpPacketFragger::AddNewPacket(HostId final_dest_id,
                                    FilterTag::Type filter_tag,
                                    const InetAddress& sendto,
                                    const SendFragRefs& data_to_send,
                                    double added_time,
                                    const UdpSendOption& send_opt) {
  // unicast만 허용하도록 한다. malware 오인당하면 즐.
  fun_check(sendto.IsUnicast());

  // 0바이트짜리 패킷은 아예 송신을 금지시키자. socket 내부에서 무슨 짓을 할지
  // 모르니.
  if (data_to_send.IsEmpty()) {
    return;
  }

  // 대응하는 송신큐를 찾되 없으면 새로 등록한다.
  auto queue = address_to_queue_map_.map.FindRef(sendto);
  if (queue == nullptr) {
    queue = new PacketQueue();
    queue->owner_ = this;
    queue->remote_addr_ = sendto;
    queue->filter_tag = filter_tag;
    queue->last_accessed_time_ = added_time;
    address_to_queue_map_.map.Add(sendto, queue);
  }

  //의미 없는 체크..
  // fun_check(queue->remote_addr_ == sendto);

  // Sanity checking
  if ((int32)send_opt.priority < 0 ||
      (int32)send_opt.priority >= (int32)MessagePriority::Last) {
    throw InvalidArgumentException();
  }

  // Copy(const -> non-const)
  UdpSendOption send_opt2(send_opt);

  // 패킷 우선순위 기능을 강제로 끄도록 만들어져 있는지 체크한다.
  if (!NetConfig::message_prioritization_enabled) {
    send_opt2.priority = (MessagePriority)0;
  }

  auto& sub_queue = queue->priorities_[(int32)send_opt2.priority];

  // UniqueId가 지정된 경우 같은 ID가 지정된 것이 있으면 그것을 제거하고 해당
  // 위치에 넣도록 한다. 성능 저하를 줄이기 위해 0이 지정된 경우 무조건
  // 무시한다.
  if (send_opt2.unique_id != 0 && final_dest_id != HostId_None) {
    // fraggable, nofraggable 중 하나만 뒤지고 하나에만 오버라이트해야.
    // 안그러면 frag 해야 여부의 반대의 큐에 들어갈 수 있으므로..
    UdpPacketContext* sub_queue_element;
    if (send_opt2.conditional_fragging) {
      sub_queue_element = sub_queue.fraggable_packets.Front();
    } else {
      sub_queue_element = sub_queue.no_fraggable_packets.Front();
    }

    while (sub_queue_element) {
      if (sub_queue_element->unique_id !=
          0) {  // UniqueId가 지정되었을 경우, UniqueId가 겹치면 하나만 보냄.
        if (send_opt2.unique_id == sub_queue_element->unique_id &&
            final_dest_id == sub_queue_element->dest_id) {
          // 중복되는게 발견됐다. 새 패킷을 추가하지 말고 기존 패킷을 교체한다.
          // sub_queue_element->packet = data_to_send.ToBytes(); // copy
          data_to_send.CopyTo(sub_queue_element->packet);
          goto L1;
        }
      }
      sub_queue_element = sub_queue_element->GetNextNode();
    }
  }

  {
    // 위 UniqueId에 의한 교체 과정을 하지 않은 경우 새 패킷을 추가한다.
    auto send_info = NewOrRecyclePacket_INTERNAL();
    if (send_opt2.conditional_fragging) {
      sub_queue.fraggable_packets.Append(send_info);
    } else {
      sub_queue.no_fraggable_packets.Append(send_info);
    }

    send_info->unique_id = send_opt2.unique_id;
    send_info->ttl = send_opt2.ttl;
    send_info->dest_id = final_dest_id;

    // Fill packet data.
    data_to_send.CopyTo(send_info->packet);
  }

L1:
  fun_check(queue->GetTotalCount() > 0);

  queue->last_accessed_time_ = added_time;

  // 아직 outstanding list에 없으면 넣도록 한다.
  ConditionalAddToOutstandings(queue, added_time);

  AssertConsistency();
}

bool UdpPacketFragger::HasNothingToSend() const {
  return outstandings_.IsEmpty();
}

void UdpPacketFragger::AssertConsistency() const {
#ifdef _DEBUG
  for (auto queue = outstandings_.Front(); queue;
       queue = queue->ListNode<PacketQueue>::GetNextNode()) {
    fun_check(queue->GetTotalCount() > 0);
  }
#endif
}

UdpPacketFragger::UdpPacketFragger(IUdpPacketFraggerDelegate* delegate)
    : address_to_queue_map_(), delegate_(delegate) {
  send_brake_enabled_ = true;

  // TODO 이 map 클래스는 신축폭이 매우 크다. 따라서 rehash 역치를 최대한 크게
  // 잡아야 한다.  줄어드는 rehash는 절대 하지 말자.
  // address_to_queue_map_.SetOptimalLoad(0.1f, 0, 2.0f);
}

void UdpPacketFragger::ConditionalAddToOutstandings(PacketQueue* queue,
                                                    double absolute_time) {
  if (queue->GetListOwner() == nullptr && queue->IsOutstanding(absolute_time)) {
    outstandings_.Append(queue);
  }
}

void UdpPacketFragger::SetReceiveSpeedAtReceiverSide(const InetAddress& dst,
                                                     double speed) {
  if (auto queue = address_to_queue_map_.map.FindRef(dst)) {
    queue->recent_receive_speed_at_receiver_side_.SetValue(speed);
  }
}

int32 UdpPacketFragger::FromTotalPacketInByteByAddr(
    const InetAddress& address) {
  auto queue = address_to_queue_map_.map.FindRef(address);
  return queue ? queue->GetTotalLengthInBytes() : 0;
}

UdpPacketFragger::~UdpPacketFragger() {
  // 이것부터 먼저 파괴해야 안전하게 packet free list를 제거할 수 있으므로..
  address_to_queue_map_.ClearAndFree();
}

UdpPacketContext* UdpPacketFragger::NewOrRecyclePacket_INTERNAL() {
  return packet_pool_.NewOrRecycle();
}

void UdpPacketFragger::ReturnPacketToPool_INTERNAL(UdpPacketContext* packet) {
  fun_check_ptr(packet);
  packet_pool_.ReturnToPool(packet);
}

//
// UdpPacketFragger::PacketQueue
//

/*
MTU 크기 이하까지 Packet을 뭉친 후 MTU 크기 직전까지 자른 패킷을 하나
리턴하거나, 1개의 full Packet을 얻는다.

FragHeader의 Splitter value에 따라 full Packet인지 아닌지 확인 가능.

send-completion 발생시에 호출되며, 전송해야할 대상을 채우는 역활을 함.
*/
void UdpPacketFragger::PacketQueue::PopFragmentOrFullPacket(
    double absolute_time, UdpPacketFraggerOutput& output) {
  // frag안할 패킷이 쌓여있으면 그냥 그것을 우선 주도록 한다.
  // frag 안할 패킷이 없을 때에만 frag될 수 있는 패킷들을 처리한다.
  for (int32 priority_index = 0; priority_index < (int32)MessagePriority::Last;
       ++priority_index) {
    auto& per_priority_queue = priorities_[priority_index];

    if (auto head = per_priority_queue.no_fraggable_packets.Front()) {
      // 일단 텅 비워버려야.
      // 이미 UDP send completion이 발생했을 터이고 안전하게
      // output.OwningPackets도 비워도 됨.
      output.ResetForReuse();

      output.sendto = remote_addr_;
      output.source = owner;
      output.ttl = head->ttl;

      // FragHeader를 싸서 보낸다. 그리고 상황 종료.
      auto& frag_header = output.frag_header;

      frag_header.splitter = (uint16)(NON_FRAGMENTED_PACKET << 14);
      frag_header.packet_length = head->packet.Count();
      frag_header.packet_id = 0;
      frag_header.frag_id = 0;

      // UDP 패킷에 최초 4byte가 동일한 값일 경우 일부 공유기에서 받은 패킷을
      // 다시 재전송하는 경우가 있으므로 앞의 4byte(splitter & filter_tag) 가
      // 동일한 값이 가지 않도록 값이 계속 변하는 PacketID로 XOR 연산을 하고
      // defrag 시 다시 PacketId로 XOR 연산을 해서 FilterTag를 비교합니다. 하위
      // 바이트만 변조해줍니다.
      // TODO 그런데, 현재 풀패킷일 경우 PacketId가 0으로 고정되어 있으므로,
      // 변화가 없을듯 싶다.
      // 랜덤값이라도 섞어주는게 좋을듯 하다.
      frag_header.splitter |=
          (uint16)((filter_tag ^ frag_header.packet_id) & 0xFF);

      // Write frag header
      frag_header.Write(output.send_frag_frag);
      // Write packet data as attachment(no copy)
      output.send_frag_frag.Add((const uint8*)head->packet.ConstData(),
                                head->packet.Count());

      // 소유권 이양.
      // send-completion이 발생하기 전까지 홀드하는 역활을함.  즉,
      // send-completion 도중에 파괴되면 안되는 것들이므로 send-completion까지
      // 보존해 두었다가, send-completion 발생시 소유권을 놓아주는 구조임.
      // output.owning_packets.Add(head);
      output.owning_packets.AddAndReturnRef(head);
      head->UnlinkSelf();

      // Done!
      return;
    }
  }

  // 2GB 딱 채우는 크기는 라운드 오버런으로 곤란
  // fun_check(NetConfig::message_max_length < int32_MAX - 10000);

  const bool fragger_was_empty = packets_.IsEmpty();
  int32 ttl = -1;

  // 도마가 아직 덜 비워졌으면 생략. 비었다면 풀패킷 큐에서 꺼내서 채운다.
  if (fragger_was_empty) {
    ResetFraggerState();

    current_packet_id_++;

    int32 appended_count = 0;

    // 가장 놓은 우선순위의 항목부터 검색해서 MTU 크기 이전까지 도마에 패킷을
    // 올린다.
    for (int32 priority_index = 0;
         priority_index < (int32)MessagePriority::Last; ++priority_index) {
      auto& per_priority_queue = priorities_[priority_index];

      while (!per_priority_queue.fraggable_packets.IsEmpty()) {
        auto head = per_priority_queue.fraggable_packets.Front();

        if (appended_count == 0) {  // 처음인 경우 무조건 하나는 올린다.
          packets_.Add(head);
          total_bytes_ += head->packet.Count();
          ttl = head->ttl;
          head->UnlinkSelf();
        } else {
          // 도마에 올리기 전에 MTU보다 큰지 체크한다.
          // 보내기 전의 소켓 옵션에의 변화가 있어도 도마에 올리는건 더 이상
          // 금지.

          if ((total_bytes_ + head->packet.Count()) < NetConfig::MTU &&
              ttl == head->ttl) {
            packets_.Add(head);
            total_bytes_ += head->packet.Count();
            head->UnlinkSelf();
          } else {
            // MTU 크기를 넘었으므로, 일단 루프를 빠져나감.
            goto GatherDone;
          }
        }

        appended_count++;
      }
    }
  }

GatherDone:
  fun_check(total_bytes_ > 0);

  output.ResetForReuse();

  output.sendto = remote_addr_;
  output.source = owner;
  output.ttl = ttl;

  // 도마에 올린 것들을 MTU 크기까지 뭉친 한 개를 리턴.
  const int32 current_frag_length =
      MathBase::Min(NetConfig::MTU, total_bytes_ - global_offset_in_fragger_);

  // Fill fragment header
  auto& frag_header = output.frag_header;
  frag_header.splitter = uint16(FRAGMENTED_PACKET << 14);
  frag_header.packet_length = total_bytes_;
  frag_header.packet_id = current_packet_id_;
  frag_header.frag_id = dest_frag_id_;

  // 일부 공유기에서 처음 4바이트가 같을 경우, 동일한 패킷이 밀려들어오는 걸로
  // 인식하는 경우가 있을 수 있으므로, 매번 다르게 설정하도록 한다. 랜덤 값을
  // 사용할수도 있겠으나, 간단히 XOR로 처리한다. 단연히 쓰기시에 XOR하였으니,
  // 읽기시에 제대로 읽어들이려면 XOR를 해주어 원래 값을 복원해야한다.
  frag_header.splitter |= uint16((filter_tag ^ frag_header.packet_id) & 0xFF);

  // 헤더부터 issue send를 해야 한다.
  // UDP를 통해서 실제로 보내지는 데이터를 채우는 것이므로, 처음 부분에는 당연히
  // 헤더가 쓰여져야함.
  frag_header.Write(output.send_frag_frag);

  // 본문을 채워넣기
  const int32 old_global_offset_in_fragger = global_offset_in_fragger_;
  while (global_offset_in_fragger_ <
         (old_global_offset_in_fragger +
          current_frag_length)) {  // MTU 이하인 동안
    auto cur_packet = packets_[src_index_in_fragger_];

    // TODO 아래 코드는 access violation을 잡기 위한, 추가 코드임.  추후
    // 제거해야함.
    fun_check(cur_packet);
    fun_check(cur_packet->packet.IsEmpty() == false);
    fun_check(cur_packet->packet.ConstData());

    const int32 frag_apendee_length =
        MathBase::Min(old_global_offset_in_fragger + current_frag_length -
                          global_offset_in_fragger_,
                      cur_packet->packet.Count() - local_offset_in_fragger_);
    fun_check(frag_apendee_length > 0);

    output.send_frag_frag.Add(
        (const uint8*)cur_packet->packet.ConstData() + local_offset_in_fragger_,
        frag_apendee_length);

    local_offset_in_fragger_ += frag_apendee_length;
    global_offset_in_fragger_ += frag_apendee_length;

    fun_check(local_offset_in_fragger_ <= cur_packet->packet.Count());
    if (local_offset_in_fragger_ == cur_packet->packet.Count()) {
      src_index_in_fragger_++;
      local_offset_in_fragger_ = 0;
    }

    fun_check((output.send_frag_frag.Length() - (int32)sizeof(FragHeader)) <=
              NetConfig::MTU);
  }

  fun_check(global_offset_in_fragger_ ==
            (old_global_offset_in_fragger + current_frag_length));
  fun_check(global_offset_in_fragger_ <= total_bytes_);

  // 헤더, 내용을 다 채웠으니 이제 frag ID 증가해도 안전
  dest_frag_id_++;

  // 끝까지 다 보낸 상태이면 도마에 있는 것들의 소유권을 output에 넘기고, 도마를
  // 비운다.
  if (global_offset_in_fragger_ == total_bytes_) {
    // 소유권을 output 객체가 가지게 한다.
    // 이제 콜러가 갖고 있다가 udp send completion이 발생하면 이 메서드를 또
    // 콜할 것이다. 콜 하면서 OwningPackets도 다 증발할 것이다.

    // output.owning_packets.Append(packets_.ConstData(), packets_.Count());
    for (int32 i = 0; i < packets_.Count(); ++i) {
      output.owning_packets.Add(packets_[i]);
    }
    packets_.Reset();  // keep capacity
  }

  const int32 len = output.send_frag_frag.Length();
  send_brake_.Accumulate(len);
  send_speed_.Accumulate(len, absolute_time);
}

int32 UdpPacketFragger::PacketQueue::GetTotalCount() const {
  int32 count = 0;

  const auto raw = priorities_.ConstData();
  for (int32 priority_index = 0; priority_index < (int32)MessagePriority::Last;
       ++priority_index) {
    count += raw[priority_index].fraggable_packets.Count();
    count += raw[priority_index].no_fraggable_packets.Count();
  }

  count += packets_.Count();

  return count;
}

UdpPacketFragger::PacketQueue::PacketQueue() {
  for (int32 priority_index = 0; priority_index < priorities_.Count();
       ++priority_index) {
    priorities_[priority_index].owner = this;
  }

  owner = nullptr;
  last_accessed_time_ = 0;
  remote_addr_ = InetAddress::None;
  filter_tag = 0;

  ResetFraggerState();

  // 매 호스트 연결시마다 서로 다른 값부터 시작하게 하면 최근 몇초 사이 재접속한
  // 경우 과거 defrag 미완료분과 꼬이는 일이 줄어드니까.
  current_packet_id_ = (PacketIdType)(intptr_t)this;  // 랜덤값의 의미를 가짐
}

bool UdpPacketFragger::PacketQueue::IsOutstanding(double absolute_time) {
  // TODO 비어 있는지 여부만 판단하는 루틴을 하나 지원하도록 하자.
  // GetTotalCount() 함수는 전체를 iteration하므로, 성능 하향에 일조한다.
  if (GetTotalCount() == 0) {
    return false;
  }

  if (!owner->send_brake_enabled_) {
    return true;
  }

  return send_brake_.BrakeNeeded(absolute_time,
                                 allowed_max_send_speed_.GetValue()) == false;
}

int32 UdpPacketFragger::PacketQueue::GetTotalLengthInBytes() {
  int32 count = 0;

  for (int32 priority_index = 0; priority_index < (int32)MessagePriority::Last;
       ++priority_index) {
    count += priorities_[priority_index].GetTotalLengthInBytes();
  }

  if (total_bytes_ > 0) {
    count += total_bytes_ - global_offset_in_fragger_;
  }

  return count;
}

UdpPacketFragger::PacketQueue::~PacketQueue() { Clear(); }

void UdpPacketFragger::PacketQueue::Clear() {
  for (auto packet : packets_) {
    owner->ReturnPacketToPool_INTERNAL(packet);
  }

  // TODO 매번 삭제되므로, 특별히 할필요는 없어보이는데... 이것도 풀링을
  // 해주어야할까??
  packets_.Reset();  // keep capacity
}

UdpPacketDefragger::AssembledPacketError
UdpPacketDefragger::PushFragmentAndPopAssembledPacket(
    uint8* frag_data, int32 frag_len, const InetAddress& sender_addr,
    HostId src_host_id, double absolute_time, assembled_packet& output,
    String& out_error) {
  // double absolute_time = delegate_->GetAbsoluteTime();

  MessageIn msg(ByteArray::FromRawData((const char*)frag_data, frag_len));

  FragHeader frag_header;
  if (!frag_header.Read(msg)) {
    out_error = "UDP frag header missing.";
    return AssembledPacketError::Error;
  }

  const uint8 frag_type = (uint8)(frag_header.splitter >> 14) & 3;
  if (!(frag_type == FRAGMENTED_PACKET || frag_type == NON_FRAGMENTED_PACKET)) {
    out_error = String::Format(
        "Cannot identify UDP fragment nor full packet.  frag_type: %u",
        frag_type);
    return AssembledPacketError::Error;
  }

  // Filter tag 체크
  const HostId P1 =
      src_host_id;  // delegate_->GetSrcHostIdByAddrAtDestSide_NOLOCK(sender_addr);
  const HostId P2 = delegate_->GetLocalHostId();

  // filter_tag 는 packet_id 로 XOR연산이 되어 있으므로 원본값으로 다시
  // 돌립니다.
  uint8 readed_filter_tag = (uint8)(frag_header.splitter & 0xFF);  //하위 8비트
  readed_filter_tag ^=
      (uint8)(frag_header.packet_id &
              0xFF);  // 패킷 ID의 하위 8비트와 XOR해주어야 원래 값이 됨.
  if (FilterTag::ShouldBeFiltered(readed_filter_tag, P1, P2)) {
    // 홀펀칭 과정에서 잘못된 호스트가 받는 경우가 으레 있으므로.
    return AssembledPacketError::Assembling;
  }

  // 조각난 패킷인 경우, 유효한지 여부 체크
  if (frag_type == FRAGMENTED_PACKET) {
    fun_check(NetConfig::MTU > 0);

    if ((int32)frag_header.packet_length <= 0 ||
        (int32)frag_header.packet_length > delegate_->GetMessageMaxLength() ||
        (int32)frag_header.frag_id < 0 ||
        frag_header.frag_id > (frag_header.packet_length / NetConfig::MTU)) {
      out_error = String::Format(
          "UDP frag length is wrong #1. packet_length: %d, max_length: %d, "
          "frag_id: %d (%d), mtu: %d",
          frag_header.packet_length, delegate_->GetMessageMaxLength(),
          frag_header.frag_id, frag_header.packet_length / NetConfig::MTU,
          NetConfig::MTU);
      return AssembledPacketError::Error;
    }
  }
  // 조각나지 않은 패킷인 경우, 유효한지 여부 체크
  else if (frag_type == NON_FRAGMENTED_PACKET) {
    if ((int32)frag_header.packet_length <= 0 ||
        (int32)frag_header.packet_length > delegate_->GetMessageMaxLength()) {
      out_error = "UDP full packet length is wrong.";
      return AssembledPacketError::Error;
    }
  }

  // FragId가 마지막을 가리키느냐 여부에 따라 FragLength가 매치되어야 한다.
  const int32 frag_offset = NetConfig::MTU * frag_header.frag_id;
  const int32 desired_frag_length = MathBase::Min(
      NetConfig::MTU, (int32)frag_header.packet_length - frag_offset);
  const int32 frag_payload_length = msg.GetReadableLength();

  if (frag_type == FRAGMENTED_PACKET) {
    if (desired_frag_length != frag_payload_length) {
      out_error = String::Format(
          "UDP frag length is wrong #2. desired_frag_length: %d, "
          "frag_payload_length: %d",
          desired_frag_length, frag_payload_length);
      return AssembledPacketError::Error;
    }
  }

  // addr-to-queue map 항목을 찾거나 없으면 새로 추가.
  // sender_addr, PacketID are keys
  DefraggingPackets* packets =
      address_to_defragging_packets_map_.map.FindRef(sender_addr);
  if (packets == nullptr) {
    packets = new DefraggingPackets();
    packets->recent_receive_speed.TouchFirstTime(absolute_time);
    address_to_defragging_packets_map_.map.Add(sender_addr, packets);
  }

  // frag를 받은 경우에 한해,
  if (frag_type == FRAGMENTED_PACKET) {
    // board에 없으면 하나 추가. 단, board에 이미 있는 경우 조립중이던 패킷과
    // 크기가 다르면 즐
    auto packet = packets->map.FindRef(frag_header.packet_id);
    if (packet == nullptr) {
      // '조립중인' 패킷이 없으므로 새로 추가
      packet = DefraggingPacket::NewOrRecycle();
      packet->created_time = absolute_time;
      packet->assembled_data.ResizeUninitialized(frag_header.packet_length);
      packet->frag_fill_flags.ResizeUninitialized(
          GetAppropriateFlagListLength(frag_header.packet_length));
      UnsafeMemory::Memzero(packet->frag_fill_flags.GetData(),
                            packet->frag_fill_flags.Count());

      // 패킷큐에 추가.
      packets->map.Add(frag_header.packet_id, packet);
    } else {
      // '조립중' 인 패킷에 받은 fragment의 원하는 크기와 서로 다르면 리셋하고
      // 다시 받는다.
      if (packet->assembled_data.Count() != FragHeader.packet_length) {
        out_error = String::Format(
            "UDP frag length is wrong #3. assembled_data_length: %d, "
            "packet_length: %d",
            packet->assembled_data.Count(), frag_header.packet_length);

        // 기 갖고있던 defrag중 상황을 버린다. 옛것이라고 간주된 경우일 수
        // 있으므로.
        packets->map.Remove(frag_header.packet_id);
        packet->ReturnToPool();

        return AssembledPacketError::Error;
      }
    }

    // '조립중' 패킷에 신규 frag를 채우기
    if (!((int32)frag_header.frag_id < packet->frag_fill_flags.Count())) {
      out_error = "UDP frag id is wrong.";
      return AssembledPacketError::Error;
    }

    if ((frag_payload_length + frag_offset) > packet->assembled_data.Count()) {
      out_error = "UDP frag payload length is wrong.";
      return AssembledPacketError::Error;
    }

    if (!packet->frag_fill_flags[frag_header.frag_id]) {
      packet->frag_fill_flags[frag_header.frag_id] = true;

      // 카운트 업
      packet->frag_filled_count++;
      packets->recent_receive_speed.Accumulate(frag_len,
                                               absolute_time);  // 송신량 카운팅

      UnsafeMemory::Memcpy(packet->assembled_data.GetData() + frag_offset,
                           msg.GetReadableData(), frag_payload_length);
    }

    // 모든 frag를 채운 경우 출력 후 true 리턴하기
    if (packet->frag_fill_flags.Count() == packet->frag_filled_count) {
      output.sender_addr = sender_addr;
      output.TakeOwnership(packet);                // 소유권 이양.
      packets->map.Remove(frag_header.packet_id);  // 도마에서도 제거한다.

      // 참고: 여기서 addrport 대응 항목을 제거하면 잦은 추가제거가 유발되므로
      // 일단은 한동안은 갖고 있는다.
      return AssembledPacketError::Ok;
    }
  } else if (frag_type == NON_FRAGMENTED_PACKET) {
    // non-fragged packet일 경우
    //
    // 패킷큐안에 패킷이 하나도 없으므로, 회수 동작에서 삭제될텐데...
    // 비효율적인거 아닌가?
    // 기껏 생성만 해두고, 회수 동작시에 회수되는 형태가 됨.
    // 단순히 속도측정용밖에는 안되는데...
    fun_check(frag_payload_length ==
              msg.GetReadableLength());  // 계산된 값이 같은지 재확인.

    if (frag_payload_length > 0) {
      output.sender_addr = sender_addr;

      const uint8* payload_ptr = msg.GetReadableData();
      packets->recent_receive_speed.Accumulate(frag_len,
                                               absolute_time);  // 송신량 카운팅

      auto packet = DefraggingPacket::NewOrRecycle();
      packet->assembled_data.ResizeUninitialized(frag_payload_length);
      UnsafeMemory::Memcpy(packet->assembled_data.GetData(), payload_ptr,
                           frag_payload_length);  // copy
      output.TakeOwnership(packet);               // 소유권 이양.

      return AssembledPacketError::Ok;
    }
  }

  return AssembledPacketError::Assembling;
}

//
// UdpPacketDefragger
//

UdpPacketDefragger::UdpPacketDefragger(IUdpPacketDefraggerDelegate* delegate)
    : delegate_(delegate) {
  // TODO 이 map 클래스는 신축폭이 매우 크다. 따라서 rehash 역치를 최대한 크게
  // 잡아야 한다. address_to_defragging_packets_map_.map.SetOptimalLoad(0.30f,
  // 0.05f, 8.0f);
}

void UdpPacketDefragger::PruneTooOldDefragBoard() {
  const double absolute_time = delegate_->GetAbsoluteTime();

  for (auto pakcets_it =
           address_to_defragging_packets_map_.map.CreateIterator();
       pakcets_it; ++pakcets_it) {
    auto packets = pakcets_it->value;

    for (auto packet_it = packets->map.CreateIterator(); packet_it;
         ++packet_it) {
      auto packet = packet_it->value;

      if ((absolute_time - packet->created_time) >
          NetConfig::assemble_fragged_packet_timeout_sec) {
        packet->ReturnToPool();
        packet_it.RemoveCurrent();
      }
    }

    // 첫번째 맵에서 제거. 단, 충분히 오래되지 않은 것을 제거하면 수신속도 측정
    // 정보가 증발해버리므로 주의.
    if (packets->map.IsEmpty() &&
        packets->recent_receive_speed.IsRemovingSafeForCalcSpeed(
            absolute_time)) {
      // TODO 풀 패킷만 있는 경우에도 큐가 비어있는 상태로 엔트리가 잡히므로,
      // 무조건 시간이 되면 삭제되는 형태임.  단순히 수신속도를 측정하기 위해
      //엔트리를 유지하는 형태인데 이게 좀 비효율적일듯 싶음.
      delete packets;
      packets = nullptr;
      pakcets_it.RemoveCurrent();
    }
  }
}

void UdpPacketDefragger::LongTick(double absolute_time) {
  // for (auto pair : address_to_defragging_packets_map_.map) {
  //  auto packets = pair.value;
  for (auto it = address_to_defragging_packets_map_.map.CreateIterator(); it;
       ++it) {
    auto packets = it->value;
    LongTick(packets, absolute_time);
  }

  PruneTooOldDefragBoard();
}

void UdpPacketDefragger::LongTick(DefraggingPackets* packets,
                                  double absolute_time) {
  // 최근 수신속도 산출
  packets->recent_receive_speed.LongTick(absolute_time);

#ifdef UPDATE_TEST_STATS
  TestStats::test_recent_recv_speed =
      packets->recent_receive_speed.GetRecentSpeed();
#endif
}

// Src로부터 온 패킷들의 최근 수신속도
double UdpPacketDefragger::GetRecentReceiveSpeed(const InetAddress& src) {
  auto packets = address_to_defragging_packets_map_.map.FindRef(src);
  return packets ? packets->recent_receive_speed.GetRecentSpeed() : 0;
}

void UdpPacketDefragger::Remove(const InetAddress& src_addr) {
  // TODO optimize: 두번 검색이됨...
  if (auto packets = address_to_defragging_packets_map_.map.FindRef(src_addr)) {
    delete packets;
    address_to_defragging_packets_map_.map.Remove(src_addr);
  }
}

void UdpPacketDefragger::Clear() {
  // for (auto pair : address_to_defragging_packets_map_.map) {
  //  delete pair.value;
  //}
  for (auto it = address_to_defragging_packets_map_.map.CreateIterator(); it;
       ++it) {
    delete it->value;
  }
  address_to_defragging_packets_map_.map.Clear();
}

//
// FilterTag
//

bool FilterTag::ShouldBeFiltered(FilterTag::Type filter_tag, HostId src_id,
                                 HostId dest_id) {
  fun_check((uint8(HostId_None) & 0xFF) ==
            0);  //@todo 의미 없는 체크인데, 유지보수시 바뀔까봐? 에이..

  // src_id, dest_id, FilterTag의 src_id, DestId가 0인 경우는 wildcard, 즉
  // 무조건 '통과'를 의미한다.
  const uint8 b1 = (filter_tag >> 4) & 0xF;
  const uint8 b2 = (filter_tag & 0xF);

  const uint8 c1 = src_id & 0xF;
  const uint8 c2 = dest_id & 0xF;

  return ((b1 != 0 && c1 != 0) && (b1 != c1)) ||
         ((b2 != 0 && c2 != 0) && (b2 != c2));
}

FilterTag::Type FilterTag::Make(HostId src_id, HostId dest_id) {
  FilterTag::Type tag;
  tag = FilterTag::Type(src_id & 0xF) << 4;  // Low nibble
  tag |= FilterTag::Type(dest_id & 0xF);     // High nibble
  return tag;
}

//
// UdpPacketFragger::PacketQueue::per_priority_queue
//

// TODO 매번 루프를 돌면서 계산해야하는지??
int32 UdpPacketFragger::PacketQueue::PerPriorityQueue::GetTotalLengthInBytes()
    const {
  int32 count = 0;

  const UdpPacketContext* packet;

  for (packet = fraggable_packets.Front(); packet;
       packet = packet->GetNextNode()) {
    count += packet->packet.Count();
  }

  for (packet = no_fraggable_packets.Front(); packet;
       packet = packet->GetNextNode()) {
    count += packet->packet.Count();
  }

  return count;
}

UdpPacketFragger::PacketQueue::PerPriorityQueue::~PerPriorityQueue() {
  UdpPacketContext* packet;

  while ((packet = fraggable_packets.Front()) != nullptr) {
    packet->UnlinkSelf();
    owner->owner_->ReturnPacketToPool_INTERNAL(packet);
  }

  while ((packet = no_fraggable_packets.Front()) != nullptr) {
    packet->UnlinkSelf();
    owner->owner_->ReturnPacketToPool_INTERNAL(packet);
  }
}

//
// DefraggingPacket
//

DefraggingPacket* DefraggingPacket::NewOrRecycle() {
  CScopedLock2 pool_guard(g_pool.CS);
  return g_pool.Pool->NewOrRecycle();
}

void DefraggingPacket::ReturnToPool() {
  frag_filled_count = 0;
  created_time = 0;

  // Clear but keep capacity.
  frag_fill_flags.Reset();
  // Clear but keep capacity.
  assembled_data.Reset();

  // Return to pool.
  CScopedLock2 pool_guard(g_pool.CS);
  g_pool.Pool->ReturnToPool(this);
}

//
// UdpPacketDefragger::AddressToDefraggingPacketsMap
//

UdpPacketDefragger::AddressToDefraggingPacketsMap::
    AddressToDefraggingPacketsMap() {
  // TODO 증감폭이 워낙 큰데다 rehash cost가 크기 때문에...
  // map.SetOptimalLoad_BestLookup();
}

UdpPacketDefragger::AddressToDefraggingPacketsMap::
    ~AddressToDefraggingPacketsMap() {
  // for (auto pair : map) {
  //  delete pair.value;
  //}
  for (auto it = map.CreateIterator(); it; ++it) {
    delete it->value;
  }
  map.Clear();
}

//
// UdpPacketFragger::AddressToQueueMap
//

UdpPacketFragger::AddressToQueueMap::AddressToQueueMap() {
  // TODO 증감폭이 워낙 큰데다 rehash cost가 크기 때문에...
  // SetOptimalLoad_BestLookup();
}

UdpPacketFragger::AddressToQueueMap::~AddressToQueueMap() { ClearAndFree(); }

void UdpPacketFragger::AddressToQueueMap::ClearAndFree() {
  for (auto it = map.CreateIterator(); it; ++it) {
    auto queue = it->value;

    queue->UnlinkSelf();
    delete queue;
  }

  map.Clear();
}

//
// UdpPacketFraggerOutput
//

UdpPacketFraggerOutput::~UdpPacketFraggerOutput() { ResetForReuse(); }

void UdpPacketFraggerOutput::ResetForReuse() {
  send_frag_frag.Clear();

  if (source) {
    for (auto packet : owning_packets) {
      source->ReturnPacketToPool_INTERNAL(packet);
    }
  }

  owning_packets.Clear();
  sendto = InetAddress::None;
  source = nullptr;
  ttl = -1;
}

//
// DefraggingPacket::PacketPool
//

DefraggingPacket::PacketPool DefraggingPacket::g_pool;

DefraggingPacket::PacketPool::PacketPool() {
  CScopedLock2 pool_guard(mutex);
  pool = new TObjectPool<DefraggingPacket>;
}

DefraggingPacket::PacketPool::~PacketPool() {
  CScopedLock2 pool_guard(mutex);
  delete pool;
}

}  // namespace net
}  // namespace fun
