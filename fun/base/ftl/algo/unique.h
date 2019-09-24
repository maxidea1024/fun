
template <typename ForwardIt>
ForwardIt Unique(ForwardIt first, ForwardIt last) {
  if (first == last) {
    return last;
  }
  
  ForwardIt result = first;
  while (++first != last) {
    if (!(*result == *first) && ++result != first) {
      *result = MoveTemp(*first);
    }
  }
  
  return ++result;
}


template <typename ForwardIt, typename BinaryPredicate>
ForwardIt Unique(ForwardIt first, ForwardIt last, BinaryPredicate pred) {
  if (first == last) {
    return last;
  }
  
  ForwardIt result = first;
  while (++first != last) {
    if (!pred(*result == *first) && ++result != first) {
      *result = MoveTemp(*first);
    }
  }
  
  return ++result;
}
