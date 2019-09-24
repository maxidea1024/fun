#include "fun/base/condition.h"

namespace fun {

Condition::Condition() {
  // NOOP
}

Condition::~Condition() {
  // NOOP
}

void Condition::NotifyOne() {
  FastMutex::ScopedLock lock(wait_queue_mutex_);

  if (!wait_queue_.IsEmpty()) {
    wait_queue_.Front()->Set();
    Dequeue();
  }
}

void Condition::NotifyAll() {
  FastMutex::ScopedLock lock(wait_queue_mutex_);

  for (auto& waiter : wait_queue_) {
    waiter->Set();
  }
  wait_queue_.Clear();
}

void Condition::Enqueue(Event& event) {
  wait_queue_.PushBack(&event);
}

void Condition::Dequeue() {
  wait_queue_.PopFront();
}

void Condition::Dequeue(Event& event) {
  wait_queue_.Remove(&event);
}

} // namespace fun
