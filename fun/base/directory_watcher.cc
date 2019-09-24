#include "fun/base/directory_watcher.h"

#ifdef FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/windows_less.h"
#endif

#ifndef FUN_NO_INOTIFY

#include "fun/base/path.h"
#include "fun/base/glob_matcher.h"
#include "fun/base/directory_iterator.h"
#include "fun/base/event.h"
#include "fun/base/exception.h"
//#include "fun/base/buffer.h"
#if FUN_PLATFORM == FUN_PLATFORM_LINUX || FUN_PLATFORM == FUN_PLATFORM_ANDROID
  #include <sys/inotify.h>
  #include <sys/select.h>
  #include <unistd.h>
#elif FUN_PLATFORM == FUN_PLATFORM_MAC_OS_X || FUN_PLATFORM == FUN_PLATFORM_FREE_BSD
  #include <fcntl.h>
  #include <sys/types.h>
  #include <sys/event.h>
  #include <sys/time.h>
  #include <unistd.h>
  #if (FUN_PLATFORM == FUN_PLATFORM_FREE_BSD) && !defined(O_EVTONLY)
    #define O_EVTONLY 0x8000
  #endif
#endif

#include <algorithm>
#include <map>

namespace fun {

class DirectoryWatcherStrategy {
 public:
  DirectoryWatcherStrategy(DirectoryWatcher& owner) : owner_(owner) {}
  virtual ~DirectoryWatcherStrategy() {}

  DirectoryWatcher& GetOwner() {
    return owner_;
  }

  virtual void Run() = 0;
  virtual void Stop() = 0;
  virtual bool SupportsMoveEvents() const = 0;

 protected:
  struct ItemInfo {
    ItemInfo() : size(0) {}

    ItemInfo(const ItemInfo& other)
      : path(other.path),
        size(other.size),
        last_modified(other.last_modified) {}

    explicit ItemInfo(const File& f)
      : path(f.GetPath()),
        size(f.IsFile() ? f.GetSize() : 0),
        last_modified(f.GetLastModified()) {}

    String path;
    File::FileSize size;
    Timestamp last_modified;
  };
  typedef std::map<String, ItemInfo> ItemInfoMap;

  void Scan(ItemInfoMap& entries) {
    DirectoryIterator it(GetOwner().GetDirectory());
    DirectoryIterator end;
    while (it != end) {
      // DirectoryWatcher should not Stop watching if it fails to get the info
      // of a file, it should just ignore it.
      try {
        entries[it.GetPath().GetFileName()] = ItemInfo(*it);
      } catch (fun::FileNotFoundException&) {
        // The file is missing, remove it from the entries so it is treated as
        // deleted.
        entries.erase(it.GetPath().GetFileName());
      }
      ++it;
    }
  }

  void Compare(ItemInfoMap& old_entries, ItemInfoMap& new_entries) {
    for (ItemInfoMap::iterator itn = new_entries.begin(); itn != new_entries.end(); ++itn) {
      ItemInfoMap::iterator ito = old_entries.find(itn->first);
      if (ito != old_entries.end()) {
        if ((GetOwner().GetEventMask() & DirectoryWatcher::DW_ITEM_MODIFIED) && !GetOwner().GetEventsSuspended()) {
          if (itn->second.size != ito->second.size || itn->second.last_modified != ito->second.last_modified) {
            fun::File f(itn->second.path);
            DirectoryWatcher::DirectoryEvent ev(f, DirectoryWatcher::DW_ITEM_MODIFIED);
            GetOwner().item_modified(&GetOwner(), ev);
          }
        }
        old_entries.erase(ito);
      } else if ((GetOwner().GetEventMask() & DirectoryWatcher::DW_ITEM_ADDED) && !GetOwner().GetEventsSuspended()) {
        fun::File f(itn->second.path);
        DirectoryWatcher::DirectoryEvent ev(f, DirectoryWatcher::DW_ITEM_ADDED);
        GetOwner().item_added(&GetOwner(), ev);
      }
    }
    if ((GetOwner().GetEventMask() & DirectoryWatcher::DW_ITEM_REMOVED) && !GetOwner().GetEventsSuspended()) {
      for (ItemInfoMap::iterator it = old_entries.begin(); it != old_entries.end(); ++it) {
        fun::File f(it->second.path);
        DirectoryWatcher::DirectoryEvent ev(f, DirectoryWatcher::DW_ITEM_REMOVED);
        GetOwner().item_removed(&GetOwner(), ev);
      }
    }
  }

