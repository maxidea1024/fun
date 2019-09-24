#pragma once

#include "fun/sql/sql.h"
#include "fun/types.h"
#include "fun/sql/extraction_base.h"
#include "fun/sql/preparation.h"
#include "fun/sql/type_handler.h"
#include "fun/sql/column.h"
#include "fun/sql/position.h"
#include "fun/sql/sql_exception.h"

#include <set>
#include <vector>
#include <list>
#include <deque>
#include <map>
#include <cstddef>

namespace fun {
namespace sql {

template <typename CollTp, typename ElemTp = typename CollTp::value_type>
struct DefaultInitializer {
  static void AppendElem(CollTp& coll, const ElemTp& default_value) {
    coll.push_back(default_value);
  }
};

template <typename CollTp>
struct DefaultInitializer<CollTp, CLOB> {
  static void AppendElem(CollTp& coll, const CLOB& default_value) {
    CLOB v(default_value.GetRawContent(), default_value.size());
    coll.push_back(v);
  }
};

template <typename CollTp>
struct DefaultInitializer<CollTp, BLOB> {
  static void AppendElem(CollTp& coll, const BLOB& default_value) {
    BLOB v(default_value.GetRawContent(), default_value.size());
    coll.push_back(v);
  }
};

template <typename CollTp>
inline void AppendElem(CollTp& coll, const typename CollTp::value_type& default_value) {
  DefaultInitializer<CollTp, typename CollTp::value_type>::AppendElem(coll, default_value);
}


/**
 * Concrete Data Type specific extraction of values from a query result set.
 */
template <typename T>
class Extraction : public ExtractionBase {
 public:
  typedef T                   ValType;
  typedef SharedPtr<ValType>  ValPtr;
  typedef Extraction<ValType> Type;
  typedef SharedPtr<Type>     Ptr;

  /**
   * Creates an Extraction object at specified position.
   * Uses an empty object T as default value.
   */
  Extraction(T& result_ref, const Position& pos = Position(0))
    : ExtractionBase(typeid(T).name(), static_cast<Limit::SizeT>(Limit::LIMIT_UNLIMITED), pos.Value()),
      result_(result_ref),
      default_(),
      extracted_(false),
      null_(false) {}

  /**
   * Creates an Extraction object at specified position.
   * Uses the provided def object as default value.
   */
  Extraction(T& result_ref, const T& def, const Position& pos = Position(0))
    : ExtractionBase(typeid(T).name(), Limit::LIMIT_UNLIMITED, pos.Value()),
      result_(result_ref),
      default_(def),
      extracted_(false),
      null_(false) {}

  /**
   * Destroys the Extraction object.
   */
  ~Extraction() {}

  size_t HandledColumnsCount() const {
    return TypeHandler<T>::size();
  }

  size_t HandledRowsCount() const {
    return extracted_ ? 1u : 0;
  }

  size_t AllowedRowCount() const {
    return 1u;
  }

  bool IsNull(size_t /*row*/ = 0) const {
    return null_;
  }

  size_t Extract(size_t pos) {
    if (extracted_) {
      throw ExtractException("value already extracted");
    }
    
    extracted_ = true;
    ExtractorBase::Ptr ext = GetExtractor();
    TypeHandler<T>::Extract(pos, result_, default_, ext);
    null_ = IsValueNull<T>(result_, ext->IsNull(pos));
    
    return 1u;
  }

  void Reset() {
    extracted_ = false;
  }

  bool CanExtract() const {
    return !extracted_;
  }

  PreparationBase::Ptr CreatePreparation(PreparatorBase::Ptr& prep, size_t pos) {
    return new Preparation<T>(prep, pos, result_);
  }

 private:
  T& result_;
  T default_;
  bool extracted_;
  bool null_;
};

/**
 * Vector Data Type specialization for extraction of values from a query result set.
 */
template <typename T>
class Extraction<std::vector<T> > : public ExtractionBase {
 public:
  typedef std::vector<T> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef Extraction<ValType> Type;
  typedef SharedPtr<Type> Ptr;

  Extraction(std::vector<T>& result_ref, const Position& pos = Position(0))
    : ExtractionBase(typeid(std::vector<T>).name(), Limit::LIMIT_UNLIMITED, pos.Value()),
      result_(result_ref),
      default_() {
    result_.Clear();
  }

