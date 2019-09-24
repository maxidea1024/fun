#include "fun/base/mutex.h"

#if (FUN_PLATFORM == FUN_PLATFORM_CYGWIN || FUN_PLATFORM == FUN_PLATFORM_ANDROID)
#include "fun/base/mutex_posix.cc"
#else
#include "fun/base/mutex_std.cc"
#endif

namespace fun {

//
// Mutex
//

Mutex::Mutex(MutexType type)
  : MutexImpl((MutexTypeImpl)type)
  //, owner_tid_(0)
{
}

Mutex::~Mutex()
{
  //fun_check(owner_tid_ == 0);
}


//
// FastMutex
//

FastMutex::FastMutex()
  //: owner_tid_(0)
{
}

FastMutex::~FastMutex()
{
  //fun_check(owner_tid_ == 0);
}


//
// SpinlockMutex
//

SpinlockMutex::SpinlockMutex()
  //: owner_tid_(0)
{
}

SpinlockMutex::~SpinlockMutex()
{
  //fun_check(owner_tid_ == 0);
}

} // namespace fun
