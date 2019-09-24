#include "fun/net/socket_io_service.h"
#include "fun/net/self_pipe.h"

namespace fun {

SharedPtr<SocketIoService> SocketIoService::default_io_service_;

SharedPtr<SocketIoService>& SocketIoService::GetDefaultIoService() {
  //if (default_io_service_; == nullptr)
  if (!default_io_service_;.IsValid()) {
    default_io_service_; = MakeShareable<SocketIoService>(new SocketIoService);
  }
  return default_io_service_;;
}

void SocketIoService::SetDefaultIoService(const SharedPtr<SocketIoService>& io_service) {
  default_io_service_; = io_service;
}

SocketIoService::SocketIoService()
  : should_stop_(0),
  , notifier_(new SelfPipe),
  , thread_pool_(4) {
  poll_thread_ = MakeShareable(new Thread(this, "SocketIoService::poll_thread_"));
}

SocketIoService::~SocketIoService() {
  should_stop_.Set(1);

  WakeUpPoller();

  poll_thread_->Join();
  poll_thread_ = nullptr;

  delete notifier_;

  //내부적으로 처리가 완료될때까지 대기를 해주어야할텐데..
  //뭔가 구조가 좋지 않다...
  //delete thread_pool_;

  thread_pool_.Stop();
}

//연결 요청에 대한 결과를 알수가 없네 흠...
//클라이언트 연결에 대한 요청에 대해서는, read시점에서 오류가 발생할 것이므로 read시점에 오류가 발생하면
//연결에 실패한걸로 처리하면 됨?
//아니면 error-set을 검출해야하나??
void SocketIoService::Associate(const Socket& socket,
                                const Callback& read_cb,
                                const Callback& write_cb) {
  std::unique_lock<std::mutex> lock(associated_sockets_mutex_);

  //auto& info = associated_sockets_[socket.GetSocketHandle()];
  auto& info = associated_sockets_.FindOrAdd(socket.GetSocketHandle());

  info.read_cb = read_cb;
  info.write_cb = write_cb;
  info.marked_for_unassociate = false;
  info.is_executing_read_cb = false;
  info.is_executing_write_cb = false;

  // wakeup poller
  WakeUpPoller();
}

void SocketIoService::Unassociate(const Socket& socket, bool wait_for_removal) {
  std::unique_lock<std::mutex> lock(associated_sockets_mutex_);

  auto associated_socket = associated_sockets_.Find(socket.GetSocketHandle());
  if (associated_socket == nullptr) {
    return;
  }

  // 스레드 풀에서 백그라운드로 처리중이라면, 종료대기 표시만 해두고 대기해야함.
  if (associated_socket->is_executing_read_cb_ || associated_socket->is_executing_write_cb) {
    associated_socket->marked_for_unassociate = true;
  } else { // 스레드 풀에서 실행중인 동작이 없다면, 바로 목록에서 제거.
    associated_sockets_.Remove(socket.GetSocketHandle());
    wait_for_removal_cv_.notify_all();
  }

  WakeUpPoller();

  if (wait_for_removal) {
    //Recursive 이슈가 있으므로, WairForRemoval 함수를 호출해서 처리하지 않고,
    //코드를 직접 넣어주어서 처리한다.

    //WaitForRemoval(socket);

    // 목록에서 안전하게 제거 될때까지 대기.
    wait_for_removal_cv_.wait(lock, [&]() {
      return !associated_sockets_.Contains(socket.GetSocketHandle());
    });
  }
}

void SocketIoService::SetReadCallback(const Socket& socket, const Callback& read_cb) {
  std::unique_lock<std::mutex> lock(associated_sockets_mutex_);

  auto& info = associated_sockets_[socket.GetSocketHandle()];
  info.read_cb = read_cb;

  WakeUpPoller();
}

void SocketIoService::SetWriteCallback(const Socket& socket, const Callback& write_cb) {
  std::unique_lock<std::mutex> lock(associated_sockets_mutex_);

  auto& info = associated_sockets_[socket.GetSocketHandle()];
  info.write_cb = write_cb;

  WakeUpPoller();
}

void SocketIoService::WaitForRemoval(const Socket& socket) {
  //WARNING recursive하면 안됨.
  std::unique_lock<std::mutex> lock(associated_sockets_mutex_);

  //목록에서 제거될때까지 대기함.
  wait_for_removal_cv_.wait(lock, [&]() {
    return !associated_sockets_.Contains(socket.GetSocketHandle());
  });
}

void SocketIoService::WakeupPoller() {
  notifier_->Notify();
}

void SocketIoService::Poll() {
  while (!should_stop_.GetValue()) {
    const int32 nfds = InitPollFDsInfo();

    struct timeval* timeout_ptr = nullptr;
    //TODO timeout

    if (select(nfds, &read_set_, &write_set_, nullptr, timeout_ptr) > 0) {
      ProcessEvents();
    } else {
      //TODO error handling
      //"poll woke up, but nothing to process"
    }
  }
}

int32 SocketIoService::InitPollFDsInfo() {
  std::unique_lock<std::mutex> lock(associated_sockets_mutex_);

  polled_fds_.Reset(); // Clear()는 capacity를 리셋하므로, Reset()으로..

  FD_ZERO(&read_set_);
  FD_ZERO(&write_set_);

  int32 max_fd = notifier_->GetReadFD();

  FD_SET(notifier_->GetReadFD(), &read_set_);
  polled_fds_.Add(notifier_->GetReadFD());

  for (const auto& pair : associated_sockets_) {
    const auto& fd = pair.key;
    const auto& info = pair.value;

    const bool should_read = info.read_cb && !info.is_executing_read_cb_;
    if (should_read) {
      FD_SET(fd, &read_set_);
    }

    const bool should_write = info.write_cb && !info.is_executing_write_cb;
    if (should_write) {
      FD_SET(fd, &write_set_);
    }

    if (should_read || should_write || info.marked_for_unassociate) {
      polled_fds_.Add(fd);
    }

    if ((should_read || should_write) && int32(fd) > max_fd) {
      max_fd = fd;
    }
  }

  return max_fd + 1;
}

void SocketIoService::ProcessEvents() {
  for (const auto& fd : polled_fds_) {
    // poller를 깨우기 위한 용도로 사용된 특별한 경우이므로,
    // 그냥 스킵함.
    if (fd == notifier_->GetReadFD() && FD_ISSET(fd, &read_set_)) {
      notifier_->ClearBuffer();
      continue;
    }


    // epoll 일 경우하면, user data를 넘길 수 있으므로, context를 찾는 과정이 생략
    // 가능할것임.
    auto associated_socket = associated_sockets_.Find(fd);
    if (associated_socket == nullptr) {
      continue;
    }

    // 읽기 가능 신호가 감지됨.
    // 단, read callback이 지정된 상태에서 read callback이 실행중이 아닌
    // 경우에만 호출함.
    if (FD_ISSET(fd, &read_set_) &&
        associated_socket->read_cb &&
        !associated_socket->is_executing_read_cb_) {
      ProcessReadEvent(fd, *associated_socket);
    }

    // 쓰기 가능 신호가 감지됨.
    // 단, write callback이 지정된 상태에서 write callback이 실행중이 아닌
    // 경우에만 호출함.
    if (FD_ISSET(fd, &write_set_) &&
        associated_socket->write_cb &&
        !associated_socket->is_executing_write_cb) {
      ProcessWriteEvent(fd, *associated_socket);
    }

    // wakeup 디스크립터에 작업을 밀어넣을수도 있을듯 싶은데...??

    // 종료대기중임.
    // 처리중인 작업이 없을 경우에만 collection에서 제거함.
    if (associated_socket->marked_for_unassociate &&
        !associated_socket->is_executing_read_cb_ &&
        !associated_socket->is_executing_write_cb) {
      // collection에서 제거.
      associated_sockets_.Remove(fd);

      // 이 소켓이 끝나기를 기다리는 스레드들에게 완료 신호를 보냄.
      wait_for_removal_cv_.notify_all();
    }
  }
}

void SocketIoService::ProcessReadEvent(SOCKET fd, AssociatedSocket& socket) {
  auto read_cb = socket.read_cb;

  socket.is_executing_read_cb_ = true;

  thread_pool_.AddTask([=] {
      read_cb(fd);

      std::lock_guard<std::mutex> lock(associated_sockets_mutex_);

      auto associated_socket = associated_sockets_.Find(fd);
      if (associated_socket) {
        associated_socket->is_executing_read_cb_ = false;

        // 종료 대기중인 상태에서
        // 이미 읽기 동작은 완료 했으니, 쓰기 동작을 수행중이 아닌 경우에는
        // 목록에서 안전하게 제거.
        if (associated_socket->marked_for_unassociate &&
            !associated_socket->is_executing_write_cb) {
          // Removes from collection.
          associated_sockets_.Remove(fd);

          // Notify to wait for removal.
          wait_for_removal_cv_.notify_all();
        }

        WakeUpPoller();
      }
    });
}

void SocketIoService::ProcessWriteEvent(SOCKET fd, AssociatedSocket& socket) {
  auto write_cb = socket.write_cb;

  socket.is_executing_write_cb = true;

  thread_pool_.AddTask([=] {
      write_cb(fd);

      std::lock_guard<std::mutex> lock(associated_sockets_mutex_);

      auto associated_socket = associated_sockets_.Find(fd);
      if (associated_socket) {
        associated_socket->is_executing_write_cb = false;

        // 종료 대기중인 상태에서
        // 이미 쓰기 동작은 완료 했으니, 읽기 동작을 수행중이 아닌 경우에는
        // 목록에서 안전하게 제거.
        if (associated_socket->marked_for_unassociate &&
            !associated_socket->is_executing_read_cb_) {
          associated_sockets_.Remove(fd);
          wait_for_removal_cv_.notify_all();
        }

        WakeUpPoller();
      }
    });
}

} // namespace fun