  Extraction(std::vector<T>& result_ref, const T& def, const Position& pos = Position(0))
    : ExtractionBase(typeid(std::vector<T>).name(), static_cast<Limit::SizeT>(Limit::LIMIT_UNLIMITED), pos.Value()),
      result_(result_ref),
      default_(def) {
    result_.Clear();
  }

  virtual ~Extraction() {}

  size_t HandledColumnsCount() const {
    return TypeHandler<T>::size();
  }

  size_t HandledRowsCount() const {
    return static_cast<size_t>(result_.size());
  }

  size_t AllowedRowCount() const {
    return GetLimit();
  }

  bool IsNull(size_t row) const {
    try {
      return nulls_.at(row);
    } catch (std::out_of_range& e) {
      throw RangeException(e.what());
    }
  }

  size_t Extract(size_t pos) {
    ExtractorBase::Ptr ext = GetExtractor();
    AppendElem(result_, default_);
    TypeHandler<T>::Extract(pos, result_.back(), default_, ext);
    nulls_.push_back(IsValueNull(result_.back(), ext->IsNull(pos)));
    return 1u;
  }

  PreparationBase::Ptr CreatePreparation(PreparatorBase::Ptr& prep, size_t pos) {
    return new Preparation<T>(prep, pos, default_);
  }

  void Reset() {
    nulls_.Clear();
  }

 protected:
  const std::vector<T>& result() const {
    return result_;
  }

 private:
  std::vector<T>& result_;
  T default_;
  std::deque<bool> nulls_;
};

/**
 * Vector bool specialization for extraction of values from a query result set.
 */
template <>
class Extraction<std::vector<bool> > : public ExtractionBase {
 public:
  typedef std::vector<bool> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef Extraction<ValType> Type;
  typedef SharedPtr<Type> Ptr;

  Extraction(std::vector<bool>& result_ref, const Position& pos = Position(0))
    : ExtractionBase(typeid(std::vector<bool>).name(), static_cast<Limit::SizeT>(Limit::LIMIT_UNLIMITED), pos.Value()),
      result_(result_ref),
      default_() {
    result_.Clear();
  }

  Extraction(std::vector<bool>& result_ref, const bool& def, const Position& pos = Position(0))
    : ExtractionBase(typeid(std::vector<bool>).name(), static_cast<Limit::SizeT>(Limit::LIMIT_UNLIMITED), pos.Value()),
      result_(result_ref),
      default_(def) {
    result_.Clear();
  }

  virtual ~Extraction() {}

  size_t HandledColumnsCount() const {
    return TypeHandler<bool>::size();
  }

  size_t HandledRowsCount() const {
    return static_cast<size_t>(result_.size());
  }

  size_t AllowedRowCount() const {
    return GetLimit();
  }

  bool IsNull(size_t row) const {
    try {
      return nulls_.at(row);
    } catch (std::out_of_range& e) {
      throw RangeException(e.what());
    }
  }

  size_t Extract(size_t pos) {
    ExtractorBase::Ptr ext = GetExtractor();

    bool tmp = default_;
    TypeHandler<bool>::Extract(pos, tmp, default_, ext);
    result_.push_back(tmp);
    nulls_.push_back(ext->IsNull(pos));
    return 1u;
  }

  PreparationBase::Ptr CreatePreparation(PreparatorBase::Ptr& prep, size_t pos) {
    return new Preparation<bool>(prep, pos, default_);
  }

  void Reset() {
    nulls_.Clear();
  }

 protected:
  const std::vector<bool>& result() const {
    return result_;
  }

 private:
  std::vector<bool>& result_;
  bool default_;
  std::deque<bool> nulls_;
};

/**
 * List Data Type specialization for extraction of values from a query result set.
 */
template <typename T>
class Extraction<std::list<T> > : public ExtractionBase {
 public:
  typedef std::list<T> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef Extraction<ValType> Type;
  typedef SharedPtr<Type> Ptr;

  Extraction(std::list<T>& result_ref, const Position& pos = Position(0))
    : ExtractionBase(typeid(std::list<T>).name(), Limit::LIMIT_UNLIMITED, pos.Value()),
      result_(result_ref),
      default_() {
    result_.Clear();
  }

  Extraction(std::list<T>& result_ref, const T& def, const Position& pos = Position(0))
    : ExtractionBase(typeid(std::list<T>).name(), static_cast<Limit::SizeT>(Limit::LIMIT_UNLIMITED), pos.Value()),
      result_(result_ref),
      default_(def) {
    result_.Clear();
  }

