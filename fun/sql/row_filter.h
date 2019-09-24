#pragma once

#include "fun/sql/sql.h"
#include "fun/base/dynamic/var.h"
#include "fun/tuple.h"
#include "fun/base/string.h"
#include "fun/ref_counted_object.h"
#include "fun/ref_counted_ptr.h"
#include "fun/weak_ref_ptr.h"
#include <map>
#include <list>
#include <utility>

namespace fun {
namespace sql {

class RecordSet;

/**
 * RowFilter class provides row filtering functionality.
 * A filter contains a set of criteria (field name, value and
 * logical operation) for row filtering.
 * Additionally, a row filter contains a map of pointers to other
 * filters with related logical operations between filters.
 * RowFilter is typically added to recordset in order to filter
 * its content. Since the recordset own iteration is dependent upon
 * filtering, whenever the filtering criteria is changed,
 * the filter automatically notifies all associated recordsets
 * by rewinding them to the first position.
 */
class FUN_SQL_API RowFilter : public WeakRefCountedObject {
 public:
  enum Comparison {
    VALUE_LESS_THAN,
    VALUE_LESS_THAN_OR_EQUAL,
    VALUE_EQUAL,
    VALUE_GREATER_THAN,
    VALUE_GREATER_THAN_OR_EQUAL,
    VALUE_NOT_EQUAL,
    VALUE_IS_NULL
  };

  enum LogicOperator {
    OP_AND,
    OP_OR,
    OP_NOT
  };

  typedef bool (*CompT)(const fun::dynamic::Var&, const fun::dynamic::Var&);
  typedef RefCountedPtr<RowFilter> Ptr;
  typedef WeakRefPtr<RowFilter> WeakPtr;
  typedef std::map<String, Comparison> Comparisons;
  typedef Tuple<fun::dynamic::Var, Comparison, LogicOperator> ComparisonEntry;
  typedef std::multimap<String, ComparisonEntry> ComparisonMap;
  typedef std::map<RefCountedPtr<RowFilter>, LogicOperator> FilterMap;

  /**
   * Creates the top-level RowFilter and associates it with the recordSet.
   * The recordSet object must outlive this RowFilter object.
   */
  RowFilter(RecordSet& recordSet);

  /**
   * Creates child RowFilter and associates it with the parent filter.
   */
  RowFilter(Ptr parent, LogicOperator op = OP_OR);

  /**
   * Destroys the RowFilter.
   */
  ~RowFilter();

  /**
   * Appends another filter to this one.
   */
  void AddFilter(Ptr filter, LogicOperator comparison);

  /**
   * Removes filter from this filter.
   */
  void RemoveFilter(Ptr filter);

  /**
   * Returns true if this filter is parent of filter;
   */
  bool Has(Ptr filter) const;

  /**
   * Adds value to the filter.
   */
  template <typename T>
  void Add(const String& name, Comparison comparison, const T& value, LogicOperator op = OP_OR) {
    RewindRecordSet();
    comparison_map_.insert(ComparisonMap::value_type(toUpper(name),
      ComparisonEntry(value, comparison, op)));
  }

  /**
   * Adds value to the filter.
   */
  template <typename T>
  void Add(const String& name, const String& comp, const T& value, LogicOperator op = OP_OR) {
    Add(name, GetComparison(comp), value, op);
  }

  /**
   * Adds logically AND-ed value to the filter.
   */
  template <typename T>
  void AddAnd(const String& name, const String& comp, const T& value) {
    Add(name, GetComparison(comp), value, OP_AND);
  }

  /**
   * Adds logically OR-ed value to the filter.
   */
  template <typename T>
  void AddOr(const String& name, const String& comp, const T& value) {
    Add(name, GetComparison(comp), value, OP_OR);
  }

  /**
   * Removes named comparisons from the filter.
   * All comparisons with specified name are removed.
   * Returns the number of comparisons removed.
   */
  int Remove(const String& name);

