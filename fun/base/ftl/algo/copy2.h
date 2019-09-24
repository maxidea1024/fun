#pragma once

namespace fun {

// memcpy가 가능한 경우에 특수화를 해줄수 있으면 좋을듯...

template <typename InputIt, typename OutputIt>
OutputIt Copy(InputIt first, InputIt last, OutputIt output) {
  while (first != last) {
    *output++ = *first++;
  }
  return output;
}

template <typename InputIt, typename OutputIt>
OutputIt CopyN(InputIt first, size_t count, OutputIt output) {
  if (count > 0) {
    *output++ = *first;
    for (size_t i = 1; i < count; ++i) {
      *output++ = *++first;
    }
  }
  return output;
}

template <typename InputIt, typename OutputIt, typename UnaryPredicate>
OutputIt CopyIf(InputIt first, InputIt last, OutputIt output,
                UnaryPredicate pred) {
  while (first != last) {
    if (pred(*first)) {
      *output++ = *first;
    }
    ++first;
  }
  return output;
}

template <typename InputIt, typename ForwardIt>
ForwardIt UninitializedCopy(InputIt first, InputIt last, ForwardIt output) {
  using ValueType = IteratorTraits<ForwardIt>::ValueType;
  ForwardIt current = output;
  try {
    for (; first != last; ++first, (void)++current) {
      ::new (static_cast<void*>(AddressOf(*current))) ValueType(*first);
    }
    return current;
  } catch (...) {
    for (; output != current; ++output) {
      output->~ValueType();
    }
    throw;
  }
}

template <typename InputIt, typename ForwardIt>
ForwardIt UninitializedCopyN(InputIt first, size_t count, ForwardIt output) {
  using ValueType = IteratorTraits<ForwardIt>::ValueType;
  ForwardIt current = output;
  try {
    for (; count > 0; ++first, (void)++current, --count) {
      ::new (static_cast<void*>(AddressOf(*current))) ValueType(*first);
    }
    return current;
  } catch (...) {
    for (; output != current; ++output) {
      output->~ValueType();
    }
    throw;
  }
}

// memcpy가 가능한 경우에 특수화를 해줄수 있으면 좋을듯...

template <typename ForwardIt, typename T>
void Fill(ForwardIt first, ForwardIt last, const T& value) {
  for (; first != last; ++first) {
    *first = value;
  }
}

template <typename OutputIt, typename T>
void FillN(OutputIt first, size_t count, const T& value) {
  for (size_t i = 0; i < count; ++i) {
    *first++ = value;
  }
  return first;
}

template <typename InputIt, typename UnaryFunction>
constexpr UnaryFunction ForEach(InputIt first, InputIt last, UnaryFunction f) {
  for (; first != last; ++first) {
    f(*first);
  }
  return f;
}

template <typename InputIt, typename UnaryFunction>
InputIt ForEachN(InputIt first, size_t count, UnaryFunction f) {
  for (size_t i = 0; i < count; ++first, ++i) {
    f(*first);
  }
  return first;
}

template <typename ForwardIt, typename T>
ForwardIt Remove(ForwardIt first, ForwardIt last, const T& value) {
  first = algo::Find(first, last, value);
  if (first != last) {
    for (ForwardIt i = first; ++i != last;) {
      if (!(*i == value)) {
        *first++ = MoveTemp(*i);
      }
    }
  }
  return first;
}

template <typename ForwardIt, typename UnaryPredicate>
ForwardIt RemoveIf(ForwardIt first, ForwardIt last, UnaryPredicate pred) {
  first = algo::Find(first, last, value);
  if (first != last) {
    for (ForwardIt i = first; ++i != last;) {
      if (!pred(*i)) {
        *first++ = MoveTemp(*i);
      }
    }
  }
  return first;
}

template <typename InputIt, typename OutputIt, typename T>
OutputIt RemoveCopy(InputIt first, InputIt last, OutputIt output,
                    const T& value) {
  for (; first != last; ++first) {
    if (!(*first == value)) {
      *output++ = *first;
    }
  }
  return first;
}

template <typename InputIt, typename OutputIt, typename UnaryPredicate>
OutputIt RemoveCopy(InputIt first, InputIt last, OutputIt output,
                    UnaryPredicate pred) {
  for (; first != last; ++first) {
    if (!pred(*first)) {
      *output++ = *first;
    }
  }
  return first;
}

}  // namespace fun