  virtual ~Extraction() {}

  size_t HandledColumnsCount() const {
    return TypeHandler<T>::size();
  }

  size_t HandledRowsCount() const {
    return result_.size();
  }

  size_t AllowedRowCount() const {
    return GetLimit();
  }

  bool IsNull(size_t row) const {
    try {
      return nulls_.at(row);
    } catch (std::out_of_range& e) {
      throw RangeException(e.what());
    }
  }

  size_t Extract(size_t pos) {
    ExtractorBase::Ptr ext = GetExtractor();
    AppendElem(result_, default_);
    TypeHandler<T>::Extract(pos, result_.back(), default_, ext);
    nulls_.push_back(IsValueNull(result_.back(), ext->IsNull(pos)));
    return 1u;
  }

  PreparationBase::Ptr CreatePreparation(PreparatorBase::Ptr& prep, size_t pos) {
    return new Preparation<T>(prep, pos, default_);
  }

  void Reset() {
    nulls_.Clear();
  }

 protected:
  const std::list<T>& result() const {
    return result_;
  }

 private:
  std::list<T>& result_;
  T default_;
  std::deque<bool> nulls_;
};

/**
 * Deque Data Type specialization for extraction of values from a query result set.
 */
template <typename T>
class Extraction<std::deque<T> > : public ExtractionBase {
 public:
  typedef std::deque<T> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef Extraction<ValType> Type;
  typedef SharedPtr<Type> Ptr;

  Extraction(std::deque<T>& result_ref, const Position& pos = Position(0))
    : ExtractionBase(typeid(std::deque<T>).name(), Limit::LIMIT_UNLIMITED, pos.Value()),
      result_(result_ref),
      default_() {
    result_.Clear();
  }

  Extraction(std::deque<T>& result_ref, const T& def, const Position& pos = Position(0))
    : ExtractionBase(typeid(std::deque<T>).name(), static_cast<Limit::SizeT>(Limit::LIMIT_UNLIMITED), pos.Value()),
      result_(result_ref),
      default_(def) {
    result_.Clear();
  }

  virtual ~Extraction() {}

  size_t HandledColumnsCount() const {
    return TypeHandler<T>::size();
  }

  size_t HandledRowsCount() const {
    return result_.size();
  }

  size_t AllowedRowCount() const {
    return GetLimit();
  }

  bool IsNull(size_t row) const {
    try {
      return nulls_.at(row);
    } catch (std::out_of_range& e) {
      throw RangeException(e.what());
    }
  }

  size_t Extract(size_t pos) {
    ExtractorBase::Ptr ext = GetExtractor();
    AppendElem(result_, default_);
    TypeHandler<T>::Extract(pos, result_.back(), default_, ext);
    nulls_.push_back(IsValueNull(result_.back(), ext->IsNull(pos)));
    return 1u;
  }

  PreparationBase::Ptr CreatePreparation(PreparatorBase::Ptr& prep, size_t pos) {
    return new Preparation<T>(prep, pos, default_);
  }

  void Reset() {
    nulls_.Clear();
  }

 protected:
  const std::deque<T>& result() const {
    return result_;
  }

 private:
  std::deque<T>& result_;
  T default_;
  std::deque<bool> nulls_;
};

/**
 * Container Data Type specialization extension for extraction of values from a query result set.
 *
 * This class is intended for PocoData internal use - it is used by StatementImpl
 * to automatically create internal Extraction in cases when statement returns data and no external storage
 * was supplied. It is later used by RecordSet to retrieve the fetched data after statement execution.
 * It takes ownership of the Column pointer supplied as constructor argument. Column object, in turn
 * owns the data container pointer.
 *
 * InternalExtraction objects can not be copied or assigned.
 */
template <typename C>
class InternalExtraction : public Extraction<C> {
 public:
  typedef typename C::value_type ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef Extraction<ValType> Type;
  typedef SharedPtr<Type> Ptr;

  /**
   * Creates InternalExtraction.
   */
  InternalExtraction(C& result_ref, Column<C>* pColumn, const Position& pos = Position(0))
    : Extraction<C>(result_ref, ValType(), pos),
      column_(pColumn) {}

  /**
   * Destroys InternalExtraction.
   */
  ~InternalExtraction() {
    delete column_;
  }

  void Reset() {
    Extraction<C>::Reset();
    column_->Reset();
  } 

