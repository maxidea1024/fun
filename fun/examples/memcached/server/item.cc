#include "item.h"

#include "fun/base/log_stream.h"
#include "fun/net/buffer.h"

#include <boost/unordered_map.hpp>

#include <String.h> // UnsafeMemory::Memcpy
#include <stdio.h>

using namespace fun;
using namespace fun::net;

Item::Item(StringPiece keyArg,
           uint32_t flagsArg,
           int exptimeArg,
           int valuelen,
           uint64_t casArg)
  : keylen_(keyArg.size())
  , flags_(flagsArg)
  , rel_exptime_(exptimeArg)
  , value_len_(valuelen)
  , received_bytes_(0)
  , cas_(casArg)
  , hash_(boost::hash_range(keyArg.begin(), keyArg.end()))
  , data_(static_cast<char*>(::malloc(totalLen()))) {
  fun_check(value_len_ >= 2);
  fun_check(received_bytes_ < totalLen());
  append(keyArg.data(), keylen_);
}

void Item::append(const char* data, size_t len) {
  fun_check(len <= neededBytes());
  UnsafeMemory::Memcpy(data_ + received_bytes_, data, len);
  received_bytes_ += static_cast<int>(len);
  fun_check(received_bytes_ <= totalLen());
}

void Item::output(Buffer* out, bool needCas) const {
  out->append("VALUE ");
  out->append(data_, keylen_);
  LogStream buf;
  buf << ' ' << flags_ << ' ' << value_len_-2;
  if (needCas) {
    buf << ' ' << cas_;
  }
  buf << "\r\n";
  out->append(buf.buffer().data(), buf.buffer().length());
  out->append(value(), value_len_);
}

void Item::resetKey(StringPiece k) {
  fun_check(k.size() <= 250);
  keylen_ = k.size();
  received_bytes_ = 0;
  append(k.data(), k.size());
  hash_ = boost::hash_range(k.begin(), k.end());
}
