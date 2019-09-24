#pragma once

#include "StreamQueue.h"

namespace fun {
namespace net {

class MessageStream {
 public:
  typedef uint16 SplitterValueType;
  static const SplitterValueType SPLITTER_VALUE = 0xBEEF;

  static void AddStreamHeader(const SendFragRefs& payload,
                              SendFragRefs& output,
                              MessageOut& header);

  static int32 ExtractMessagesAndFlushStream( StreamQueue& input,
                                              ReceivedMessageList& output,
                                              HostId sender_id,
                                              int32 message_max_length,
                                              ResultCode& out_error);

  //TODO
  //static int32 ExtractMessages(const uint8* input, int32 input_length, ReceivedMessageList& output, HostId sender_id, int32 message_max_length, int32& out_last_success_offset, ResultCode& out_error);
};


//TODO 이건 없애고, CMessageStream에 기능을 통합하는게 좋을듯.. 구차하다.
class MessageStreamExtractor {
 public:
  const uint8* input;
  int32 input_length;
  ReceivedMessageList* output;
  HostId sender_id;
  int32 message_max_length;
  int32 out_last_success_offset;

  MessageStreamExtractor();

  int32 Extract(ResultCode& out_error);
};

} // namespace net
} // namespace fun