  const ValType& ValueAt(int index) const {
    try {
      return Extraction<C>::result().at(index);
    } catch (std::out_of_range& e) {
      throw RangeException(e.what());
    }
  }

  bool IsNull(size_t row) const {
    return Extraction<C>::IsNull(row);
  }

  const Column<C>& column() const {
    return *column_;
  }

 private:
  InternalExtraction() = delete;
  InternalExtraction(const InternalExtraction&) = delete;
  InternalExtraction& operator = (const InternalExtraction&) = delete;

  Column<C>* column_;
};

/**
 * Set Data Type specialization for extraction of values from a query result set.
 */
template <typename T>
class Extraction<std::set<T> > : public ExtractionBase {
 public:
  typedef std::set<T> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef Extraction<ValType> Type;
  typedef SharedPtr<Type> Ptr;
  typedef typename ValType::iterator Iterator;

  Extraction(std::set<T>& result_ref, const Position& pos = Position(0))
    : ExtractionBase(typeid(std::set<T>).name(), Limit::LIMIT_UNLIMITED, pos.Value()),
      result_(result_ref),
      default_() {
    result_.Clear();
  }

  Extraction(std::set<T>& result_ref, const T& def, const Position& pos = Position(0))
    : ExtractionBase(typeid(std::set<T>).name(), Limit::LIMIT_UNLIMITED, pos.Value()),
      result_(result_ref),
      default_(def) {
    result_.Clear();
  }

  ~Extraction() {}

  size_t HandledColumnsCount() const {
    return TypeHandler<T>::size();
  }

  size_t HandledRowsCount() const {
    return static_cast<size_t>(result_.size());
  }

  size_t AllowedRowCount() const {
    return GetLimit();
  }

  size_t Extract(size_t pos) {
    T tmp;
    TypeHandler<T>::Extract(pos, tmp, default_, GetExtractor());
    result_.insert(tmp);
    return 1u;
  }

  PreparationBase::Ptr CreatePreparation(PreparatorBase::Ptr& prep, size_t pos) {
    return new Preparation<T>(prep, pos, default_);
  }

 private:
  std::set<T>& result_;
  T default_;
};

/**
 * Multiset Data Type specialization for extraction of values from a query result set.
 */
template <typename T>
class Extraction<std::multiset<T> > : public ExtractionBase {
 public:
  typedef std::multiset<T> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef Extraction<ValType> Type;
  typedef SharedPtr<Type> Ptr;

  Extraction(std::multiset<T>& result_ref, const Position& pos = Position(0))
    : ExtractionBase(typeid(std::multiset<T>).name(), Limit::LIMIT_UNLIMITED, pos.Value()),
      result_(result_ref),
      default_() {
    result_.Clear();
  }

  Extraction(std::multiset<T>& result_ref, const T& def, const Position& pos = Position(0))
    : ExtractionBase(typeid(std::multiset<T>).name(), Limit::LIMIT_UNLIMITED, pos.Value()),
      result_(result_ref),
      default_(def) {
    result_.Clear();
  }

  ~Extraction() {}

  size_t HandledColumnsCount() const {
    return TypeHandler<T>::size();
  }

  size_t HandledRowsCount() const {
    return static_cast<size_t>(result_.size());
  }

  size_t AllowedRowCount() const {
    return GetLimit();
  }

  size_t Extract(size_t pos) {
    T tmp;
    TypeHandler<T>::Extract(pos, tmp, default_, GetExtractor());
    result_.insert(tmp);
    return 1u;
  }

  PreparationBase::Ptr CreatePreparation(PreparatorBase::Ptr& prep, size_t pos) {
    return new Preparation<T>(prep, pos, default_);
  }

 private:
  std::multiset<T>& result_;
  T default_;
};

/**
 * Map Data Type specialization for extraction of values from a query result set.
 */
template <class K, class V>
class Extraction<std::map<K, V> > : public ExtractionBase {
 public:
  typedef std::map<K, V> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef Extraction<ValType> Type;
  typedef SharedPtr<Type> Ptr;

  Extraction(std::map<K, V>& result_ref, const Position& pos = Position(0))
    : ExtractionBase(typeid(std::map<K, V>).name(), Limit::LIMIT_UNLIMITED, pos.Value()),
      result_(result_ref),
      default_() {
    result_.Clear();
  }