  /**
   * Toggles the NOT operator for this filter;
   */
  void ToggleNot();

  /**
   * Returns true if filter is NOT-ed, false otherwise.
   */
  bool IsNot() const;

  /**
   * Returns true if there is not filtering criteria specified.
   */
  bool IsEmpty() const;

  /**
   * Returns true if name and value are allowed.
   */
  bool IsAllowed(size_t row) const;

  /**
   * Returns true if name is known to this row filter.
   */
  bool Exists(const String& name) const;

 private:
  RowFilter();
  RowFilter(const RowFilter&);
  RowFilter& operator=(const RowFilter&);

  void init();

  static bool Equal(const fun::dynamic::Var& p1, const fun::dynamic::Var& p2);
  static bool NotEqual(const fun::dynamic::Var& p1, const fun::dynamic::Var& p2);
  static bool Less(const fun::dynamic::Var& p1, const fun::dynamic::Var& p2);
  static bool Greater(const fun::dynamic::Var& p1, const fun::dynamic::Var& p2);
  static bool LessOrEqual(const fun::dynamic::Var& p1, const fun::dynamic::Var& p2);
  static bool GreaterOrEqual(const fun::dynamic::Var& p1, const fun::dynamic::Var& p2);
  static bool LogicalAnd(const fun::dynamic::Var& p1, const fun::dynamic::Var& p2);
  static bool LogicalOr(const fun::dynamic::Var& p1, const fun::dynamic::Var& p2);
  static bool IsNull(const fun::dynamic::Var& p1, const fun::dynamic::Var&);

  static void DoCompare(fun::dynamic::Var& ret,
                        fun::dynamic::Var& val,
                        CompT comp,
                        const ComparisonEntry& ce);

  RecordSet& GetRecordSet() const;

  Comparison GetComparison(const String& comp) const;

  void RewindRecordSet();

  Comparisons comparisons_;
  ComparisonMap comparison_map_;
  mutable RecordSet* record_set_;
  WeakPtr parent_;
  FilterMap fileter_map_;
  bool not_;

  friend class RecordSet;
};


//
// inlines
//

inline bool RowFilter::Has(Ptr filter) const {
  return fileter_map_.find(filter) != fileter_map_.end();
}

inline bool RowFilter::IsEmpty() const {
  return comparison_map_.size() == 0;
}

inline bool RowFilter::Exists(const String& name) const {
  return comparison_map_.find(name) != comparison_map_.end();
}

inline void RowFilter::ToggleNot() {
  not_ = !not_;
}

inline bool RowFilter::IsNot() const {
  return not_;
}

inline bool RowFilter::Equal(const fun::dynamic::Var& p1, const fun::dynamic::Var& p2) {
  return p1 == p2;
}

inline bool RowFilter::NotEqual(const fun::dynamic::Var& p1, const fun::dynamic::Var& p2) {
  return p1 != p2;
}

inline bool RowFilter::Less(const fun::dynamic::Var& p1, const fun::dynamic::Var& p2) {
  return p1 < p2;
}

inline bool RowFilter::Greater(const fun::dynamic::Var& p1, const fun::dynamic::Var& p2) {
  return p1 > p2;
}

inline bool RowFilter::LessOrEqual(const fun::dynamic::Var& p1, const fun::dynamic::Var& p2) {
  return p1 <= p2;
}

inline bool RowFilter::GreaterOrEqual(const fun::dynamic::Var& p1, const fun::dynamic::Var& p2) {
  return p1 >= p2;
}

inline bool RowFilter::LogicalAnd(const fun::dynamic::Var& p1, const fun::dynamic::Var& p2) {
  return p1 && p2;
}

inline bool RowFilter::LogicalOr(const fun::dynamic::Var& p1, const fun::dynamic::Var& p2) {
  return p1 || p2;
}

inline bool RowFilter::IsNull(const fun::dynamic::Var& p1, const fun::dynamic::Var&) {
  return p1.IsEmpty();
}

} // namespace sql
} // namespace fun
