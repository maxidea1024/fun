//TODO 파일이름을 Fragmentation.h으로 변경 하도록 하자.

#pragma once

#include "Misc/ObjectPool.h"
#include "send_brake_.h"

namespace fun {
namespace net {

/**
uint16로 하면 더 효과적일지 모르겠으나 아직은 모험하지 말자.
*/
typedef uint32 PacketIdType;

class FilterTag {
public:
  // sender, recver hostid의 하위 4비트들이 조합됨
  typedef uint8 Type;

  static Type Make(HostId src_id, HostId dest_id);
  static bool ShouldBeFiltered(FilterTag::Type filter_tag, HostId src_id, HostId dest_id);
};


/**
UDP 프래그먼트의 헤더입니다.

실제로 네트워크로 전송되는 블럭의 헤더입니다.
*/
class FragHeader {
 public:
  /** 앞부분.  fragment 타입 및 각 필드의 길이와 하위 8비트 twisted 값으로 구성되어 있음. */
  uint16 splitter;

  /** 패킷의 바이트 단위 길이입니다. */
  uint32 packet_length;
  /** 패킷의 ID(sequencial number)입니다. */
  uint32 packet_id;
  /** 조각의 ID입니다. */
  uint32 frag_id;

  /** 전송중이 완료되기 전까지 유지되어야하므로, 멤버 형태로 가지고 있어야함. */
  uint8 tx_buffer[14]; //14 = 2 + 4 + 4 + 4

  void Write(FragmentedBuffer& output);
  bool Read(IMessageIn& input);

 private:
  // 아래 Write* 함수들은 little-endian 기준으로 동작하도록 되어 있음.
  // 현재 사용하고 있는 MessageFormat 체계도 little-endian 기준으로 동작하도록 되어 있기 때문에 문제 없음.

  inline static void WriteOptimalUInt32AndAdvance(uint8*& target, const uint32 value, const uint8 length_class) {
    switch (length_class) {
      case 0: WriteUInt8ToBufferAndAdvance(target, (uint8)value); break;
      case 1: WriteUInt16ToBufferAndAdvance(target, (uint16)value); break;
      case 3: WriteUInt32ToBufferAndAdvance(target, value); break;
      default: fun_unexpected(); break;
    }
  }

  inline static void WriteUInt8ToBufferAndAdvance(uint8*& target, const uint8 value) {
    *target++ = value;
  }

  inline static void WriteUInt16ToBufferAndAdvance(uint8*& target, const uint16 value) {
    *target++ = uint8(value);
    *target++ = uint8(value >> 8);
  }

  inline static void WriteUInt32ToBufferAndAdvance(uint8*& target, const uint32 value) {
    *target++ = uint8(value);
    *target++ = uint8(value >> 8);
    *target++ = uint8(value >> 16);
    *target++ = uint8(value >> 24);
  }

  static bool ReadOptimalUInt32(IMessageIn& input, uint32& out_value, const uint8 length_class);
};


class PacketQueue;
class SendFragRefs;
class UdpPacketFraggerOutput;

class UdpPacketContext
  : public ListNode<UdpPacketContext>
  , public Noncopyable {
 public:
  /** MTU보다 클 수 있다. */
  Array<uint8> packet;

  /** 패킷이 겹치는지 여부를 판단하기위한 키 값입니다. */
  uint64 unique_id;

  /** UDP 전송시 hop 카운터를 변경할 경우에 지정합니다. -1일 경우, 지정안함 */
  int32 ttl;

  /**
  unique_id 와 함께 식별자로 사용.
  ToServerUdp의 경우 릴레이 메시지와 서버에게 주는 메시지가 겹쳐서 문제 되므로 변경.
  */
  HostId dest_id;

 public:
  UdpPacketContext()
    : unique_id(0),
      ttl(-1),
      dest_id(HostId_None) {}
};

class IUdpPacketFraggerDelegate {
 public:
  virtual ~IUdpPacketFraggerDelegate() {}

