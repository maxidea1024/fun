// RTT 체계에 통합하던지... 다듬어서 쓰던지...

#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * The common base class for all Instantiator instantiations.
 * Used by DynamicFactory.
 */
template <typename Base>
class InstantiatorBase {
 public:
  InstantiatorBase() {}
  virtual ~InstantiatorBase() {}

  InstantiatorBase(const InstantiatorBase&) = delete;
  InstantiatorBase& operator=(const InstantiatorBase&) = delete;

  virtual Base* CreateInstance() const = 0;
};

/**
 * A template class for the easy instantiation of
 * instantiators.
 *
 * For the Instantiator to work, the class of which
 * instances are to be instantiated must have a no-argument
 * constructor.
 */
template <typename C, typename Base>
class Instantiator : public InstantiatorBase<Base> {
 public:
  Instantiator() {}
  virtual ~Instantiator() {}

  Base* CreateInstance() const override { return new C; }
};

}  // namespace fun
