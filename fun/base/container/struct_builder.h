#pragma once

#include "fun/base/base.h"
#include "fun/base/math/math_base.h"

namespace fun {

class StructBuilder {
 public:
  FUN_ALWAYS_INLINE StructBuilder() : end_of_last_member_(0), alignment_(0) {}

  FUN_ALWAYS_INLINE
  int32 AddMember(int32 member_size, int32 member_alignment) {
    int32 offset = Align(end_of_last_member_, member_alignment);
    end_of_last_member_ = offset + member_size;
    alignment_ = MathBase::Max(alignment_, member_alignment);
    return offset;
  }

  FUN_ALWAYS_INLINE int32 GetSize() const {
    return Align(end_of_last_member_, alignment_);
  }

  FUN_ALWAYS_INLINE int32 GetAlignment() const { return alignment_; }

 private:
  int32 end_of_last_member_;
  int32 alignment_;
};

}  // namespace fun