  virtual double GetAbsoluteTime() = 0;
  virtual void RequestReceiveSpeedAtReceiverSide_NoRelay(const InetAddress& dst) = 0;
  virtual int32 GetOverSendSuspectingThresholdInByte() = 0;
};

/**
 * 보낼 패킷을 frag들로 쪼개는 곳. 송신자에서 씀.
 * coalesce 때문에 dictionary class로 만들었다.
 */
class UdpPacketFragger {
 public:
  /** MTU 크기로 아직 쪼개지 않은 패킷들 콜렉션, 각 우선순위들을 모두 갖고 있음. */
  struct PacketQueue
    : public ListNode<PacketQueue>
    , public Noncopyable {
    /** 1개 우선순위 내의 패킷 모음  */
    struct PerPriorityQueue {
      /** 조각난 UDP 패킷 목록 */
      ListNode<UdpPacketContext>::ListOwner fraggable_packets;
      /** 조각나지 않은 온전한 UDP 패킷 목록 */
      ListNode<UdpPacketContext>::ListOwner no_fraggable_packets;
      /** 소유자 Queue */
      PacketQueue* owner;

      ~PerPriorityQueue();

      int32 GetTotalLengthInBytes() const;
    };

    /**
    패킷 송신 우선순위별로 정렬된다. 0번째 항목은 FunNet level 전용이다.
    PacketQueue 객체 자체는 new/delete 횟수가 잦으므로 고정 크기 배열.
    단, 바운드 체크를 준수하자.
    */
    StaticArray<PerPriorityQueue,(int32)MessagePriority::Last> priorities_;

    /** 가장 마지막에 패킷이 추가되거나 제거된 시간.  coalesce case도 포함해서. */
    double last_accessed_time_;
    InetAddress remote_addr_;
    FilterTag::Type filter_tag_;
    UdpPacketFragger* owner_;

    // 도마.
    // - 사용자가 보내려는 1개 이상의 메시지의 집합. MTU보다 크거나 작을 수 있다.
    // - 이들을 1개의 긴 스트림으로 이어붙인 것이 도마이다.
    // - 그리고 MTU 이하 단위로 하나씩 뜯어낸 후 헤더(FragHeader)를 붙인다. 그게 fragment다.
    // - Fragment가 실제 UDP 소켓으로 전달된다.

    Array<UdpPacketContext*> packets_;
    int32 total_bytes_;
    int32 dest_frag_id_;
    int32 global_offset_in_fragger_;
    int32 local_offset_in_fragger_;
    int32 src_index_in_fragger_;
    PacketIdType current_packet_id_;

    SendBrake send_brake_;
    RecentSpeedMeasurer send_speed_;
    AllowedMaxSendSpeed allowed_max_send_speed_;
    RecentReceiveSpeedAtReceiverSide recent_receive_speed_at_receiver_side_;

   public:
    PacketQueue();
    ~PacketQueue();

    void Clear();

    bool IsOutstanding(double absolute_time);

    inline void ResetFraggerState() {
      total_bytes_ = 0;
      dest_frag_id_ = 0;
      global_offset_in_fragger_ = 0;
      local_offset_in_fragger_ = 0;
      src_index_in_fragger_ = 0;

      // 아래 속도 제어를 위한 상태 변수는 초기화하면 안됨!
      //send_brake_ = SendBrake();
      //send_speed_ = RecentSpeedMeasurer();
    }

    int32 GetTotalCount() const;

    /**
    이 함수를 별도 만들어서 성능 가속화.
    */
    inline bool IsEmpty() const {
      const PerPriorityQueue* raw = priorities_.ConstData(); // 바운드체크 피함
      for (int32 i = 0; i < (int32)MessagePriority::Last; ++i) {
        if (raw[i].fraggable_packets.Count() > 0 || raw[i].no_fraggable_packets.Count() > 0) {
          return false;
        }
      }
      return packets_.IsEmpty();
    }

    int32 GetTotalLengthInBytes();

    void PopFragmentOrFullPacket(double absolute_time, UdpPacketFraggerOutput& output);
  };

 private:
  /**
  packet queue가 1개라도 들어있는 항목들의 linked list이다. 즉 outstanding list.
  규칙: 일단 issue send가 된 것은 맨 뒤로 옮겨진다. for lesser starvation.
  */
  PacketQueue::ListOwner outstandings_;

  IUdpPacketFraggerDelegate* delegate_;

  struct AddressToQueueMap {
    Map<InetAddress, PacketQueue*> map;

    AddressToQueueMap();
    ~AddressToQueueMap();
    void ClearAndFree();
  };

  AddressToQueueMap address_to_queue_map_;

