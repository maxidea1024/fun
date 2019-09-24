#pragma once

#include "fun/base/base.h"
#include "fun/base/container/array.h"

namespace fun {
namespace algo {

template <typename T, typename ArrayAllocator>
void UnionDuplicateds(Array<T, ArrayAllocator>& array,
                      bool assume_already_sorted = false) {
  if (array.Count() < 2) {
    // nothing to do...
    return;
  }

  // 이미 정렬된 상태가 아니라면, 여기서 정렬을 하도록 한다.
  // 이 함수 자체가 정렬된 상태를 가정하고 동작하기 때문이다.
  if (!assume_already_sorted) {
    array.Sort();
  }

  // move나 copy는 느리니, 우선 겹치는게 있는지부터 찾아볼까...
  bool is_duplicated = false;
  for (int32 i = 1; i < array.Count(); ++i) {
    if (array[i] == array[i - 1]) {
      is_duplicated = true;
      break;
    }
  }

  if (!is_duplicated) {
    // nothing to do...
    return;
  }

  // 자, 이제 겹치는게 있다는것을 알았으니, 겹치는 것들을 제거하도록 하자.
  // 제거 형태로 작업을 하게 되면, move 가 많이 발생할 수 있으니 새 사본을
  // 만들어서 채운뒤 swap 하도록 하자.

  Array<T, ArrayAllocator> working_set;
  working_set.Reserve(array.Count());
  for (int32 i = 0; i < array.Count(); ++i) {
    if (working_set.IsEmpty() || array[i] != working_set.Last()) {
      working_set.Add(array[i]);
    }
  }

  // 맞바꾸기. (복사 비용 없음)
  fun::Swap(working_set, array);
}

}  // namespace algo
}  // namespace fun
