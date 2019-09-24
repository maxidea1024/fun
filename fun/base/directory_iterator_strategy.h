#pragma once

#include "fun/base/base.h"
#include "fun/base/directory_iterator.h"
#include "fun/base/multicast_event.h"
#include "fun/base/event_args.h"

#include <stack>
#include <queue>
#include <functional>

namespace fun {

class FUN_BASE_API TraverseBase {
 public:
  typedef std::stack<DirectoryIterator> Stack;
  typedef std::pointer_to_unary_function<const Stack&, uint16> DepthFunPtr;

  enum {
    /** Special value for infinite traverse depth. */
    D_INFINITE = 0
  };

  MulticastEvent<const String> traverseError;

  TraverseBase(DepthFunPtr depth_determiner, uint16 max_depth = D_INFINITE);

 protected:
  bool IsFiniteDepth();
  bool IsDirectory(fun::File& file);

  DepthFunPtr depth_determiner_;
  uint16 max_depth_;
  DirectoryIterator it_end_;

 private:
  TraverseBase() = delete;
  TraverseBase(const TraverseBase&) = delete;
  TraverseBase& operator = (const TraverseBase&) = delete;
};


class FUN_BASE_API ChildrenFirstTraverse : public TraverseBase {
 public:
  ChildrenFirstTraverse(DepthFunPtr depth_determiner, uint16 max_depth = D_INFINITE);

  const String Next(Stack* it_stack, bool* is_finished);

 private:
  ChildrenFirstTraverse() = delete;
  ChildrenFirstTraverse(const ChildrenFirstTraverse&) = delete;
  ChildrenFirstTraverse& operator = (const ChildrenFirstTraverse&) = delete;
};


class FUN_BASE_API SiblingsFirstTraverse : public TraverseBase {
 public:
  SiblingsFirstTraverse(DepthFunPtr depth_determiner, uint16 max_depth = D_INFINITE);

  const String Next(Stack* it_stack, bool* is_finished);

 private:
  SiblingsFirstTraverse() = delete;
  SiblingsFirstTraverse(const SiblingsFirstTraverse&) = delete;
  SiblingsFirstTraverse& operator = (const SiblingsFirstTraverse&) = delete;

  std::stack<std::queue<String> > dirs_stack_;
};

} // namespace fun