  Extraction(std::map<K, V>& result_ref, const V& def, const Position& pos = Position(0))
    : ExtractionBase(typeid(std::map<K, V>).name(), Limit::LIMIT_UNLIMITED, pos.Value()),
      result_(result_ref),
      default_(def) {
    result_.Clear();
  }

  ~Extraction() {}

  size_t HandledColumnsCount() const {
    return TypeHandler<V>::size();
  }

  size_t HandledRowsCount() const {
    return static_cast<size_t>(result_.size());
  }

  size_t AllowedRowCount() const {
    return GetLimit();
  }

  size_t Extract(size_t pos) {
    V tmp;
    TypeHandler<V>::Extract(pos, tmp, default_, GetExtractor());
    result_.insert(std::make_pair(tmp(), tmp));
    return 1u;
  }

  PreparationBase::Ptr CreatePreparation(PreparatorBase::Ptr& prep, size_t pos) {
    return new Preparation<V>(prep, pos, default_);
  }

 private:
  std::map<K, V>& result_;
  V default_;
};

/**
 * Multimap Data Type specialization for extraction of values from a query result set.
 */
template <class K, class V>
class Extraction<std::multimap<K, V> >: public ExtractionBase {
 public:
  typedef std::multimap<K, V> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef Extraction<ValType> Type;
  typedef SharedPtr<Type> Ptr;

  Extraction(std::multimap<K, V>& result_ref, const Position& pos = Position(0))
    : ExtractionBase(typeid(std::multimap<K, V>).name(), Limit::LIMIT_UNLIMITED, pos.Value()),
      result_(result_ref),
      default_() {
    result_.Clear();
  }

  Extraction(std::multimap<K, V>& result_ref, const V& def, const Position& pos = Position(0))
    : ExtractionBase(typeid(std::multimap<K, V>).name(), Limit::LIMIT_UNLIMITED, pos.Value()),
      result_(result_ref),
      default_(def) {
    result_.Clear();
  }

  ~Extraction() {}

  size_t HandledColumnsCount() const {
    return TypeHandler<V>::size();
  }

  size_t HandledRowsCount() const {
    return static_cast<size_t>(result_.size());
  }

  size_t AllowedRowCount() const {
    return GetLimit();
  }

  size_t Extract(size_t pos) {
    V tmp;
    TypeHandler<V>::Extract(pos, tmp, default_, GetExtractor());
    result_.insert(std::make_pair(tmp(), tmp));
    return 1u;
  }

  PreparationBase::Ptr CreatePreparation(PreparatorBase::Ptr& prep, size_t pos) {
    return new Preparation<V>(prep, pos, default_);
  }

 private:
  std::multimap<K, V>& result_;
  V default_;
};


namespace Keywords {

/**
 * Convenience function to allow for a more compact creation of an extraction object.
 */
template <typename T>
inline ExtractionBase::Ptr into(T& t) {
  return new Extraction<T>(t);
}

/**
 * Convenience function to allow for a more compact creation of an extraction object.
 */
template <>
inline ExtractionBase::Ptr into(long& t) {
#ifndef FUN_LONG_IS_64_BIT
  return new Extraction<long>(t);
#else
  return new Extraction<int64>(reinterpret_cast<int64&>(t));
#endif
}

/**
 * Convenience function to allow for a more compact creation of an extraction object.
 */
template <>
inline ExtractionBase::Ptr into(unsigned long& t) {
#ifndef FUN_LONG_IS_64_BIT
  return new Extraction<unsigned long>(t);
#else
  return new Extraction<uint64>(reinterpret_cast<uint64&>(t));
#endif
}

/**
 * Convenience function to allow for a more compact creation of an extraction object
 * with multiple recordset support.
 */
template <typename T>
inline ExtractionBase::Ptr into(T& t, const Position& pos) {
  return new Extraction<T>(t, pos);
}

/**
 * Convenience function to allow for a more compact creation of an extraction object
 * with multiple recordset support and the given default
 */
template <typename T>
inline ExtractionBase::Ptr into(T& t, const Position& pos, const T& def) {
  return new Extraction<T>(t, def, pos);
}

/**
 * Convenience dummy function (for syntax purposes only).
 */
inline ExtractionBaseVecVec& into(ExtractionBaseVecVec& evv) {
  return evv;
}

/**
 * Convenience dummy function (for syntax purposes only).
 */
inline ExtractionBaseVec& into(ExtractionBaseVec& ev) {
  return ev;
}

} // namespace Keywords

} // namespace sql
} // namespace fun
