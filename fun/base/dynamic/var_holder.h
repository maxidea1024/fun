#pragma once

#include "fun/base/base.h"
#include "fun/base/number_formatter.h"
#include "fun/base/number_parser.h"
#include "fun/base/date_time.h"
#include "fun/base/timestamp.h"
#include "fun/base/local_date_time.h"
#include "fun/base/date_time_format.h"
#include "fun/base/date_time_formatter.h"
#include "fun/base/date_time_parser.h"
#include "fun/base/string.h"
#include "fun/base/unicode_converter.h"
#include "fun/base/utf_string.h"
#include "fun/base/utf8_string.h"
#include "fun/base/any.h"
#include "fun/base/exception.h"

#include <vector>
#include <list>
#include <deque>
#include <typeinfo>

#undef min
#undef max
#include <limits>

namespace fun {
namespace dynamic {

class Var;

namespace Impl {

/**
 * Returns true for values that should be JSON-formatted as string.
 */
bool FUN_BASE_API IsJsonString(const Var& any);

/**
 * Converts the any to a JSON key (i.e. wraps it into double quotes
 * regardless of the underlying type) and appends it to val.
 */
void FUN_BASE_API AppendJsonKey(String& val, const Var& any);

/**
 * Converts the any to a JSON string (i.e. wraps it into double quotes)
 * regardless of the underlying type) and appends it to val.
 */
void FUN_BASE_API AppendJsonString(String& val, const Var& any);

/**
 * Converts the any to a JSON value (if underlying type qualifies
 * as string - see IsJsonString() - , it is wrapped into double quotes)
 * and appends it to val
 */
void FUN_BASE_API AppendJsonValue(String& val, const Var& any);

template <typename C>
void ContainerToJson(C& cont, String& val) {
  // Serialize in JSON format. Note: although this is a vector<T>, the code only
  // supports vector<Var>. Total specialization is not possible
  // because of the cyclic dependency between Var and VarHolder

  // JSON format definition: [ n times: elem ',' ], no ',' for last elem
  val.Append("[ ");

  typename C::const_iterator it = cont.begin();
  typename C::const_iterator itEnd = cont.end();
  if (!cont.IsEmpty()) {
    AppendJsonValue(val, *it);
    ++it;
  }

  for (; it != itEnd; ++it) {
    val.Append(", ");
    AppendJsonValue(val, *it);
  }

  val.Append(" ]");
}

} // namespace Impl

/**
 * Interface for a data holder used by the Var class.
 * Provides methods to Convert between data types.
 * Only data types for which VarHolder specialization exists are supported.
 *
 * Provided are specializations for all C++ built-in types with addition of
 * String, UString, DateTime, Timestamp, std::vector<Var> and DynamicStruct.
 *
 * Additional types can be supported by adding specializations. When implementing specializations,
 * the only condition is that they reside in fun namespace and implement the pure virtual functions
 * Clone() and Type().
 *
 * Those conversions that are not implemented shall fail back to this base
 * class implementation. All the Convert() function overloads in this class
 * throw BadCastException.
 */
class FUN_BASE_API VarHolder {
 public:
  typedef Var ArrayValueType;

  /**
   * Destroys the VarHolder.
   */
  virtual ~VarHolder();

  /**
   * Implementation must implement this function to
   * deep-copy the VarHolder.
   * If small object optimization is enabled (i.e. if
   * FUN_NO_SOO is not defined), VarHolder will be
   * instantiated in-place if it's size is smaller
   * than FUN_SMALL_OBJECT_SIZE.
   */
  virtual VarHolder* Clone(Placeholder<VarHolder>* holder = nullptr) const = 0;

  /**
   * Implementation must return the type information
   * (typeid) for the stored content.
   */
  virtual const std::type_info& Type() const = 0;

  /**
   * Throws BadCastException. Must be overriden in a type
   * specialization in order to support the conversion.
   */
  virtual void Convert(int8& val) const;

  /**
   * Throws BadCastException. Must be overriden in a type
   * specialization in order to support the conversion.
   */
  virtual void Convert(int16& val) const;

  /**
   * Throws BadCastException. Must be overriden in a type
   * specialization in order to support the conversion.
   */
  virtual void Convert(int32& val) const;

  /**
   * Throws BadCastException. Must be overriden in a type
   * specialization in order to support the conversion.
   */
  virtual void Convert(int64& val) const;

  /**
   * Throws BadCastException. Must be overriden in a type
   * specialization in order to support the conversion.
   */
  virtual void Convert(uint8& val) const;

  /**
   * Throws BadCastException. Must be overriden in a type
   * specialization in order to support the conversion.
   */
  virtual void Convert(uint16& val) const;

  /**
   * Throws BadCastException. Must be overriden in a type
   * specialization in order to support the conversion.
   */
  virtual void Convert(uint32& val) const;

  /**
   * Throws BadCastException. Must be overriden in a type
   * specialization in order to support the conversion.
   */
  virtual void Convert(uint64& val) const;

  /**
   * Throws BadCastException. Must be overriden in a type
   * specialization in order to support the conversion.
   */
  virtual void Convert(DateTime& val) const;

  /**
   * Throws BadCastException. Must be overriden in a type
   * specialization in order to support the conversion.
   */
  virtual void Convert(Timestamp& val) const;

#ifndef FUN_LONG_IS_64_BIT
  /**
   * Calls Convert(int32).
   */
  void Convert(long& val) const;

  /**
   * Calls Convert(uint32).
   */
  void Convert(unsigned long& val) const;
#endif

  /**
   * Throws BadCastException. Must be overriden in a type
   * specialization in order to support the conversion.
   */
  virtual void Convert(bool& val) const;

  /**
   * Throws BadCastException. Must be overriden in a type
   * specialization in order to support the conversion.
   */
  virtual void Convert(float& val) const;

  /**
   * Throws BadCastException. Must be overriden in a type
   * specialization in order to support the conversion.
   */
  virtual void Convert(double& val) const;

  /**
   * Throws BadCastException. Must be overriden in a type
   * specialization in order to support the conversion.
   */
  virtual void Convert(char& val) const;

  /**
   * Throws BadCastException. Must be overriden in a type
   * specialization in order to support the conversion.
   */
  virtual void Convert(String& val) const;

  /**
   * Throws BadCastException. Must be overriden in a type
   * specialization in order to support the conversion.
   */
  virtual void Convert(UString& val) const;

  /**
   * Returns true.
   */
  virtual bool IsArray() const;

  /**
   * Returns false. Must be properly overriden in a type
   * specialization in order to support the diagnostic.
   */
  virtual bool IsVector() const;

  /**
   * Returns false. Must be properly overriden in a type
   * specialization in order to support the diagnostic.
   */
  virtual bool IsList() const;

  /**
   * Returns false. Must be properly overriden in a type
   * specialization in order to support the diagnostic.
   */
  virtual bool IsDeque() const;

  /**
   * Returns false. Must be properly overriden in a type
   * specialization in order to support the diagnostic.
   */
  virtual bool IsStruct() const;

