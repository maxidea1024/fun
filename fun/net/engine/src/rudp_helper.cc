#include "fun/net/net.h"
#include "RUdpHelper.h"
#include "RUdpFrame.h"
#include "SendData.h"

namespace fun {
namespace net {

using lf = LiteFormat;

FrameNumber RUdpHelper::GetRandomFrameNumber(RandomMT& Random) {
  return (FrameNumber)Random.RangedInt(1, int32_MAX - 1); // 0은 허용안함.
}

void RUdpHelper::BuildSendDataFromFrame(RUdpFrame& frame,
                                        SendFragRefs& out_result,
                                        MessageOut& out_header) {
  out_result.Clear(); // just in case

  lf::Write(out_header, MessageType::RUdp_Frame);
  lf::Write(out_header, frame.type);

  switch (frame.type) {
    case RUdpFrameType::Data:
      lf::Write(out_header, frame.frame_number);
      lf::Write(out_header, OptimalCounter32(frame.data.Len()));

      out_result.Add(out_header);
      out_result.Add(frame.data);
      break;

    case RUdpFrameType::Ack:
      fun_check(frame.acked_frame_numbers);
      frame.acked_frame_numbers->Write(out_header);
      lf::Write(out_header, frame.expected_frame_number);
      lf::Write(out_header, frame.recent_receive_speed);
      out_result.Add(out_header);
      break;

    default:
      fun_unexpected();
      break;
  }
}

void RUdpHelper::BuildRelayed2LongDataFrame(FrameNumber frame_number,
                                            const SendFragRefs& content,
                                            RUdpFrame& out_result) {
  // Build long frame, compatible to reliable UDP data frame
  out_result.type = RUdpFrameType::Data;
  out_result.frame_number = frame_number;
  out_result.data = content.ToBytes(); // Copy
}

} // namespace net
} // namespace fun
