#pragma once

#include "fun/sql/meta_column.h"

#include <vector>

#include <libpq-fe.h>

namespace fun {
namespace sql {
namespace postgresql {

/**
 * Oid constants duplicated from PostgreSQL "include/postgresql/server/catalog/pg_type.h"
 * because PostgreSQL compile time definitions are too onerous to reproduce for this module
 */
const Oid BOOLOID = 16;

const Oid INT2OID = 21;
const Oid INT4OID = 23;
const Oid INT8OID = 20;

const Oid FLOAT8OID = 701; // double
const Oid FLOAT4OID = 700;
const Oid NUMERICOID = 1700;

const Oid CHAROID = 18;
const Oid BPCHAROID = 1042; // fixed length char
const Oid VARCHAROID = 1043;

const Oid BYTEAOID = 17; // BLOB
const Oid TEXTOID = 25; // CLOB

const Oid DATEOID = 1082;
const Oid TIMEOID = 1083;
const Oid TIMETZOID = 1266;
const Oid TIMESTAMPOID = 1114;
const Oid TIMESTAMPZOID = 1184;

// future use
const Oid BITOID = 1560;
const Oid VARYBITOID = 1562;
const Oid CASHOID = 790;
const Oid MACADDROID = 829;
const Oid UUIDOID = 2950;

fun::sql::MetaColumn::ColumnDataType OidToColumnDataType(const Oid oid);

/**
 * PostgreSQL class to record values for input parameters to sql statements
 */
class InputParameter {
 public:
  typedef fun::sql::MetaColumn::ColumnDataType CDT;

  explicit InputParameter(CDT field_type, const void* data, size_t data_len);
  explicit InputParameter();

  ~InputParameter();

  CDT fieldType() const;
  const void* pData() const;
  size_t size() const;
  bool IsBinary() const;

  void SetStringVersionRepresentation(const String& aString);
  void SetNonStringVersionRepresentation(const void* aPtr, size_t theSize);

  const void* pInternalRepresentation() const;

 private:
  CDT field_type_;
  const void* data_;
  size_t size_;
  bool is_binary_;
  String _stringVersionRepresentation;
  void* _pNonStringVersionRepresentation;
};

typedef std::vector <InputParameter> InputParameterVector;


/**
 * PostgreSQL class to record values for output parameters to capture the results
 */
class OutputParameter {
 public:
  typedef fun::sql::MetaColumn::ColumnDataType CDT;

  OutputParameter(CDT field_type, Oid internalFieldType, size_t rowNumber,
    const char* dataPtr, size_t size, bool IsNull);

  OutputParameter();
  ~OutputParameter();

  void setValues(CDT field_type, Oid internalFieldType, size_t rowNumber,
    const char* dataPtr, size_t size, bool IsNull);

  CDT fieldType() const;
  Oid internalFieldType() const;
  size_t rowNumber() const;
  const char* pData() const;
  size_t size() const;
  bool IsNull() const;

 private:
  CDT field_type_;
  Oid internal_field_type_;
  size_t row_number_;
  const char* data_;
  size_t size_;
  bool is_null_;
};

typedef std::vector <OutputParameter> OutputParameterVector;


/**
 * PostgreSQL connection Info Options free (RAII)
 */
class PQConnectionInfoOptionsFree {
 public:
  explicit PQConnectionInfoOptionsFree(PQconninfoOption* aConnectionInfoOptionPtr);
  ~PQConnectionInfoOptionsFree();

 private:
  PQConnectionInfoOptionsFree(const PQConnectionInfoOptionsFree&) = delete;
  PQConnectionInfoOptionsFree& operator=(const PQConnectionInfoOptionsFree&) = delete;

 private:
  PQconninfoOption* connection_info_option_;
};


/**
 * PostgreSQL statement result free (RAII)
 */
class PQResultClear {
 public:
  explicit PQResultClear(PGresult * aPQResultPtr);
  ~PQResultClear();

 private:
  PQResultClear(const PQResultClear&);
  PQResultClear& operator=(const PQResultClear&);

 private:
  PGresult* pg_result_;
};


/**
 * PostgreSQL Cancel Info Options free (RAII)
 */
class PGCancelFree {
 public:
  explicit PGCancelFree(PGcancel * aStatementCancelPtr);
  ~PGCancelFree();

 private:
  PGCancelFree(const PGCancelFree&);
  PGCancelFree& operator=(const PGCancelFree&);

