#pragma once

#include "fun/base/atomic.h"
#include "fun/base/string_piece.h"
#include "fun/base/types.h"

#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

using String;
using fun::StringPiece;

namespace fun {
namespace net {
class Buffer;
}
}  // namespace fun

class Item;
typedef fun::SharedPtr<Item> ItemPtr;             // TODO: use unique_ptr
typedef fun::SharedPtr<const Item> ConstItemPtr;  // TODO: use unique_ptr

// Item is immutable once added into hash table
class Item : Noncopyable {
 public:
  enum UpdatePolicy {
    kInvalid,
    kSet,
    kAdd,
    kReplace,
    kAppend,
    kPrepend,
    kCas,
  };

  static ItemPtr makeItem(StringPiece keyArg, uint32_t flagsArg, int exptimeArg,
                          int valuelen, uint64_t casArg) {
    return boost::make_shared<Item>(keyArg, flagsArg, exptimeArg, valuelen,
                                    casArg);
    // return ItemPtr(new Item(keyArg, flagsArg, exptimeArg, valuelen, casArg));
  }

  Item(StringPiece keyArg, uint32_t flagsArg, int exptimeArg, int valuelen,
       uint64_t casArg);

  ~Item() { ::free(data_); }

  fun::StringPiece key() const { return fun::StringPiece(data_, keylen_); }

  uint32_t flags() const { return flags_; }

  int rel_exptime() const { return rel_exptime_; }

  const char* value() const { return data_ + keylen_; }

  size_t valueLength() const { return value_len_; }

  uint64_t cas() const { return cas_; }

  size_t hash() const { return hash_; }

  void setCas(uint64_t casArg) { cas_ = casArg; }

  size_t neededBytes() const { return totalLen() - received_bytes_; }

  void append(const char* data, size_t len);

  bool EndsWithCRLF() const {
    return received_bytes_ == totalLen() && data_[totalLen() - 2] == '\r' &&
           data_[totalLen() - 1] == '\n';
  }

  void output(fun::net::Buffer* out, bool needCas = false) const;

  void resetKey(StringPiece k);

 private:
  int totalLen() const { return keylen_ + value_len_; }

  int keylen_;
  const uint32_t flags_;
  const int rel_exptime_;
  const int value_len_;
  int received_bytes_;  // FIXME: remove this member
  uint64_t cas_;
  size_t hash_;
  char* data_;
};
