// Copyright 2011, Shuo Chen.  All rights reserved.
// http://code.google.com/p/red/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "codec.h"

#include "fun/base/logging.h"
#include <red/net/Endian.h>
#include <red/net/protorpc/google-inl.h>

#include <google/protobuf/descriptor.h>

#include <zlib.h>  // adler32

using namespace fun;
using namespace fun::net;


void ProtobufCodec::FillEmptyBuffer(Buffer* buf,
                                    const google::protobuf::Message& message)
{
  // buf->DrainAll();
  fun_check(buf->GetReadableLength() == 0);

  const String& type_name = message.GetTypeName();
  int32_t name_len = static_cast<int32_t>(type_name.size()+1);
  buf->AppendInt32(name_len);
  buf->Append(type_name.c_str(), name_len);

  // code copied from MessageLite::SerializeToArray() and MessageLite::SerializePartialToArray().
  GOOGLE_DCHECK(message.IsInitialized()) << InitializationErrorMessage("serialize", message);

  int byte_size = message.ByteSize();
  buf->ensureWritableBytes(byte_size);

  uint8_t* start = reinterpret_cast<uint8_t*>(buf->beginWrite());
  uint8_t* end = message.SerializeWithCachedSizesToArray(Start);
  if (end - start != byte_size)
  {
    ByteSizeConsistencyError(byte_size, message.ByteSize(), static_cast<int>(end - start));
  }
  buf->hasWritten(byte_size);

  int32_t chksum = static_cast<int32_t>(
      ::adler32(1,
                reinterpret_cast<const Bytef*>(buf->GetReadablePtr()),
                static_cast<int>(buf->GetReadableLength())));
  buf->AppendInt32(chksum);
  fun_check(buf->GetReadableLength() == sizeof name_len + name_len + byte_size + sizeof chksum);
  int32_t len = sockets::hostToNetwork32(static_cast<int32_t>(buf->GetReadableLength()));
  buf->Prepend(&len, sizeof len);
}

//
// no more google code after this
//

//
// FIXME: merge with RpcCodec
//

namespace
{
  const String kNoErrorStr = "NoError";
  const String kInvalidLengthStr = "InvalidLength";
  const String kCheckSumErrorStr = "CheckSumError";
  const String kInvalidNameLenStr = "InvalidNameLen";
  const String kUnknownMessageTypeStr = "UnknownMessageType";
  const String kParseErrorStr = "ParseError";
  const String kUnknownErrorStr = "UnknownError";
}

const String& ProtobufCodec::ErrorCodeToString(ErrorCode error_code)
{
  switch (error_code) {
   case kNoError:
     return kNoErrorStr;
   case kInvalidLength:
     return kInvalidLengthStr;
   case kCheckSumError:
     return kCheckSumErrorStr;
   case kInvalidNameLen:
     return kInvalidNameLenStr;
   case kUnknownMessageType:
     return kUnknownMessageTypeStr;
   case kParseError:
     return kParseErrorStr;
   default:
     return kUnknownErrorStr;
  }
}

void ProtobufCodec::DefaultErrorCallback(const fun::net::TcpConnectionPtr& conn,
                                         fun::net::Buffer* buf,
                                         const fun::Timestamp&,
                                         ErrorCode error_code)
{
  LOG_ERROR << "ProtobufCodec::DefaultErrorCallback - " << ErrorCodeToString(error_code);
  if (conn && conn->IsConnected())
  {
    conn->Shutdown();
  }
}

int32_t AsInt32(const char* buf)
{
  int32_t be32 = 0;
  UnsafeMemory::Memcpy(&be32, buf, sizeof(be32));
  return sockets::networkToHost32(be32);
}

void ProtobufCodec::OnMessage(const TcpConnectionPtr& conn,
                              Buffer* buf,
                              const Timestamp& received_time)
{
  while (buf->GetReadableLength() >= kMinMessageLen + kHeaderLen)
  {
    const int32_t len = buf->peekInt32();
    if (len > kMaxMessageLen || len < kMinMessageLen)
    {
      error_cb_(conn, buf, received_time, kInvalidLength);
      break;
    }
    else if (buf->GetReadableLength() >= ImplicitCast<size_t>(len + kHeaderLen))
    {
      ErrorCode error_code = kNoError;
      MessagePtr message = Parse(buf->GetReadablePtr()+kHeaderLen, len, &error_code);
      if (error_code == kNoError && message)
      {
        message_cb_(conn, message, received_time);
        buf->Drain(kHeaderLen+len);
      }
      else
      {
        error_cb_(conn, buf, received_time, error_code);
        break;
      }
    }
    else
    {
      break;
    }
  }
}

google::protobuf::Message*
  ProtobufCodec::CreateMessage(const String& type_name)
{
  google::protobuf::Message* message = NULL;
  const google::protobuf::Descriptor* descriptor =
    google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(type_name);
  if (descriptor)
  {
    const google::protobuf::Message* prototype =
      google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
    if (prototype)
    {
      message = prototype->New();
    }
  }
  return message;
}

MessagePtr ProtobufCodec::Parse(const char* buf, int len, ErrorCode* error)
{
  MessagePtr message;

  // check sum
  int32_t expected_chksum = AsInt32(buf + len - kHeaderLen);
  int32_t chksum = static_cast<int32_t>(
      ::adler32(1,
                reinterpret_cast<const Bytef*>(buf),
                static_cast<int>(len - kHeaderLen)));
  if (chksum == expected_chksum)
  {
    // get message type name
    int32_t name_len = AsInt32(buf);
    if (name_len >= 2 && name_len <= len - 2*kHeaderLen)
    {
      String type_name(buf + kHeaderLen, buf + kHeaderLen + name_len - 1);
      // create message object
      message.Reset(CreateMessage(type_name));
      if (message)
      {
        // Parse from buffer
        const char* data = buf + kHeaderLen + name_len;
        int32_t data_len = len - name_len - 2*kHeaderLen;
        if (message->ParseFromArray(data, data_len))
        {
          *error = kNoError;
        }
        else
        {
          *error = kParseError;
        }
      }
      else
      {
        *error = kUnknownMessageType;
      }
    }
    else
    {
      *error = kInvalidNameLen;
    }
  }
  else
  {
    *error = kCheckSumError;
  }

  return message;
}
