#pragma once

#include "fun/sql/postgresql/postgresql.h"
#include "fun/sql/postgresql/postgresql_types.h"
#include "fun/sql/postgresql/postgresql_exception.h"

#include "fun/sql/binder_base.h"
#include "fun/sql/binding.h"
#include "fun/sql/meta_column.h"
#include "fun/sql/lob.h"
#include "fun/base/types.h"

#include <libpq-fe.h>

namespace fun {
namespace sql {
namespace postgresql {

/**
 * Binds INPUT (only) placeholders in the sql query to the provided values.
 * Allows data type mapping at statement execution time.
 */
class FUN_POSTGRESQL_API Binder : public fun::sql::BinderBase {
 public:
  typedef SharedPtr<Binder> Ptr;

  /**
   * Creates the Binder.
   */
  Binder();

  /**
   * Destroys the Binder.
   */
  virtual ~Binder();


  virtual void Bind(size_t pos, const int8& val, Direction dir = PD_IN, const WhenNullCb& null_cb = WhenNullCb());
  virtual void Bind(size_t pos, const uint8& val, Direction dir = PD_IN, const WhenNullCb& null_cb = WhenNullCb());
  virtual void Bind(size_t pos, const int16& val, Direction dir = PD_IN, const WhenNullCb& null_cb = WhenNullCb());
  virtual void Bind(size_t pos, const uint16& val, Direction dir = PD_IN, const WhenNullCb& null_cb = WhenNullCb());
  virtual void Bind(size_t pos, const int32& val, Direction dir = PD_IN, const WhenNullCb& null_cb = WhenNullCb());
  virtual void Bind(size_t pos, const uint32& val, Direction dir = PD_IN, const WhenNullCb& null_cb = WhenNullCb());
  virtual void Bind(size_t pos, const int64& val, Direction dir = PD_IN, const WhenNullCb& null_cb = WhenNullCb());
  virtual void Bind(size_t pos, const uint64& val, Direction dir = PD_IN, const WhenNullCb& null_cb = WhenNullCb());

#ifndef FUN_LONG_IS_64_BIT
  virtual void Bind(size_t pos, const long& val, Direction dir = PD_IN, const WhenNullCb& null_cb = WhenNullCb());
  virtual void Bind(size_t pos, const unsigned long& val, Direction dir = PD_IN, const WhenNullCb& null_cb = WhenNullCb());
#endif

  virtual void Bind(size_t pos, const bool& val, Direction dir = PD_IN, const WhenNullCb& null_cb = WhenNullCb());
  virtual void Bind(size_t pos, const float& val, Direction dir = PD_IN, const WhenNullCb& null_cb = WhenNullCb());
  virtual void Bind(size_t pos, const double& val, Direction dir = PD_IN, const WhenNullCb& null_cb = WhenNullCb());
  virtual void Bind(size_t pos, const char& val, Direction dir = PD_IN, const WhenNullCb& null_cb = WhenNullCb());
  virtual void Bind(size_t pos, const String& val, Direction dir = PD_IN, const WhenNullCb& null_cb = WhenNullCb());
  virtual void Bind(size_t pos, const fun::sql::BLOB& val, Direction dir = PD_IN, const WhenNullCb& null_cb = WhenNullCb());
  virtual void Bind(size_t pos, const fun::sql::CLOB& val, Direction dir = PD_IN, const WhenNullCb& null_cb = WhenNullCb());
  virtual void Bind(size_t pos, const DateTime& val, Direction dir = PD_IN, const WhenNullCb& null_cb = WhenNullCb());
  virtual void Bind(size_t pos, const Date& val, Direction dir = PD_IN, const WhenNullCb& null_cb = WhenNullCb());
  virtual void Bind(size_t pos, const Time& val, Direction dir = PD_IN, const WhenNullCb& null_cb = WhenNullCb());
  virtual void Bind(size_t pos, const NullData& val, Direction dir = PD_IN, const std::type_info& bind_type = typeid(void));

