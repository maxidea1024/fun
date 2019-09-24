#pragma once

#include "fun/sql/sql.h"
#include "fun/sql/preparation_base.h"
#include "fun/sql/type_handler.h"

#include <cstddef>
#include <vector>

namespace fun {
namespace sql {

/**
 * Class for calling the appropriate PreparatorBase method.
 */
template <typename T>
class Preparation : public PreparationBase {
 public:
  /**
   * Creates the Preparation.
   */
  Preparation(PreparatorBase::Ptr& preparator, size_t pos, T& val)
    : PreparationBase(preparator), pos_(pos), val_(val) {}

  /**
   * Destroys the Preparation.
   */
  ~Preparation() {}

  /**
   * Prepares data.
   */
  void Prepare() {
    TypeHandler<T>::Prepare(pos_, val_, GetPreparation());
  }

 private:
  size_t pos_;
  T& val_;
};

/**
 * Preparation specialization for std::vector.
 * This specialization is needed for bulk operations to enforce
 * the whole vector preparation, rather than only individual contained values.
 */
template <typename T>
class Preparation<std::vector<T>> : public PreparationBase {
 public:
  /**
   * Creates the Preparation.
   */
  Preparation(PreparatorBase::Ptr preparator, size_t pos, std::vector<T>& val = std::vector<T>()):
    PreparationBase(preparator), pos_(pos), val_(val) {}

  /**
   * Destroys the Preparation.
   */
  ~Preparation() {}

  /**
   * Prepares data.
   */
  void Prepare() {
    TypeHandler<std::vector<T> >::Prepare(pos_, val_, GetPreparation());
  }

 private:
  size_t pos_;
  std::vector<T>& val_;
};

/**
 * Preparation specialization for std::deque.
 * This specialization is needed for bulk operations to enforce
 * the whole deque preparation, rather than only individual contained values.
 */
template <typename T>
class Preparation<std::deque<T>> : public PreparationBase {
 public:
  /**
   * Creates the Preparation.
   */
  Preparation(PreparatorBase::Ptr preparator, size_t pos, std::deque<T>& val = std::deque<T>())
    : PreparationBase(preparator), pos_(pos), val_(val) {}

  /**
   * Destroys the Preparation.
   */
  ~Preparation() {}

  /**
   * Prepares data.
   */
  void Prepare() {
    TypeHandler<std::deque<T> >::Prepare(pos_, val_, GetPreparation());
  }

 private:
  size_t pos_;
  std::deque<T>& val_;
};

/**
 * Preparation specialization for std::list.
 * This specialization is needed for bulk operations to enforce
 * the whole list preparation, rather than only individual contained values.
 */
template <typename T>
class Preparation<std::list<T>> : public PreparationBase {
 public:
  /**
   * Creates the Preparation.
   */
  Preparation(PreparatorBase::Ptr preparator, size_t pos, std::list<T>& val = std::list<T>())
    : PreparationBase(preparator), pos_(pos), val_(val) {}

  /**
   * Destroys the Preparation.
   */
  ~Preparation() {}

  /**
   * Prepares data.
   */
  void Prepare() {
    TypeHandler<std::list<T>>::Prepare(pos_, val_, GetPreparation());
  }

 private:
  size_t pos_;
  std::list<T>& val_;
};

} // namespace sql
} // namespace fun
