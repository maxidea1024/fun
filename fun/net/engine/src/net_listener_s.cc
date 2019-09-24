#include "fun/net/net.h"
#include "NetListener_S.h"
#include "NetServer.h"

namespace fun {
namespace net {

NetListener_S::NetListener_S(NetServerImpl* owner)
  : owner_(fun_check_ptr(owner_)), should_stop_(false) {}

NetListener_S::~NetListener_S() {
  StopListening();
}

void NetListener_S::StartListening() {
  fun_check_ptr(owner_->server_socket_pool_);

  StopListening();
  thread_.Reset(RunnableThread::Create(this, "NetListenerThread"));
}

void NetListener_S::StopListening() {
  if (!thread_) {
    return;
  }

  //TODO 락을 구태여 걸어야할까??
  {
    CScopedLock2 owner_guard(owner_->GetMutex());
    owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

    should_stop_ = true;
  }

  thread_->Join();
  thread_.Reset();

  should_stop_ = false;
}

bool NetListener_S::IsListening() const {
  return thread_.IsValid();
}

InternalSocket* NetListener_S::NewAcceptPendedSocket() {
  while (true) {
    auto new_socket = owner_->server_socket_pool_->NewTcpSocket(owner_, owner_);
    if (new_socket == nullptr) {
      owner_->EnqueueError(ResultInfo::From(ResultCode::Unexpected, HostId_Server, "FATAL: Cannot create TCP socket for AcceptEx."));
      return nullptr;
    }

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
          const String comment = String::Format("FATAL: AcceptEx failed with error code %d.  No more accept will be possible.", (int32)socket_error);
          owner_->EnqueueError(ResultInfo::From(ResultCode::Unexpected, HostId_Server, comment));
        }
        delete new_socket;
        return nullptr;
    }
  }
}

uint32 NetListener_S::Run() {
  try {
    owner_->AssertIsNotLockedByCurrentThread();

    UniquePtr<InternalSocket> new_socket;
    bool listening_works_ok = true;

    while (should_stop_ == false) {
      if (!new_socket && listening_works_ok) {
        new_socket.Reset(NewAcceptPendedSocket());

        if (!new_socket) {
          listening_works_ok = false;
        }
      }

      CompletionStatus completion;
      if (owner_->tcp_accept_cp_->GetQueuedCompletionStatus(&completion, NetConfig::wait_completion_timeout_msec)) {
        if (new_socket) {
          InetAddress local_addr, remote_addr;
          new_socket->FinalizeAcceptEx(owner_->tcp_listening_socket_.Get(), local_addr, remote_addr);

          if (remote_addr.IsUnicast()) {
            if (AcceptNewSocket(new_socket.Get(), remote_addr)) {
              new_socket.Detach();
            }
          } else {
            // 보통 이경우는 접속을 하자마자, 원격지에서 끊어진 경우임.
            // 그러므로, 접속을 받아주지 말고 소켓을 닫아주는것으로 처리합니다.
            // 실제로 로깅 조차도 불필요합니다. 괜시리 지저분해지기만 할뿐...
            LOG(LogNetEngine, Warning, "Invalid accepted socket address: %s", *remote_addr.ToString());
          }

          new_socket.Reset();
        } else {
          owner_->EnqueueError(ResultInfo::From(ResultCode::Unexpected, HostId_None, "AcceptEx GetQueuedCompletionStatus but socket null"));
        }
      } else {
        if (!owner_->tcp_listening_socket_->IsClosed()) {
          fun_check(owner_->tcp_listening_socket_->AcceptExIssued());
        }
      }
    }
  } catch (Exception& e) {
    CatchThreadUnexpectedExit(__FUNCTION__, *String::Format("Exception(%s)", *e.Message()));
  } catch (std::exception& e) {
    CatchThreadUnexpectedExit(__FUNCTION__, *String::Format("std.exception(%s)", UTF8_TO_TCHAR(e.what())));
  } //catch (_com_error& e) {
  //  CatchThreadUnexpectedExit(__FUNCTION__, *String::Format("_com_error(%s)", (const char*)e.Description()));
  //} //catch (void*) {
  //  CatchThreadUnexpectedExit(__FUNCTION__, "void*");
  //}

  return 0;
}

bool NetListener_S::AcceptNewSocket(InternalSocket* new_socket, const InetAddress& remote_addr) {
  fun_check_ptr(new_socket);
  fun_check(remote_addr.IsUnicast());

  CScopedLock2 owner_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  auto rc = new RemoteClient_S(owner_, new_socket, remote_addr);

  // NetServerImpl::ProcessOnClientJoinApproved(...) 에서 HostId가 할당 된다.
  // 즉, 기본 연결 절차가 완료된 이후에 정상적인 연결이라고 인정하고,
  // HostId를 부여하는 것이다.
  fun_check(rc->GetHostId() == HostId_None);

  // 주의: 여기까지 실행됐으면 이제 NewSocket의 소유권은 이양.
  // 생성하자마자 바로 여기에 넣어야 나중에 제거해도 안전하게 제거됨
  owner_->remote_client_instances_.Add(rc);

  // Add it to list (아직 인증된 상태가 아니므로, 후보 목록에 등록해둡니다.)
  owner_->candidate_remote_clients_.Add(remote_addr, rc);

  // 첫번째 이슈를 던진다. IssueRecv는 worker thread에서만 작동해야 하니까.
  // completion context를 세팅해야 completion_port_ 객체가 지대루 작동한다.
  rc->to_client_tcp_->socket_->SetCompletionContext((ICompletionContext*)rc);
  // post전에 increase를 한다.  decrease는 Networker의 completion에서 한다.
  rc->IncreaseUseCount();
  owner_->net_thread_pool_->PostCompletionStatus(rc->to_client_tcp_->socket_.Get(), (UINT_PTR)IocpCustomValue::NewClient);

  if (owner_->intra_logger_) {
    const String text = String::Format("New TCP socket is accepted.  net_remote_client_s: %ph, tcp_socket: %s", rc, *remote_addr.ToString());
    owner_->intra_logger_->WriteLine(LogCategory::System, *text);
  }

  return true;
}

void NetListener_S::CatchThreadUnexpectedExit(const char* where, const char* reason) {
  if (owner_ && owner_->callbacks_) {
    owner_->AssertIsNotLockedByCurrentThread();

    const String text = String::Format("(%s): Unexpected thread exit with (%s)", where, reason);
    auto result_info = ResultInfo::From(ResultCode::Unexpected, HostId_Server, text);
    owner_->ShowError_NOLOCK(result_info);
  }
}

} // namespace net
} // namespace fun
