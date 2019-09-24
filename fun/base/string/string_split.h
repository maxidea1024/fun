#pragma once

#include "fun/base/base.h"
#include "fun/base/container/array.h"
#include "fun/base/string/string_algo.h"

namespace fun {

#define DEFAULT_STRING_SPLIT_CAPACITY 10

template <typename ResultStringType, typename SourceStringType,
          typename SeparatorsType, typename MidMethod>
inline static int32 SplitByCharacters(
    Array<ResultStringType>& list, const SourceStringType& source,
    MidMethod mid, const SeparatorsType& separators, int32 max_splits,
    StringSplitOptions split_options, CaseSensitivity casesense) {
  list.Clear(max_splits > 0 ? max_splits + 1 : DEFAULT_STRING_SPLIT_CAPACITY);

  int32 splitted_count = 0, start = 0;
  for (;;) {
    int32 separator_len = -1;
    const int32 separator_pos = source.IndexOfAny(separators, casesense, start,
                                                  nullptr, &separator_len);
    const bool term = (separator_pos == INVALID_INDEX ||
                       (max_splits > 0 && splitted_count == max_splits));
    const int32 end = term ? source.Len() : separator_pos;

    int32 token_len;
    if (split_options & StringSplitOption::Trimming) {
      using CharType = typename SourceStringType::CharType;
      const CharType* token_begin = source.cbegin() + start;
      const CharType* token_end = source.cbegin() + end;
      token_len = StringAlgo<SourceStringType>::TrimmedPositions(token_begin,
                                                                 token_end);
      start = token_begin - source.cbegin();
    } else {
      token_len = end - start;
    }

    ++splitted_count;

    fun_check(token_len >= 0);

    if (!(split_options & StringSplitOption::CullEmpty) || token_len) {
      list.Add((source.*mid)(start, token_len));
    }

    if (term) {
      break;
    }

    start = separator_pos +
            separator_len;  //인코딩 변환이 있을 수 있으므로, 길이를 고려해야함.
  }

  return list.Count();
}

template <typename ResultStringType, typename SourceStringType,
          typename SeparatorsType, typename MidMethod>
inline static int32 SplitByStrings(Array<ResultStringType>& list,
                                   const SourceStringType& source,
                                   MidMethod mid,
                                   const SeparatorsType* separators,
                                   int32 separator_count, int32 max_splits,
                                   StringSplitOptions split_options,
                                   CaseSensitivity casesense) {
  list.Clear(max_splits > 0 ? max_splits + 1 : DEFAULT_STRING_SPLIT_CAPACITY);

  int32 splitted_count = 0, start = 0;
  for (;;) {
    int32 matched_separator_len;
    const int32 separator_pos =
        IndexOfAnyStrings_helper(source, separators, separator_count, casesense,
                                 start, nullptr, &matched_separator_len);
    const bool term = (separator_pos == INVALID_INDEX ||
                       (max_splits > 0 && splitted_count == max_splits));
    const int32 end = term ? source.Len() : separator_pos;

    int32 token_len;
    if (split_options & StringSplitOption::Trimming) {
      using CharType = typename SourceStringType::CharType;
      const CharType* token_begin = source.cbegin() + start;
      const CharType* token_end = source.cbegin() + end;
      token_len = StringAlgo<SourceStringType>::TrimmedPositions(token_begin,
                                                                 token_end);
      start = token_begin - source.cbegin();
    } else {
      token_len = end - start;
    }

    ++splitted_count;

    fun_check(token_len >= 0);

    if (!(split_options & StringSplitOption::CullEmpty) || token_len) {
      list.Add((source.*mid)(start, token_len));
    }

    if (term) {
      break;
    }

    start =
        separator_pos + matched_separator_len;  //인코딩 변환이 있을 수
                                                //있으므로, 길이를 고려해야함.
  }

  return list.Count();
}

}  // namespace fun
