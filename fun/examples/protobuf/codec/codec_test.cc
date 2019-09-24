#include "codec.h"
#include <examples/protobuf/codec/query.pb.h>
#include <red/net/Endian.h>

#include <stdio.h>
#include <zlib.h>  // adler32

using namespace fun;
using namespace fun::net;

void print(const Buffer& buf) {
  printf("encoded to %zd bytes\n", buf.GetReadableLength());
  for (size_t i = 0; i < buf.GetReadableLength(); ++i) {
    unsigned char ch = static_cast<unsigned char>(buf.peek()[i]);

    printf("%2zd:  0x%02x  %c\n", i, ch, isgraph(ch) ? ch : ' ');
  }
}

void testQuery() {
  fun::Query query;
  query.set_id(1);
  query.set_questioner("Chen Shuo");
  query.add_question("Running?");

  Buffer buf;
  ProtobufCodec::FillEmptyBuffer(&buf, query);
  print(buf);

  const int32_t len = buf.ReadInt32();
  fun_check(len == static_cast<int32_t>(buf.GetReadableLength()));

  ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
  MessagePtr message = ProtobufCodec::Parse(buf.peek(), len, &error_code);
  fun_check(error_code == ProtobufCodec::kNoError);
  fun_check(message != NULL);
  message->PrintDebugString();
  fun_check(message->DebugString() == query.DebugString());

  fun::SharedPtr<fun::Query> newQuery = down_pointer_cast<fun::Query>(message);
  fun_check(newQuery != NULL);
}

void testAnswer() {
  fun::Answer answer;
  answer.set_id(1);
  answer.set_questioner("Chen Shuo");
  answer.set_answerer("blog.csdn.net/Solstice");
  answer.add_solution("Jump!");
  answer.add_solution("Win!");

  Buffer buf;
  ProtobufCodec::FillEmptyBuffer(&buf, answer);
  print(buf);

  const int32_t len = buf.ReadInt32();
  fun_check(len == static_cast<int32_t>(buf.GetReadableLength()));

  ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
  MessagePtr message = ProtobufCodec::Parse(buf.peek(), len, &error_code);
  fun_check(error_code == ProtobufCodec::kNoError);
  fun_check(message != NULL);
  message->PrintDebugString();
  fun_check(message->DebugString() == answer.DebugString());

  fun::SharedPtr<fun::Answer> newAnswer =
      down_pointer_cast<fun::Answer>(message);
  fun_check(newAnswer != NULL);
}

void testEmpty() {
  fun::Empty empty;

  Buffer buf;
  ProtobufCodec::FillEmptyBuffer(&buf, empty);
  print(buf);

  const int32_t len = buf.ReadInt32();
  fun_check(len == static_cast<int32_t>(buf.GetReadableLength()));

  ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
  MessagePtr message = ProtobufCodec::Parse(buf.peek(), len, &error_code);
  fun_check(message != NULL);
  message->PrintDebugString();
  fun_check(message->DebugString() == empty.DebugString());
}

void redoCheckSum(String& data, int len) {
  int32_t checkSum = sockets::hostToNetwork32(static_cast<int32_t>(
      ::adler32(1, reinterpret_cast<const Bytef*>(data.c_str()),
                static_cast<int>(len - 4))));
  data[len - 4] = reinterpret_cast<const char*>(&checkSum)[0];
  data[len - 3] = reinterpret_cast<const char*>(&checkSum)[1];
  data[len - 2] = reinterpret_cast<const char*>(&checkSum)[2];
  data[len - 1] = reinterpret_cast<const char*>(&checkSum)[3];
}

