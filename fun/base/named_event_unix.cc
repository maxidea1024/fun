#include "fun/base/named_event_unix.h"
#include "fun/base/format.h"
#include "fun/base/exception.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#if defined(sun) || defined(__APPLE__) || defined(__osf__) || defined(__QNX__) || defined(_AIX) || defined(__EMSCRIPTEN__)
#include <semaphore.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#endif

namespace fun {

#if (FUN_PLATFORM == FUN_PLATFORM_LINUX) || (FUN_PLATFORM == FUN_PLATFORM_ANDROID) || (FUN_PLATFORM == FUN_PLATFORM_CYGWIN) || (FUN_PLATFORM == FUN_PLATFORM_FREE_BSD) || (FUN_PLATFORM == FUN_PLATFORM_NACL)
  union semun {
    int val;
    struct semid_ds* buf;
    unsigned short int* array;
    struct seminfo* __buf;
  };
#elif (FUN_PLATFORM == FUN_PLATFORM_HPUX)
  union semun {
    int val;
    struct semid_ds* buf;
    ushort* array;
  };
#endif

NamedEventImpl::NamedEventImpl(const String& name)
  : name_(name) {
  String filename = GetFileName();
#if defined(sun) || defined(__APPLE__) || defined(__osf__) || defined(__QNX__) || defined(_AIX) || defined(__EMSCRIPTEN__)
  sem_ = sem_open(filename.c_str(), O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO, 0);
  if ((long) sem_ == (long) SEM_FAILED) {
    throw SystemException(fun::Format("cannot create named mutex %s (sem_open() failed, errno=%d)", filename, errno), name_);
  }
#else
  int fd = open(filename.c_str(), O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd != -1) {
    close(fd);
  } else {
    throw SystemException(fun::Format("cannot create named event %s (lockfile)", filename), name_);
  }
  key_t key = ftok(filename.c_str(), 'p');
  if (key == -1) {
    throw SystemException(fun::Format("cannot create named mutex %s (ftok() failed, errno=%d)", filename, errno), name_);
  }
  sem_id_ = semget(key, 1, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH | IPC_CREAT | IPC_EXCL);
  if (sem_id_ >= 0) {
    union semun arg;
    arg.val = 0;
    semctl(sem_id_, 0, SETVAL, arg);
  } else if (errno == EEXIST) {
    sem_id_ = semget(key, 1, 0);
  } else {
    throw SystemException(fun::Format("cannot create named mutex %s (semget() failed, errno=%d)", filename, errno), name_);
  }
#endif // defined(sun) || defined(__APPLE__) || defined(__osf__) || defined(__QNX__) || defined(_AIX) || defined(__EMSCRIPTEN__)
}

NamedEventImpl::~NamedEventImpl() {
#if defined(sun) || defined(__APPLE__) || defined(__osf__) || defined(__QNX__) || defined(_AIX) || defined(__EMSCRIPTEN__)
  sem_close(sem_);
#endif
}

void NamedEventImpl::SetImpl() {
#if defined(sun) || defined(__APPLE__) || defined(__osf__) || defined(__QNX__) || defined(_AIX) || defined(__EMSCRIPTEN__)
  if (sem_post(sem_) != 0) {
    throw SystemException("cannot set named event", name_);
  }
#else
  struct sembuf op;
  op.sem_num = 0;
  op.sem_op  = 1;
  op.sem_flg = 0;
  if (semop(sem_id_, &op, 1) != 0) {
    throw SystemException("cannot set named event", name_);
  }
#endif
}

void NamedEventImpl::WaitImpl() {
#if defined(sun) || defined(__APPLE__) || defined(__osf__) || defined(__QNX__) || defined(_AIX) || defined(__EMSCRIPTEN__)
  int err;
  do {
    err = sem_wait(sem_);
  } while (err && errno == EINTR);
  if (err) {
    throw SystemException("cannot wait for named event", name_);
  }
#else
  struct sembuf op;
  op.sem_num = 0;
  op.sem_op  = -1;
  op.sem_flg = 0;
  int err;
  do {
    err = semop(sem_id_, &op, 1);
  } while (err && errno == EINTR);
  if (err) {
    throw SystemException("cannot wait for named event", name_);
  }
#endif
}

String NamedEventImpl::GetFileName() {
#if defined(sun) || defined(__APPLE__) || defined(__QNX__)
  String fn = "/";
#else
  String fn = "/tmp/";
#endif
  fn.Append(name_);
  fn.Append(".event");
  return fn;
}

} // namespace fun