 private:
  PGcancel* pg_cancel_;
};


//
// inlines
//

// InputParameter

inline InputParameter::InputParameter(fun::sql::MetaColumn::ColumnDataType  field_type,
                                      const void* aDataPtr, size_t theSize)
  : field_type_(field_type),
    data_(aDataPtr),
    size_(theSize),
    is_binary_(false),
    _pNonStringVersionRepresentation(0) {
  if (fun::sql::MetaColumn::FDT_BLOB == field_type_
   || fun::sql::MetaColumn::FDT_CLOB == field_type_) {
    is_binary_ = true;
  }
}

inline InputParameter::InputParameter()
  : field_type_(fun::sql::MetaColumn::FDT_UNKNOWN),
    data_(0),
    size_(0),
    is_binary_(false),
    _pNonStringVersionRepresentation(0) {}

inline InputParameter::~InputParameter() {}

inline const void* InputParameter::pData() const {
  return data_;
}

inline fun::sql::MetaColumn::ColumnDataType InputParameter::fieldType() const {
  return field_type_;
}

inline size_t InputParameter::size() const {
  return size_;
}

inline bool InputParameter::IsBinary() const {
  return is_binary_;
}

inline void InputParameter::SetStringVersionRepresentation(const String& aString) {
  _pNonStringVersionRepresentation = 0;
  _stringVersionRepresentation = aString;
  size_ = _stringVersionRepresentation.size();
}

inline void InputParameter::SetNonStringVersionRepresentation(const void* aPtr, size_t theDataLength) {
  _stringVersionRepresentation = String();
  _pNonStringVersionRepresentation = const_cast<void *> (aPtr);
  size_ = theDataLength;
}

inline const void* InputParameter::pInternalRepresentation() const {
  switch (field_type_) {
    case fun::sql::MetaColumn::FDT_BOOL:
    case fun::sql::MetaColumn::FDT_INT8:
    case fun::sql::MetaColumn::FDT_UINT8:
    case fun::sql::MetaColumn::FDT_INT16:
    case fun::sql::MetaColumn::FDT_UINT16:
    case fun::sql::MetaColumn::FDT_INT32:
    case fun::sql::MetaColumn::FDT_UINT32:
    case fun::sql::MetaColumn::FDT_INT64:
    case fun::sql::MetaColumn::FDT_UINT64:
    case fun::sql::MetaColumn::FDT_FLOAT:
    case fun::sql::MetaColumn::FDT_DOUBLE:
    case fun::sql::MetaColumn::FDT_STRING:
    case fun::sql::MetaColumn::FDT_DATE:
    case fun::sql::MetaColumn::FDT_TIME:
    case fun::sql::MetaColumn::FDT_TIMESTAMP:
      return _stringVersionRepresentation.c_str();

    case fun::sql::MetaColumn::FDT_BLOB:
    case fun::sql::MetaColumn::FDT_CLOB:
      return _pNonStringVersionRepresentation;

    case fun::sql::MetaColumn::FDT_UNKNOWN:
    default: return nullptr;
  }
}


// OutputParameter

inline OutputParameter::OutputParameter(fun::sql::MetaColumn::ColumnDataType field_type,
                                        Oid anInternalFieldType,
                                        size_t aRowNumber,
                                        const char* aDataPtr,
                                        size_t theSize,
                                        bool anIsNull)
  : field_type_(field_type),
    internal_field_type_(anInternalFieldType),
    row_number_(aRowNumber),
    data_(aDataPtr),
    size_(theSize),
    is_null_(anIsNull) {}

inline OutputParameter::OutputParameter()
  : field_type_(fun::sql::MetaColumn::FDT_UNKNOWN),
    internal_field_type_(static_cast<Oid>(-1)),
    row_number_(0),
    data_(0),
    size_(0),
    is_null_(true) {}

inline OutputParameter::~OutputParameter() {}

inline void OutputParameter::setValues( fun::sql::MetaColumn::ColumnDataType field_type,
                                        Oid anInternalFieldType,
                                        size_t aRowNumber,
                                        const char* aDataPtr,
                                        size_t theSize,
                                        bool anIsNull) {
    field_type_ = field_type;
    internal_field_type_ = anInternalFieldType;
    row_number_ = aRowNumber;
    data_ = aDataPtr;
    size_ = theSize;
    is_null_ = anIsNull;
}

inline fun::sql::MetaColumn::ColumnDataType OutputParameter::fieldType() const {
  return field_type_;
}

inline Oid OutputParameter::internalFieldType() const {
  return internal_field_type_;
}

inline size_t OutputParameter::rowNumber() const {
  return row_number_;
}

inline const char* OutputParameter::pData() const {
  return data_;
}

inline size_t OutputParameter::size() const {
  return size_;
}

inline bool OutputParameter::IsNull() const {
  return is_null_;
}


// PQConnectionInfoOptionsFree

inline PQConnectionInfoOptionsFree::PQConnectionInfoOptionsFree(PQconninfoOption* aConnectionInfoOptionPtr)
  : connection_info_option_(aConnectionInfoOptionPtr) {}

inline PQConnectionInfoOptionsFree::~PQConnectionInfoOptionsFree() {
  if (connection_info_option_) {
    PQconninfoFree(connection_info_option_);
    connection_info_option_ = 0;
  }
}


// PQResultClear

inline PQResultClear::PQResultClear(PGresult* aPQResultPtr)
  : pg_result_(aPQResultPtr) {}

inline PQResultClear::~PQResultClear() {
  if (pg_result_) {
    PQclear(pg_result_);
    pg_result_ = 0;
  }
}


// PGCancelFree

inline PGCancelFree::PGCancelFree(PGcancel* aStatementCancelPtr)
  : pg_cancel_(aStatementCancelPtr) {}

inline PGCancelFree::~PGCancelFree() {
  if (pg_cancel_) {
    PQfreeCancel(pg_cancel_);
    pg_cancel_ = nullptr;
  }
}

} // namespace postgresql
} // namespace sql
} // namespace fun
