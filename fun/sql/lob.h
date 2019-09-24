#pragma once

#include <algorithm>
#include <vector>
#include "fun/base/dynamic/var_holder.h"
#include "fun/base/exception.h"
#include "fun/base/shared_ptr.h"
#include "fun/sql/sql.h"

namespace fun {
namespace sql {

/**
 * Representation of a Large OBject.
 *
 * A LOB can hold arbitrary data.
 * The maximum size depends on the underlying database.
 *
 * The LOBInputStream and LOBOutputStream classes provide
 * a convenient way to access the data in a LOB.
 */
template <typename T>
class LOB {
 public:
  typedef typename std::vector<T>::const_iterator Iterator;
  typedef T ValueType;
  typedef typename std::vector<T> Container;
  typedef fun::SharedPtr<Container> ContentPtr;

  /**
   * Creates an empty LOB.
   */
  LOB() : content_(new std::vector<T>()) {}

  /**
   * Creates the LOB, content is deep-copied.
   */
  LOB(const std::vector<T>& content) : content_(new std::vector<T>(content)) {}

  /**
   * Creates the LOB by deep-copying content.
   */
  LOB(const T* const content, size_t size)
      : content_(new std::vector<T>(content, content + size)) {}

  /**
   * Creates a LOB from a string.
   */
  LOB(const std::basic_string<T>& content)
      : content_(new std::vector<T>(content.begin(), content.end())) {}

  /**
   * Creates a LOB by copying another one.
   */
  LOB(const LOB& other) : content_(other.content_) {}

  /**
   * Destroys the LOB.
   */
  ~LOB() {}

  /**
   * Assignment operator.
   */
  LOB& operator=(const LOB& other) {
    LOB tmp(other);
    Swap(tmp);
    return *this;
  }

  /**
   * Compares for equality LOB by value.
   */
  bool operator==(const LOB& other) const {
    return *content_ == *other.content_;
  }

  /**
   * Compares for inequality LOB by value.
   */
  bool operator!=(const LOB& other) const {
    return *content_ != *other.content_;
  }

  /**
   * Swaps the LOB with another one.
   */
  void Swap(LOB& other) { fun::Swap(content_, other.content_); }

  /**
   * Returns the content.
   */
  const std::vector<T>& GetContent() const { return *content_; }

  /**
   * Returns the raw content.
   *
   * If the LOB is empty, returns NULL.
   */
  const T* GetRawContent() const {
    if (content_->IsEmpty()) {
      return nullptr;
    } else {
      return &(*content_)[0];
    }
  }

  /**
   * Assigns raw content to internal storage.
   */
  void AssignValue(size_t count, const T& val) {
    ContentPtr tmp = new Container(count, val);
    content_.Swap(tmp);
  }

  /**
   * Assigns raw content to internal storage.
   */
  void AssignRaw(const T* ptr, size_t count) {
    fun_check_dbg(ptr);
    LOB tmp(ptr, count);
    Swap(tmp);
  }

  /**
   * Assigns raw content to internal storage.
   */
  void AppendRaw(const T* pChar, size_t count) {
    fun_check_dbg(pChar);
    content_->insert(content_->end(), pChar, pChar + count);
  }

  /**
   * Clears the content of the blob.
   * If doCompact is true, trims the excess capacity.
   */
  void Clear(bool compaction = false) {
    content_->Clear();

    if (compaction) {
      Compact();
    }
  }

  /**
   * Trims the internal storage excess capacity.
   */
  void Compact() { std::vector<T>(*content_).Swap(*content_); }

  Iterator begin() const { return content_->begin(); }

  Iterator end() const { return content_->end(); }

  /**
   * Returns the size of the LOB in bytes.
   */
  size_t size() const { return static_cast<size_t>(content_->size()); }

 private:
  ContentPtr content_;
};

typedef LOB<unsigned char> BLOB;
typedef LOB<char> CLOB;

//
// inlines
//

template <typename T>
inline void Swap(LOB<T>& b1, LOB<T>& b2) {
  b1.Swap(b2);
}

}  // namespace sql
}  // namespace fun

namespace std {

/**
 * Full template specialization of std:::Swap for BLOB
 */
template <>
inline void Swap<fun::sql::BLOB>(fun::sql::BLOB& b1, fun::sql::BLOB& b2) {
  b1.Swap(b2);
}

/**
 * Full template specialization of std:::Swap for CLOB
 */
template <>
inline void Swap<fun::sql::CLOB>(fun::sql::CLOB& c1, fun::sql::CLOB& c2) {
  c1.Swap(c2);
}

}  // namespace std

//
// VarHolderImpl<LOB>
//

namespace fun {
namespace Dynamic {

template <>
class VarHolderImpl<fun::sql::BLOB> : public VarHolder {
 public:
  VarHolderImpl(const fun::sql::BLOB& val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const { return typeid(fun::sql::BLOB); }

  void Convert(String& val) const { val.Assign(val_.begin(), val_.end()); }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const fun::sql::BLOB& Value() const { return val_; }

 private:
  VarHolderImpl();

  fun::sql::BLOB val_;
};

template <>
class VarHolderImpl<fun::sql::CLOB> : public VarHolder {
 public:
  VarHolderImpl(const fun::sql::CLOB& val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const { return typeid(fun::sql::CLOB); }

  void Convert(String& val) const { val.Assign(val_.begin(), val_.end()); }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const fun::sql::CLOB& Value() const { return val_; }

 private:
  VarHolderImpl();

  fun::sql::CLOB val_;
};

}  // namespace Dynamic
}  // namespace fun