  /**
   * Returns false. Must be properly overriden in a type
   * specialization in order to support the diagnostic.
   */
  virtual bool IsOrdered() const;

  /**
   * Returns false. Must be properly overriden in a type
   * specialization in order to support the diagnostic.
   */
  virtual bool IsInteger() const;

  /**
   * Returns false. Must be properly overriden in a type
   * specialization in order to support the diagnostic.
   */
  virtual bool IsSigned() const;

  /**
   * Returns false. Must be properly overriden in a type
   * specialization in order to support the diagnostic.
   */
  virtual bool IsNumeric() const;

  /**
   * Returns false. Must be properly overriden in a type
   * specialization in order to support the diagnostic.
   */
  virtual bool IsBoolean() const;

  /**
   * Returns false. Must be properly overriden in a type
   * specialization in order to support the diagnostic.
   */
  virtual bool IsString() const;

  /**
   * Returns false. Must be properly overriden in a type
   * specialization in order to support the diagnostic.
   */
  virtual bool IsDate() const;

  /**
   * Returns false. Must be properly overriden in a type
   * specialization in order to support the diagnostic.
   */
  virtual bool IsTime() const;

  /**
   * Returns false. Must be properly overriden in a type
   * specialization in order to support the diagnostic.
   */
  virtual bool IsDateTime() const;

  /**
   * Returns 1 iff Var is not empty or this function overriden.
   */
  virtual size_t size() const;

 protected:
  /**
   * Creates the VarHolder.
   */
  VarHolder();

  /**
   * Instantiates value holder wrapper. If size of the wrapper is
   * larger than FUN_SMALL_OBJECT_SIZE, holder is instantiated on
   * the heap, otherwise it is instantiated in-place (in the
   * pre-allocated buffer inside the holder).
   *
   * Called from Clone() member function of the implementation when
   * small object optimization is enabled.
   */
  template <typename T>
  VarHolder* CloneHolder(Placeholder<VarHolder>* var_holder, const T& val) const {
#ifdef FUN_NO_SOO
    (void)var_holder;
    return new VarHolderImpl<T>(val);
#else
    fun_check_ptr(var_holder);

    if ((sizeof(VarHolderImpl<T>) <= Placeholder<T>::Size::Value)) {
      new((VarHolder*) var_holder->holder) VarHolderImpl<T>(val);
      var_holder->SetLocal(true);
      return (VarHolder*)var_holder->holder;
    } else {
      var_holder->holder_ptr = new VarHolderImpl<T>(val);
      var_holder->SetLocal(false);
      return var_holder->holder_ptr;
    }
#endif
  }

  /**
   * This function is meant to Convert signed numeric values from
   * larger to smaller type. It checks the upper and lower bound and
   * if from value is within limits of type T (i.e. check calls do not throw),
   * it is converted.
   */
  template <typename F, typename T>
  void ConvertToSmaller(const F& from, T& to) const {
    fun_static_assert(std::numeric_limits<F>::is_specialized);
    fun_static_assert(std::numeric_limits<T>::is_specialized);
    fun_static_assert(std::numeric_limits<F>::is_signed);
    fun_static_assert(std::numeric_limits<T>::is_signed);

    if (std::numeric_limits<F>::is_integer) {
      CheckUpperLimit<F,T>(from);
      CheckLowerLimit<F,T>(from);
    } else {
      CheckUpperLimitFloat<F,T>(from);
      CheckLowerLimitFloat<F,T>(from);
    }

    to = static_cast<T>(from);
  }

  /**
   * This function is meant for converting unsigned integral data types,
   * from larger to smaller type. Since lower limit is always 0 for unsigned types,
   * only the upper limit is checked, thus saving some cycles compared to the signed
   * version of the function. If the value to be converted is smaller than
   * the maximum value for the target type, the conversion is performed.
   */
  template <typename F, typename T>
  void ConvertToSmallerUnsigned(const F& from, T& to) const {
    fun_static_assert(std::numeric_limits<F>::is_specialized);
    fun_static_assert(std::numeric_limits<T>::is_specialized);
    fun_static_assert(!std::numeric_limits<F>::is_signed);
    fun_static_assert(!std::numeric_limits<T>::is_signed);

    CheckUpperLimit<F,T>(from);
    to = static_cast<T>(from);
  }

  /**
   * This function is meant for converting signed integral data types to
   * unsigned data types. Negative values can not be converted and if one
   * is encountered, RangeException is thrown.
   * If upper limit is within the target data type limits, the conversion is performed.
   */
  template <typename F, typename T>
  void ConvertSignedToUnsigned(const F& from, T& to) const {
    fun_static_assert(std::numeric_limits<F>::is_specialized);
    fun_static_assert(std::numeric_limits<T>::is_specialized);
    fun_static_assert(std::numeric_limits<F>::is_signed);
    fun_static_assert(!std::numeric_limits<T>::is_signed);

    if (from < 0) {
      throw RangeException("Value too small.");
    }

    CheckUpperLimit<F,T>(from);
    to = static_cast<T>(from);
  }

  /**
   * This function is meant for converting floating point data types to
   * unsigned integral data types. Negative values can not be converted and if one
   * is encountered, RangeException is thrown.
   * If upper limit is within the target data type limits, the conversion is performed.
   */
  template <typename F, typename T>
  void ConvertSignedFloatToUnsigned(const F& from, T& to) const
    fun_static_assert(std::numeric_limits<F>::is_specialized);
    fun_static_assert(std::numeric_limits<T>::is_specialized);
    fun_static_assert(!std::numeric_limits<F>::is_integer);
    fun_static_assert(std::numeric_limits<T>::is_integer);
    fun_static_assert(!std::numeric_limits<T>::is_signed);

    if (from < 0) {
      throw RangeException("Value too small.");
    }

    CheckUpperLimitFloat<F,T>(from);
    to = static_cast<T>(from);
  }

  /**
   * This function is meant for converting unsigned integral data types to
   * signed integral data types. Negative values can not be converted and if one
   * is encountered, RangeException is thrown.
   * If upper limit is within the target data type limits, the conversion is performed.
   */
  template <typename F, typename T>
  void ConvertUnsignedToSigned(const F& from, T& to) const
    fun_static_assert(std::numeric_limits<F>::is_specialized);
    fun_static_assert(std::numeric_limits<T>::is_specialized);
    fun_static_assert(!std::numeric_limits<F>::is_signed);
    fun_static_assert(std::numeric_limits<T>::is_signed);

    CheckUpperLimit<F,T>(from);
    to = static_cast<T>(from);
  }