 private:
  DirectoryWatcherStrategy() = delete;
  DirectoryWatcherStrategy(const DirectoryWatcherStrategy&) = delete;
  DirectoryWatcherStrategy& operator = (const DirectoryWatcherStrategy&) = delete;

  DirectoryWatcher& owner_;
};


#if FUN_PLATFORM == FUN_PLATFORM_WINDOWS_NT

class WindowsDirectoryWatcherStrategy: public DirectoryWatcherStrategy {
 public:
  WindowsDirectoryWatcherStrategy(DirectoryWatcher& owner)
    : DirectoryWatcherStrategy(owner) {
    hstopped_ = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!hstopped_) {
      throw SystemException("cannot create event");
    }
  }

  ~WindowsDirectoryWatcherStrategy() {
    CloseHandle(hstopped_);
  }

  void Run() {
    ItemInfoMap entries;
    Scan(entries);

    DWORD filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME;
    if (GetOwner().GetEventMask() & DirectoryWatcher::DW_ITEM_MODIFIED) {
      filter |= FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE;
    }

    String path(GetOwner().GetDirectory().GetPath());
    UString upath;
    FileImpl::ConvertPath(path.ConstData(), upath);
    HANDLE hchange = FindFirstChangeNotificationW(upath.c_str(), FALSE, filter);

    if (hchange == INVALID_HANDLE_VALUE) {
      try {
        FileImpl::HandleLastErrorImpl(path);
      } catch (Exception& e) {
        GetOwner().scan_error(&GetOwner(), e);
      }
      return;
    }

    bool stopped = false;
    while (!stopped) {
      try {
        HANDLE h[2] = { hstopped_, hchange };
        switch (WaitForMultipleObjects(2, h, FALSE, INFINITE)) {
          case WAIT_OBJECT_0:
            stopped = true;
            break;
          case WAIT_OBJECT_0 + 1: {
              ItemInfoMap new_entries;
              Scan(new_entries);
              Compare(entries, new_entries);
              fun::Swap(entries, new_entries);
              if (FindNextChangeNotification(hchange) == FALSE) {
                FileImpl::HandleLastErrorImpl(path);
              }
            }
            break;
          default:
            throw SystemException("failed to wait for directory changes");
        }
      } catch (fun::Exception& e) {
        GetOwner().scan_error(&GetOwner(), e);
      }
    }
    FindCloseChangeNotification(hchange);
  }

  void Stop() {
    SetEvent(hstopped_);
  }

  bool SupportsMoveEvents() const {
    return false;
  }

 private:
  HANDLE hstopped_;
};

#elif FUN_PLATFORM == FUN_PLATFORM_LINUX || FUN_PLATFORM == FUN_PLATFORM_ANDROID

class LinuxDirectoryWatcherStrategy : public DirectoryWatcherStrategy {
 public:
  LinuxDirectoryWatcherStrategy(DirectoryWatcher& owner)
    : DirectoryWatcherStrategy(owner),
      fd_(-1),
      stopped_(false) {
    fd_ = inotify_init();
    if (fd_ == -1) {
      throw fun::IOException("cannot initialize inotify", errno);
    }
  }

  ~LinuxDirectoryWatcherStrategy() {
    close(fd_);
  }

