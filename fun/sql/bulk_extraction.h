#pragma once

#include <vector>
#include "fun/sql/bulk.h"
#include "fun/sql/extraction_base.h"
#include "fun/sql/preparation.h"
#include "fun/sql/sql.h"

namespace fun {
namespace sql {

/**
 * Specialization for bulk extraction of values from a query result set.
 * Bulk extraction support is provided only for following STL containers:
 * - std::vector
 * - std::deque
 * - std::list
 */
template <typename C>
class BulkExtraction : public ExtractionBase {
 public:
  typedef C ValType;
  typedef typename C::value_type CValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef BulkExtraction<ValType> Type;
  typedef SharedPtr<Type> Ptr;

  BulkExtraction(C& result_ref, uint32 limit, const Position& pos = Position(0))
      : ExtractionBase(typeid(C).name(), limit, pos.value(), true),
        result_(result_ref),
        default_() {
    if (static_cast<uint32>(result_ref.size()) != limit) {
      result_ref.resize(limit);
    }
  }

  BulkExtraction(C& result_ref, const CValType& def, uint32 limit,
                 const Position& pos = Position(0))
      : ExtractionBase(typeid(C).name(), limit, pos.value(), true),
        result_(result_ref),
        default_(def) {
    if (static_cast<uint32>(result_ref.size()) != limit) {
      result_ref.resize(limit);
    }
  }

  virtual ~BulkExtraction() {}

  size_t HandledColumnsCount() const { return TypeHandler<C>::size(); }

  size_t HandledRowsCount() const { return result_.size(); }

  size_t AllowedRowCount() const { return GetLimit(); }

  bool IsNull(size_t row) const {
    try {
      return nulls_.at(row);
    } catch (std::out_of_range& ex) {
      throw RangeException(ex.what());
    }
  }

  size_t Extract(size_t col) {
    ExtractorBase::Ptr ext = GetExtractor();
    TypeHandler<C>::Extract(col, result_, default_, ext);
    typename C::iterator it = result_.begin();
    typename C::iterator end = result_.end();
    for (int row = 0; it != end; ++it, ++row) {
      nulls_.push_back(IsValueNull(*it, ext->IsNull(col, row)));
    }

    return result_.size();
  }

  virtual void Reset() {}

  PreparationBase::Ptr CreatePreparation(PreparatorBase::Ptr& prep,
                                         size_t col) {
    uint32 limit = GetLimit();
    if (limit != result_.size()) {
      result_.resize(limit);
    }
    prep->SetLength(limit);
    prep->SetBulk(true);
    return new Preparation<C>(prep, col, result_);
  }

 protected:
  const C& result() const { return result_; }

 private:
  C& result_;
  CValType default_;
  std::deque<bool> nulls_;
};

/**
 * Container Data Type specialization extension for extraction of values from a
 * query result set.
 *
 * This class is intended for PocoData internal use - it is used by
 * StatementImpl to automatically create internal BulkExtraction in cases when
 * statement returns data and no external storage was supplied. It is later used
 * by RecordSet to retrieve the fetched data after statement execution. It takes
 * ownership of the Column pointer supplied as constructor argument. Column
 * object, in turn owns the data container pointer.
 *
 * InternalBulkExtraction objects can not be copied or assigned.
 */
template <typename C>
class InternalBulkExtraction : public BulkExtraction<C> {
 public:
  typedef C ValType;
  typedef typename C::value_type CValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef InternalBulkExtraction<ValType> Type;
  typedef SharedPtr<Type> Ptr;

  /**
   * Creates InternalBulkExtraction.
   */
  InternalBulkExtraction(C& result_ref, Column<C>* pColumn, uint32 limit,
                         const Position& pos = Position(0))
      : BulkExtraction<C>(result_ref, CValType(), limit, pos),
        column_(pColumn) {}

  /**
   * Destroys InternalBulkExtraction.
   */
  ~InternalBulkExtraction() { delete column_; }

  void Reset() { column_->Reset(); }

  const CValType& value(int index) const {
    try {
      return BulkExtraction<C>::result().At(index);
    } catch (std::out_of_range& ex) {
      throw RangeException(ex.what());
    }
  }

  bool IsNull(size_t row) const { return BulkExtraction<C>::IsNull(row); }

  const Column<C>& column() const { return *column_; }

 private:
  InternalBulkExtraction() = delete;
  InternalBulkExtraction(const InternalBulkExtraction&) = delete;
  InternalBulkExtraction& operator=(const InternalBulkExtraction&) = delete;

  Column<C>* column_;
};

namespace Keywords {

/**
 * Convenience function to allow for a more compact creation of an extraction
 * object with std::vector bulk extraction support.
 */
template <typename T>
ExtractionBase::Ptr into(std::vector<T>& t, const Bulk& bulk,
                         const Position& pos = Position(0)) {
  return new BulkExtraction<std::vector<T> >(t, bulk.size(), pos);
}

/**
 * Convenience function to allow for a more compact creation of an extraction
 * object with std::vector bulk extraction support.
 */
template <typename T>
ExtractionBase::Ptr into(std::vector<T>& t, BulkFnType,
                         const Position& pos = Position(0)) {
  uint32 size = static_cast<uint32>(t.size());
  if (0 == size) {
    throw InvalidArgumentException("Zero length not allowed.");
  }

  return new BulkExtraction<std::vector<T> >(t, size, pos);
}

/**
 * Convenience function to allow for a more compact creation of an extraction
 * object with std::deque bulk extraction support.
 */
template <typename T>
ExtractionBase::Ptr into(std::deque<T>& t, const Bulk& bulk,
                         const Position& pos = Position(0)) {
  return new BulkExtraction<std::deque<T> >(t, bulk.size(), pos);
}

/**
 * Convenience function to allow for a more compact creation of an extraction
 * object with std::deque bulk extraction support.
 */
template <typename T>
ExtractionBase::Ptr into(std::deque<T>& t, BulkFnType,
                         const Position& pos = Position(0)) {
  uint32 size = static_cast<uint32>(t.size());
  if (0 == size) {
    throw InvalidArgumentException("Zero length not allowed.");
  }

  return new BulkExtraction<std::deque<T> >(t, size, pos);
}

/**
 * Convenience function to allow for a more compact creation of an extraction
 * object with std::list bulk extraction support.
 */
template <typename T>
ExtractionBase::Ptr into(std::list<T>& t, const Bulk& bulk,
                         const Position& pos = Position(0)) {
  return new BulkExtraction<std::list<T> >(t, bulk.size(), pos);
}

/**
 * Convenience function to allow for a more compact creation of an extraction
 * object with std::list bulk extraction support.
 */
template <typename T>
ExtractionBase::Ptr into(std::list<T>& t, BulkFnType,
                         const Position& pos = Position(0)) {
  uint32 size = static_cast<uint32>(t.size());
  if (0 == size) {
    throw InvalidArgumentException("Zero length not allowed.");
  }

  return new BulkExtraction<std::list<T> >(t, size, pos);
}

}  // namespace Keywords

}  // namespace sql
}  // namespace fun