  void AssertConsistency() const;

  void ConditionalAddToOutstandings(PacketQueue* queue, double absolute_time);

 private:
  /** 재활용 객체 모음. 즉 객체 풀링용. */
  TObjectPool<UdpPacketContext> packet_pool_;

 public:
  UdpPacketContext* NewOrRecyclePacket_INTERNAL();
  void ReturnPacketToPool_INTERNAL(UdpPacketContext* packet);

  void Remove(const InetAddress& Key);
  void Clear();

  bool send_brake_enabled_; // 이름 그대로. 서버에서는 이걸 false로 할거다.

 public:
  UdpPacketFragger(IUdpPacketFraggerDelegate* delegate);
  ~UdpPacketFragger();

 public:
  void AddNewPacket(HostId final_dest_id,
                    FilterTag::Type filter_tag,
                    const InetAddress& send_to,
                    const SendFragRefs& data_to_send,
                    double added_time,
                    const UdpSendOption& send_opt);

  int32 GetTotalPacketCountOfAddr(const InetAddress& addr);

  inline bool IsUdpSendBufferPacketEmpty(const InetAddress& addr) {
    // cache for frequent case
    if (address_to_queue_map_.map.IsEmpty()) {
      return true;
    }

    auto queue = address_to_queue_map_.map.FindRef(addr);
    return queue ? queue->IsEmpty() : false;
  }

  int32 FromTotalPacketInByteByAddr(const InetAddress& addr);
  bool PopAnySendQueueFilledOneWithCoalesce(UdpPacketFraggerOutput& output, double absolute_time);
  void LongTick(double absolute_time);
  void ShortTick(double absolute_time);
  bool HasNothingToSend() const;
  void InitHashTableForClient(); // 클라이언트쪽 코드에서만 호출됨.  현재는 죽어있는 기능. 추후에 살리던지 해야지...
  void SetReceiveSpeedAtReceiverSide(const InetAddress& dst, double speed);
  int32 GetEndPointToQueueMapKeyCount() const { return address_to_queue_map_.map.Count(); }
};


/**
 * send issue를 위해 packet fragger로부터 받은 출력물.
 * packet fragger는 0개 이상의 frag를 이 객체에 전달한다.
 * 그리고 전달된 frag들의 파괴 권리를 이 객체가 가진다.
 * 이 객체가 파괴되기 전에 packet fragger가 파괴되어서는 안된다.
 */
class UdpPacketFraggerOutput {
 public:
  /**
  중요포인트:
  소켓에 바로 전달되는 데이터임.
  그러므로, 보내야할 데이터는 SendFragFrag에 모아두어야함. (참조형태)
  */
  FragmentedBuffer send_frag_frag;

  /**
  FragmentedBuffer에서 FragHeader를 참조하므로, i/o completion이 뜨기전에
  제거하면, 메모리 오류가 발생함.
  */
  FragHeader frag_header;

  /**
  이 출력물을 만들어낸 공급원.
  */
  UdpPacketFragger* source;

  /**
  SendFragFrag이 참조하고 있을 수 있음.
  frag board가 계속 갖고 있을 수도 있고. 하여튼 여기에 들어오면 곧 파괴된다.
  */
  Array<UdpPacketContext*> owning_packets;

  /**
  목적지 주소.
  */
  InetAddress sendto;

  /**
  TTL(Hop counter)
  */
  int32 ttl;

  //virtual ~UdpPacketFraggerOutput();
  ~UdpPacketFraggerOutput();

  /**
  재사용 직전에 호출해줌.  send completion 직후에 호출해줌.
  */
  void ResetForReuse();
};

/**
 * 조립중인 패킷 1개
 * 
 * 주의: new or delete를 쓰지 말고 NewInstance or Drop을 쓸 것! delete는 g_pool 전용!
 * TODO Capaicity 보존하는 부분만 제외하면 구태여 풀링을 할 필요도 없어보이는데...
 */
class DefraggingPacket {
 public:
  /** TODO BitArray로 교체해주는게 바람직함. */
  Array<bool> frag_fill_flags;
  Array<uint8> assembled_data;
  int32 frag_filled_count;
  double created_time;

  inline DefraggingPacket()
    : frag_filled_count(0),
      created_time(0.0) {
  }

  //
  // Pooling
  //

