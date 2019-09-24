#pragma once

namespace fun {
namespace net {

FUN_MESSAGEFIELD_ENUM_FXED8(direct_p2p_start_condition);
FUN_MESSAGEFIELD_ENUM_FXED8(EncryptionMode);
FUN_MESSAGEFIELD_ENUM_FXED8(FallbackMethod);
FUN_MESSAGEFIELD_ENUM_VARINT(FrameNumber);
FUN_MESSAGEFIELD_ENUM_VARINT(HostId);
FUN_MESSAGEFIELD_ENUM_FXED8(LocalEventType);
FUN_MESSAGEFIELD_ENUM_FXED8(LogCategory);
FUN_MESSAGEFIELD_ENUM_FXED8(MessagePriority);
FUN_MESSAGEFIELD_ENUM_FXED8(MessageType);
FUN_MESSAGEFIELD_ENUM_FXED8(RUdpFrameType);
FUN_MESSAGEFIELD_ENUM_FXED8(ResultCode);
FUN_MESSAGEFIELD_ENUM_VARINT(RpcId);

//TODO TArray를 WellKownType으로 처리하고 있으므로 구지 필요 없을듯??
template <> struct MessageFieldTypeTraits<HostIdArray> {
  typedef HostIdArray CppValueType;

  inline static void Write(IMessageOut& output, const CppValueType& value) {
    const OptimalCounter32 count = value.Count();
    LiteFormat::Write(output, count);

    for (const auto& element : value) {
      LiteFormat::Write(output, element);
    }
  }

  inline static bool Read(IMessageIn& input, CppValueType& out_value) {
    out_value.Clear();

    OptimalCounter32 count;
    FUN_DO_CHECKED(LiteFormat::Read(input, count));

    out_value.Resize(count);
    for (int32 i = 0; i < count; ++i) {
      FUN_DO_CHECKED(LiteFormat::Read(input, out_value[i]));
    }

    return true;
  }
};

template <> struct MessageFieldTypeTraits<RelayDest> {
  typedef RelayDest CppValueType;

  inline static void Write(IMessageOut& output, const CppValueType& value) {
    LiteFormat::Write(output, value.send_to);
    LiteFormat::Write(output, value.frame_number);
  }

  inline static bool Read(IMessageIn& input, CppValueType& out_value) {
    FUN_DO_CHECKED(LiteFormat::Read(input, out_value.send_to));
    FUN_DO_CHECKED(LiteFormat::Read(input, out_value.frame_number));
    return true;
  }
};

//TODO TArray를 WellKownType으로 처리하고 있으므로 구지 필요 없을듯??
template <> struct MessageFieldTypeTraits<RelayDestList> {
  typedef RelayDestList CppValueType;

  inline static void Write(IMessageOut& output, const CppValueType& value) {
    const OptimalCounter32 count = value.Count();
    LiteFormat::Write(output, count);

    for (const auto& element : value) {
      LiteFormat::Write(output, element);
    }
  }

  inline static bool Read(IMessageIn& input, CppValueType& out_value) {
    out_value.Clear();

    OptimalCounter32 count;
    FUN_DO_CHECKED(LiteFormat::Read(input, count));

    out_value.Resize(count);
    for (int32 i = 0; i < count; ++i) {
      FUN_DO_CHECKED(LiteFormat::Read(input, out_value[i]));
    }

    return true;
  }
};

//warning
//쓰이는데가 한곳밖에 없기도 하고, 카운터가 들어가는 문제로 인해서 메시지를 읽어들이는데 문제가 있을수 있으므로
//제거하자.
//template <> struct MessageFieldTypeTraits<SendFragRefs> {
//  typedef SendFragRefs CppValueType;
//
//  inline static void Write(IMessageOut& output, const CppValueType& value) {
//    //FIXED 이 카운터가 의도치 않게 들어가다 보니, 메시지를 잘못읽어들이는 문제가 있었음.
//    const OptimalCounter32 count = value.Count();
//    LiteFormat::Write(output, count);
//
//    for (int32 i = 0; i < count; ++i) {
//      output.WriteRawBytes(value[i].Data, value[i].Length);
//    }
//  }
//
//  static bool Read(IMessageIn& input, CppValueType& out_value) = delete;
//};

} // namespace net
} // namespace fun