  void Run() {
    int mask = 0;
    if (GetOwner().GetEventMask() & DirectoryWatcher::DW_ITEM_ADDED) {
      mask |= IN_CREATE;
    }
    if (GetOwner().GetEventMask() & DirectoryWatcher::DW_ITEM_REMOVED) {
      mask |= IN_DELETE;
    }
    if (GetOwner().GetEventMask() & DirectoryWatcher::DW_ITEM_MODIFIED) {
      mask |= IN_MODIFY;
    }
    if (GetOwner().GetEventMask() & DirectoryWatcher::DW_ITEM_MOVED_FROM) {
      mask |= IN_MOVED_FROM;
    }
    if (GetOwner().GetEventMask() & DirectoryWatcher::DW_ITEM_MOVED_TO) {
      mask |= IN_MOVED_TO;
    }
    int wd = inotify_add_watch(fd_, GetOwner().GetDirectory().GetPath().c_str(), mask);
    if (wd == -1) {
      try {
        FileImpl::HandleLastErrorImpl(GetOwner().GetDirectory().GetPath());
      } catch (fun::Exception& e) {
        GetOwner().ScanError(&GetOwner(), e);
      }
    }

    fun::Buffer<char> buffer(4096);
    while (!stopped_) {
      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(fd_, &fds);

      struct timeval tv;
      tv.tv_sec  = 0;
      tv.tv_usec = 200000;

      if (select(fd_ + 1, &fds, NULL, NULL, &tv) == 1) {
        int n = read(fd_, buffer.begin(), buffer.size());
        int i = 0;
        if (n > 0) {
          while (n > 0) {
            struct inotify_event* event = reinterpret_cast<struct inotify_event*>(buffer.begin() + i);

            if (event->len > 0) {
              if (!GetOwner().GetEventsSuspended()) {
                fun::Path p(GetOwner().GetDirectory().GetPath());
                p.MakeDirectory();
                p.SetFileName(event->name);
                fun::File f(p.ToString());

                if ((event->mask & IN_CREATE) && (GetOwner().GetEventMask() & DirectoryWatcher::DW_ITEM_ADDED)) {
                  DirectoryWatcher::DirectoryEvent ev(f, DirectoryWatcher::DW_ITEM_ADDED);
                  GetOwner().item_added(&GetOwner(), ev);
                }

                if ((event->mask & IN_DELETE) && (GetOwner().GetEventMask() & DirectoryWatcher::DW_ITEM_REMOVED)) {
                  DirectoryWatcher::DirectoryEvent ev(f, DirectoryWatcher::DW_ITEM_REMOVED);
                  GetOwner().item_removed(&GetOwner(), ev);
                }

                if ((event->mask & IN_MODIFY) && (GetOwner().GetEventMask() & DirectoryWatcher::DW_ITEM_MODIFIED)) {
                  DirectoryWatcher::DirectoryEvent ev(f, DirectoryWatcher::DW_ITEM_MODIFIED);
                  GetOwner().item_modified(&GetOwner(), ev);
                }

                if ((event->mask & IN_MOVED_FROM) && (GetOwner().GetEventMask() & DirectoryWatcher::DW_ITEM_MOVED_FROM)) {
                  DirectoryWatcher::DirectoryEvent ev(f, DirectoryWatcher::DW_ITEM_MOVED_FROM);
                  GetOwner().item_moved_from(&GetOwner(), ev);
                }

                if ((event->mask & IN_MOVED_TO) && (GetOwner().GetEventMask() & DirectoryWatcher::DW_ITEM_MOVED_TO)) {
                  DirectoryWatcher::DirectoryEvent ev(f, DirectoryWatcher::DW_ITEM_MOVED_TO);
                  GetOwner().item_moved_to(&GetOwner(), ev);
                }
              }
            }

            i += sizeof(inotify_event) + event->len;
            n -= sizeof(inotify_event) + event->len;
          }
        }
      }
    }
  }

  void Stop() {
    stopped_ = true;
  }

  bool SupportsMoveEvents() const {
    return true;
  }

 private:
  int fd_;
  bool stopped_;
};

#elif FUN_PLATFORM == FUN_PLATFORM_MAC_OS_X || FUN_PLATFORM == FUN_PLATFORM_FREE_BSD

class BsdDirectoryWatcherStrategy : public DirectoryWatcherStrategy {
 public:
  BsdDirectoryWatcherStrategy(DirectoryWatcher& owner)
    : DirectoryWatcherStrategy(owner),
      queue_fd_(-1),
      dir_fd_(-1),
      stopped_(false) {
    dir_fd_ = open(owner.Getdirectory().GetPath().c_str(), O_EVTONLY);
    if (dir_fd_ < 0) {
      throw fun::FileNotFoundException(owner.GetDirectory().GetPath());
    }
    queue_fd_ = kqueue();
    if (queue_fd_ < 0) {
      close(dir_fd_);
      throw fun::SystemException("Cannot create kqueue", errno);
    }
  }

  ~BsdDirectoryWatcherStrategy() {
    close(dir_fd_);
    close(queue_fd_);
  }

