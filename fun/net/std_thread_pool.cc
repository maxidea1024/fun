// TODO 제거 대상.

// TODO 제거하던지, core쪽으로 빼주자...
// TODO 제거하던지, core쪽으로 빼주자...
// TODO 제거하던지, core쪽으로 빼주자...
// TODO 제거하던지, core쪽으로 빼주자...
// TODO 제거하던지, core쪽으로 빼주자...
// TODO 제거하던지, core쪽으로 빼주자...
// TODO 제거하던지, core쪽으로 빼주자...
// TODO 제거하던지, core쪽으로 빼주자...
#include "CorePrivatePCH.h"
#include "Net/Core/StdThreadPool.h"

namespace fun {

StdThreadPool::StdThreadPool(int32 thread_count) {
  for (int32 i = 0; i < thread_count; ++i) {
    workers_.push_back(std::thread(std::bind(&StdThreadPool::Run, this)));
  }
}

StdThreadPool::~StdThreadPool() { Stop(); }

void StdThreadPool::AddTask(const Task& task) {
  std::lock_guard<std::mutex> lock(task_mutex_);
  tasks_.push(task);
  task_cv_.notify_all();
}

void StdThreadPool::AddTask(const Task& task, const String& tag) {
  // TODO 특정 스레드에서 작동하도록 함.
  //같은 스레드를 점유해도 문제될건 없음.
  // task의 직렬화 즉, 처리 순서만 지켜지면 될뿐...
  //스레드를 지정해서 처리할 경우, 한스레드에만 처리가 몰릴 수 있는데
  //이 부분에 대해선 명확한 정책이 필요할 수 있겠음.
  //할당을 하되 유후 스레드를 사용하고, 이미 같은 소속(태그가 같은)의 task가
  //수행중이라면 큐에 넣어서 대기하도록 해야함. 아니면, 워커마다 별도의 스레드를
  //두어야할듯 싶음..

  // TODO
  fun_check(0);
}

StdThreadPool& StdThreadPool::operator<<(const Task& task) {
  AddTask(task);
  return *this;
}

void StdThreadPool::Stop() {
  if (should_stop_) {
    return;
  }

  should_stop_ = true;
  task_cv_.notify_all();

  for (auto& worker : workers_) {
    worker.join();
  }
  workers_.clear();
}

bool StdThreadPool::IsRunning() const { return !should_stop_; }

void StdThreadPool::Run() {
  while (!should_stop_) {
    Task task = FetchTask();
    if (task) {
      task();
    }
  }
}

StdThreadPool::Task StdThreadPool::FetchTask() {
  std::unique_lock<std::mutex> lock(task_mutex_);
  task_cv_.wait(lock, [&] { return should_stop_ || !tasks_.empty(); });

  if (tasks_.empty()) {
    return nullptr;
  }

  Task task = std::move(tasks_.front());
  tasks_.pop();
  return task;
}

}  // namespace fun
