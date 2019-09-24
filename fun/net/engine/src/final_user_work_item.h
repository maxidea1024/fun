#pragma once

#include "local_event.h"

namespace fun {
namespace net {

class LocalEvent;
class FinalUserWorkItem_S;

/**
 * 내부 네트워크 스레드에서 실행되지 않고, 유저 스레드내에서
 * 실행되는 작업 타입들입니다.
 */
enum class FinalUserWorkItemType {
  /**
   * 내부 로컬 이벤트입니다.  유저 콜백등을 호출을 네트워크 스레드내에서
   * 할수 없으므로 (deadlock등의 위험이 있습니다.)
   *
   * 유저 큐에 넣어두었다가 유저 스레드내에서 처리합니다.
   */
  LocalEvent,

  /** 사용자 정의 RPC는 유저 스레드 내에서 실행되어야 합니다. */
  RPC,

  /** 사용자 정의 메시지는 유저 스레드 내에서 실행되어야 합니다. */
  FreeformMessage,

  /** 사용자 함수(lambda)는 유저 스레드 내에서 실행되어야 합니다. */
  UserTask,

  /** 마지막 엔트리입니다. (갯수의 의미로 사용 가능합니다.) */
  Last,
};


//
// Client side
//

class FinalUserWorkItem {
 public:
  FinalUserWorkItemType type;
  ReceivedMessage unsafe_message; // MessageType은 빠져있음(MessageType::RPC 같은거 제외)
  LocalEvent local_event;
  Function<void()> user_func;

  FinalUserWorkItem() : type(FinalUserWorkItemType::Last) {}

  FinalUserWorkItem(Function<void()> user_func)
    : user_func(user_func),
      type(FinalUserWorkItemType::UserTask) {}

  FinalUserWorkItem(ReceivedMessage& msg, FinalUserWorkItemType type)
    : unsafe_message(msg), type(type) {}

  inline FinalUserWorkItem(LocalEvent& local_event)
    : local_event(local_event),
      type(FinalUserWorkItemType::LocalEvent) {}

  // TODO 이걸 없애는 쪽으로 생각해보자....
  void From(FinalUserWorkItem_S& src, HostId host_id);
};


class FinalUserWorkItem_HasLockedDtor {
 public:
  inline FinalUserWorkItem& GetUWI() { return *uwi_; }

  FinalUserWorkItem_HasLockedDtor(CCriticalSection2* mutex);
  ~FinalUserWorkItem_HasLockedDtor();

 private:
  TypeCompatibleStorage<FinalUserWorkItem> storage_;
  FinalUserWorkItem* uwi_;
  CCriticalSection2* mutex_;
};

typedef List<FinalUserWorkItem> FinalUserWorkItemQueue;


//-----------------------------------------------------------------------------


//
// Server side
//

/**
 * 서버용 유저 태스크 아이템입니다.
 */
class FinalUserWorkItem_S {
 public:
  /**
   * 메시지로 생성합니다.
   */
  inline FinalUserWorkItem_S(MessageIn& msg, FinalUserWorkItemType type)
    : unsafe_message(msg), type(type) {}

  /**
   * 로컬 이벤트로 생성합니다.
   */
  inline FinalUserWorkItem_S(LocalEvent& local_event)
    : local_event(local_event),
      type(FinalUserWorkItemType::LocalEvent) {}

  /**
   * 사용자 함수(lambda)로 생성합니다.
   */
  inline FinalUserWorkItem_S(Function<void()> user_func)
    : user_func(user_func),
      type(FinalUserWorkItemType::UserTask) {}

  /** 유저 태스크 아이템 타입 */
  FinalUserWorkItemType type;
  /** 클라와 달리 서버측은 낮은 footprint 를 위해 이거만! */
  MessageIn unsafe_message;
  /** 로컬(내부) 이벤트 */
  LocalEvent local_event;
  /** 사용자 함수(lambda) */
  Function<void()> user_func;
};

/**
 * 서버용 유저 태스크 큐 입니다.
 */
typedef List<FinalUserWorkItem_S> FinalUserWorkItemQueue_S;

} // namespace net
} // namespace fun
