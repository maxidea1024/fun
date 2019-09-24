#pragma once

#include "fun/sql/sql.h"
#include "fun/sql/extractor_base.h"
#include "fun/sql/preparation_base.h"
#include "fun/sql/limit.h"
#include "fun/ref_counted_object.h"
#include "fun/UTFString.h"
#include "fun/ref_counted_ptr.h"

#include <vector>
#include <deque>
#include <list>
#include <cstddef>

namespace fun {
namespace sql {

class PreparatorBase;

/**
 * ExtractionBase is the interface class that connects output positions to concrete values
 * retrieved via an ExtractorBase.
 */
class FUN_SQL_API ExtractionBase {
 public:
  typedef SharedPtr<ExtractionBase> Ptr;
  typedef SharedPtr<ExtractorBase> ExtractorPtr;
  typedef SharedPtr<PreparatorBase> PreparatorPtr;

  /**
   * Creates the ExtractionBase. A limit value equal to EXTRACT_UNLIMITED (0xffffffffu)
   * means that we extract as much data as possible during one execute.
   * Otherwise the limit value is used to partition data extracting to a limited amount of rows.
   */
  ExtractionBase( const String& type, uint32 limit = Limit::LIMIT_UNLIMITED,
                  uint32 position = 0, bool bulk = false);

  /**
   * Destroys the ExtractionBase.
   */
  virtual ~ExtractionBase();

  /**
   * Sets the class used for extracting the data. Does not take ownership of the pointer.
   */
  void SetExtractor(ExtractorPtr extractor);

  /**
   * Retrieves the extractor object
   */
  ExtractorPtr GetExtractor() const;

  /**
   * Returns the extraction position.
   */
  uint32 position() const;

  /**
   * Returns the number of columns that the extraction handles.
   *
   * The trivial case will be one single column but when
   * complex types are used this value can be larger than one.
   */
  virtual size_t HandledColumnsCount() const = 0;

  /**
   * Returns the number of rows that the extraction handles.
   *
   * The trivial case will be one single row but
   * for collection data types (ie vector) it can be larger.
   */
  virtual size_t HandledRowsCount() const = 0;

  /**
   * Returns the upper limit on number of rows that the extraction will handle.
   */
  virtual size_t AllowedRowCount() const = 0;

  /**
   * Extracts a value from the param, starting at the given column position.
   * Returns the number of rows extracted.
   */
  virtual size_t Extract(size_t pos) = 0;

  /**
   * Resets the extractor so that it can be re-used.
   * Does nothing in this implementation.
   * Implementations should override it for different behavior.
   */
  virtual void Reset();

  /**
   * Returns true. Implementations should override it for different behavior.
   */
  virtual bool CanExtract() const;

  /**
   * Creates and returns shared pointer to Preparation object for the extracting object.
   */
  virtual PreparationBase::Ptr CreatePreparation(PreparatorPtr& prep, size_t pos) = 0;

  /**
   * Sets the limit.
   */
  void SetLimit(uint32 limit);

  /**
   * Gets the limit.
   */
  uint32 GetLimit() const;

  /**
   * In implementations, this function returns true if value at row is null,
   * false otherwise.
   * Normal behavior is to replace nulls with default values.
   * However, extraction implementations may remember the underlying database
   * null values and be able to later provide information about them.
   * Here, this function throws NotImplementedException.
   */
  virtual bool IsNull(std::size_t row) const;

  /**
   * Returns true if this is bulk extraction.
   */
  bool IsBulk() const;

  /**
   * Sets the empty string handling flag.
   */
  void SetEmptyStringIsNull(bool empty_string_is_null);

  /**
   * Returns the empty string handling flag.
   */
  bool GetEmptyStringIsNull() const;

  /**
   * Sets the force empty string flag.
   */
  void SetForceEmptyString(bool force_empty_string);

  /**
   * Returns the force empty string flag.
   */
  bool GetForceEmptyString() const;

  /**
   * Utility function to determine the nullness of the value.
   * This generic version always returns default value
   * (i.e. does nothing). The String overload does
   * the actual work.
   */
  template <typename T>
  bool IsValueNull(const T& /*str*/, bool default_value) {
    return default_value;
  }

  /**
   * Overload for const reference to String.
   *
   * Returns true when following conditions are met:
   *
   * - string is empty
   * - GetEmptyStringIsNull() returns true
   */
  bool IsValueNull(const String& str, bool default_value);

  /**
   * Overload for const reference to UString.
   *
   * Returns true when following conditions are met:
   *
   * - string is empty
   * - GetEmptyStringIsNull() returns true
   */
  bool IsValueNull(const fun::UString& str, bool default_value);

  const String& Type() const {
    return type_;
  }

 private:
  template <typename S>
  bool IsStringNull(const S& str, bool default_value) {
    if (GetForceEmptyString()) {
      return false;
    }

    if (GetEmptyStringIsNull() && str.IsEmpty()) {
      return true;
    }

    return default_value;
  }

  String type_;
  ExtractorPtr extractor_;
  uint32 limit_;
  uint32 position_;
  bool bulk_;
  bool empty_string_is_null_;
  bool force_empty_string_;
};


typedef std::vector<ExtractionBase::Ptr> ExtractionBaseVec;
typedef std::vector<ExtractionBaseVec>   ExtractionBaseVecVec;

//
// inlines
//

inline void ExtractionBase::SetExtractor(ExtractorPtr extractor) {
  extractor_ = extractor;
}

inline ExtractionBase::ExtractorPtr ExtractionBase::GetExtractor() const {
  return extractor_;
}

inline void ExtractionBase::SetLimit(uint32 limit) {
  limit_ = limit;
}

inline uint32 ExtractionBase::GetLimit() const {
  return limit_;
}

inline bool ExtractionBase::IsNull(std::size_t /*row*/) const {
  throw NotImplementedException("Check for null values not implemented.");
}

inline uint32 ExtractionBase::position() const {
  return position_;
}

inline bool ExtractionBase::IsBulk() const {
  return bulk_;
}

inline void ExtractionBase::reset() {
}

inline bool ExtractionBase::CanExtract() const {
  return true;
}

inline void ExtractionBase::SetEmptyStringIsNull(bool empty_string_is_null) {
  empty_string_is_null_ = empty_string_is_null;
}

inline bool ExtractionBase::GetEmptyStringIsNull() const {
  return empty_string_is_null_;
}

inline void ExtractionBase::SetForceEmptyString(bool force_empty_string) {
  force_empty_string_ = force_empty_string;
}

inline bool ExtractionBase::GetForceEmptyString() const {
  return force_empty_string_;
}

inline bool ExtractionBase::IsValueNull(const String& str, bool default_value) {
  return IsStringNull(str, default_value);
}

inline bool ExtractionBase::IsValueNull(const fun::UString& str, bool default_value) {
  return IsStringNull(str, default_value);
}

} // namespace sql
} // namespace fun
