#pragma once

#include "fun/base/base.h"
#include "fun/base/runnable.h"
#include "fun/base/thread.h"
#include "fun/base/async_starter.h"
#include "fun/base/async_runnable.h"
#include "fun/base/notification_queue.h"

namespace fun {

/**
 * This class is used to implement an active object
 * with strictly serialized method execution.
 * 
 * An active object, which is an ordinary object
 * containing AsyncMethod members, executes all
 * active methods in their own thread.
 * This behavior does not fit the "classic"
 * definition of an active object, which serializes
 * the execution of active methods (in other words,
 * only one active method can be running at any given
 * time).
 * 
 * Using this class as a base class, the serializing
 * behavior for active objects can be implemented.
 * 
 * The following example shows how this is done:
 * 
 *   class AsyncObject : public AsyncDispatcher {
 *   public:
 *     AsyncObject()
 *       : example_active_method(this, &AsyncObject::ExampleActiveMethodImpl) {
 *     }
 * 
 *     AsyncMethod<String, String, AsyncObject, AsyncStarter<AsyncDispatcher> > example_active_method;
 * 
 *   protected:
 *     String ExampleActiveMethodImpl(const String& arg) {
 *       ...
 *     }
 *   };
 * 
 * The only things different from the example in
 * AsyncMethod is that the AsyncObject in this case
 * inherits from AsyncDispatcher, and that the AsyncMethod
 * template for exampleActiveMethod has an additional parameter,
 * specifying the specialized AsyncStarter for AsyncDispatcher.
 */
class FUN_BASE_API AsyncDispatcher : protected Runnable {
 public:
  /**
   * Creates the AsyncDispatcher.
   */
  AsyncDispatcher();

  /**
   * Creates the AsyncDispatcher and sets
   * the priority of its thread.
   */
  AsyncDispatcher(Thread::Priority priority);

  /**
   * Destroys the AsyncDispatcher.
   */
  virtual ~AsyncDispatcher();

  /**
   * Adds the Runnable to the dispatch queue.
   */
  void Start(AsyncRunnableBase::Ptr runnable);

  /**
   * Cancels all queued methods.
   */
  void Cancel();

 protected:
  void Run() override;
  void Stop();

 private:
  Thread thread_;
  NotificationQueue queue_;
};


/**
 * A specialization of AsyncStarter for AsyncStarter.
 */
template <>
class AsyncStarter<AsyncDispatcher> {
 public:
  static void Start(AsyncDispatcher* owner, AsyncRunnableBase::Ptr runnable) {
    fun_check_ptr(owner);
    fun_check_ptr(runnable);

    owner->Start(runnable);
  }
};

} // namespace fun
