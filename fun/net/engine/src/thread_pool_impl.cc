//@deprecated
#include "thread_pool_impl.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

ThreadPool2* ThreadPool2::New() { return new ThreadPoolImpl(); }

ThreadPoolImpl::ThreadPoolImpl()
    : stop_all_threads_(false), start_flag_(false), callbacks_(nullptr) {}

ThreadPoolImpl::~ThreadPoolImpl() { Stop(); }

void ThreadPoolImpl::Run() {
  SetThreadPriorityBoost(GetCurrentThread(), FALSE);

  // COM 초기화는 per-thread이므로, CRunnableThread내에서 처리해주는게
  // 바람직해보임. CCoInitializer COI; // Per thread.

  AssertIsNotLockedByCurrentThread();
  if (callbacks_) {
    callbacks_->OnThreadBegin();
  }

  // 아랫것들은 스레드별로 여기서만 쓰일 것들이다. 성능이 좋아지길 기대.
  //자꾸만 RecvMessageList에서 크래쉬가 발생함.  확인이 필요한 사항 같음.
  Array<IHostObject*> send_issue_pool;
  ReceivedMessageList recv_message_list;

  if (IsGqcsExSupported()) {
    Array<CompletionStatus> completions;
    completions.Resize(CompletionPort::GQCS_EX_REMOVED_COUNT);

    int32 dequeued_count = 0;
    while (stop_all_threads_ == false) {
      fun_check(NetConfig::wait_completion_timeout_msec <
                20);  // 20ms보다는 작아야 coalesce가 지나침으로 인한 랙이 예방.
      const bool succeeded = completion_port_->GetQueuedCompletionStatusEx(
          completions, dequeued_count, NetConfig::wait_completion_timeout_msec);
      if (succeeded && dequeued_count > 0) {
        for (int32 i = 0; i < dequeued_count; ++i) {
          completions[i].key->OnIoCompletion(send_issue_pool, recv_message_list,
                                             completions[i]);
        }
      }
    }
  } else {
    while (stop_all_threads_ == false) {
      CompletionStatus completion;
      if (completion_port_->GetQueuedCompletionStatus(
              &completion, NetConfig::wait_completion_timeout_msec)) {
        fun_check(
            NetConfig::wait_completion_timeout_msec <
            20);  // 20ms보다는 작아야 coalesce가 지나침으로 인한 랙이 예방.
        completion.key->OnIoCompletion(send_issue_pool, recv_message_list,
                                       completion);
      }
    }
  }

  AssertIsNotLockedByCurrentThread();
  if (callbacks_) {
    callbacks_->OnThreadEnd();
  }
}

void ThreadPoolImpl::SetCallbacks(IThreadPoolCallbacks* callbacks) {
  if (!thread_pool_worker_
           .IsEmpty()) {  // threadpool이 시작하기 전에만 설정가능. 일단 시작한
                          // 후에는 설정이 불가능.
    throw Exception(
        "Already async callback may occur.  Thread-pool start should have not "
        "been done before here.");
  }

  AssertIsNotLockedByCurrentThread();
  callbacks_ = callbacks;
}

void ThreadPoolImpl::Start(int32 thread_count) {
  if (thread_count <= 0) {
    throw Exception("Invalid thread count is specified.");
  }

  // 주의 : 워커 스레드 내에서 호출되지 않아야함.
  AssertIsNotLockedByCurrentThread();

  // Worker 스레드 풀 준비
  {
    CScopedLock2 guard(mutex_);

    // completion_port_ 준비.
    completion_port_.Reset(new CompletionPort(
        this, true, thread_count));  // AcceptEx때문에 무조건 true

    fun_check(stop_all_threads_ == false);
    fun_check(thread_pool_worker_.IsEmpty());

    for (int32 i = 0; i < thread_count; ++i) {
      const String thread_name = String::Format("ThreadPoolWorker#%d", i);
      thread_pool_worker_.Add(UniquePtr<RunnableThread>(
          RunnableThread::Create(this, *thread_name)));
      // thread_pool_worker_.Add(SharedPtr<RunnableThread>(RunnableThread::Create(this,
      // *thread_name)));
    }
  }

  // TODO initsynch event.

  // Set running flag.
  start_flag_ = true;
}

void ThreadPoolImpl::Stop() {
  // 주의 : 워커 스레드 내에서 호출되지 않아야함.
  // AssertIsNotLockedByCurrentThread();
  {
    CScopedLock2 guard(mutex_);

    // 종료하기 전에, Referer들을 등록 해재해 주어야함.
    if (!referers_.IsEmpty()) {
      throw Exception(
          "Referrer exist.  Thread-pool stop should have all referrers clear "
          "before here.");
    }

    // Request stop all threads.
    stop_all_threads_ = true;

    // Wait until all threads are stopped.
    //어짜피 RunnableThread 소멸자에서 대기했다가 종료하므로, 아래 코드는
    //불필요함.
    for (int32 i = 0; i < thread_pool_worker_.Count(); ++i) {
      thread_pool_worker_[i]->Join();
    }

    // Clean up all thread handles.
    thread_pool_worker_.Clear();

    stop_all_threads_ = false;

    // Close completion port object.
    completion_port_.Reset();
  }

  start_flag_ = false;
}

bool ThreadPoolImpl::RegistReferer(IThreadReferer* referer) {
  fun_check_ptr(referer);

  CScopedLock2 guard(mutex_);

  for (auto& it : referers_) {
    if (it == referer) {
      return false;
    }
  }

  referers_.Append(referer);
  return true;
}

void ThreadPoolImpl::UnregisterReferer(IThreadReferer* referer) {
  fun_check_ptr(referer);
  CScopedLock2 guard(mutex_);

  referers_.Remove(referer);
}

bool ThreadPoolImpl::IsCurrentThread() {
  CScopedLock2 guard(mutex_);

  if (thread_pool_worker_.IsEmpty()) {
    return false;
  }

  const uint32 current_tid = Thread::CurrentTid();
  for (int32 i = 0; i < thread_pool_worker_.Count(); ++i) {
    const uint32 tid = thread_pool_worker_[i]->GetTid();
    if (tid == current_tid) {
      return true;
    }
  }

  return false;
}

void ThreadPoolImpl::GetThreadInfos(Array<ThreadInfo>& out_info_list) {
  CScopedLock2 guard(mutex_);

  if (thread_pool_worker_.IsEmpty()) {
    out_info_list.Clear();  // just in case
    return;
  }

  out_info_list.Clear(thread_pool_worker_.Count());  // just in case

  ThreadInfo info;
  for (int32 i = 0; i < thread_pool_worker_.Count(); ++i) {
    info.thread_id = thread_pool_worker_[i]->GetTid();
    info.thread_handle =
        thread_pool_worker_[i]->GetHandle();  // TODO 핸들을 구지 가져가야하나??
    out_info_list.Add(info);
  }
}

bool ThreadPoolImpl::IsActive() { return start_flag_; }

// TODO 사용 목적이 부족하다면, 제거하도록 하자.
void ThreadPoolImpl::OnCompletionPortWarning(CompletionPort* completion_port,
                                             const char* msg) {
  // nothing to do... yet?
}

void ThreadPoolImpl::AssociateSocket(InternalSocket* socket) {
  completion_port_->AssociateSocket(socket);
}

void ThreadPoolImpl::PostCompletionStatus(ICompletionKey* key,
                                          uintptr_t custom_value) {
  completion_port_->PostCompletionStatus(key, custom_value);
}

}  // namespace net
}  // namespace fun