void testBadBuffer() {
  fun::Empty empty;
  empty.set_id(43);

  Buffer buf;
  ProtobufCodec::FillEmptyBuffer(&buf, empty);
  // print(buf);

  const int32_t len = buf.ReadInt32();
  fun_check(len == static_cast<int32_t>(buf.GetReadableLength()));

  {
    String data(buf.peek(), len);
    ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
    MessagePtr message =
        ProtobufCodec::Parse(data.c_str(), len - 1, &error_code);
    fun_check(message == NULL);
    fun_check(error_code == ProtobufCodec::kCheckSumError);
  }

  {
    String data(buf.peek(), len);
    ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
    data[len - 1]++;
    MessagePtr message = ProtobufCodec::Parse(data.c_str(), len, &error_code);
    fun_check(message == NULL);
    fun_check(error_code == ProtobufCodec::kCheckSumError);
  }

  {
    String data(buf.peek(), len);
    ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
    data[0]++;
    MessagePtr message = ProtobufCodec::Parse(data.c_str(), len, &error_code);
    fun_check(message == NULL);
    fun_check(error_code == ProtobufCodec::kCheckSumError);
  }

  {
    String data(buf.peek(), len);
    ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
    data[3] = 0;
    redoCheckSum(data, len);
    MessagePtr message = ProtobufCodec::Parse(data.c_str(), len, &error_code);
    fun_check(message == NULL);
    fun_check(error_code == ProtobufCodec::kInvalidNameLen);
  }

  {
    String data(buf.peek(), len);
    ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
    data[3] = 100;
    redoCheckSum(data, len);
    MessagePtr message = ProtobufCodec::Parse(data.c_str(), len, &error_code);
    fun_check(message == NULL);
    fun_check(error_code == ProtobufCodec::kInvalidNameLen);
  }

  {
    String data(buf.peek(), len);
    ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
    data[3]--;
    redoCheckSum(data, len);
    MessagePtr message = ProtobufCodec::Parse(data.c_str(), len, &error_code);
    fun_check(message == NULL);
    fun_check(error_code == ProtobufCodec::kUnknownMessageType);
  }

  {
    String data(buf.peek(), len);
    ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
    data[4] = 'M';
    redoCheckSum(data, len);
    MessagePtr message = ProtobufCodec::Parse(data.c_str(), len, &error_code);
    fun_check(message == NULL);
    fun_check(error_code == ProtobufCodec::kUnknownMessageType);
  }

  {
    // FIXME: reproduce Parse error
    String data(buf.peek(), len);
    ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
    redoCheckSum(data, len);
    MessagePtr message = ProtobufCodec::Parse(data.c_str(), len, &error_code);
    // fun_check(message == NULL);
    // fun_check(error_code == ProtobufCodec::kParseError);
  }
}

int g_count = 0;

void OnMessage(const fun::net::TcpConnectionPtr& conn,
               const MessagePtr& message, fun::Timestamp received_time) {
  g_count++;
}

void testOnMessage() {
  fun::Query query;
  query.set_id(1);
  query.set_questioner("Chen Shuo");
  query.add_question("Running?");

  Buffer buf1;
  ProtobufCodec::FillEmptyBuffer(&buf1, query);

  fun::Empty empty;
  empty.set_id(43);
  empty.set_id(1982);

  Buffer buf2;
  ProtobufCodec::FillEmptyBuffer(&buf2, empty);

  size_t totalLen = buf1.GetReadableLength() + buf2.GetReadableLength();
  Buffer all;
  all.append(buf1.peek(), buf1.GetReadableLength());
  all.append(buf2.peek(), buf2.GetReadableLength());
  fun_check(all.GetReadableLength() == totalLen);
  fun::net::TcpConnectionPtr conn;
  fun::Timestamp t;
  ProtobufCodec codec(OnMessage);
  for (size_t len = 0; len <= totalLen; ++len) {
    Buffer input;
    input.append(all.peek(), len);

    g_count = 0;
    codec.OnMessage(conn, &input, t);
    int expected = len < buf1.GetReadableLength() ? 0 : 1;
    if (len == totalLen) expected = 2;
    fun_check(g_count == expected);
    (void)expected;
    // printf("%2zd %d\n", len, g_count);

    input.append(all.peek() + len, totalLen - len);
    codec.OnMessage(conn, &input, t);
    fun_check(g_count == 2);
  }
}

int main() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  testQuery();
  puts("");
  testAnswer();
  puts("");
  testEmpty();
  puts("");
  testBadBuffer();
  puts("");
  testOnMessage();
  puts("");

  puts("All pass!!!");

  google::protobuf::ShutdownProtobufLibrary();
}
