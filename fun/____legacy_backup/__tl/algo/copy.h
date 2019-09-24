#pragma once

namespace fun {
namespace algo {

/**
 * Conditionally copies a range into a container
 *
 * \param input - Any iterable type
 * \param output - Container to hold the output
 * \param pred - Condition which returns true for elements that should be copied and false for elements that should be skipped
 */
template <typename Input, typename Output, typename Predicate>
inline void CopyIf(const Input& input, Output& output, const Predicate& pred)
{
  for (auto&& value : input) {
    if (pred(value)) {
      output.Add(value);
    }
  }
}

/**
 * Copies a range into a container
 *
 * \param input - Any iterable type
 * \param output - Container to hold the output
 */
template <typename Input, typename Output>
inline void Copy(const Input& input, Output& output)
{
  for (auto&& value : input) {
    output.Add(value);
  }
}

} // namespace algo
} // namespace fun
