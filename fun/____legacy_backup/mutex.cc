#include "fun/base/mutex.h"

#if FUN_PLATFORM_WINDOWS_FAMILY
#include "mutex_win32.cc"
#else
#include "mutex_posix.cc"
#endif

namespace fun {

//
// Mutex
//

Mutex::Mutex(MutexType type)
  : MutexImpl((MutexTypeImpl)type)
{
}

Mutex::~Mutex()
{
}


//
// FastMutex
//

FastMutex::FastMutex()
{
}

FastMutex::~FastMutex()
{
}

} // namespace fun
