#pragma once

#include <istream>
#include <ostream>
#include "fun/UnbufferedStreamBuf.h"
#include "fun/base.h"
#include "fun/sql/lob.h"

namespace fun {
namespace sql {

/**
 * This is the streambuf class used for reading from and writing to a LOB.
 */
template <typename T>
class LOBStreamBuf : public BasicUnbufferedStreamBuf<T, std::char_traits<T> > {
 public:
  /// Creates LOBStreamBuf.
  LOBStreamBuf(LOB<T>& lob) : lob_(lob), it_(lob_.begin()) {}

  /// Destroys LOBStreamBuf.
  ~LOBStreamBuf() {}

 protected:
  typedef std::char_traits<T> TraitsType;
  typedef BasicUnbufferedStreamBuf<T, TraitsType> BaseType;

  typename BaseType::int_type readFromDevice() {
    if (it_ != lob_.end())
      return BaseType::charToInt(*it_++);
    else
      return -1;
  }

  typename BaseType::int_type writeToDevice(T c) {
    lob_.AppendRaw(&c, 1);
    return 1;
  }

 private:
  LOB<T>& lob_;
  typename LOB<T>::Iterator it_;
};

/// The base class for LOBInputStream and
/// LOBOutputStream.
///
/// This class is needed to ensure the correct initialization
/// order of the stream buffer and base classes.
template <typename T>
class LOBIOS : public virtual std::ios {
 public:
  /// Creates the LOBIOS with the given LOB.
  LOBIOS(LOB<T>& lob, openmode mode) : _buf(lob) { fun_ios_init(&_buf); }

  /// Destroys the LOBIOS.
  ~LOBIOS() {}

  /// Returns a pointer to the internal LOBStreamBuf.
  LOBStreamBuf<T>* rdbuf() { return &_buf; }

 protected:
  LOBStreamBuf<T> _buf;
};

/// An output stream for writing to a LOB.
template <typename T>
class LOBOutputStream : public LOBIOS<T>,
                        public std::basic_ostream<T, std::char_traits<T> > {
 public:
  /// Creates the LOBOutputStream with the given LOB.
  LOBOutputStream(LOB<T>& lob)
      : LOBIOS<T>(lob, std::ios::out), std::ostream(LOBIOS<T>::rdbuf()) {}

  /// Destroys the LOBOutputStream.
  ~LOBOutputStream() {}
};

/// An input stream for reading from a LOB.
template <typename T>
class LOBInputStream : public LOBIOS<T>,
                       public std::basic_istream<T, std::char_traits<T> > {
 public:
  /// Creates the LOBInputStream with the given LOB.
  LOBInputStream(LOB<T>& lob)
      : LOBIOS<T>(lob, std::ios::in), std::istream(LOBIOS<T>::rdbuf()) {}

  /// Destroys the LOBInputStream.
  ~LOBInputStream() {}
};

typedef LOBOutputStream<unsigned char> BLOBOutputStream;
typedef LOBOutputStream<char> CLOBOutputStream;

typedef LOBInputStream<unsigned char> BLOBInputStream;
typedef LOBInputStream<char> CLOBInputStream;

}  // namespace sql
}  // namespace fun