  struct PacketPool {
    TObjectPool<DefraggingPacket>* pool;
    CCriticalSection2 mutex;

    PacketPool();
    ~PacketPool();
  };

  static PacketPool g_pool;

  static DefraggingPacket* NewOrRecycle();
  void ReturnToPool();
};


class DefraggingPackets;

/**
1개 주소로부터 도착한 조립중인 패킷들의 집합
*/
class DefraggingPackets {
 public:
  Map<PacketIdType, DefraggingPacket*> map;
  RecentSpeedMeasurer recent_receive_speed;

  DefraggingPackets() {
    //TODO 구현 주어야함. 이거 영 걸리적 거리네...
    //map.SetOptimalLoad_BestLookup(); // 증감폭이 워낙 큰데다 rehash cost가 크기 때문에
  }

  ~DefraggingPackets() {
    Clear();
  }

  void Clear() {
    for (auto pair : map) {
      pair.value->ReturnToPool();
    }

    map.Clear();
  }
};

/**
 * 조립이 다 끝난 패킷
 */
class AssembledPacket {
 public:
  InetAddress sender_addr;

  inline AssembledPacket()
    : packet_string_ptr_(nullptr) {}

  inline ~AssembledPacket() {
    if (packet_string_ptr_) {
      packet_string_ptr_->ReturnToPool(); // 객체 풀에 반환
    }
  }

  inline void TakeOwnership(DefraggingPacket* ptr) {
    fun_check(packet_string_ptr_ == nullptr); // just once
    packet_string_ptr_ = ptr;
  }

  inline const uint8* ConstData() {
    fun_check_ptr(packet_string_ptr_);
    return packet_string_ptr_->assembled_data.ConstData();
  }

  inline int32 Len() const {
    fun_check_ptr(packet_string_ptr_);
    return packet_string_ptr_->assembled_data.Count();
  }

 private:
  DefraggingPacket* packet_string_ptr_;
};


class IUdpPacketDefraggerDelegate {
 public:
  virtual ~IUdpPacketDefraggerDelegate() {}

  virtual void EnqueuePacketDefragWarning(const InetAddress& sender, const char* text) = 0;
  virtual int32 GetMessageMaxLength() = 0;
  virtual double GetAbsoluteTime() = 0;

  /**
  for filter tag
  addr이 localhost를 가리키는 경우도 체크해야 함!
  addr이 unicase addr이 아니면 none을 리턴해야 함!
  보수적이어야 한다. 차라리 none을 리턴하는게 낫지, 엉뚱한 HostId를 리턴하면 안됨!
  for filter tag
  */
  virtual HostId GetLocalHostId() = 0;
};


/**
frag들을 받아 패킷으로 조립하는 공간. 수신자에서 씀.
*/
class UdpPacketDefragger {
 private:
  IUdpPacketDefraggerDelegate* delegate_;

  /**
  2중 맵이다. 첫번째는 EndPoint, 두번째는 PacketId이다.
  2중 맵으로 굳이 되어있는 이유: EndPoint별 최근 수신 속도 산출을 위해.
  */
  struct AddressToDefraggingPacketsMap {
    Map<InetAddress, DefraggingPackets*> map;

    AddressToDefraggingPacketsMap();
    ~AddressToDefraggingPacketsMap();
  };
  AddressToDefraggingPacketsMap address_to_defragging_packets_map_;

  inline static int32 GetAppropriateFlagListLength(int32 packet_length) {
    fun_check(packet_length >= 0);
    return packet_length > 0 ? (((packet_length - 1) / NetConfig::MTU) + 1) : 0;
  }

  void PruneTooOldDefragBoard();

 public:
  enum class AssembledPacketError {
    Ok,
    Assembling,
    Error
  };

  UdpPacketDefragger(IUdpPacketDefraggerDelegate* delegate);

  AssembledPacketError PushFragmentAndPopAssembledPacket(
        uint8* frag_data,
        int32 frag_len,
        const InetAddress& sender_addr,
        HostId src_host_id,
        double absolute_time,
        assembled_packet& output,
        String& out_error);

  void LongTick(double absolute_time);

  double GetRecentReceiveSpeed(const InetAddress& src);

  void Remove(const InetAddress& src_addr);

  void Clear();

 private:
  void LongTick(DefraggingPackets* packets, double absolute_time);
};

} // namespace net
} // namespace fun
