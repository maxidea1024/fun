#include "fun/base/timer.h"
#include "fun/base/timestamp.h"
#include "fun/base/timespan.h"
#include "fun/base/uuid.h"
#include "fun/base/exception.h"

#undef StartService //winsv.h

#include <thread>
#include <condition_variable>
#include <map>

//NOTE 우선은 std::의 것들을 사용하고 차후에 자체제작한걸로 대체 하도록 하자.

namespace fun {

//
// Timer::Impl
//

//여기서는 스케줄링만 하고, thread-pool에서 task를 수행하는 형태로 구현하도록 하자.

class Timer::Impl {
 public:
  //TODO new/delete로 처리하지 않고, 풀에서 index 형태로 접근하는것도 좋을듯 싶음.
  struct Task {
    /** Timer task id */
    TimerTaskId id;
    /** tag */
    String tag;
    /** Function object */
    TimerFunction func;
    /** next fire time */
    Timestamp next;
    /** Last expired time */
    Timestamp prev;
    /** Fired count */
    int32 fired_count;
    /** period */
    Timespan period; //0보다 클 경우에만, repeat
    bool fixed_rate;
  };

  //Array<Task> task_pool_;
  //Array<int32> avail_task_indices_;
  //Task* AllocateTask();
  //void FreeTask(Task* task_to_free);

  std::thread thread_;
  std::multimap<Timestamp, Task*> task_queue_;
  std::condition_variable new_task_scheduled_;
  mutable std::mutex task_queue_mutex_;
  int32 num_threads_service_queue_;
  bool stop_requested_;
  bool stop_when_empty_;
  bool started_;
  TimerTaskId next_task_id_;
  String name_;

  Timer* owner_;

  Impl(Timer* owner)
    : owner_(owner),
      next_task_id_(1),
      //name_(Uuid::NewSecuredRandomUuid().ToString(UuidFormat::Digits)), //너무느림
      name_(Uuid::NewUuid().ToString(UuidFormat::Digits)),
      num_threads_service_queue_(0),
      stop_requested_(false),
      stop_when_empty_(false),
      started_(false) {}

  Impl(Timer* owner, const String& name)
    : owner_(owner),
      next_task_id_(1),
      name_(name),
      num_threads_service_queue_(0),
      stop_requested_(false),
      stop_when_empty_(false),
      started_(false) {}

  ~Impl() {
    fun_check(num_threads_service_queue_ == 0);

    Stop(true);
  }

  void Start() {
    started_ = true;
    //thread_ = std::move(std::thread(&ServiceQueue));
    thread_ = std::thread([this]() { ServiceQueue(); });
  }

  void Stop(bool drain) {
    if (!started_) {
      return;
    }

    {
      std::unique_lock<std::mutex> guard(task_queue_mutex_);

      started_ = false;

      if (drain) {
        stop_when_empty_ = true; //완전하게 큐가 빌때까지 대기후 종료. (bWaitQueueIsEmptyAndStop)
      } else {
        stop_requested_ = true;
      }
    }

    new_task_scheduled_.notify_all();

    thread_.join();
  }

  TimerTaskId Schedule(const Timestamp& time, const TimerFunction& func, const String& tag) {
    return ScheduleInternal(time, Timespan::Zero, false, func, tag);
  }

  TimerTaskId Schedule(const Timespan& delay, const TimerFunction& func, const String& tag) {
    return ScheduleInternal(Timestamp::Now() + delay, Timespan::Zero, false, func, tag);
  }

  TimerTaskId Schedule(const Timestamp& first_time, const Timespan& period, const TimerFunction& func, const String& tag) {
    return ScheduleInternal(first_time, period, false, func, tag);
  }

  TimerTaskId Schedule(const Timespan& delay, const Timespan& period, const TimerFunction& func, const String& tag) {
    return ScheduleInternal(Timestamp::Now() + delay, period, false, func, tag);
  }

  TimerTaskId ScheduleAtFixedRate(const Timestamp& first_time, const Timespan& period, const TimerFunction& func, const String& tag) {
    return ScheduleInternal(first_time, period, true, func, tag);
  }

  TimerTaskId ScheduleAtFixedRate(const Timespan& delay, const Timespan& period, const TimerFunction& func, const String& tag) {
    return ScheduleInternal(Timestamp::Now() + delay, period, true, func, tag);
  }

  TimerTaskId ScheduleInternal(const Timestamp& first_time, const Timespan& period, bool fixed_rate, const TimerFunction& func, const String& tag) {
    std::unique_lock<std::mutex> guard(task_queue_mutex_);

    Task* task = new Task();
    task->id = next_task_id_++;
    task->func = func;
    task->next = first_time;
    task->prev = Timestamp::TIMEVAL_MAX;
    task->period = period;
    task->fired_count = 0;
    task->fixed_rate = fixed_rate;
    task->tag = tag;

    task_queue_.insert(std::make_pair(task->next, task)); // ordered-insertion

    new_task_scheduled_.notify_one();

    return task->id;
  }

  bool IsValid(TimerTaskId timer_task_id) {
    std::unique_lock<std::mutex> guard(task_queue_mutex_);
    return FindTask_NOLOCK(timer_task_id) != nullptr;
  }