  void Run() {
    fun::Timestamp last_scan;
    ItemInfoMap entries;
    Scan(entries);

    while (!stopped_) {
      struct timespec timeout;
      timeout.tv_sec = 0;
      timeout.tv_nsec = 200000000;
      unsigned eventFilter = NOTE_WRITE;
      struct kevent event;
      struct kevent event_data;
      EV_SET(&event, dir_fd_, EVFILT_VNODE, EV_ADD | EV_CLEAR, eventFilter, 0, 0);
      int event_count = kevent(queue_fd_, &event, 1, &event_data, 1, &timeout);
      if (event_count < 0 || event_data.flags == EV_ERROR) {
        try {
          FileImpl::HandleLastErrorImpl(GetOwner().GetDirectory().GetPath());
        } catch (fun::Exception& e) {
          GetOwner().ScanError(&GetOwner(), e);
        }
      } else if (event_count > 0 || ((GetOwner().GetEventMask() & DirectoryWatcher::DW_ITEM_MODIFIED) && last_scan.isElapsed(GetOwner().GetScanInterval()*1000000))) {
        ItemInfoMap new_entries;
        Scan(new_entries);
        Compare(entries, new_entries);
        fun::Swap(entries, new_entries);
        last_scan.Update();
      }
    }
  }

  void Stop() {
    stopped_ = true;
  }

  bool SupportsMoveEvents() const {
    return false;
  }

 private:
  int queue_fd_;
  int dir_fd_;
  bool stopped_;
};

#endif

class PollingDirectoryWatcherStrategy : public DirectoryWatcherStrategy {
 public:
  PollingDirectoryWatcherStrategy(DirectoryWatcher& owner)
    : DirectoryWatcherStrategy(owner) {}

  ~PollingDirectoryWatcherStrategy() {}

  void Run() {
    ItemInfoMap entries;
    Scan(entries);
    while (!stopped_.TryWait(1000*GetOwner().GetScanInterval())) {
      try {
        ItemInfoMap new_entries;
        Scan(new_entries);
        Compare(entries, new_entries);
        fun::Swap(entries, new_entries);
      } catch (fun::Exception& e) {
        GetOwner().scan_error(&GetOwner(), e);
      }
    }
  }

  void Stop() {
    stopped_.Set();
  }

  bool SupportsMoveEvents() const {
    return false;
  }

 private:
  fun::Event stopped_;
};


DirectoryWatcher::DirectoryWatcher(
      const String& path, int event_mask, int scan_interval, bool force_scan)
  : directory_(path),
    event_mask_(event_mask),
    events_suspended_(0),
    scan_interval_(scan_interval),
    force_scan_(force_scan) {
  Init();
}

DirectoryWatcher::DirectoryWatcher(
        const fun::File& directory, int event_mask, int scan_interval, bool force_scan)
  : directory_(directory),
    event_mask_(event_mask),
    events_suspended_(0),
    scan_interval_(scan_interval),
    force_scan_(force_scan) {
  Init();
}

DirectoryWatcher::~DirectoryWatcher() {
  try {
    Stop();
    delete strategy_;
  } catch (...) {
    fun_unexpected();
  }
}

void DirectoryWatcher::SuspendEvents() {
  fun_check(events_suspended_ > 0);

  events_suspended_--;
}

void DirectoryWatcher::ResumeEvents() {
  events_suspended_++;
}

void DirectoryWatcher::Init() {
  if (!directory_.Exists()) {
    throw fun::FileNotFoundException(directory_.GetPath());
  }

  if (!directory_.IsDirectory()) {
    throw fun::InvalidArgumentException("not a directory", directory_.GetPath());
  }

  if (!force_scan_) {
#if FUN_PLATFORM == FUN_PLATFORM_WINDOWS_NT
    strategy_ = new WindowsDirectoryWatcherStrategy(*this);
#elif FUN_PLATFORM == FUN_PLATFORM_LINUX || FUN_PLATFORM == FUN_PLATFORM_ANDROID
    strategy_ = new LinuxDirectoryWatcherStrategy(*this);
#elif FUN_PLATFORM == FUN_PLATFORM_MAC_OS_X || FUN_PLATFORM == FUN_PLATFORM_FREE_BSD
    strategy_ = new BsdDirectoryWatcherStrategy(*this);
#else
    strategy_ = new PollingDirectoryWatcherStrategy(*this);
#endif
  } else {
    strategy_ = new PollingDirectoryWatcherStrategy(*this);
  }

  thread_.Start(*this);
}

void DirectoryWatcher::Run() {
  strategy_->Run();
}

void DirectoryWatcher::Stop() {
  strategy_->Stop();
  thread_.Join();
}

bool DirectoryWatcher::SupportsMoveEvents() const {
  return strategy_->SupportsMoveEvents();
}

} // namespace fun

#endif // FUN_NO_INOTIFY
