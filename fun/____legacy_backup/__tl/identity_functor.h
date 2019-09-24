#pragma once

namespace fun {

/**
 * A functor which returns whatever is passed to it.
 * Mainly used for generic composition.
 */
struct IdentityFunctor
{
  template <typename T>
  inline T&& operator()(T&& val) const
  {
    return (T&&)val;
  }
};

} // namespace fun
