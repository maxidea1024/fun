#include "fun/base/directory_iterator_strategy.h"

namespace fun {

//
// TraverseBase
//

TraverseBase::TraverseBase(DepthFunPtr depth_determiner, uint16 max_depth)
  : depth_determiner_(depth_determiner), max_depth_(max_depth) {}

bool TraverseBase::IsFiniteDepth() {
  return max_depth_ != D_INFINITE;
}

bool TraverseBase::IsDirectory(fun::File& file) {
  try {
    return file.IsDirectory();
  } catch (...) {
    return false;
  }
}


//
// ChildrenFirstTraverse
//

ChildrenFirstTraverse::ChildrenFirstTraverse(DepthFunPtr depth_determiner, uint16 max_depth)
  : TraverseBase(depth_determiner, max_depth) {}

const String ChildrenFirstTraverse::Next(Stack* it_stack, bool* is_finished)
{
  // pointer mustn't point to NULL and iteration mustn't be finished
  fun_check_ptr(is_finished);
  fun_check(!(*is_finished));

  // go deeper into not empty directory
  // (if depth limit allows)
  bool is_depth_limit_reached = IsFiniteDepth() && depth_determiner_(*it_stack) >= max_depth_;
  if (!is_depth_limit_reached && IsDirectory(*it_stack->top())) {
    try {
      DirectoryIterator child_it(it_stack->top().GetPath());
      // check if directory is empty
      if (child_it != it_end_) {
        it_stack->push(child_it);
        return child_it->GetPath();
      }
    } catch (...) {
      // Failed to iterate child dir.
      traverseError.Notify(this, it_stack->top()->GetPath());
    }
  }

  ++(it_stack->top());

  fun_check(!it_stack->empty());
  // return up until there isn't right sibling
  while (it_stack->top() == it_end_) {
    it_stack->pop();

    // detect end of traversal
    if (it_stack->empty()) {
      *is_finished = true;
      return it_end_->GetPath();
    } else {
      ++(it_stack->top());
    }
  }

  return it_stack->top()->GetPath();
}


//
// SiblingsFirstTraverse
//

SiblingsFirstTraverse::SiblingsFirstTraverse(DepthFunPtr depth_determiner, uint16 max_depth)
  : TraverseBase(depth_determiner, max_depth) {
  dirs_stack_.push(std::queue<String>());
}

const String SiblingsFirstTraverse::Next(Stack* it_stack, bool* is_finished) {
  // pointer mustn't point to NULL and iteration mustn't be finished
  fun_check_ptr(is_finished);
  fun_check(!(*is_finished));

  // add dirs to queue (if depth limit allows)
  bool is_depth_limit_reached = IsFiniteDepth() && depth_determiner_(*it_stack) >= max_depth_;
  if (!is_depth_limit_reached && IsDirectory(*it_stack->top())) {
    const String& p = it_stack->top()->GetPath();
    dirs_stack_.top().push(p);
  }

  ++(it_stack->top());

  fun_check(!it_stack->empty());
  // return up until there isn't right sibling
  while (it_stack->top() == it_end_) {
    // try to find first not empty directory and go deeper
    while (!dirs_stack_.top().empty()) {
      String dir = dirs_stack_.top().front();
      dirs_stack_.top().pop();

      try {
        DirectoryIterator child_it(dir);

        // check if directory is empty
        if (child_it != it_end_) {
          it_stack->push(child_it);
          dirs_stack_.push(std::queue<String>());
          return child_it->GetPath();
        }
      } catch (...) {
        // Failed to iterate child dir.
        traverseError.Notify(this, dir);
      }
    }

    // if fail go upper
    it_stack->pop();
    dirs_stack_.pop();

    // detect end of traversal
    if (it_stack->empty()) {
      *is_finished = true;
      return it_end_->GetPath();
    }
  }

  return it_stack->top()->GetPath();
}

} // namespace fun
