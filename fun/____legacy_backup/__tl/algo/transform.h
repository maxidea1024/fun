#pragma once

namespace fun {
namespace algo {

template <typename InT, typename OutT, typename Predicate, typename Transform>
inline void TransformIf(const InT& input, OutT& output, const Predicate& pred, Transform xform)
{
  for (auto&& value : input) {
    if (pred(value)) {
      output.Add(xform(value));
    }
  }
}

template <typename InT, typename OutT, typename Transform>
inline void Transform(const InT& input, OutT& output, Transform xform)
{
  for (auto&& value : input) {
    output.Add(xform(value));
  }
}

} // namespace algo
} // namespace fun
