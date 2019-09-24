#include "fun/sql/session_pool.h"
#include <algorithm>
#include "fun/sql/session_factory.h"
#include "fun/sql/sql_exception.h"

namespace fun {
namespace sql {

SessionPool::SessionPool(const String& connector,
                         const String& connection_string, int32 min_sessions,
                         int32 max_sessions, int32 idle_time)
    : connector_(connector),
      connection_string_(connection_string),
      min_sessions_(min_sessions),
      max_sessions_(max_sessions),
      idle_time_(idle_time),
      session_count_(0),
      janitor_timer_(1000 * idle_time, 1000 * idle_time / 4),
      shutdown_(false) {
  fun::TimerCallback<SessionPool> callback(*this, &SessionPool::OnJanitorTimer);
  janitor_timer_.Start(callback);
}

SessionPool::~SessionPool() {
  try {
    Shutdown();
  } catch (...) {
    fun_unexpected();
  }
}

Session SessionPool::Get(const String& name, bool value) {
  Session s = Get();
  add_feature_map_.insert(AddFeatureMap::value_type(
      s.GetImpl(), std::make_pair(name, s.GetFeature(name))));
  s.SetFeature(name, value);

  return s;
}

Session SessionPool::Get() {
  if (shutdown_) {
    throw InvalidAccessException("Session pool has been shut down.");
  }

  PurgeDeadSessions();

  fun::Mutex::ScopedLock guard(mutex_);
  if (idle_sessions_.IsEmpty()) {
    if (session_count_ < max_sessions_) {
      Session new_session(
          SessionFactory::Instance().Create(connector_, connection_string_));
      ApplySettings(new_session.GetImpl());
      CustomizeSession(new_session);

      PooledSessionHolderPtr holder(
          new PooledSessionHolder(*this, new_session.GetImpl()));
      idle_sessions_[holder.Get()] = holder;
      ++session_count_;
    } else {
      throw SessionPoolExhaustedException(connector_);
    }
  }

  PooledSessionHolderPtr holder(idle_sessions_.begin()->second);
  PooledSessionImplPtr pPSI(new PooledSessionImpl(holder));

  active_sessions_[holder.Get()] = holder;
  idle_sessions_.erase(holder.Get());
  return Session(pPSI);
}

void SessionPool::PurgeDeadSessions() {
  if (shutdown_) {
    return;
  }

  fun::Mutex::ScopedLock guard(mutex_);
  SessionList::iterator it = idle_sessions_.begin();
  for (; it != idle_sessions_.end();) {
    PooledSessionHolderPtr holder = it->second;
    if (!holder->GetSession()->IsConnected()) {
      it = idle_sessions_.erase(it);
      --session_count_;
    } else {
      ++it;
    }
  }
}

int32 SessionPool::Capacity() const { return max_sessions_; }

int32 SessionPool::UsedCount() const {
  fun::Mutex::ScopedLock guard(mutex_);
  return (int32)active_sessions_.size();
}

int32 SessionPool::IdleCount() const {
  fun::Mutex::ScopedLock guard(mutex_);
  return (int32)idle_sessions_.size();
}

int32 SessionPool::DeadCount() {
  int32 count = 0;

  fun::Mutex::ScopedLock guard(mutex_);
  SessionList::iterator it = active_sessions_.begin();
  SessionList::iterator itEnd = active_sessions_.end();
  for (; it != itEnd; ++it) {
    PooledSessionHolderPtr holder = it->second;
    if (!holder->GetSession()->IsConnected()) {
      ++count;
    }
  }

  return count;
}

int32 SessionPool::AllocatedCount() const {
  fun::Mutex::ScopedLock guard(mutex_);
  return session_count_;
}

int32 SessionPool::AvailableCount() const {
  if (shutdown_) {
    return 0;
  }
  return max_sessions_ - UsedCount();
}

void SessionPool::SetFeature(const String& name, bool state) {
  if (shutdown_) {
    throw InvalidAccessException("Session pool has been shut down.");
  }

  fun::Mutex::ScopedLock guard(mutex_);
  if (session_count_ > 0) {
    throw InvalidAccessException(
        "Features can not be set after the first session was created.");
  }

  feature_map_.insert(FeatureMap::value_type(name, state));
}

bool SessionPool::GetFeature(const String& name) {
  if (shutdown_) {
    throw InvalidAccessException("Session pool has been shut down.");
  }

  FeatureMap::const_iterator it = feature_map_.find(name);
  if (feature_map_.end() == it) {
    throw NotFoundException("Feature not found:" + name);
  }

  return it->second;
}

void SessionPool::SetProperty(const String& name, const fun::Any& value) {
  if (shutdown_) {
    throw InvalidAccessException("Session pool has been shut down.");
  }

  fun::Mutex::ScopedLock guard(mutex_);
  if (session_count_ > 0) {
    throw InvalidAccessException(
        "Properties can not be set after first session was created.");
  }

  property_map_.insert(PropertyMap::value_type(name, value));
}

fun::Any SessionPool::GetProperty(const String& name) {
  PropertyMap::const_iterator it = property_map_.find(name);
  if (property_map_.end() == it) {
    throw NotFoundException("Property not found:" + name);
  }

  return it->second;
}

void SessionPool::ApplySettings(SessionImpl::Ptr impl) {
  FeatureMap::iterator fmIt = feature_map_.begin();
  FeatureMap::iterator fmEnd = feature_map_.end();
  for (; fmIt != fmEnd; ++fmIt) {
    impl->SetFeature(fmIt->first, fmIt->second);
  }

  PropertyMap::iterator pmIt = property_map_.begin();
  PropertyMap::iterator pmEnd = property_map_.end();
  for (; pmIt != pmEnd; ++pmIt) {
    impl->SetProperty(pmIt->first, pmIt->second);
  }
}

void SessionPool::CustomizeSession(Session&) {
  // NOOP...
}

void SessionPool::PutBack(PooledSessionHolderPtr holder) {
  if (shutdown_) {
    return;
  }

  fun::Mutex::ScopedLock guard(mutex_);

  PooledSessionHolder* psh = holder.Get();
  SessionList::iterator it = active_sessions_.find(psh);
  if (it != active_sessions_.end()) {
    if (holder->GetSession()->IsConnected()) {
      holder->GetSession()->Reset();

      // reverse settings applied at acquisition time, if any
      AddPropertyMap::iterator pIt =
          add_property_map_.find(holder->GetSession());
      if (pIt != add_property_map_.end()) {
        holder->GetSession()->SetProperty(pIt->second.first,
                                          pIt->second.second);
      }

      AddFeatureMap::iterator fIt = add_feature_map_.find(holder->GetSession());
      if (fIt != add_feature_map_.end()) {
        holder->GetSession()->SetFeature(fIt->second.first, fIt->second.second);
      }

      // re-apply the default pool settings
      ApplySettings(holder->GetSession());

      holder->Access();
      idle_sessions_[holder.Get()] = holder;
    } else {
      --session_count_;
    }

    active_sessions_.erase(it);
  } else {
    it = idle_sessions_.find(psh);
    if (it != idle_sessions_.end()) {
      return;
    }
    fun_bugcheck_msg("Unknown session passed to SessionPool::PutBack()");
  }
}

void SessionPool::OnJanitorTimer(fun::Timer&) {
  if (shutdown_) {
    return;
  }

  fun::Mutex::ScopedLock guard(mutex_);

  SessionList::iterator it = idle_sessions_.begin();
  while (session_count_ > min_sessions_ && it != idle_sessions_.end()) {
    PooledSessionHolderPtr holder = it->second;
    if (holder->IdleTime() > idle_time_ ||
        !holder->GetSession()->IsConnected()) {
      try {
        holder->GetSession()->Close();
      } catch (...) {
      }
      it = idle_sessions_.erase(it);
      --session_count_;
    } else {
      ++it;
    }
  }
}

void SessionPool::Shutdown() {
  if (shutdown_) {
    return;
  }

  fun::Mutex::ScopedLock guard(mutex_);
  shutdown_ = true;
  janitor_timer_.Stop();
  CloseAll(idle_sessions_);
  CloseAll(active_sessions_);
}

void SessionPool::CloseAll(SessionList& session_list) {
  SessionList::iterator it = session_list.begin();
  for (; it != session_list.end();) {
    PooledSessionHolderPtr holder = it->second;
    try {
      holder->GetSession()->Close();
    } catch (...) {
    }
    it = session_list.erase(it);
    if (session_count_ > 0) {
      --session_count_;
    }
  }
}

}  // namespace sql
}  // namespace fun
