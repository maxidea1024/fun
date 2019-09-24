#pragma once

namespace fun {
namespace net {

// class DecompressedFrameNumberArray : public Array<FrameNumber> //{
// public:
//  DecompressedFrameNumberArray()
//  {
//    //UseFastHeap();
//  }
//};
typedef Array<FrameNumber> DecompressedFrameNumberArray;

// TODO 클라에서도 자주 사용하므로, thread-safety는 가져가지 않아도 되려나?
// typedef TUnsafeFastPtr<struct CompressedFrameNumbers>
// CompressedFrameNumbersPtr;
typedef SharedPtr<class CompressedFrameNumbers> CompressedFrameNumbersPtr;

/**
 * 패킷 전송을 위해 압축된 frame number list
 */
class CompressedFrameNumbers {
 private:
  struct Elem {
    FrameNumber left;
    FrameNumber right;
  };

  Array<Elem> array;

 public:
  CompressedFrameNumbers() {
    // array.SetGrowPolicy(GrowBufferPolicy::HighSpeed);
  }

  void AddSortedNumber(FrameNumber n);
  void Decompress(DecompressedFrameNumberArray& dst);
  bool Read(MessageIn& msg);
  void Write(MessageOut& msg);
  inline int32 Count() const { return array.Count(); }
  CompressedFrameNumbersPtr Clone();  // TODO 흠...
};

class RUdpFrame {
 public:
  /** for everyone */
  RUdpFrameType type;

  /** for Normal, Disconnect */
  FrameNumber frame_number;

  /** for Ack */
  CompressedFrameNumbersPtr acked_frame_numbers;

  // TODO 처리되기전까지는 유지되어야하므로, raw -> bytestring으로 처리하면
  // 안되므로 sharedbytestring을 하나 만들어서 유지시켜주어야함.  이름을
  // 뭘로할까?? CByteStringRef에서 참조를 유지하는 형태로 처리하는건 어떨런지??
  //현재는 포인터만 가지고 있는데 말이지... 참조를 유지하는게 여러모로 좋겠지??
  /** for Normal, Disconnect */
  ByteArray data;
  // MessageIn data;

  /** for Ack */
  int32 recent_receive_speed;
  FrameNumber expected_frame_number;

  // TODO 이런거 그다지...
  void CloneTo(RUdpFrame& dst);

  RUdpFrame()
      : acked_frame_numbers(),
        type(RUdpFrameType::None),
        frame_number((FrameNumber)0),
        recent_receive_speed(0),
        expected_frame_number((FrameNumber)0) {}
};

class SenderFrame : public RUdpFrame {
 public:
  double last_send_time;
  double resend_cooltime;
  double first_send_time;
  int32 resend_count;

  inline SenderFrame()
      : RUdpFrame(),
        first_send_time(0),
        last_send_time(0),
        resend_cooltime(0),
        resend_count(0) {}
};

class ReceiverFrame : public RUdpFrame {
 public:
  ReceiverFrame(RUdpFrame& from) : RUdpFrame() {
    // TODO 이런거 그다지...
    from.CloneTo(*this);
  }
};

// TODO FrameNumber -> FrameSequence로 바꾸는게 좋을듯...
class FrameNumberUtil {
 public:
  static inline FrameNumber NextFrameNumber(FrameNumber n) {
    return (FrameNumber)(uint32(n) + 1);
  }

  static inline bool Adjucent(FrameNumber a, FrameNumber b) {
    return NextFrameNumber(a) == b;
  }

  /** sign overflow 문제로 인해 이 메서드 사용이 필수. */
  static inline int32 Compare(FrameNumber a, FrameNumber b) {
    const uint32 diff = uint32(a) - uint32(b);
    if (diff == 0) {
      return 0;
    } else if (diff <= 0xFFFFFFFF / 2) {
      return 1;
    } else {
      return -1;
    }
  }
};

}  // namespace net
}  // namespace fun