 private:

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4127)
#pragma warning(disable : 4018)
#endif

  template <typename F, typename T>
  void FUN_UNUSED CheckUpperLimit(const F& from) const {
    // casting to type of smaller size AND
    // 'from' is greater than 'T' max value
    if ((sizeof(T) < sizeof(F)) &&
        (from > static_cast<F>(std::numeric_limits<T>::max()))) {
      throw RangeException("Value too large.");
    }
    // casting to type of equal/bigger size AND
    // 'F' is signed AND
    // 'T' is unsigned AND
    // 'from' is negative
    else if (std::numeric_limits<F>::is_signed &&
            !std::numeric_limits<T>::is_signed && from < 0) {
      throw RangeException("Value too small.");
    }
    // casting to type of equal/bigger size AND
    // 'F' is unsigned AND
    // 'T' is signed AND
    // 'from' is greater than 'T' max value
    else if (!std::numeric_limits<F>::is_signed &&
            std::numeric_limits<T>::is_signed &&
            static_cast<uint64>(from) > std::numeric_limits<T>::max()) {
      throw RangeException("Value too large.");
    }
  }

#ifdef _MSC_VER
#pragma warning(pop)
#endif

  template <typename F, typename T>
  void CheckUpperLimitFloat(const F& from) const {
    if (from > std::numeric_limits<T>::max()) {
      throw RangeException("Value too large.");
    }
  }

  template <typename F, typename T>
  void CheckLowerLimitFloat(const F& from) const {
    if (from < -std::numeric_limits<T>::max()) {
      throw RangeException("Value too small.");
    }
  }

  template <typename F, typename T>
  void CheckLowerLimit(const F& from) const {
    if (from < std::numeric_limits<T>::min()) {
      throw RangeException("Value too small.");
    }
  }
};


//
// inlines
//

inline void VarHolder::Convert(int8& /*val*/) const {
  throw BadCastException("Can not convert to int8");
}

inline void VarHolder::Convert(int16& /*val*/) const {
  throw BadCastException("Can not convert to int16");
}

inline void VarHolder::Convert(int32& /*val*/) const {
  throw BadCastException("Can not convert to int32");
}

inline void VarHolder::Convert(int64& /*val*/) const {
  throw BadCastException("Can not convert to int64");
}

inline void VarHolder::Convert(uint8& /*val*/) const {
  throw BadCastException("Can not convert to uint8");
}

inline void VarHolder::Convert(uint16& /*val*/) const {
  throw BadCastException("Can not convert to uint16");
}

inline void VarHolder::Convert(uint32& /*val*/) const {
  throw BadCastException("Can not convert to uint32");
}

inline void VarHolder::Convert(uint64& /*val*/) const {
  throw BadCastException("Can not convert to uint64");
}

inline void VarHolder::Convert(DateTime& /*val*/) const {
  throw BadCastException("Can not convert to DateTime");
}

inline void VarHolder::Convert(Timestamp& /*val*/) const {
  throw BadCastException("Can not convert to Timestamp");
}

#ifndef FUN_LONG_IS_64_BIT
inline void VarHolder::Convert(long& val) const {
  int32 tmp;
  Convert(tmp);
  val = tmp;
}

inline void VarHolder::Convert(unsigned long& val) const {
  uint32 tmp;
  Convert(tmp);
  val = tmp;
}
#endif

inline void VarHolder::Convert(bool& /*val*/) const {
  throw BadCastException("Can not convert to bool");
}

inline void VarHolder::Convert(float& /*val*/) const {
  throw BadCastException("Can not convert to float");
}

inline void VarHolder::Convert(double& /*val*/) const {
  throw BadCastException("Can not convert to double");
}

inline void VarHolder::Convert(char& /*val*/) const {
  throw BadCastException("Can not convert to char");
}

inline void VarHolder::Convert(String& /*val*/) const {
  throw BadCastException("Can not convert to String");
}

inline void VarHolder::Convert(UString& /*val*/) const {
  throw BadCastException("Can not convert to UString");
}

inline bool VarHolder::IsArray() const {
  return true;
}

inline bool VarHolder::IsVector() const {
  return false;
}

inline bool VarHolder::IsList() const {
  return false;
}

inline bool VarHolder::IsDeque() const {
  return false;
}

inline bool VarHolder::IsStruct() const {
  return false;
}

inline bool VarHolder::IsOrdered() const {
  return false;
}

inline bool VarHolder::IsInteger() const {
  return false;
}

inline bool VarHolder::IsSigned() const {
  return false;
}

inline bool VarHolder::IsNumeric() const {
  return false;
}

inline bool VarHolder::IsBoolean() const {
  return false;
}

inline bool VarHolder::IsString() const {
  return false;
}

inline bool VarHolder::IsDate() const {
  return false;
}

inline bool VarHolder::IsTime() const {
  return false;
}

inline bool VarHolder::IsDateTime() const {
  return false;
}

inline size_t VarHolder::size() const {
  return 1u;
}

/**
 * Template based implementation of a VarHolder.
 * This class provides type storage for user-defined types
 * that do not have VarHolderImpl specialization.
 *
 * The actual conversion work happens in the template specializations
 * of this class.
 *
 * VarHolderImpl throws following exceptions:
 *   BadCastException (if the requested conversion is not implemented)
 *   RangeException (if an attempt is made to assign a numeric value outside of the target min/max limits
 *   SyntaxException (if an attempt is made to Convert a string containing non-numeric characters to number)
 *
 * In order to support efficient direct extraction of the held value,
 * all specializations must additionally implement a public member function:
 *
 *     const T& Value() const
 *
 * returning a const reference to the actual stored value.
 */
