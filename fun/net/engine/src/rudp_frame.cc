#include "fun/net/net.h"
#include "RUdpFrame.h"
#include "RUdpConfig.h"

namespace fun {
namespace net {

using lf = LiteFormat;

void RUdpFrame::CloneTo(RUdpFrame& to) {
  to.frame_number = frame_number;
  to.data = data;
  to.type = type;

  //TODO 사본을 꼭 만들어야하나??  AckedFrameNumbers이 외부에서 수정된다면, 사본을 만들어 주어야함.
  // (AckedFrameNumbers는 COW 형태로 관리 되지 않으므로..)
  if (acked_frame_numbers) {
    to.acked_frame_numbers = acked_frame_numbers->Clone();
  } else {
    to.acked_frame_numbers = CompressedFrameNumbersPtr();
  }

  to.recent_receive_speed = recent_receive_speed;
  to.expected_frame_number = expected_frame_number;
}

CompressedFrameNumbersPtr CompressedFrameNumbers::Clone() {
  CompressedFrameNumbersPtr to(new CompressedFrameNumbers);
  to->array = array; // Copy
  return to;
}

void CompressedFrameNumbers::AddSortedNumber(FrameNumber n) {
  if (array.IsEmpty()) {
    Elem e;
    e.left = e.right = n;
    array.Add(e);
  } else {
    auto& prev = array.Last();

    if (prev.left == n || prev.right == n) {
      // Do nothing
    } else if (FrameNumberUtil::Adjucent(prev.right, n)) {
      prev.right = n;
    } else if (FrameNumberUtil::Adjucent(prev.left, n)) {
      prev.right = n;
    } else {
      Elem e;
      e.left = e.right = n;
      array.Add(e);
    }
  }
}

void CompressedFrameNumbers::Write(MessageOut& output) {
  fun_check(RUdpConfig::max_ack_count_in_one_frame < 65534);
  fun_check(array.Count() <= RUdpConfig::max_ack_count_in_one_frame); // 최대 258개라 이거지....

  const OptimalCounter32 length = array.Count();
  lf::Write(output, length);

  for (int32 i = 0; i < length; ++i) {
    const auto& e = Array[i];

    if (e.left == e.right) {
      lf::Write(output, uint8(0));
      lf::Write(output, e.left);
    } else {
      lf::Write(output, uint8(1));
      lf::Write(output, e.left);
      lf::Write(output, e.right);
    }
  }
}

bool CompressedFrameNumbers::Read(MessageIn& input) {
  OptimalCounter32 length;
  FUN_DO_CHECKED(lf::Read(input, length));

  //array.ResizeUninitialized(length);
  array.Resize(length);

  for (int32 i = 0; i < length; ++i) {
    auto& e = array[i];

    uint8 flag;
    FUN_DO_CHECKED(lf::Read(input, flag));

    if (flag == 0) {
      FUN_DO_CHECKED(lf::Read(input, e.left));
      e.right = e.left;
    } else {
      FUN_DO_CHECKED(lf::Read(input, e.left));
      FUN_DO_CHECKED(lf::Read(input, e.right));
    }
  }

  return true;
}

// TODO 구태여 프레임의 압축해제를 할 수 없을듯...
void CompressedFrameNumbers::Decompress(DecompressedFrameNumberArray& output) {
  output.Clear();

  for (int32 i = 0; i < array.Count(); ++i) {
    Elem e = array[i]; // Copy
    while (true) {
      output.Add(e.left);
      if (e.left == e.right) {
        break;
      }
      e.left = FrameNumberUtil::NextFrameNumber(e.left);
    }
  }
}

} // namespace net
} // namespace fun
