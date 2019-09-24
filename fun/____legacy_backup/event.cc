#include "fun/base/event.h"

namespace fun {

/*
Event::Event(EventType type)
  : state_(false)
  , auto_reset_(type == EventResetType::Auto)
{
}

Event::Event(bool auto_reset)
  : state_(false)
  , auto_reset_(auto_reset)
{
}

Event::~Event()
{
}

void Event::Set()
{
  try {
    std::lock_guard<std::mutex> guard(mutex_);
    state_ = true;
    if (auto_reset_) cond_.notify_one();
    else             cond_.notify_all();
  }
  catch (std::system_error& e) {
    throw SystemException(e.what());
  }
}

void Event::Reset()
{
  try {
    std::lock_guard<std::mutex> guard(mutex_);
    state_ = false;
  }
  catch (std::system_error& e) {
    throw SystemException(e.what());
  }
}

void Event::Wait()
{
  try
  {
    std::unique_lock<std::mutex> guard(mutex_);
    while (!state_)
      cond_.wait(lock, [this]() { return this->state_.load(); });
    if (auto_reset_) state_ = false;
  }
  catch (std::system_error& e) {
    throw SystemException(e.what());
  }
}

bool Event::WaitImpl(int32 milliseconds)
{
  try {
    std::unique_lock<std::mutex> guard(mutex_);
    bool ret = cond_.wait_for(lock, std::chrono::milliseconds(milliseconds), [this]() { return this->state_.load(); });
    if (ret && auto_reset_) state_ = false;
    return ret;
  }
  catch (std::system_error& e) {
    throw SystemException(e.what());
  }
}
*/

} // namespace fun
