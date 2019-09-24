#include "fun/net/net.h"
#include "MessageStream.h"
#include <assert.h> // CHECK에 버그가 있는지 확인을 위해서...

namespace fun {
namespace net {

using lf = LiteFormat;

#define SPLITTER_VALUE_1  0x1122
#define SPLITTER_VALUE_2  0x3344

void MessageStream::AddStreamHeader(const SendFragRefs& payload,
                                    SendFragRefs& output,
                                    MessageOut& header) {
  lf::Write(header, SPLITTER_VALUE);         // Spliiter marker
  lf::Write(header, OptimalCounter32(payload.GetTotalLength()));  // Length of payload

  output.Add(header);   // header segment
  output.Add(payload);  // payload segment
}

int32 MessageStream::ExtractMessagesAndFlushStream(
      StreamQueue& input,
      ReceivedMessageList& output,
      HostId sender_id,
      int32 message_max_length,
      ResultCode& out_error) {
  MessageStreamExtractor extractor;
  extractor.input = input.ConstData();
  extractor.input_length = input.Length();
  extractor.output = &output;
  extractor.sender_id = sender_id;
  extractor.message_max_length = message_max_length;

  const int32 extracted_msg_count = extractor.Extract(out_error);
  input.DequeueNoCopy(extractor.out_last_success_offset); // drain
  return extracted_msg_count;
}

/**
스트림으로부터 메시지들을 추려낸다.
  - 메서드 호출 전에 멤버 변수들을 다 채워야 한다.

return 추출한 메시지 갯수. -1이면 잘못된 스트림 데이터가 있음을 의미한다.
*/
int32 MessageStreamExtractor::Extract(ResultCode& out_error) {
  out_error = ResultCode::Ok;

  fun_check(message_max_length > 0);
  fun_check(out_last_success_offset == 0);

  // 읽을게 업으면 바로 리턴 (에러아님)
  if (input_length == 0) {
    return 0;
  }

  // 여기에서만 사용될 것이므로, raw 형태로 attach해서 메모리 할당 및 복사를 제거하도록 함.
  MessageIn reader(ByteArray::FromRawData((const char*)input, input_length));

  int32 extracted_msg_count = 0;
  int32 last_success_offset = 0;
  while (true) {
    MessageStream::SplitterValueType splitter = 0;

    //@fixme 왜 여기서 test-splitter 가 실패하는가 말이다??
    // 더 이상 없을때까지 계속 추가한다.
    //
    // 서버에서 첫번째로 보내주는 policy 문자열 때문에 그러함.
    // 이를 어찌 처리한다??
    //
    // 클라이언트도 서버로 보내나??
    // 더이상 읽을게 없다면, 바로 루프 빠짐(실패 아님; 여기까지 했다는거지..)
    if (!lf::Read(reader, splitter)) {
      break;
    }

    // 잘못된 패킷이면 이를 예외 이벤트로 전달하고
    // (C++ 예외 발생을 하지 말자. 디버깅시 혼란만 가중하고 fail over에 약하다)
    // 폐기한다.
    // 그리고 스트림을 끝까지 다 처리한 것처럼 간주한다.
    // TCP인 경우라면 상대와의 연결을 끊고, UDP인 경우라면 제3자 해커가 쓰레기 메시지를 보낸
    // 것일 수도 있으므로 무시한다.

    //수정사항:
    // -> NetClientImpl::ExtractMessagesFromTcpStream 에서 최초 policy text 를 수신할때
    //    별도로 처리했으므로 아래와 같은 상황이 나오면 안됨.
    //    말 그대로 에러가 됨!!!
    if (splitter != MessageStream::SPLITTER_VALUE) {
      LOG(LogNetEngine,Error,"Wrong splitter: %04Xh", splitter);

      out_last_success_offset = reader.GetLength(); // 그냥 다 읽어버린 걸로 처리
      out_error = ResultCode::InvalidPacketFormat;
      TRACE_SOURCE_LOCATION();
      return -1;
    }

    // 메시지의 크기를 읽는다.
    OptimalCounter32 payload_length;
    if (!lf::Read(reader, payload_length)) {
      out_last_success_offset = last_success_offset;
      return extracted_msg_count;
    }

    // 메시지의 크기가 잘못된 크기이면 오류 처리를 한다.

    //fun_check(__splitter1 == SPLITTER_VALUE_1);
    //fun_check(__splitter2 == SPLITTER_VALUE_2);
    
    fun_check(message_max_length > 1024); //TODO 현재 문제점을 찾기 위해서 임시로 넣어둔것임.  어이하야 16바이트가 설정되는지??

    if (payload_length < 0) {
      out_last_success_offset = reader.Length(); // 그냥 다 읽어버린 걸로 처리
      out_error = ResultCode::InvalidPacketFormat;
      TRACE_SOURCE_LOCATION();
      return -2;
    } else if (payload_length > message_max_length) {
      LOG(LogNetEngine,Warning,"Heavy payload length: %d, message_max_length: %d", (int32)payload_length, message_max_length);

      out_last_success_offset = reader.Length(); // 그냥 다 읽어버린 걸로 처리
      out_error = ResultCode::TooLargeMessageDetected;
      TRACE_SOURCE_LOCATION();
      return -3;
    }

    // 밑에 부분에서 메모리 할당 부분이 있으므로, 데이터가 아직 모자를 경우에는 아예 시도를 하지 않는것이
    // 최적화에 유리하겠다.
    if (!reader.CanRead(payload_length)) {
      out_last_success_offset = last_success_offset;
      return extracted_msg_count;
    }

    //위에서 이미 길이를 체크했으므로, 여기서 실패할수는 없음!!

    // 받을 메시지 버퍼를 준비한다.
    // 임시버퍼에서 데이터를 가져오므로, 복사를 제거할 수 없음...
    //LiteFormat::Read 로 CByteString을 바로 읽어버리면, 앞에 길이가 있는걸로 간주하기 때문에
    //여기서는 길이만큼 읽는걸로 대체해야함.
    /*
    //ByteArray payload;
    //if (!lf::Read(reader, payload))
    ByteArray payload(payload_length, NoInit);
    if (!reader.ReadRawBytes(payload.MutableData(), payload_length)) {
      out_last_success_offset = last_success_offset;
      return extracted_msg_count;
    }
    */
    ByteArray payload(payload_length, NoInit);
    reader.ReadRawBytes(payload.MutableData(), payload_length);

    // success one packet.
    ReceivedMessage received_msg;
    received_msg.remote_id = sender_id;
    received_msg.unsafe_message = MessageIn(payload); // shared

    output->Add(received_msg);
    extracted_msg_count++;

    // 마지막으로 성공적으로 읽은 위치를 저장해둠.
    // (여기까지는 성공적으로 읽어 들였다에 해당 힌트임.)
    last_success_offset = reader.Tell();
  }

  // 마지막 성공적으로 읽어진 위치 결과에 저장.
  out_last_success_offset = last_success_offset;
  return extracted_msg_count;
}

MessageStreamExtractor::MessageStreamExtractor()
  : input(nullptr),
    input_length(0),
    output(nullptr),
    sender_id(HostId_None),
    message_max_length(0),
    out_last_success_offset(0) {
}

} // namespace net
} // namespace fun
