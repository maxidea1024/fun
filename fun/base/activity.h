#pragma once

#include "fun/base/base.h"
#include "fun/base/event.h"
#include "fun/base/mutex.h"
#include "fun/base/runnable_adapter.h"
#include "fun/base/thread_pool.h"

namespace fun {

/**
 * This template class helps to implement active objects.
 * An active object uses threads to decouple method
 * execution from method invocation, or to perform tasks
 * autonomously, without intervention of a caller.
 *
 * An activity is a (typically longer running) method
 * that executes within its own task. Activities can
 * be started automatically (upon object construction)
 * or manually at a later time. Activities can also
 * be stopped at any time. However, to make stopping
 * an activity work, the method implementing the
 * activity has to check periodically whether it
 * has been requested to stop, and if so, return.
 * Activities are stopped before the object they belong to is
 * destroyed. Methods implementing activities cannot have arguments
 * or return values.
 *
 * Activity objects are used as follows:
 *
 * \code
 *   class ActiveObject {
 *   public:
 *     ActiveObject()
 *       : activity_(this, &ActiveObject::RunActivity) {
 *       ...
 *     }
 *
 *     ...
 *
 *   protected:
 *     void RunActivity() {
 *       while (!activity_.IsStopped()) {
 *         ...
 *       }
 *     }
 *
 *   private:
 *     Activity<ActiveObject> activity_;
 *   };
 * \endcode
 */
template <typename C>
class Activity : public Runnable {
 public:
  typedef RunnableAdapter<C> RunnableAdapterType;
  typedef typename RunnableAdapterType::Callback Callback;

  /**
   * Creates the activity. Call start() to start it.
   */
  Activity(C* owner, Callback method)
      : owner_(owner),
        runnable_(*owner, method),
        stopped_(true),
        running_(false),
        done_(EventResetType::Manual) {
    fun_check_ptr(owner_);
  }

  /**
   * Stops and destroys the activity.
   */
  ~Activity() {
    try {
      Stop();
      Wait();
    } catch (...) {
      fun_unexpected();
    }
  }

  Activity() = delete;
  Activity(const Activity&) = delete;
  Activity& operator=(const Activity&) = delete;

  /**
   * Starts the activity by acquiring a
   * thread for it from the default thread pool.
   */
  void Start() { Start(ThreadPool::DefaultPool()); }

  void Start(ThreadPool& pool) {
    ScopedLock<FastMutex> guard(mutex_);

    if (!running_) {
      done_.Reset();
      stopped_ = false;
      running_ = true;
      try {
        pool.Start(*this);
      } catch (...) {
        running_ = false;
        throw;
      }
    }
  }

  /**
   * Requests to stop the activity.
   */
  void Stop() {
    ScopedLock<FastMutex> guard(mutex_);

    stopped_ = true;
  }

  /**
   * Waits for the activity to complete.
   */
  void Wait() {
    if (running_) {
      done_.Wait();
    }
  }

  /**
   * Waits the given interval for the activity to complete.
   * An TimeoutException is thrown if the activity does not
   * complete within the given interval.
   */
  void Wait(int32 milliseconds) {
    if (running_) {
      done_.Wait(milliseconds);
    }
  }

  /**
   * Returns true if the activity has been requested to stop.
   */
  bool IsStopped() const { return stopped_; }

  /**
   * Returns true if the activity is running.
   */
  bool IsRunning() const { return running_; }

 protected:
  // Runnable interface
  void Run() override {
    try {
      runnable_.Run();
    } catch (...) {
      running_ = false;
      done_.Set();

      // Rethrow... will be cought at outside caller. (Thread procedure)
      throw;
    }
    running_ = false;
    done_.Set();
  }

 private:
  C* owner_;
  RunnableAdapterType runnable_;
  std::atomic<bool> stopped_;
  std::atomic<bool> running_;
  Event done_;
  FastMutex mutex_;
};

}  // namespace fun
