#include "fun/sql/sqlite/notifier.h"

namespace fun {
namespace sql {
namespace sqlite {

Notifier::Notifier(const Session& session, EnabledEventType enabled)
  : session_(session),
    row_(),
    enabled_events_() {
  if (enabled & SQLITE_NOTIFY_UPDATE)   EnableUpdate();
  if (enabled & SQLITE_NOTIFY_COMMIT)   EnableCommit();
  if (enabled & SQLITE_NOTIFY_ROLLBACK) EnableRollback();
}

Notifier::Notifier(const Session& session, const Any& value, EnabledEventType enabled)
  : session_(session),
    value_(value),
    row_(),
    enabled_events_() {
  if (enabled & SQLITE_NOTIFY_UPDATE)   EnableUpdate();
  if (enabled & SQLITE_NOTIFY_COMMIT)   EnableCommit();
  if (enabled & SQLITE_NOTIFY_ROLLBACK) EnableRollback();
}

Notifier::~Notifier() {
  try {
    DisableAll();
  } catch (...) {
    fun_unexpected();
  }
}

bool Notifier::EnableUpdate() {
  fun::Mutex::ScopedLock l(mutex_);

  if (Utility::RegisterUpdateHandler(Utility::GetDbHandle(session_), &SqliteUpdateCallbackFn, this)) {
    enabled_events_ |= SQLITE_NOTIFY_UPDATE;
  }

  return UpdateEnabled();
}

bool Notifier::DisableUpdate() {
  fun::Mutex::ScopedLock l(mutex_);

  if (Utility::RegisterUpdateHandler(Utility::GetDbHandle(session_), (Utility::UpdateCallbackType) 0, this)) {
    enabled_events_ &= ~SQLITE_NOTIFY_UPDATE;
  }

  return !UpdateEnabled();
}

bool Notifier::UpdateEnabled() const {
  return 0 != (enabled_events_ & SQLITE_NOTIFY_UPDATE);
}

bool Notifier::EnableCommit() {
  fun::Mutex::ScopedLock l(mutex_);

  if (Utility::RegisterUpdateHandler(Utility::GetDbHandle(session_), &SqliteCommitCallbackFn, this)) {
    enabled_events_ |= SQLITE_NOTIFY_COMMIT;
  }

  return CommitEnabled();
}

bool Notifier::DisableCommit() {
  fun::Mutex::ScopedLock l(mutex_);

  if (Utility::RegisterUpdateHandler(Utility::GetDbHandle(session_), (Utility::CommitCallbackType) 0, this)) {
    enabled_events_ &= ~SQLITE_NOTIFY_COMMIT;
  }

  return !CommitEnabled();
}

bool Notifier::CommitEnabled() const {
  return 0 != (enabled_events_ & SQLITE_NOTIFY_COMMIT);
}

bool Notifier::EnableRollback() {
  fun::Mutex::ScopedLock l(mutex_);

  if (Utility::RegisterUpdateHandler(Utility::GetDbHandle(session_), &SqliteRollbackCallbackFn, this)) {
    enabled_events_ |= SQLITE_NOTIFY_ROLLBACK;
  }

  return RollbackEnabled();
}

bool Notifier::DisableRollback() {
  fun::Mutex::ScopedLock l(mutex_);

  if (Utility::RegisterUpdateHandler(Utility::GetDbHandle(session_), (Utility::RollbackCallbackType) 0, this)) {
    enabled_events_ &= ~SQLITE_NOTIFY_ROLLBACK;
  }

  return !RollbackEnabled();
}

bool Notifier::RollbackEnabled() const {
  return 0 != (enabled_events_ & SQLITE_NOTIFY_ROLLBACK);
}

bool Notifier::EnableAll() {
  return EnableUpdate() && EnableCommit() && EnableRollback();
}

bool Notifier::DisableAll() {
  return DisableUpdate() && DisableCommit() && DisableRollback();
}

void Notifier::SqliteUpdateCallbackFn(void* pVal, int opCode, const char* /*db*/, const char* pTable, int64 row) {
  fun_check_ptr(pVal);
  Notifier* pV = reinterpret_cast<Notifier*>(pVal);
  if (opCode == Utility::OPERATION_INSERT) {
    pV->row_ = row;
    pV->table_ = pTable;
    pV->insert.Notify(pV);
  } else if (opCode == Utility::OPERATION_UPDATE) {
    pV->row_ = row;
    pV->table_ = pTable;
    pV->update.Notify(pV);
  } else if (opCode == Utility::OPERATION_DELETE) {
    pV->row_ = row;
    pV->table_ = pTable;
    pV->erase.Notify(pV);
  }
}

int Notifier::SqliteCommitCallbackFn(void* pVal) {
  Notifier* pV = reinterpret_cast<Notifier*>(pVal);

  try {
    pV->commit.Notify(pV);
  } catch (...) {
    return -1;
  }

  return 0;
}

void Notifier::SqliteRollbackCallbackFn(void* pVal) {
  Notifier* pV = reinterpret_cast<Notifier*>(pVal);
  pV->rollback.Notify(pV);
}

} // namespace sqlite
} // namespace sql
} // namespace fun