  virtual void Bind(size_t pos, const std::vector<int8>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<int8>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<int8>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<uint8>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<uint8>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<uint8>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<int16>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<int16>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<int16>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<uint16>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<uint16>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<uint16>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<int32>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<int32>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<int32>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<uint32>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<uint32>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<uint32>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<int64>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<int64>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<int64>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<uint64>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<uint64>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<uint64>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<bool>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<bool>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<bool>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<float>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<float>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<float>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<double>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<double>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<double>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<char>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<char>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<char>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<BLOB>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<BLOB>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<BLOB>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<CLOB>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<CLOB>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<CLOB>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<DateTime>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<DateTime>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<DateTime>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Date>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Date>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Date>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Time>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Time>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Time>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<NullData>& val, Direction dir = PD_IN, const std::type_info& bind_type = typeid(void));
  virtual void Bind(size_t pos, const std::deque<NullData>& val, Direction dir = PD_IN, const std::type_info& bind_type = typeid(void));
  virtual void Bind(size_t pos, const std::list<NullData>& val, Direction dir = PD_IN, const std::type_info& bind_type = typeid(void));
  virtual void Bind(size_t pos, const std::vector<String>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<String>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<String>& val, Direction dir = PD_IN);

  template <typename T>
  void Bind(size_t pos, const std::vector<T>& val, Direction dir = PD_IN) {
    fun_check(dir == PD_IN);

    typename std::vector<T>::iterator first = const_cast<std::vector<T> &>(val).begin();
    typename std::vector<T>::iterator last = const_cast<std::vector<T> &>(val).end();

    RealContainerBind<typename std::vector<T>::iterator, T>(pos, first, last);
  }

  template <typename T>
  void Bind(size_t pos, const std::deque<T>& val, Direction dir = PD_IN) {
    fun_check(dir == PD_IN);

    typename std::deque<T>::iterator first = const_cast<std::deque<T> &>(val).begin();
    typename std::deque<T>::iterator last = const_cast<std::deque<T> &>(val).end();

    RealContainerBind<typename std::deque<T>::iterator, T>(pos, first, last);
  }

  template <typename T>
  void Bind(size_t pos, const std::list<T>& val, Direction dir = PD_IN) {
    fun_check(dir == PD_IN);

    typename std::list<T>::iterator first = const_cast<std::list<T> &>(val).begin();
    typename std::list<T>::iterator last = const_cast<std::list<T> &>(val).end();

    RealContainerBind<typename std::list<T>::iterator, T>(pos, first, last);
  }

  /**
   * Return count of bound parameters
   */
  size_t Count() const;

  /**
   * Return the vector of bound parameters.
   */
  InputParameterVector BindVector() const;

  /**
   * obtain the current version of the bound data and update the internal representation
   */
  void UpdateBindVectorToCurrentValues();

 private:
  /**
   * Don't copy the binder
   */
  Binder(const Binder&);

  /**
   * Binds a const char ptr.
   * This is a private no-op in this implementation
   * due to security risk.
   */
  virtual void Bind(size_t, const char* const&, Direction, const WhenNullCb&) {
  }

  /**
   * Common Bind implementation
   */
  void RealBind(size_t position, fun::sql::MetaColumn::ColumnDataType field_type, const void* aBufferPtr, size_t aLength);

  template <class Iterator, typename T>
  void RealContainerBind(size_t pos, Iterator first, Iterator last) {
    try {
      String prepared_string = "";
      Iterator begin = first;

      for (; first != last; ++first) {
        if (first != begin) {
          prepared_string.Append(1, '\n');
        }

        Binder::Ptr tmp_binder = new Binder();

        size_t pos = 0;

        BindingBase::Ptr tmp_binding = new Binding<T>(*first);
        tmp_binding->SetBinder(tmp_binder);
        tmp_binding->Bind(pos);
        tmp_binder->UpdateBindVectorToCurrentValues();

        InputParameterVector params = tmp_binder->BindVector();

        InputParameterVector::const_iterator paramsIt = params.cbegin();
        InputParameterVector::const_iterator ParamsItEnd = params.cend();

        for (; paramsIt != ParamsItEnd; ++paramsIt) {
          if (paramsIt != params.cbegin()) {
            prepared_string.Append(1, '\t');
          }

          prepared_string.Append(static_cast<const char*>(paramsIt->pInternalRepresentation()));
        }
      }

      InputParameter input_param(fun::sql::MetaColumn::FDT_STRING, nullptr, 0);
      input_param.SetStringVersionRepresentation(prepared_string);

      if (pos >= bind_vector_.size()) {
        bind_vector_.resize(pos + 1);
      }

      bind_vector_[pos] = input_param;
    } catch (std::bad_alloc&) {
      PostgreSqlException("Memory allocation error while binding");
    }
  }

 private:
  InputParameterVector bind_vector_;
};

} // namespace postgresql
} // namespace sql
} // namespace fun
