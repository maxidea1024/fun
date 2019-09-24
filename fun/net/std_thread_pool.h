// TODO 제거 대상..

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace fun {

class StdThreadPool {
 public:
  StdThreadPool(int32 thread_count);
  ~StdThreadPool();

  StdThreadPool(const StdThreadPool&) = delete;
  StdThreadPool& operator=(const StdThreadPool&) = delete;

 public:
  typedef Function<void()> Task;

  void AddTask(const Task& task);
  void AddTask(const Task& task, const String& tag);
  StdThreadPool& operator<<(const Task& task);

  void Stop();

  bool IsRunning() const;

  void Run();

  Task FetchTask();

 private:
  std::vector<std::thread> workers_;
  std::atomic<bool> should_stop_ = ATOMIC_VAR_INIT(false);
  std::queue<Task> tasks_;
  std::mutex task_mutex_;
  std::condition_variable task_cv_;
};

}  // namespace fun