template <typename T, typename Enable>
class VarHolderImpl : public VarHolder {
 public:
  VarHolderImpl(const T& val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(T);
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const T& Value() const {
    return val_;
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  T val_;
};

template <>
class VarHolderImpl<int8> : public VarHolder {
 public:
  VarHolderImpl(int8 val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(int8);
  }

  void Convert(int8& val) const {
    val = val_;
  }

  void Convert(int16& val) const {
    val = val_;
  }

  void Convert(int32& val) const {
    val = val_;
  }

  void Convert(int64& val) const {
    val = val_;
  }

  void Convert(uint8& val) const {
    ConvertSignedToUnsigned(val_, val);
  }

  void Convert(uint16& val) const {
    ConvertSignedToUnsigned(val_, val);
  }

  void Convert(uint32& val) const {
    ConvertSignedToUnsigned(val_, val);
  }

  void Convert(uint64& val) const {
    ConvertSignedToUnsigned(val_, val);
  }

  void Convert(bool& val) const {
    val = (val_ != 0);
  }

  void Convert(float& val) const {
    val = static_cast<float>(val_);
  }

  void Convert(double& val) const {
    val = static_cast<double>(val_);
  }

  void Convert(char& val) const {
    val = static_cast<char>(val_);
  }

  void Convert(String& val) const {
    val = NumberFormatter::Format(val_);
  }

  void Convert(UString& val) const {
    String str = NumberFormatter::Format(val_);
    fun::UnicodeConverter::Convert(str, val);
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const int8& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return false;
  }

  bool IsInteger() const {
    return std::numeric_limits<int8>::is_integer;
  }

  bool IsSigned() const {
    return std::numeric_limits<int8>::is_signed;
  }

  bool IsNumeric() const {
    return std::numeric_limits<int8>::is_specialized;
  }

  bool IsBoolean() const {
    return false;
  }

  bool IsString() const {
    return false;
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  int8 val_;
};

template <>
class VarHolderImpl<int16> : public VarHolder {
 public:
  VarHolderImpl(int16 val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(int16);
  }

  void Convert(int8& val) const {
    ConvertToSmaller(val_, val);
  }

  void Convert(int16& val) const {
    val = val_;
  }

  void Convert(int32& val) const {
    val = val_;
  }

  void Convert(int64& val) const {
    val = val_;
  }

  void Convert(uint8& val) const {
    ConvertSignedToUnsigned(val_, val);
  }

  void Convert(uint16& val) const {
    ConvertSignedToUnsigned(val_, val);
  }

  void Convert(uint32& val) const {
    ConvertSignedToUnsigned(val_, val);
  }

  void Convert(uint64& val) const {
    ConvertSignedToUnsigned(val_, val);
  }

  void Convert(bool& val) const {
    val = (val_ != 0);
  }

  void Convert(float& val) const {
    val = static_cast<float>(val_);
  }

  void Convert(double& val) const {
    val = static_cast<double>(val_);
  }

  void Convert(char& val) const {
    uint8 tmp;
    Convert(tmp);
    val = static_cast<char>(tmp);
  }

  void Convert(String& val) const {
    val = NumberFormatter::Format(val_);
  }

  void Convert(UString& val) const {
    String str = NumberFormatter::Format(val_);
    fun::UnicodeConverter::Convert(str, val);
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const int16& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return false;
  }

  bool IsInteger() const {
    return std::numeric_limits<int16>::is_integer;
  }

  bool IsSigned() const {
    return std::numeric_limits<int16>::is_signed;
  }

  bool IsNumeric() const {
    return std::numeric_limits<int16>::is_specialized;
  }

  bool IsString() const {
    return false;
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  int16 val_;
};

template <>
class VarHolderImpl<int32> : public VarHolder {
 public:
  VarHolderImpl(int32 val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(int32);
  }

  void Convert(int8& val) const {
    ConvertToSmaller(val_, val);
  }

  void Convert(int16& val) const {
    ConvertToSmaller(val_, val);
  }

  void Convert(int32& val) const {
    val = val_;
  }

  void Convert(int64& val) const {
    val = val_;
  }

  void Convert(uint8& val) const {
    ConvertSignedToUnsigned(val_, val);
  }

  void Convert(uint16& val) const {
    ConvertSignedToUnsigned(val_, val);
  }

  void Convert(uint32& val) const {
    ConvertSignedToUnsigned(val_, val);
  }

  void Convert(uint64& val) const {
    ConvertSignedToUnsigned(val_, val);
  }

  void Convert(bool& val) const {
    val = (val_ != 0);
  }

  void Convert(float& val) const {
    val = static_cast<float>(val_);
  }

  void Convert(double& val) const {
    val = static_cast<double>(val_);
  }

  void Convert(char& val) const {
    uint8 tmp;
    Convert(tmp);
    val = static_cast<char>(tmp);
  }

  void Convert(String& val) const {
    val = NumberFormatter::Format(val_);
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const int32& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return false;
  }

  bool IsInteger() const {
    return std::numeric_limits<int32>::is_integer;
  }

  bool IsSigned() const {
    return std::numeric_limits<int32>::is_signed;
  }

  bool IsNumeric() const {
    return std::numeric_limits<int32>::is_specialized;
  }

  bool IsBoolean() const {
    return false;
  }

  bool IsString() const {
    return false;
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  int32 val_;
};

template <>
class VarHolderImpl<int64> : public VarHolder {
 public:
  VarHolderImpl(int64 val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(int64);
  }

  void Convert(int8& val) const {
    ConvertToSmaller(val_, val);
  }

  void Convert(int16& val) const {
    ConvertToSmaller(val_, val);
  }

  void Convert(int32& val) const {
    ConvertToSmaller(val_, val);
  }

  void Convert(int64& val) const {
    val = val_;
  }

  void Convert(uint8& val) const {
    ConvertSignedToUnsigned(val_, val);
  }

  void Convert(uint16& val) const {
    ConvertSignedToUnsigned(val_, val);
  }

  void Convert(uint32& val) const {
    ConvertSignedToUnsigned(val_, val);
  }

  void Convert(uint64& val) const {
    ConvertSignedToUnsigned(val_, val);
  }

  void Convert(bool& val) const {
    val = (val_ != 0);
  }

  void Convert(float& val) const {
    val = static_cast<float>(val_);
  }

  void Convert(double& val) const {
    val = static_cast<double>(val_);
  }

  void Convert(char& val) const {
    uint8 tmp;
    Convert(tmp);
    val = static_cast<char>(tmp);
  }

  void Convert(String& val) const {
    val = NumberFormatter::Format(val_);
  }

  void Convert(DateTime& val) const {
    val = Timestamp(val_);
  }

  void Convert(Timestamp& val) const {
    val = Timestamp(val_);
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const int64& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return false;
  }

  bool IsInteger() const {
    return std::numeric_limits<int64>::is_integer;
  }

  bool IsSigned() const {
    return std::numeric_limits<int64>::is_signed;
  }

  bool IsNumeric() const {
    return std::numeric_limits<int64>::is_specialized;
  }

  bool IsBoolean() const {
    return false;
  }

  bool IsString() const {
    return false;
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  int64 val_;
};

template <>
class VarHolderImpl<uint8> : public VarHolder {
 public:
  VarHolderImpl(uint8 val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(uint8);
  }

  void Convert(int8& val) const {
    ConvertUnsignedToSigned(val_, val);
  }

  void Convert(int16& val) const {
    ConvertUnsignedToSigned(val_, val);
  }

  void Convert(int32& val) const {
    val = static_cast<int32>(val_);
  }

  void Convert(int64& val) const {
    val = static_cast<int64>(val_);
  }

  void Convert(uint8& val) const {
    val = val_;
  }

  void Convert(uint16& val) const {
    val = val_;
  }

  void Convert(uint32& val) const {
    val = val_;
  }

  void Convert(uint64& val) const {
    val = val_;
  }

  void Convert(bool& val) const {
    val = (val_ != 0);
  }

  void Convert(float& val) const {
    val = static_cast<float>(val_);
  }

  void Convert(double& val) const {
    val = static_cast<double>(val_);
  }

  void Convert(char& val) const {
    uint8 tmp;
    Convert(tmp);
    val = static_cast<char>(tmp);
  }

  void Convert(String& val) const {
    val = NumberFormatter::Format(val_);
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const uint8& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return false;
  }

  bool IsInteger() const {
    return std::numeric_limits<uint8>::is_integer;
  }

  bool IsSigned() const {
    return std::numeric_limits<uint8>::is_signed;
  }

  bool IsNumeric() const {
    return std::numeric_limits<uint8>::is_specialized;
  }

  bool IsBoolean() const {
    return false;
  }

  bool IsString() const {
    return false;
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  uint8 val_;
};

template <>
class VarHolderImpl<uint16> : public VarHolder {
 public:
  VarHolderImpl(uint16 val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(uint16);
  }

  void Convert(int8& val) const {
    ConvertUnsignedToSigned(val_, val);
  }

  void Convert(int16& val) const {
    ConvertUnsignedToSigned(val_, val);
  }

  void Convert(int32& val) const {
    ConvertUnsignedToSigned(val_, val);
  }

  void Convert(int64& val) const {
    val = static_cast<int64>(val_);
  }

  void Convert(uint8& val) const {
    ConvertToSmallerUnsigned(val_, val);
  }

  void Convert(uint16& val) const {
    val = val_;
  }

  void Convert(uint32& val) const {
    val = val_;
  }

  void Convert(uint64& val) const {
    val = val_;
  }

  void Convert(bool& val) const {
    val = (val_ != 0);
  }

  void Convert(float& val) const {
    val = static_cast<float>(val_);
  }

  void Convert(double& val) const {
    val = static_cast<double>(val_);
  }

  void Convert(char& val) const {
    uint8 tmp;
    Convert(tmp);
    val = static_cast<char>(tmp);
  }

  void Convert(String& val) const {
    val = NumberFormatter::Format(val_);
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const uint16& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return false;
  }

  bool IsInteger() const {
    return std::numeric_limits<uint16>::is_integer;
  }

  bool IsSigned() const {
    return std::numeric_limits<uint16>::is_signed;
  }

  bool IsNumeric() const {
    return std::numeric_limits<uint16>::is_specialized;
  }

  bool IsBoolean() const {
    return false;
  }

  bool IsString() const {
    return false;
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  uint16 val_;
};

template <>
class VarHolderImpl<uint32> : public VarHolder {
 public:
  VarHolderImpl(uint32 val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(uint32);
  }

  void Convert(int8& val) const {
    ConvertUnsignedToSigned(val_, val);
  }

  void Convert(int16& val) const {
    ConvertUnsignedToSigned(val_, val);
  }

  void Convert(int32& val) const {
    ConvertUnsignedToSigned(val_, val);
  }

  void Convert(int64& val) const {
    ConvertUnsignedToSigned(val_, val);
  }

  void Convert(uint8& val) const {
    ConvertToSmallerUnsigned(val_, val);
  }

  void Convert(uint16& val) const {
    ConvertToSmallerUnsigned(val_, val);
  }

  void Convert(uint32& val) const {
    val = val_;
  }

  void Convert(uint64& val) const {
    val = val_;
  }

  void Convert(bool& val) const {
    val = (val_ != 0);
  }

  void Convert(float& val) const {
    val = static_cast<float>(val_);
  }

  void Convert(double& val) const {
    val = static_cast<double>(val_);
  }

  void Convert(char& val) const {
    uint8 tmp;
    Convert(tmp);
    val = static_cast<char>(tmp);
  }

  void Convert(String& val) const {
    val = NumberFormatter::Format(val_);
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const uint32& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return false;
  }

  bool IsInteger() const {
    return std::numeric_limits<uint32>::is_integer;
  }

  bool IsSigned() const {
    return std::numeric_limits<uint32>::is_signed;
  }

  bool IsNumeric() const {
    return std::numeric_limits<uint32>::is_specialized;
  }

  bool IsBoolean() const {
    return false;
  }

  bool IsString() const {
    return false;
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  uint32 val_;
};

template <>
class VarHolderImpl<uint64> : public VarHolder {
 public:
  VarHolderImpl(uint64 val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(uint64);
  }

  void Convert(int8& val) const {
    ConvertUnsignedToSigned(val_, val);
  }

  void Convert(int16& val) const {
    ConvertUnsignedToSigned(val_, val);
  }

  void Convert(int32& val) const {
    ConvertUnsignedToSigned(val_, val);
  }

  void Convert(int64& val) const {
    ConvertUnsignedToSigned(val_, val);
  }

  void Convert(uint8& val) const {
    ConvertToSmallerUnsigned(val_, val);
  }

  void Convert(uint16& val) const {
    ConvertToSmallerUnsigned(val_, val);
  }

  void Convert(uint32& val) const {
    ConvertToSmallerUnsigned(val_, val);
  }

  void Convert(uint64& val) const {
    val = val_;
  }

  void Convert(bool& val) const {
    val = (val_ != 0);
  }

  void Convert(float& val) const {
    val = static_cast<float>(val_);
  }

  void Convert(double& val) const {
    val = static_cast<double>(val_);
  }

  void Convert(char& val) const {
    uint8 tmp;
    Convert(tmp);
    val = static_cast<char>(tmp);
  }

  void Convert(String& val) const {
    val = NumberFormatter::Format(val_);
  }

  void Convert(DateTime& dt) const {
    int64 val;
    ConvertUnsignedToSigned(val_, val);
    dt = Timestamp(val);
  }

  void Convert(Timestamp& val) const {
    int64 tmp;
    ConvertUnsignedToSigned(val_, tmp);
    val = Timestamp(tmp);
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const uint64& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return false;
  }

  bool IsInteger() const {
    return std::numeric_limits<uint64>::is_integer;
  }

  bool IsSigned() const {
    return std::numeric_limits<uint64>::is_signed;
  }

  bool IsNumeric() const {
    return std::numeric_limits<uint64>::is_specialized;
  }

  bool IsBoolean() const {
    return false;
  }

  bool IsString() const {
    return false;
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  uint64 val_;
};

template <>
class VarHolderImpl<bool> : public VarHolder {
 public:
  VarHolderImpl(bool val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(bool);
  }

  void Convert(int8& val) const {
    val = static_cast<int8>(val_ ? 1 : 0);
  }

  void Convert(int16& val) const {
    val = static_cast<int16>(val_ ? 1 : 0);
  }

  void Convert(int32& val) const {
    val = static_cast<int32>(val_ ? 1 : 0);
  }

  void Convert(int64& val) const {
    val = static_cast<int64>(val_ ? 1 : 0);
  }

  void Convert(uint8& val) const {
    val = static_cast<uint8>(val_ ? 1 : 0);
  }

  void Convert(uint16& val) const {
    val = static_cast<uint16>(val_ ? 1 : 0);
  }

  void Convert(uint32& val) const {
    val = static_cast<uint32>(val_ ? 1 : 0);
  }

  void Convert(uint64& val) const {
    val = static_cast<uint64>(val_ ? 1 : 0);
  }

  void Convert(bool& val) const {
    val = val_;
  }

  void Convert(float& val) const {
    val = (val_ ? 1.0f : 0.0f);
  }

  void Convert(double& val) const {
    val = (val_ ? 1.0 : 0.0);
  }

  void Convert(char& val) const {
    val = static_cast<char>(val_ ? 1 : 0);
  }

  void Convert(String& val) const {
    val = (val_ ? "true" : "false");
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const bool& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return false;
  }

  bool IsInteger() const {
    return std::numeric_limits<bool>::is_integer;
  }

  bool IsSigned() const {
    return std::numeric_limits<bool>::is_signed;
  }

  bool IsNumeric() const {
    return std::numeric_limits<bool>::is_specialized;
  }

  bool IsBoolean() const {
    return true;
  }

  bool IsString() const {
    return false;
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  bool val_;
};

template <>
class VarHolderImpl<float> : public VarHolder {
 public:
  VarHolderImpl(float val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(float);
  }

  void Convert(int8& val) const {
    ConvertToSmaller(val_, val);
  }

  void Convert(int16& val) const {
    ConvertToSmaller(val_, val);
  }

  void Convert(int32& val) const {
    ConvertToSmaller(val_, val);
  }

  void Convert(int64& val) const {
    ConvertToSmaller(val_, val);
  }

  void Convert(uint8& val) const {
    ConvertSignedFloatToUnsigned(val_, val);
  }

  void Convert(uint16& val) const {
    ConvertSignedFloatToUnsigned(val_, val);
  }

  void Convert(uint32& val) const {
    ConvertSignedFloatToUnsigned(val_, val);
  }

  void Convert(uint64& val) const {
    ConvertSignedFloatToUnsigned(val_, val);
  }

  void Convert(bool& val) const {
    val = !(val_ <= std::numeric_limits<float>::min() &&
            val_ >= -1 * std::numeric_limits<float>::min());
  }

  void Convert(float& val) const {
    val = val_;
  }

  void Convert(double& val) const {
    val = val_;
  }

  void Convert(char& val) const {
    uint8 tmp;
    Convert(tmp);
    val = static_cast<char>(tmp);
  }

  void Convert(String& val) const {
    val = NumberFormatter::Format(val_);
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const float& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return false;
  }

  bool IsInteger() const {
    return std::numeric_limits<float>::is_integer;
  }

  bool IsSigned() const {
    return std::numeric_limits<float>::is_signed;
  }

  bool IsNumeric() const {
    return std::numeric_limits<float>::is_specialized;
  }

  bool IsBoolean() const {
    return false;
  }

  bool IsString() const {
    return false;
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  float val_;
};

template <>
class VarHolderImpl<double> : public VarHolder {
 public:
  VarHolderImpl(double val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(double);
  }

  void Convert(int8& val) const {
    ConvertToSmaller(val_, val);
  }

  void Convert(int16& val) const {
    ConvertToSmaller(val_, val);
  }

  void Convert(int32& val) const {
    ConvertToSmaller(val_, val);
  }

  void Convert(int64& val) const {
    ConvertToSmaller(val_, val);
  }

  void Convert(uint8& val) const {
    ConvertSignedFloatToUnsigned(val_, val);
  }

  void Convert(uint16& val) const {
    ConvertSignedFloatToUnsigned(val_, val);
  }

  void Convert(uint32& val) const {
    ConvertSignedFloatToUnsigned(val_, val);
  }

  void Convert(uint64& val) const {
    ConvertSignedFloatToUnsigned(val_, val);
  }

  void Convert(bool& val) const {
    val = !(val_ <= std::numeric_limits<double>::min() &&
            val_ >= -1 * std::numeric_limits<double>::min());
  }

  void Convert(float& val) const {
    double fMin = -1 * std::numeric_limits<float>::max();
    double fMax = std::numeric_limits<float>::max();

    if (val_ < fMin) {
      throw RangeException("Value too small.");
    }

    if (val_ > fMax) {
      throw RangeException("Value too large.");
    }

    val = static_cast<float>(val_);
  }

  void Convert(double& val) const {
    val = val_;
  }

  void Convert(char& val) const {
    uint8 tmp;
    Convert(tmp);
    val = static_cast<char>(tmp);
  }

  void Convert(String& val) const {
    val = NumberFormatter::Format(val_);
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const double& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return false;
  }

  bool IsInteger() const {
    return std::numeric_limits<double>::is_integer;
  }

  bool IsSigned() const {
    return std::numeric_limits<double>::is_signed;
  }

  bool IsNumeric() const {
    return std::numeric_limits<double>::is_specialized;
  }

  bool IsBoolean() const {
    return false;
  }

  bool IsString() const {
    return false;
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  double val_;
};

template <>
class VarHolderImpl<char> : public VarHolder {
 public:
  VarHolderImpl(char val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(char);
  }

  void Convert(int8& val) const {
    val = static_cast<int8>(val_);
  }

  void Convert(int16& val) const {
    val = static_cast<uint8>(val_);
  }

  void Convert(int32& val) const {
    val = static_cast<uint8>(val_);
  }

  void Convert(int64& val) const {
    val = static_cast<uint8>(val_);
  }

  void Convert(uint8& val) const {
    val = static_cast<uint8>(val_);
  }

  void Convert(uint16& val) const {
    val = static_cast<uint8>(val_);
  }

  void Convert(uint32& val) const {
    val = static_cast<uint8>(val_);
  }

  void Convert(uint64& val) const {
    val = static_cast<uint8>(val_);
  }

  void Convert(bool& val) const {
    val = (val_ != '\0');
  }

  void Convert(float& val) const {
    val = static_cast<float>(val_);
  }

  void Convert(double& val) const {
    val = static_cast<double>(val_);
  }

  void Convert(char& val) const {
    val = val_;
  }

  void Convert(String& val) const {
    val = String(1, val_);
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const char& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return false;
  }

  bool IsInteger() const {
    return std::numeric_limits<char>::is_integer;
  }

  bool IsSigned() const {
    return std::numeric_limits<char>::is_signed;
  }

  bool IsNumeric() const {
    return std::numeric_limits<char>::is_specialized;
  }

  bool IsBoolean() const {
    return false;
  }

  bool IsString() const {
    return false;
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  char val_;
};

template <>
class VarHolderImpl<String> : public VarHolder {
 public:
  VarHolderImpl(const char* pVal) : val_(pVal) {}
  VarHolderImpl(const String& val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(String);
  }

  void Convert(int8& val) const {
    int v = NumberParser::Parse(val_);
    ConvertToSmaller(v, val);
  }

  void Convert(int16& val) const {
    int v = NumberParser::Parse(val_);
    ConvertToSmaller(v, val);
  }

  void Convert(int32& val) const {
    val = NumberParser::Parse(val_);
  }

  void Convert(int64& val) const {
    val = NumberParser::Parse64(val_);
  }

  void Convert(uint8& val) const {
    unsigned int v = NumberParser::ParseUnsigned(val_);
    ConvertToSmallerUnsigned(v, val);
  }

  void Convert(uint16& val) const {
    unsigned int v = NumberParser::ParseUnsigned(val_);
    ConvertToSmallerUnsigned(v, val);
  }

  void Convert(uint32& val) const {
    val = NumberParser::ParseUnsigned(val_);
  }

  void Convert(uint64& val) const {
    val = NumberParser::ParseUnsigned64(val_);
  }

  void Convert(bool& val) const {
    if (val_.IsEmpty()) {
      val = false;
      return;
    }

    static const String VAL_FALSE("false");
    static const String VAL_INT_FALSE("0");
    val = (val_ != VAL_INT_FALSE && (icompare(val_, VAL_FALSE) != 0));
  }

  void Convert(float& val) const {
    double v = NumberParser::ParseFloat(val_);
    ConvertToSmaller(v, val);
  }

  void Convert(double& val) const {
    val = NumberParser::ParseFloat(val_);
  }

  void Convert(char& val) const {
    if (val_.IsEmpty()) {
      val = '\0';
    } else {
      val = val_[0];
    }
  }

  void Convert(String& val) const {
    val = val_;
  }

  void Convert(UString& val) const {
    fun::UnicodeConverter::Convert(val_, val);
  }

  void Convert(DateTime& val) const {
    int tzd = 0;
    if (!DateTimeParser::TryParse(DateTimeFormat::ISO8601_FORMAT, val_, val, tzd)) {
      throw BadCastException("String -> DateTime");
    }
  }

  void Convert(Timestamp& ts) const {
    int tzd = 0;
    DateTime tmp;
    if (!DateTimeParser::TryParse(DateTimeFormat::ISO8601_FORMAT, val_, tmp, tzd)) {
      throw BadCastException("String -> Timestamp");
    }

    ts = tmp.timestamp();
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const std:: string& Value() const {
    return val_;
  }

  bool IsString() const {
    return true;
  }

  size_t size() const {
    return val_.length();
  }

  char& operator[](String::size_type n) {
    if (n < size()) {
      return val_.operator[](n);
    }

    throw RangeException("String index out of range");
  }

  const char& operator[](String::size_type n) const {
    if (n < size()) {
      return val_.operator[](n);
    }

    throw RangeException("String index out of range");
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  String val_;
};

template <>
class VarHolderImpl<UString> : public VarHolder {
 public:
  VarHolderImpl(const char* val) : val_(fun::UnicodeConverter::To<UString>(val)) {}
  VarHolderImpl(const UString& val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(UString);
  }

  void Convert(int8& val) const {
    int v = NumberParser::Parse(ToStdString());
    ConvertToSmaller(v, val);
  }

  void Convert(int16& val) const {
    int v = NumberParser::Parse(ToStdString());
    ConvertToSmaller(v, val);
  }

  void Convert(int32& val) const {
    val = NumberParser::Parse(ToStdString());
  }

  void Convert(int64& val) const {
    val = NumberParser::Parse64(ToStdString());
  }

  void Convert(uint8& val) const {
    unsigned int v = NumberParser::ParseUnsigned(ToStdString());
    ConvertToSmallerUnsigned(v, val);
  }

  void Convert(uint16& val) const {
    unsigned int v = NumberParser::ParseUnsigned(ToStdString());
    ConvertToSmallerUnsigned(v, val);
  }

  void Convert(uint32& val) const {
    val = NumberParser::ParseUnsigned(ToStdString());
  }

  void Convert(uint64& val) const {
    val = NumberParser::ParseUnsigned64(ToStdString());
  }

  void Convert(bool& val) const {
    static const String VAL_FALSE("false");
    static const String VAL_INT_FALSE("0");

    if (val_.IsEmpty()) {
      val = false;
    }

    String str;
    UnicodeConverter::Convert(val_, str);
    val = (str != VAL_INT_FALSE && (icompare(str, VAL_FALSE) != 0));
  }

  void Convert(float& val) const {
    double v = NumberParser::ParseFloat(ToStdString());
    ConvertToSmaller(v, val);
  }

  void Convert(double& val) const {
    val = NumberParser::ParseFloat(ToStdString());
  }

  void Convert(char& val) const {
    if (val_.IsEmpty()) {
      val = '\0';
    } else {
      String s;
      UnicodeConverter::Convert(val_, s);
      val = s[0];
    }
  }

  void Convert(UString& val) const {
    val = val_;
  }

  void Convert(String& val) const {
    UnicodeConverter::Convert(val_, val);
  }

  void Convert(DateTime& val) const {
    int tzd = 0;
    if (!DateTimeParser::TryParse(DateTimeFormat::ISO8601_FORMAT, ToStdString(), val, tzd)) {
      throw BadCastException("String -> DateTime");
    }
  }

  void Convert(Timestamp& ts) const {
    int tzd = 0;
    DateTime tmp;
    if (!DateTimeParser::TryParse(DateTimeFormat::ISO8601_FORMAT, ToStdString(), tmp, tzd)) {
      throw BadCastException("String -> Timestamp");
    }

    ts = tmp.timestamp();
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const UString& Value() const {
    return val_;
  }

  bool IsString() const {
    return true;
  }

  size_t size() const {
    return val_.length();
  }

  UTF16Char& operator[](UString::size_type n) {
    if (n < size()) {
      return val_.operator[](n);
    }

    throw RangeException("String index out of range");
  }

  const UTF16Char& operator[](UString::size_type n) const {
    if (n < size()) {
      return val_.operator[](n);
    }

    throw RangeException("String index out of range");
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  String ToStdString() const {
    String str;
    UnicodeConverter::Convert(val_, str);
    return str;
  }

  UString val_;
};

template <typename T>
class VarHolderImpl<T,
  typename EnableIf<
     IsSame<T, long >::Value &&
    !IsSame<T, int64>::Value &&
    !IsSame<T, int32>::Value
  >::Type
> : public VarHolder {
 public:
  VarHolderImpl(long val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(long);
  }

  void Convert(int8& val) const {
    ConvertToSmaller(val_, val);
  }

  void Convert(int16& val) const {
    ConvertToSmaller(val_, val);
  }

  void Convert(int32& val) const {
    val = static_cast<int32>(val_);
  }

  void Convert(int64& val) const {
    val = static_cast<int64>(val_);
  }

  void Convert(uint8& val) const {
    ConvertSignedToUnsigned(val_, val);
  }

  void Convert(uint16& val) const {
    ConvertSignedToUnsigned(val_, val);
  }

  void Convert(uint32& val) const {
    ConvertSignedToUnsigned(val_, val);
  }

  void Convert(uint64& val) const {
    ConvertSignedToUnsigned(val_, val);
  }

  void Convert(bool& val) const {
    val = (val_ != 0);
  }

  void Convert(float& val) const {
    val = static_cast<float>(val_);
  }

  void Convert(double& val) const {
    val = static_cast<double>(val_);
  }

  void Convert(char& val) const {
    uint8 tmp;
    Convert(tmp);
    val = static_cast<char>(tmp);
  }

  void Convert(String& val) const {
    val = NumberFormatter::Format(static_cast<int64 >(val_));
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const long& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return false;
  }

  bool IsInteger() const {
    return std::numeric_limits<long>::is_integer;
  }

  bool IsSigned() const {
    return std::numeric_limits<long>::is_signed;
  }

  bool IsNumeric() const {
    return std::numeric_limits<long>::is_specialized;
  }

  bool IsBoolean() const {
    return false;
  }

  bool IsString() const {
    return false;
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  long val_;
};

template <typename T>
class VarHolderImpl<T,
  typename EnableIf<
     IsSame<T, unsigned long>::Value &&
    !IsSame<T, uint64       >::Value &&
    !IsSame<T, uint32       >::Value
    >::Type
> : public VarHolder {
 public:
  VarHolderImpl(unsigned long val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(unsigned long);
  }

  void Convert(int8& val) const {
    ConvertUnsignedToSigned(val_, val);
  }

  void Convert(int16& val) const {
    ConvertUnsignedToSigned(val_, val);
  }

  void Convert(int32& val) const {
    ConvertUnsignedToSigned(val_, val);
  }

  void Convert(int64& val) const {
    ConvertUnsignedToSigned(val_, val);
  }

  void Convert(uint8& val) const {
    ConvertToSmallerUnsigned(val_, val);
  }

  void Convert(uint16& val) const {
    ConvertToSmallerUnsigned(val_, val);
  }

  void Convert(uint32& val) const {
    ConvertToSmallerUnsigned(val_, val);
  }

  void Convert(uint64& val) const {
    val = static_cast<uint64>(val_);
  }

  void Convert(bool& val) const {
    val = (val_ != 0);
  }

  void Convert(float& val) const {
    val = static_cast<float>(val_);
  }

  void Convert(double& val) const {
    val = static_cast<double>(val_);
  }

  void Convert(char& val) const {
    uint8 tmp;
    Convert(tmp);
    val = static_cast<char>(tmp);
  }

  void Convert(String& val) const {
    val = NumberFormatter::Format(static_cast<uint64>(val_));
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const unsigned long& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return false;
  }

  bool IsInteger() const {
    return std::numeric_limits<unsigned long>::is_integer;
  }

  bool IsSigned() const {
    return std::numeric_limits<unsigned long>::is_signed;
  }

  bool IsNumeric() const {
    return std::numeric_limits<unsigned long>::is_specialized;
  }

  bool IsBoolean() const {
    return false;
  }

  bool IsString() const {
    return false;
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  unsigned long val_;
};

template <typename T>
class VarHolderImpl<std::vector<T> > : public VarHolder {
 public:
  VarHolderImpl(const std::vector<T>& val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(std::vector<T>);
  }

  void Convert(String& val) const {
    Impl::ContainerToJson(val_, val);
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const std::vector<T>& Value() const {
    return val_;
  }

  bool IsVector() const {
    return true;
  }

  size_t size() const {
    return val_.size();
  }

  T& operator[](typename std::vector<T>::size_type n) {
    if (n < size()) {
      return val_.operator[](n);
    }

    throw RangeException("List index out of range");
  }

  const T& operator[](typename std::vector<T>::size_type n) const {
    if (n < size()) {
      return val_.operator[](n);
    }

    throw RangeException("List index out of range");
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  std::vector<T> val_;
};

template <typename T>
class VarHolderImpl<std::list<T> > : public VarHolder {
 public:
  VarHolderImpl(const std::list<T>& val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(std::list<T>);
  }

  void Convert(String& val) const {
    Impl::ContainerToJson(val_, val);
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const std::list<T>& Value() const {
    return val_;
  }

  bool IsList() const {
    return true;
  }

  size_t size() const {
    return val_.size();
  }

  T& operator[](typename std::list<T>::size_type n) {
    if (n >= size()) {
      throw RangeException("List index out of range");
    }

    typename std::list<T>::size_type counter = 0;
    typename std::list<T>::iterator it = val_.begin();
    for (; counter < n; ++counter) ++it;

    return *it;
  }

  const T& operator[](typename std::list<T>::size_type n) const {
    if (n >= size()) {
      throw RangeException("List index out of range");
    }

    typename std::list<T>::size_type counter = 0;
    typename std::list<T>::const_iterator it = val_.begin();
    for (; counter < n; ++counter) ++it;

    return *it;
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  std::list<T> val_;
};

template <typename T>
class VarHolderImpl<std::deque<T> > : public VarHolder {
 public:
  VarHolderImpl(const std::deque<T>& val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(std::deque<T>);
  }

  void Convert(String& val) const {
    Impl::ContainerToJson(val_, val);
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const std::deque<T>& Value() const {
    return val_;
  }

  bool IsDeque() const {
    return true;
  }

  size_t size() const {
    return val_.size();
  }

  T& operator[](typename std::deque<T>::size_type n) {
    if (n < size()) {
      return val_.operator[](n);
    }

    throw RangeException("List index out of range");
  }

  const T& operator[](typename std::deque<T>::size_type n) const {
    if (n < size()) {
      return val_.operator[](n);
    }

    throw RangeException("List index out of range");
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  std::deque<T> val_;
};

template <>
class VarHolderImpl<DateTime> : public VarHolder {
 public:
  VarHolderImpl(const DateTime& val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(DateTime);
  }

  void Convert(int8& /*val*/) const {
    throw BadCastException();
  }

  void Convert(int16& /*val*/) const {
    throw BadCastException();
  }

  void Convert(int32& /*val*/) const {
    throw BadCastException();
  }

  void Convert(int64& val) const {
    val = val_.timestamp().EpochMicroseconds();
  }

  void Convert(uint64& val) const {
    val = val_.timestamp().EpochMicroseconds();
  }

  void Convert(String& val) const {
    val = DateTimeFormatter::Format(val_, fun::DateTimeFormat::ISO8601_FORMAT);
  }

  void Convert(DateTime& val) const {
    val = val_;
  }

  void Convert(Timestamp& val) const {
    val = val_.timestamp();
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const DateTime& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return false;
  }

  bool IsInteger() const {
    return false;
  }

  bool IsSigned() const {
    return false;
  }

  bool IsNumeric() const {
    return false;
  }

  bool IsBoolean() const {
    return false;
  }

  bool IsString() const {
    return false;
  }

  bool IsDate() const {
    return true;
  }

  bool IsTime() const {
    return true;
  }

  bool IsDateTime() const {
    return true;
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  DateTime val_;
};

template <>
class VarHolderImpl<Timestamp> : public VarHolder {
 public:
  VarHolderImpl(const Timestamp& val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(Timestamp);
  }

  void Convert(int64& val) const {
    val = val_.EpochMicroseconds();
  }

  void Convert(uint64& val) const {
    val = val_.EpochMicroseconds();
  }

  void Convert(String& val) const {
    val = DateTimeFormatter::Format(val_, fun::DateTimeFormat::ISO8601_FORMAT);
  }

  void Convert(DateTime& val) const {
    val = val_;
  }

  void Convert(Timestamp& val) const {
    val = val_;
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const Timestamp& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return false;
  }

  bool IsInteger() const {
    return false;
  }

  bool IsSigned() const {
    return false;
  }

  bool IsNumeric() const {
    return false;
  }

  bool IsBoolean() const {
    return false;
  }

  bool IsString() const {
    return false;
  }

  bool IsDate() const {
    return true;
  }

  bool IsTime() const {
    return true;
  }

  bool IsDateTime() const {
    return true;
  }

 private:
  VarHolderImpl();
  VarHolderImpl(const VarHolderImpl&);
  VarHolderImpl& operator = (const VarHolderImpl&);

  Timestamp val_;
};

typedef std::vector<Var> Vector;
typedef std::deque<Var>  Deque;
typedef std::list<Var>   List;
typedef Vector           Array;

} // namespace dynamic
} // namespace fun