  bool Cancel(TimerTaskId timer_task_id) {
    std::unique_lock<std::mutex> guard(task_queue_mutex_);
    for (auto it = task_queue_.begin(); it != task_queue_.end(); ++it) {
      if (it->second->id == timer_task_id) {
        delete it->second;
        task_queue_.erase(it);
        return true;
      }
    }
    return false;
  }

  void Cancel() {
    std::unique_lock<std::mutex> guard(task_queue_mutex_);

    for (auto& pair : task_queue_) {
      delete pair.second;
    }
    task_queue_.clear();
  }

  bool SetTag(TimerTaskId timer_task_id, const String& tag) {
    std::unique_lock<std::mutex> guard(task_queue_mutex_);

    Task* task = FindTask_NOLOCK(timer_task_id);
    if (task) {
      task->tag = tag;
      return true;
    }
    return false;
  }

  Task* FindTask_NOLOCK(TimerTaskId timer_task_id) {
    for (auto& pair : task_queue_) {
      if (pair.second->id == timer_task_id) {
        return pair.second;
      }
    }
    return nullptr;
  }

  void ServiceQueue() {
    std::unique_lock<std::mutex> guard(task_queue_mutex_);

    ++num_threads_service_queue_;
    stop_requested_ = false;
    stop_when_empty_ = false;

    // task_queue_mutex_ is locked throughout this loop EXCEPT
    // when the thread is waiting or when the user's function
    // is calleed.
    while (!stop_requested_ && !(stop_when_empty_ && task_queue_.empty())) {
      try {
        // 새로운 잡이 있을때까지 대기함.
        while (!stop_requested_ && !stop_when_empty_ && task_queue_.empty()) {
          // Wait until there is something to do.
          new_task_scheduled_.wait(guard);
        }

        if (stop_requested_ || task_queue_.empty()) {
          continue;
        }

        // Wait until either there is a new task, or until
        // the time of the first item on the queue:

        //TODO 특정시간동안 대기를 해야하는데 말이지??
        // 첫번째로 실행할 스케줄 타임까지 대기..
        const auto time_left_to_run_task = task_queue_.begin()->first - Timestamp::Now();
        while (!stop_when_empty_ && !task_queue_.empty() &&
            new_task_scheduled_.wait_for(guard, std::chrono::milliseconds((long)time_left_to_run_task.TotalMilliseconds())) != std::cv_status::timeout
          )
        {
          // Keep waiting until timeout.
        }

        // 종료가 요청된 경우에는, 태스크를 outstanding으로 만들지 않고, 바로 실행 루프를 빠져나감.
        if (stop_requested_) {
          break;
        }

        // If there are multiple threads, the queue can empty while we're waiting
        // (another thread may service the task we were waiting on).
        if (task_queue_.empty()) {
          continue;
        }


        const Timestamp fired_time = Timestamp::Now();

        Task* task_to_execute = task_queue_.begin()->second;
        task_queue_.erase(task_queue_.begin());

        task_to_execute->prev = fired_time;
        task_to_execute->fired_count++;

        // 유저 함수 수행이 다소 오랜시간 걸릴 수 있으므로, lock을 해제한 상태에서 시도함.
        // 만약, 스레드풀에서 실행하는 형태로 변경한다면, lock을 해제할 필요는 없어보임.

        guard.unlock();

        TimerTaskContext task_context;
        task_context.timer = owner_;
        task_context.id = task_to_execute->id;
        task_context.fired_time = task_to_execute->prev;
        task_context.fired_count = task_to_execute->fired_count;
        task_context.tag = task_to_execute->tag;

        bool error_occurred = false;

        //TODO thread-pool에서 수행하도록 변경해야함.
        //Task 자체가 무거울 경우에는 다른 Task의 스케줄을 방해하게 되므로 문제가 될 수 있음.

        //스레드풀링 처리시 시리얼라이징 처리시 채널을 유지시킬 수 있도록 하자.
        //TLS를 할당해서, 현재그룹을 설정하는 형태로 하던지...
        try {
          //TODO
          //만약 스레드풀에 넘기는 경우로 변경할 경우에는, 컨텍스트 유지에 조금더 신경을 써야함.
          //왜냐하면, 언제 완료될지 모르므로...
          //컨텍스트 해제를 스레드풀에서 태스크가 완료되는 시점에서 처리해할듯...
          task_to_execute->func(task_context); //참조로 넘기는게 좋을듯 싶다. 안에서 변경할 수 있도록...
        } catch (Exception& e) {
          //TODO handling exception...
          //TODO
          //fun_log(LogCore, Error, "exception cought: {0}", e.GetDisplayText());
          error_occurred = true;
        }

        guard.lock();

        // Once or Repeat

        const bool should_repeat = !error_occurred && (!stop_requested_ && !stop_when_empty_ && task_to_execute->period != Timespan::Zero);
        if (should_repeat) {
          if (task_to_execute->fixed_rate) {
            // fixed-rate
            // 처리 직전의 시간에서 period만큼 이후에 재개할 것이므로, 처리 간격이 일정함.
            //
            // 콜카운터 x period로 부여하는게 좋을듯도?
            task_to_execute->next = fired_time + task_to_execute->period;
          } else {
            // fixed-delay
            // 완료된 이후에 period만큼후에 재개할 것이므로, 처리후의 간격이 일정함.
            task_to_execute->next = Timestamp::Now() + task_to_execute->period;
          }

          // ordered-map이므로, 다시 추가 해주는 것으로 re-scheduling이 됨.
          task_queue_.insert(std::make_pair(task_to_execute->next, task_to_execute));
        } else {
          //TODO 풀링으로 처리하도록 하자.
          delete task_to_execute;
        }
      } catch (...) {
        //TODO 모든 태스크를 취소하는 형태로 하는게 어떨런지??
        --num_threads_service_queue_;

        // 예외를 외부로 전파하면 어떻게 되는걸까??
        // 여기서 graceful하게 제어를 해주어야할터인데?
        throw;
      }
    }

    --num_threads_service_queue_;
  }
};


//
// Timer
//

Timer::Timer() : impl_(new Impl(this)) {}

Timer::Timer(const String& name) : impl_(new Impl(this, name)) {}

Timer::~Timer() {
  delete impl_;
}

void Timer::Start() {
  impl_->Start();
}

void Timer::Stop(bool drain) {
  impl_->Stop(drain);
}

TimerTaskId Timer::Schedule(const Timestamp& time, const TimerFunction& func, const String& tag) {
  return impl_->Schedule(time, func, tag);
}

TimerTaskId Timer::Schedule(const Timespan& delay, const TimerFunction& func, const String& tag) {
  return impl_->Schedule(delay, func, tag);
}

TimerTaskId Timer::Schedule(const Timestamp& first_time, const Timespan& period, const TimerFunction& func, const String& tag) {
  return impl_->Schedule(first_time, period, func, tag);
}

TimerTaskId Timer::Schedule(const Timespan& delay, const Timespan& period, const TimerFunction& func, const String& tag) {
  return impl_->Schedule(delay, period, func, tag);
}

TimerTaskId Timer::ScheduleAtFixedRate(const Timestamp& first_time, const Timespan& period, const TimerFunction& func, const String& tag) {
  return impl_->ScheduleAtFixedRate(first_time, period, func, tag);
}

TimerTaskId Timer::ScheduleAtFixedRate(const Timespan& delay, const Timespan& period, const TimerFunction& func, const String& tag) {
  return impl_->ScheduleAtFixedRate(delay, period, func, tag);
}

bool Timer::IsValid(TimerTaskId timer_task_id) {
  return impl_->IsValid(timer_task_id);
}

bool Timer::Cancel(TimerTaskId timer_task_id) {
  return impl_->Cancel(timer_task_id);
}

void Timer::Cancel() {
  impl_->Cancel();
}

bool Timer::SetTag(TimerTaskId timer_task_id, const String& tag) {
  return impl_->SetTag(timer_task_id, tag);
}



//
// Timer API functions
//

namespace TimerService {

//서비스 시작/종료시에 초기화/종료 루틴이 호출될 수 있는 체계를 만들어 주어야할듯함.
//명시적으로 해도 상관 없을듯 싶기는 한데..

static Timer* default_timer = nullptr;

void StartService() {
  fun_check(default_timer == nullptr);
  default_timer = new Timer("__default_timer__");
  default_timer->Start();
}

void StopService() {
  fun_check(default_timer != nullptr);
  default_timer->Stop(true);
  delete default_timer;
  default_timer = nullptr;
}

TimerTaskId Schedule(const Timestamp& time, const TimerFunction& func, const String& tag) {
  return default_timer->Schedule(time, func, tag);
}

TimerTaskId Schedule(const Timespan& delay, const TimerFunction& func, const String& tag) {
  return default_timer->Schedule(delay, func, tag);
}

TimerTaskId Schedule(const Timestamp& first_time, const Timespan& period, const TimerFunction& func, const String& tag) {
  return default_timer->Schedule(first_time, period, func, tag);
}

TimerTaskId Schedule(const Timespan& delay, const Timespan& period, const TimerFunction& func, const String& tag) {
  return default_timer->Schedule(delay, period, func, tag);
}

TimerTaskId ScheduleAtFixedRate(const Timestamp& first_time, const Timespan& period, const TimerFunction& func, const String& tag) {
  return default_timer->ScheduleAtFixedRate(first_time, period, func, tag);
}

TimerTaskId ScheduleAtFixedRate(const Timespan& delay, const Timespan& period, const TimerFunction& func, const String& tag) {
  return default_timer->ScheduleAtFixedRate(delay, period, func, tag);
}

bool IsValid(TimerTaskId timer_task_id) {
  return default_timer->IsValid(timer_task_id);
}

bool Cancel(TimerTaskId timer_task_id) {
  return default_timer->Cancel(timer_task_id);
}

void Cancel() {
  default_timer->Cancel();
}

bool SetTag(TimerTaskId timer_task_id, const String& tag) {
  return default_timer->SetTag(timer_task_id, tag);
}

} // end of namespace TimerService

} // namespace fun
