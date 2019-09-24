//@deprecated
/*

Listener를 여러개 두었을 경우, 무엇을 위해서 여러개 두어야 하는가??
서버를 여러개 띄우기 위해서?

그렇다면, 클라이언트는 어디로 접속을 해야하나?

여러개를 지정하면 랜덤으로 선택해야하나??

*/
#include "CorePrivatePCH.h"
#include "fun/net/net.h"

#include "LanListener_S.h"
#include "LanServer.h"
#include "server_socket_pool_.h"

namespace fun {
namespace net {

LanListener_S::LanListener_S(LanServerImpl* owner)
    : owner_(fun_check_ptr(owner)), should_stop_(false) {}

LanListener_S::~LanListener_S() { StopListening(); }

void LanListener_S::StartListening() {
  fun_check_ptr(owner_->server_socket_pool_);

  StopListening();
  thread_.Reset(RunnableThread::Create(this, "LanListenerThread"));
}

void LanListener_S::StopListening() {
  if (!thread_) {
    return;
  }

  // TODO 구지 락이 필요한가??
  {
    CScopedLock2 owner_guard(owner_->GetMutex());
    owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

    // Shutdown all threads.
    should_stop_ = true;
  }

  // Wait until all threads are shutdowned.
  thread_->Join();
  thread_.Reset();

  should_stop_ = false;
}

bool LanListener_S::IsListening() const { return thread_.IsValid(); }

uint32 LanListener_S::Run() {
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
      if (owner_->tcp_accept_cp_->GetQueuedCompletionStatus(
              &completion, NetConfig::wait_completion_timeout_msec)) {
        if (new_socket) {
          InetAddress local_addr;
          InetAddress remote_addr;
          new_socket->FinalizeAcceptEx(owner_->tcp_listening_socket_.Get(),
                                       local_addr, remote_addr);

          if (remote_addr.IsUnicast()) {
            if (AcceptNewSocket(new_socket.Get(), remote_addr)) {
              new_socket.Detach();
            }
          } else {
            // 접속을 하자마자, 원격지에서 소켓을 닫아버리면 주소가 안나올 수
            // 있으므로 여기서는 그냥 무시하도록 합니다.
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
      owner_->intra_logger_->WriteLine(
          LogCategory::System,
          *String::Format("%s thread terminated.", __FUNCTION__));
    }
  } catch (std::exception& e) {
    CatchThreadUnexpectedExit(
        __FUNCTION__, *String::Format("std.exception(%s)",
                                      (const char*)UTF8_TO_TCHAR(e.what())));
  }  // catch (_com_error& e) {
  //  CatchThreadUnexpectedExit(__FUNCTION__, *String::Format("_com_error(%s)",
  //  (const char*)e.Description()));
  //} catch (void*) {
  //  CatchThreadUnexpectedExit(__FUNCTION__, "void*");
  //}

  return 0;
}

InternalSocket* LanListener_S::NewAcceptPendedSocket() {
  while (true) {
    auto new_socket = owner_->server_socket_pool_->NewTcpSocket(owner_, owner_);
    if (new_socket == nullptr) {
      owner_->EnqueueError(
          ResultInfo::From(ResultCode::Unexpected, HostId_Server,
                           "FATAL: Cannot create TCP socket for AcceptEx."));
      return nullptr;
    }

    const SocketErrorCode socket_error =
        owner_->tcp_listening_socket_->AcceptEx(new_socket);
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
          const String text = String::Format(
              "FATAL: AcceptEx failed with error code %d.  No more accept will "
              "be possible.",
              (int)socket_error);
          owner_->EnqueueError(
              ResultInfo::From(ResultCode::Unexpected, HostId_Server, text));
        }
        delete new_socket;
        return nullptr;
    }
  }
}

bool LanListener_S::AcceptNewSocket(InternalSocket* new_socket,
                                    const InetAddress& remote_addr) {
  fun_check_ptr(new_socket);
  fun_check(remote_addr.IsUnicast());

  CScopedLock2 owner_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  auto lc = new LanClient_S(owner_, new_socket, remote_addr);

  // Instance 등록.
  owner_->lan_client_instances_.Add(lc);

  // completion Context를 세팅해야 completion_port_ 객체가 지대루 작동한다.
  lc->to_client_tcp_->socket_->SetCompletionContext((ICompletionContext*)lc);

  // Increase UseCount
  lc->IncreaseUseCount();

  // 새 접속이 받아졌음을 알려줌. 처리순서가 자연스럽게 이루어지도록 network
  // thread-pool에 post함.
  owner_->net_thread_pool_->Post(lc->to_client_tcp_->socket_.Get(),
                                 (uintptr_t)IocpCustomValue::NewClient);

  // Add it to list
  owner_->candidate_lan_clients_.Add(remote_addr, lc);

  if (owner_->intra_logger_) {
    const String text = String::Format(
        "New TCP socket is accepted.  lan_remote_client_s: %ph, tcp_socket: %s",
        lc, *remote_addr.ToString());
    owner_->intra_logger_->WriteLine(LogCategory::System, *text);
  }

  return true;
}

void LanListener_S::CatchThreadUnexpectedExit(const char* where,
                                              const char* reason) {
  if (owner_ && owner_->callbacks_) {
    owner_->AssertIsNotLockedByCurrentThread();

    const String text =
        String::Format("(%s): Unexpected thread exit with (%s)", where, reason);
    auto result_info =
        ResultInfo::From(ResultCode::Unexpected, HostId_Server, text);
    owner_->ShowError_NOLOCK(result_info);
  }
}

}  // namespace net
}  // namespace fun
