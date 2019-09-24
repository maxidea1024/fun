#pragma once

#include "fun/base/base.h"


#ifndef FUN_NO_INOTIFY

#include "fun/base/file.h"
#include "fun/base/multicast_event.h"
#include "fun/base/runnable.h"
#include "fun/base/thread.h"
#include "fun/base/atomic_counter.h"

namespace fun {

class DirectoryWatcherStrategy;

/**
 * This class is used to get notifications about changes
 * to the filesystem, more specifically, to a specific
 * directory. Changes to a directory are reported via
 * events.
 * 
 * A thread will be created that watches the specified
 * directory for changes. Events are reported in the context
 * of this thread.
 * 
 * Note that changes to files in subdirectories of the watched
 * directory are not reported. Separate DirectoryWatcher objects
 * must be created for these directories if they should be watched.
 * 
 * Changes to file attributes are not reported.
 * 
 * On Windows, this class is implemented using FindFirstChangeNotification()/
 * FindNextChangeNotification().
 * On Linux, this class is implemented using inotify.
 * On FreeBSD and Darwin (Mac OS X, iOS), this class uses kevent/kqueue.
 * On all other platforms, the watched directory is periodically scanned
 * for changes. This can negatively affect performance if done too often.
 * Therefore, the interval in which scans are done can be specified in
 * the constructor. Note that periodic scanning will also be done on FreeBSD
 * and Darwin if events for changes to files (DW_ITEM_MODIFIED) are enabled.
 * To avoid problems (e.g. with network shares notifications), scanning
 * can be forced as the only mechanism, regardless of platform default.
 * 
 * DW_ITEM_MOVED_FROM and DW_ITEM_MOVED_TO events will only be reported
 * on Linux. On other platforms, a file rename or move operation
 * will be reported via a DW_ITEM_REMOVED and a DW_ITEM_ADDED event.
 * The order of these two events is not defined.
 * 
 * An event mask can be specified to enable only certain events.
 */
class FUN_BASE_API DirectoryWatcher : protected Runnable {
 public:
  enum DirectoryEventType {
    /**
     * A new item has been created and added to the directory.
     */
    DW_ITEM_ADDED = 1,

    /**
     * An item has been removed from the directory.
     */
    DW_ITEM_REMOVED = 2,

    /**
     * An item has been modified.
     */
    DW_ITEM_MODIFIED = 4,

    /**
     * An item has been renamed or moved. This event delivers the old name.
     */
    DW_ITEM_MOVED_FROM = 8,

    /**
     * An item has been renamed or moved. This event delivers the new name.
     */
    DW_ITEM_MOVED_TO = 16
  };

  enum DirectoryEventMask {
    /**
     * Enables all event types.
     */
    DW_FILTER_ENABLE_ALL = 31,

    /**
     * Disables all event types.
     */
    DW_FILTER_DISABLE_ALL = 0
  };

  enum {
    /**
     * Default scan interval for platforms that don't provide
     * a native notification mechanism.
     */
    DW_DEFAULT_SCAN_INTERVAL = 5
  };

  struct DirectoryEvent {
    DirectoryEvent(const File& f, DirectoryEventType ev)
      : item(f), event(ev) {}

    /**
     * The directory or file that has been changed.
     */
    const File& item;

    /**
     * The kind of event.
     */
    DirectoryEventType event;
  };

  /**
   * Fired when a file or directory has been created or added to the directory.
   */
  MulticastEvent<const DirectoryEvent> item_added;

  /**
   * Fired when a file or directory has been removed from the directory.
   */
  MulticastEvent<const DirectoryEvent> item_removed;

  /**
   * Fired when a file or directory has been modified.
   */
  MulticastEvent<const DirectoryEvent> item_modified;

  /**
   * Fired when a file or directory has been renamed.
   * This event delivers the old name.
   */
  MulticastEvent<const DirectoryEvent> item_moved_from;

  /**
   * Fired when a file or directory has been moved.
   * This event delivers the new name.
   */
  MulticastEvent<const DirectoryEvent> item_move_to;

  /**
   * Fired when an error occurs while scanning for changes.
   */
  MulticastEvent<const Exception> scan_error;

  /**
   * Creates a DirectoryWatcher for the directory given in path.
   * To enable only specific events, an event_mask can be specified by
   * OR-ing the desired event IDs (e.g., DW_ITEM_ADDED | DW_ITEM_MODIFIED).
   * On platforms where no native filesystem notifications are available,
   * scan_interval specifies the interval in seconds between scans
   * of the directory. Native notification mechanism can also be disabled
   * (i.e. replaced with scanning) by setting force_scan to true.
   */
  DirectoryWatcher(const String& path,
                  int event_mask = DW_FILTER_ENABLE_ALL,
                  int scan_interval = DW_DEFAULT_SCAN_INTERVAL,
                  bool force_scan = false);

  /**
   * Creates a DirectoryWatcher for the specified directory
   * To enable only specific events, an event_mask can be specified by
   * OR-ing the desired event IDs (e.g., DW_ITEM_ADDED | DW_ITEM_MODIFIED).
   * On platforms where no native filesystem notifications are available,
   * scan_interval specifies the interval in seconds between scans
   * of the directory. Native notification mechanism can also be disabled
   * (i.e. replaced with scanning) by setting force_scan to true.
   */
  DirectoryWatcher(const File& directory,
                  int event_mask = DW_FILTER_ENABLE_ALL,
                  int scan_interval = DW_DEFAULT_SCAN_INTERVAL,
                  bool force_scan = false);

  /**
   * Destroys the DirectoryWatcher.
   */
  ~DirectoryWatcher();

  /**
   * Suspends sending of events. Can be called multiple times, but every
   * call to SuspendEvent() must be matched by a call to ResumeEvents().
   */
  void SuspendEvents();

  /**
   * Resumes events, after they have been suspended with a call to SuspendEvents().
   */
  void ResumeEvents();

  /**
   * Returns true if events are suspended.
   */
  bool GetEventsSuspended() const;

  /**
   * Returns the value of the GetEventMask passed to the constructor.
   */
  int GetEventMask() const;

  /**
   * Returns the scan interval in seconds.
   */
  int GetScanInterval() const;

  /**
   * Returns the directory being watched.
   */
  const File& GetDirectory() const;

  /**
   * Returns true if the platform supports DW_ITEM_MOVED_FROM/itemMovedFrom and
   * DW_ITEM_MOVED_TO/itemMovedTo events.
   */
  bool SupportsMoveEvents() const;

 protected:
  void Init();
  void Stop();

  void Run() override;

 private:
  DirectoryWatcher() = delete;
  DirectoryWatcher(const DirectoryWatcher&) = delete;
  DirectoryWatcher& operator = (const DirectoryWatcher&) = delete;

  Thread thread_;
  File directory_;
  int event_mask_;
  AtomicCounter32 events_suspended_;
  int scan_interval_;
  bool force_scan_;
  DirectoryWatcherStrategy* strategy_;
};


//
// inlines
//

FUN_ALWAYS_INLINE bool DirectoryWatcher::GetEventsSuspended() const {
  return events_suspended_.Value() > 0;
}

FUN_ALWAYS_INLINE int DirectoryWatcher::GetEventMask() const {
  return event_mask_;
}

FUN_ALWAYS_INLINE int DirectoryWatcher::GetScanInterval() const {
  return scan_interval_;
}

FUN_ALWAYS_INLINE const File& DirectoryWatcher::GetDirectory() const {
  return directory_;
}

} // namespace fun

#endif // FUN_NO_INOTIFY
