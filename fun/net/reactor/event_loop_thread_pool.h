#pragma once

#include "fun/net/net.h"
#include "fun/base/container/array.h"
#include "fun/base/string/string.h"
#include "fun/base/function.h"

namespace fun {
namespace net {

/**
 * 각각의 이벤트 루프를 가지고 있는 스레드들의 Pool
 */
class EventLoopThreadPool : Noncopyable {
 public:
  typedef Function<void (EventLoop*)> ThreadInitCallback;

  /**
   * EventLoopThreadPool 객체를 초기화합니다.
   *
   * \param base_loop 기본 이벤트 루프입니다.
   * \param name 이벤트 루프 스레드풀 이름입니다.
   */
  EventLoopThreadPool(EventLoop* base_loop, const String& name);

  /**
   * EventLoopThreadPool 객체를 파괴합니다.
   */
  ~EventLoopThreadPool();

  /**
   * 스레드 갯수를 지정합니다. 시작하기 전에 설정해주어야합니다.
   */
  void SetThreadCount(int32 thread_count);

  /**
   * 이벤트 루프 스레드풀을 시작합니다.
   */
  void Start(const ThreadInitCallback& cb = ThreadInitCallback());

  /**
   * round-robin
   */
  EventLoop* GetNextLoop();

  EventLoop* GetLoopForHash(size_t hash);

  /**
   * TODO 반듯이 사본으로 접근해야하는지??
   */
  Array<EventLoop*> GetAllLoops() const;

  bool IsStarted() const { return started_; }

  const String& GetName() const { return name_; }

 private:
  EventLoop* base_loop_;
  String name_;
  bool started_;
  int32 thread_count_;
  int32 next_;
  Array<EventLoopThread> threads_;
  Array<EventLoop*> loops_;
};

} // namespace net
} // namespace fun
