#pragma once

namespace fun {
namespace net {

class RandomMT;
class RUdpFrame;
class SendFragRefs;
class MessageOut;

class RUdpHelper {
 public:
  static FrameNumber GetRandomFrameNumber(RandomMT& random);
  static void BuildSendDataFromFrame(RUdpFrame& frame, SendFragRefs& out_result, MessageOut& header);
  static void BuildRelayed2LongDataFrame(FrameNumber frame_number, const SendFragRefs& content, RUdpFrame& out_result);
};

} // namespace net
} // namespace fun
