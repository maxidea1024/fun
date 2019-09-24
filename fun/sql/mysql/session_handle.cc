#include "fun/sql/mysql/session_handle.h"
#include "fun/sql/sql_exception.h"
#include "fun/base/singleton.h"

#ifdef FUN_PLATFORM_UNIX_FAMILY
#include <pthread.h>
#endif

#define FUN_MYSQL_VERSION_NUMBER  ((NDB_VERSION_MAJOR<<16) | (NDB_VERSION_MINOR<<8) | (NDB_VERSION_BUILD&0xFF))

namespace fun {
namespace sql {
namespace mysql {

#ifdef FUN_PLATFORM_UNIX_FAMILY
class ThreadCleanupHelper {
 public:
  ThreadCleanupHelper() {
    if (pthread_key_create(&_key, &ThreadCleanupHelper::Cleanup) != 0) {
      throw fun::SystemException("cannot create TLS key for mysql cleanup");
    }
  }

  void Init() {
    if (pthread_setspecific(_key, reinterpret_cast<void*>(1))) {
      throw fun::SystemException("cannot set TLS key for mysql cleanup");
    }
  }

  static ThreadCleanupHelper& Instance() {
    return *sh_.Get();
  }

  static void Cleanup(void* data) {
    mysql_thread_end();
  }

 private:
  pthread_key_t _key;
  static fun::SingletonHolder<ThreadCleanupHelper> sh_;
};

fun::SingletonHolder<ThreadCleanupHelper> ThreadCleanupHelper::sh_;
#endif //FUN_PLATFORM_UNIX_FAMILY


SessionHandle::SessionHandle(MYSQL* mysql) : handle_(nullptr) {
  init(mysql);
#ifdef FUN_PLATFORM_UNIX_FAMILY
  ThreadCleanupHelper::Instance().Init();
#endif
}

void SessionHandle::Init(MYSQL* mysql) {
  if (!handle_) {
    handle_ = mysql_init(mysql);
    if (!handle_) {
      throw ConnectionException("mysql_init error");
    }
  }
}

SessionHandle::~SessionHandle() {
  Close();
}

void SessionHandle::SetOptions(mysql_option opt) {
  if (mysql_options(handle_, opt, 0) != 0)
    throw ConnectionException("mysql_options error", handle_);
}

void SessionHandle::SetOptions(mysql_option opt, bool b) {
  my_bool tmp = b;
  if (mysql_options(handle_, opt, &tmp) != 0)
    throw ConnectionException("mysql_options error", handle_);
}

void SessionHandle::SetOptions(mysql_option opt, const char* c) {
  if (mysql_options(handle_, opt, c) != 0)
    throw ConnectionException("mysql_options error", handle_);
}

void SessionHandle::SetOptions(mysql_option opt, unsigned int i) {
#if (FUN_MYSQL_VERSION_NUMBER < 0x050108)
  const char* tmp = (const char*)&i;
#else
  const void* tmp = (const void*)&i;
#endif
  if (mysql_options(handle_, opt, tmp) != 0) {
    throw ConnectionException("mysql_options error", handle_);
  }
}

void SessionHandle::Connect(const char* host, const char* user, const char* password, const char* db, unsigned int port) {
#ifdef HAVE_MYSQL_REAL_CONNECT
  if (!mysql_real_connect(handle_, host, user, password, db, port, 0, 0)) {
    throw ConnectionFailedException(mysql_error(handle_));
  }
#else
  if (!mysql_connect(handle_, host, user, password)) {
    throw ConnectionFailedException(mysql_error(handle_))
  }
#endif
}

void SessionHandle::Close() {
  if (handle_) {
    mysql_close(handle_);
    handle_ = 0;
  }
}

void SessionHandle::StartTransaction() {
  int rc = mysql_autocommit(handle_, false);
  if (rc != 0) {
    // retry if connection lost
    int err = mysql_errno(handle_);
    if (err == 2006 /* CR_SERVER_GONE_ERROR */ || err == 2013 /* CR_SERVER_LOST */) {
      rc = mysql_autocommit(handle_, false);
    }
  }
  if (rc != 0) {
    throw TransactionException("Start transaction failed.", handle_);
  }
}

void SessionHandle::Commit() {
  if (mysql_commit(handle_) != 0) {
    throw TransactionException("Commit failed.", handle_);
  }
}

void SessionHandle::Rollback() {
  if (mysql_rollback(handle_) != 0) {
    throw TransactionException("Rollback failed.", handle_);
  }
}

void SessionHandle::Reset() {
#if ((defined (MYSQL_VERSION_ID)) && (MYSQL_VERSION_ID >= 50700)) || ((defined (MARIADB_PACKAGE_VERSION_ID)) && (MARIADB_PACKAGE_VERSION_ID >= 30000))
  if (mysql_reset_connection(handle_) != 0) {
#else
  if (mysql_refresh(handle_, REFRESH_TABLES | REFRESH_STATUS | REFRESH_THREADS | REFRESH_READ_LOCK) != 0) {
#endif
    throw TransactionException("Reset connection failed.", handle_);
  }
}

} // namespace mysql
} // namespace sql
} // namespace fun
