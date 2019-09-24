#include "NetEnginePrivate.h"
#include "UserTask.h"

namespace fun {
namespace net {

void UserTaskQueue::AddTaskSubject(ITaskSubject* subject) {
  owner_->GetMutex().AssertIsLockedByCurrentThread();

  if (subject->task_subject_node_.GetListOwner() == nullptr) {
    task_subjects_.Append(&subject->task_subject_node);
  }

  // Request a task in thread-pool.
  owner_->PostUserTask();
}

// 이미 실행중인것이 있을 경우에는 대기해야함. (serialized execution)
bool UserTaskQueue::PopAnyTaskNotRunningAndMarkAsRunning(
                  FinalUserWorkItem& output, void** out_host_tag) {
  CScopedLock2 lock_guard(owner_->GetMutex());

  while (!task_subjects_.IsEmpty()) {
    auto subject = task_subjects_.Front()->owner_;

    // Request a task in thread-pool.
    subject->task_subject_node_.UnlinkSelf();

    // Skip if there are no principals, if the queue is already empty, or if the user worker thread is still running.
    // If you are skipping because it's being processed by a worker thread, then what was in the queue will be lost
    // After the worker thread finishes checking that the subject's dedicated receive queue is not empty.
    if (subject->IsFinalReceiveQueueEmpty() == false &&
        subject->IsTaskRunning() == false &&
        subject->PopFirstUserWorkItem(output) &&
        owner_->IsValidHostId_NOLOCK(output.unsafe_message.remote_id)) {
      // Marks that it is occupied by a thread.
      subject->OnSetTaskRunningFlag(true);

      // Take host-tag value.
      *out_host_tag = subject->host_tag_;

      return true;
    }
  }

  return false;
}

void UserTaskQueue::SetTaskRunningFlagByHostId(
      HostId subject_host_id, bool running) {
  CScopedLock2 owner_guard(owner_->GetMutex());

  if (auto subject = owner_->GetTaskSubjectByHostId_NOLOCK(subject_host_id)) {
    subject->OnSetTaskRunningFlag(running);

    if (!running) {
      // Add more to the main queue if there is still more to process.
      if (!subject->IsFinalReceiveQueueEmpty()) {
        AddTaskSubject(subject);
      }
    }
  }
}

} // namespace net
} // namespace fun
