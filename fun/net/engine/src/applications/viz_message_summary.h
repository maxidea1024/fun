#pragma once

namespace fun {
namespace net {

/**
 * VizMessageSummary 별개로 쓴다. RpcName이 성능을 먹으므로.
 */
class VizMessageSummary
{
 public:
  /** Byte length of payload. */
  int32 payload_length;

  /** RPC protocol ID. */
  RpcId rpc_id;

  //TODOraw 포인터로 처리하거나, rpc이름 자체가 아예 CString이면 복사를 줄일 수 있을듯 싶은데...
  /** RPC funciton name. */
  String rpc_name;

  /** Encryption mode. */
  EncryptionMode encryption_mode;

  /** Compression mode. */
  CompressionMode compression_mode;

 public:
  /** Default constructor. */
  inline VizMessageSummary()
    : payload_length(0)
    , rpc_id(RpcId_None)
    , encryption_mode(EncryptionMode::None)
    , compression_mode(CompressionMode::None)
  {
  }

  /** Copy constructor. */
  inline VizMessageSummary(const MessageSummary& rhs)
    : payload_length(rhs.payload_length)
    , rpc_id(rhs.rpc_id)
    , rpc_name(rhs.rpc_name)
    , encryption_mode(rhs.encryption_mode)
    , compression_mode(rhs.compression_mode)
  {
  }
};

template <> struct MessageFieldTypeTraits<VizMessageSummary>
{
  typedef VizMessageSummary CppValueType;

  inline static void Write(IMessageOut& output, const CppValueType& value)
  {
    LiteFormat::Write(output, value.payload_length);
    LiteFormat::Write(output, value.rpc_id);
    LiteFormat::Write(output, value.rpc_name);
    LiteFormat::Write(output, value.encryption_mode);
    LiteFormat::Write(output, value.compression_mode);
  }

  inline static bool Read(IMessageIn& input, CppValueType& out_value)
  {
    FUN_DO_CHECKED(LiteFormat::Read(input, out_value.payload_length));
    FUN_DO_CHECKED(LiteFormat::Read(input, out_value.rpc_id));
    FUN_DO_CHECKED(LiteFormat::Read(input, out_value.rpc_name));
    FUN_DO_CHECKED(LiteFormat::Read(input, out_value.encryption_mode));
    FUN_DO_CHECKED(LiteFormat::Read(input, out_value.compression_mode));
    return true;
  }
};

inline static String ToString(const VizMessageSummary& v)
{
  String result;
  result << "{";
  result << "payload_length:" << ToString(v.payload_length);
  result << ",rpc_id:" << ToString(v.rpc_id);
  result << ",rpc_name:" << ToString(v.rpc_name);
  result << ",encryption_mode:" << ToString(v.encryption_mode);
  result << ",compression_mode:" << ToString(v.compression_mode);
  result << "}";
  return result;
}

} // namespace net
} // namespace fun
