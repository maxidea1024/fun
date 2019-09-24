#include "CorePrivatePCH.h"
#include "fun/net/net.h"
#include "LanListener_C.h"
#include "LanClient.h"
#include "LanRemotePeer.h"
#include "LeanDynamicCast.h"

namespace fun {

LanListener_C::LanListener_C(LanClientImpl* owner)
  : owner_(fun_check_ptr(owner)), should_stop_(false) {}

LanListener_C::~LanListener_C() {
  StopListening();
}

void LanListener_C::StartListening() {
  owner_->LockMain_AssertIsLockedByCurrentThread();
  fun_check(owner_->server_socket_pool_.IsValid());

  StopListening();
  thread_.Reset(RunnableThread::Create(this, "LanListenerThread"));
}

void LanListener_C::StopListening() {
  if (!thread_) {
    return;
  }

  //TODO 구태여 락이 필요한가???
  {
    owner_->LockMain_AssertIsNotLockedByCurrentThread();

    CScopedLock2 owner_guard(owner_->GetMutex());
    owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

    should_stop_ = true;
  }

  thread_->Join();
  thread_.Reset();

  should_stop_ = false;
}

bool LanListener_C::IsListening() const {
  return thread_.IsValid();
}

uint32 LanListener_C::Run() {
  try {
    owner_->LockMain_AssertIsNotLockedByCurrentThread();

    UniquePtr<InternalSocket> new_socket;
    bool listening_works_ok = false;

    while (should_stop_ == false) {
      if (!new_socket && !listening_works_ok) {
        new_socket.Reset(NewAcceptPendedSocket());

        if (!new_socket) {
          listening_works_ok = true;
        }
      }

      CompletionStatus completion;
      if (owner_->tcp_accept_cp_->GetQueuedCompletionStatus(&completion, NetConfig::wait_completion_timeout_msec)) {
        if (new_socket) {
          CScopedLock2 owner_guard(owner_->GetMutex());
          owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

          InetAddress local_addr;
          InetAddress remote_addr;
          new_socket->FinalizeAcceptEx(owner_->tcp_listening_socket_.Get(), local_addr, remote_addr);

          if (remote_addr.IsUnicast()) {
            if (owner_->intra_logger_) {
              const String text = String::Format("TCP accept from address: %s", *remote_addr.ToString());
              owner_->intra_logger_->WriteLine(LogCategory::System, *text);
            }

            if (AcceptNewSocket(new_socket.Get(), remote_addr)) {
              new_socket.Detach();
            }
          } else {
            // 접속하는 중에 원격지에서 접속을 끊을 경우, 주소가 제대로 인식되지 않을 수 있습니다.
            // 이 경우 접속을 받아줘봐야 의미가 없으므로, 소켓을 drop합니다.
          }

          new_socket.Reset();
        }
      } else {
        if (!owner_->tcp_listening_socket_->IsClosed()) {
          fun_check(owner_->tcp_listening_socket_->AcceptExIssued());
        }
      }
    }

    if (owner_->intra_logger_) {
      const String text = String::Format("%s thread is terminated.", __FUNCTION__);
      owner_->intra_logger_->WriteLine(LogCategory::System, *text);
    }
  } catch (std::exception& e) {
    CatchThreadUnexpectedExit(__FUNCTION__, *String::Format("std.exception(%s)", UTF8_TO_TCHAR(e.what())));
  } //catch (_com_error& e) {
  //  CatchThreadUnexpectedExit(__FUNCTION__, *String::Format("_com_error(%s)", (const char*)e.Description()));
  //}
  //catch (void*) {
  //  CatchThreadUnexpectedExit(__FUNCTION__, "void*");
  //}

  return 0;
}

bool LanListener_C::AcceptNewSocket(InternalSocket* new_socket,
                                    const InetAddress& remote_addr) {
  CScopedLock2 owner_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  //TODO 어떤 시간을 사용하는게 좋을까?  real-time / snapped-time
  //const double absolute_time = owner_->Timer->GetAbsoluteTime();
  const double absolute_time = owner_->GetAbsoluteTime();

  // AcceptedInfo에 저장한다.
  // Accept 과정에서만 사용되며, Accept가 완료되면(실패하던 성공하던..)
  // 제거된다.
  // 성공 했을 경우에는 completion Context가 AcceptedInfo에서 RemotePeer로 교체 된다.
  // 즉, Accept 과정중에만 사용되는 임시 객체다.
  auto accepted_info = new AcceptedInfo(owner_);

  //@maxidea: 여기서 구지 락을 해야할까?  contention이 일어나는 상황도 아닌데 말이지...
  CScopedLock2 accepted_info_guard(accepted_info->GetMutex());

  // 타임아웃을 측정하기 위해서, Accept가 개시된 시점의 시각을 저장해 둔다.
  accepted_info->accepted_time_ = absolute_time;

  // socket 객체를 Attach 시켜준다.  Accept가 성공 여부에 상관 없이 완료되면 Detach되며,
  // Socket이 Detach된 AcceptedInfo 객체는 제거 된다.
  accepted_info->socket_.Reset(new_socket);

  // 기본 TCP 소켓 동작을 설정한다.
  NetUtil::SetTcpDefaultBehavior(new_socket);

  //@maxidea:
  //  LanClientImpl::PurgeTooOldUnmatureAcceptedPeers() 함수
  //  외에 제거하는 곳은 어디에도 없다.
  //  임의로 접속이 끊기는 등의 경우에는 어디에서 제거해야 할런지??
  //  -> Accept하는 곳에서 조건이 반대로 되어 있어서 접속을 받아주지 않았음!
  owner_->accepted_peers_.Add(remote_addr, accepted_info);

  // completion Context를 세팅해야 completion_port_ 객체가 제대로 동작함.
  accepted_info->socket_->SetCompletionContext((ICompletionContext*)accepted_info);

  //@maxidea:
  fun_check(LeanDynamicCastAcceptedInfo(accepted_info) != nullptr);

  // 첫번째 이슈를 던진다.  IssueRecv는 worker thread에서만 작동해야 하니까.
  owner_->net_thread_pool_->AssociateSocket(new_socket);
  accepted_info->IncreaseUseCount();
  owner_->net_thread_pool_->PostCompletionStatus(accepted_info->socket_.Get(), (UINT_PTR)IocpCustomValue::NewPeerAccepted);

  if (owner_->intra_logger_) {
    const String text = String::Format("New peer TCP socket is accepted.  accepted_info: %ph, tcp_socket: %s", accepted_info, *remote_addr.ToString());
    owner_->intra_logger_->WriteLine(LogCategory::System, *text);
  }

  return true;
}

InternalSocket* LanListener_C::NewAcceptPendedSocket() {
  while (true) {
    auto new_socket = owner_->server_socket_pool_->NewTcpSocket(owner_, owner_);
    if (new_socket == nullptr) {
      owner_->EnqueueError(ResultInfo::From(ResultCode::Unexpected, HostId_Server, "FATAL: Cannot create TCP socket for AcceptEx!"));
      return nullptr;
    }

    // 만들어진 소켓에 AcceptEx를 걸어준다.
    const SocketErrorCode socket_error = owner_->tcp_listening_socket_->AcceptEx(new_socket);
    switch (socket_error) {
      case SocketErrorCode::Ok:
        return new_socket;

      case SocketErrorCode::ConnectResetByRemote:
        if (should_stop_) {
          delete new_socket;
          return nullptr;
        } else {
          delete new_socket;
        }
        break;

      default:
        if (socket_error != SocketErrorCode::NotSocket) {
          const String text = String::Format("FATAL: AcceptEx failed with error code %d.  No more accept will be possible.", (int32)socket_error);
          owner_->EnqueueError(ResultInfo::From(ResultCode::Unexpected, owner_->GetLocalHostId(), text));
        }
        delete new_socket;
        return nullptr;
    }
  }
}

void LanListener_C::CatchThreadUnexpectedExit(const char* where, const char* reason) {
  if (owner_ && owner_->callbacks_) {
    owner_->LockMain_AssertIsNotLockedByCurrentThread();

    auto result_info = ResultInfo::From(ResultCode::Unexpected,
                                        HostId_Server,
                                        String::Format("(%s): Unexpected thread exit with (%s)", where, reason));
    owner_->ShowError_NOLOCK(result_info);
  }
}

} // namespace net
} // namespace fun
