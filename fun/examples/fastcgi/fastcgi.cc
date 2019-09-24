#include "examples/fastcgi/fastcgi.h"
#include "fun/base/logging.h"
#include "fun/net/endian.h"

struct FastCgiCodec::RecordHeader {
  uint8_t version;
  uint8_t type;
  uint16_t id;
  uint16_t length;
  uint8_t padding;
  uint8_t unused;
};

const unsigned FastCgiCodec::kRecordHeader = static_cast<unsigned>(sizeof(FastCgiCodec::RecordHeader));

enum FcgiType {
  kFcgiInvalid = 0,
  kFcgiBeginRequest = 1,
  kFcgiAbortRequest = 2,
  kFcgiEndRequest = 3,
  kFcgiParams = 4,
  kFcgiStdin = 5,
  kFcgiStdout = 6,
  kFcgiStderr = 7,
  kFcgiData = 8,
  kFcgiGetValues = 9,
  kFcgiGetValuesResult = 10,
};

enum FcgiRole {
  // kFcgiInvalid = 0,
  kFcgiResponder = 1,
  kFcgiAuthorizer = 2,
};

enum FcgiConstant {
  kFcgiKeepConn = 1,
};

using namespace fun::net;

bool FastCgiCodec::OnParams(const char* content, uint16_t length) {
  if (length > 0) {
    params_stream_.Append(content, length);
  }
  else if (!ParseAllParams()) {
    LOG_ERROR << "ParseAllParams() failed";
    return false;
  }
  return true;
}

void FastCgiCodec::OnStdin(const char* content, uint16_t length) {
  if (length > 0) {
    stdin_.Append(content, length);
  }
  else {
    got_request_ = true;
  }
}

bool FastCgiCodec::ParseAllParams() {
  while (params_stream_.GetReadableLength() > 0) {
    uint32_t name_len = ReadLength();
    if (name_len == static_cast<uint32_t>(-1)) {
      return false;
    }

    uint32_t value_len = ReadLength();
    if (value_len == static_cast<uint32_t>(-1)) {
      return false;
    }

    if (params_stream_.GetReadableLength() >= name_len + value_len) {
      String name = params_stream_.retrieveAsString(name_len);
      params_[name] = params_stream_.retrieveAsString(value_len);
    }
    else {
      return false;
    }
  }

  return true;
}

uint32_t FastCgiCodec::ReadLength() {
  if (params_stream_.GetReadableLength() >= 1) {
    uint8_t byte = params_stream_.PeekInt8();
    if (byte & 0x80) {
      if (params_stream_.GetReadableLength() >= sizeof(uint32_t)) {
        return params_stream_.ReadInt32() & 0x7fffffff;
      }
      else {
        return -1;
      }
    }
    else {
      return params_stream_.ReadInt8();
    }
  }
  else {
    return -1;
  }
}

using fun::net::Buffer;

void FastCgiCodec::EndStdout(Buffer* buf) {
  RecordHeader header = {
    1,
    kFcgiStdout,
    sockets::hostToNetwork16(1),
    0,
    0,
    0,
  };
  buf->Append(&header, kRecordHeader);
}

void FastCgiCodec::EndRequest(Buffer* buf) {
  RecordHeader header = {
    1,
    kFcgiEndRequest,
    sockets::hostToNetwork16(1),
    sockets::hostToNetwork16(kRecordHeader),
    0,
    0,
  };
  buf->Append(&header, kRecordHeader);
  buf->AppendInt32(0);
  buf->AppendInt32(0);
}

void FastCgiCodec::Respond(Buffer* response) {
  if (response->GetReadableLength() < 65536
      && response->GetPrependableLength() >= kRecordHeader) {
    RecordHeader header = {
      1,
      kFcgiStdout,
      sockets::hostToNetwork16(1),
      sockets::hostToNetwork16(static_cast<uint16_t>(response->GetReadableLength())),
      static_cast<uint8_t>(-response->GetReadableLength() & 7),
      0,
    };
    response->Prepend(&header, kRecordHeader);
    response->Append("\0\0\0\0\0\0\0\0", header.padding);
  }
  else {
    // FIXME:
  }

  EndStdout(response);
  EndRequest(response);
}

bool FastCgiCodec::ParseRequest(Buffer* buf) {
  while (buf->GetReadableLength() >= kRecordHeader) {
    RecordHeader header;
    UnsafeMemory::Memcpy(&header, buf->GetReadablePtr(), kRecordHeader);
    header.id = sockets::networkToHost16(header.id);
    header.length = sockets::networkToHost16(header.length);
    size_t total = kRecordHeader + header.length + header.padding;
    if (buf->GetReadableLength() >= total) {
      switch (header.type) {
        case kFcgiBeginRequest:
          OnBeginRequest(header, buf);
          // FIXME: check
          break;
        case kFcgiParams:
          OnParams(buf->GetReadablePtr() + kRecordHeader, header.length);
          // FIXME: check
          break;
        case kFcgiStdin:
          OnStdin(buf->GetReadablePtr() + kRecordHeader, header.length);
          break;
        case kFcgiData:
          // FIXME:
          break;
        case kFcgiGetValues:
          // FIXME:
          break;
        default:
          // FIXME:
          break;
      }
      buf->Drain(total);
    }
    else {
      break;
    }
  }
  return true;
}

uint16_t ReadInt16(const void* p) {
  uint16_t be16 = 0;
  UnsafeMemory::Memcpy(&be16, p, sizeof be16);
  return sockets::networkToHost16(be16);
}

bool FastCgiCodec::OnBeginRequest(const RecordHeader& header,
                                  const Buffer* buf) {
  fun_check(buf->GetReadableLength() >= header.length);
  fun_check(header.type == kFcgiBeginRequest);

  if (header.length >= kRecordHeader) {
    uint16_t role = ReadInt16(buf->GetReadablePtr() + kRecordHeader);
    uint8_t flags = buf->GetReadablePtr()[kRecordHeader + sizeof(int16_t)];
    if (role == kFcgiResponder) {
      keep_conn_ = flags == kFcgiKeepConn;
      return true;
    }
  }
  return false;
}
