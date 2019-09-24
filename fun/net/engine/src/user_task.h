#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

class FinalUserWorkItem;

class ITaskSubject {
 public:
  class Node : public ListNode<Node> {
   public:
    ITaskSubject* owner;

    Node(ITaskSubject* owner) : owner(owner) {}
  };

  Node task_subject_node;
  void* host_tag;

 public:
  ITaskSubject() : task_subject_node(this), host_tag(nullptr) {}
  virtual ~ITaskSubject() {}

 public:
  virtual HostId GetHostId() const = 0;
  virtual bool IsFinalReceiveQueueEmpty() = 0;
  virtual bool IsTaskRunning() = 0;
  virtual void OnSetTaskRunningFlag(bool running) = 0;
  virtual bool PopFirstUserWorkItem(FinalUserWorkItem& out_item) = 0;
};

class IUserTaskQueueOwner {
 public:
  virtual ~IUserTaskQueueOwner() {}

 public:
  virtual CCriticalSection2& GetMutex() = 0;
  virtual ITaskSubject* GetTaskSubjectByHostId_NOLOCK(HostId host_id) = 0;
  virtual bool IsValidHostId_NOLOCK(HostId host_id) = 0;
  virtual void PostUserTask() = 0;
};

class UserTaskQueue {
 public:
  UserTaskQueue(IUserTaskQueueOwner* owner) : owner_(owner) {}

 public:
  void AddTaskSubject(ITaskSubject* subject);
  bool PopAnyTaskNotRunningAndMarkAsRunning(FinalUserWorkItem& output, void** out_host_tag);
  void SetTaskRunningFlagByHostId(HostId subject_host_id, bool running);

 private:
  IUserTaskQueueOwner* owner_;
  ListNode<ITaskSubject::Node>::ListOwner task_subjects_;
};

} // namespace net
} // namespace fun
