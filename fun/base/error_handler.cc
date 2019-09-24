#include "fun/base/error_handler.h"
#include "fun/base/singleton.h"

namespace fun {

ErrorHandler* ErrorHandler::handler_ = ErrorHandler::DefaultHandler();
FastMutex ErrorHandler::mutex_;

ErrorHandler::ErrorHandler() {}

ErrorHandler::~ErrorHandler() {}

void ErrorHandler::OnException(const Exception& e) {
  fun_debugger_msg(e.what());
}

void ErrorHandler::OnException(const std::exception& e) {
  fun_debugger_msg(e.what());
}

void ErrorHandler::OnException() { fun_debugger_msg("unknown exception"); }

void ErrorHandler::Handle(const Exception& e) {
  ScopedLock<FastMutex> guard(mutex_);
  try {
    handler_->OnException(e);
  } catch (...) {
  }
}

void ErrorHandler::Handle(const std::exception& e) {
  ScopedLock<FastMutex> guard(mutex_);
  try {
    handler_->OnException(e);
  } catch (...) {
  }
}

void ErrorHandler::Handle() {
  ScopedLock<FastMutex> guard(mutex_);
  try {
    handler_->OnException();
  } catch (...) {
  }
}

ErrorHandler* ErrorHandler::Set(ErrorHandler* handler) {
  fun_check_ptr(handler);

  ScopedLock<FastMutex> guard(mutex_);
  ErrorHandler* old_handler = handler_;
  handler_ = handler;
  return old_handler;
}

ErrorHandler* ErrorHandler::Get() { return handler_; }

ErrorHandler* ErrorHandler::DefaultHandler() {
  static Singleton<ErrorHandler>::Holder sh;
  return sh.Get();
}

}  // namespace fun
