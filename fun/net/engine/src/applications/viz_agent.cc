#include "../NetEnginePrivate.h"
#include "Net/Engine/Apps/viz_agent_.h"
#include "../net_client_.h"
#include "../NetServer.h"
#include "GeneratedRPCs/Viz_Viz_proxy.cc"
#include "GeneratedRPCs/Viz_Viz_stub.cc"

namespace fun {
namespace net {

const Uuid VizAgent::PROTOCOL_VERSION("{64A860A0-91FC-45FC-9EF0-E110D200A1B1}");

VizAgent::VizAgent( IVizAgentDelegate* delegate,
                    const char* server_ip,
                    int32 server_port,
                    const String& login_key)
{
  delegate_ = delegate;
  server_ip_ = server_ip;
  server_port_ = server_port;
  login_key_ = login_key;

  net_client_.Attach(new NetClientImpl());
  net_client_->SetCallbacks(this);
  net_client_->AttachProxy(&c2s_proxy_);
  net_client_->AttachStub(this);

  try_connect_begin_time_ = 0;
  state_ = State::Disconnected;

  //@todo bMuteXXX 이런식으로 이름을 바꾸는게 좋을듯..
  // 이게 반드시 있어야 됨! 안그러면 내부 RPC 추적까지 나와버림!
  c2s_proxy_.engine_specific_only_ = true;
  RpcStub::engine_specific_only_ = true;

  //CHeartbeatWorkThread::Get().Register(this, 10);
  timer_.Start();
  timer_.ExpireRepeatedly(Timespan::FromMilliseconds(10), [&](TimeContext&) { Heartbeat(); });
}

VizAgent::~VizAgent()
{
  //CHeartbeatWorkThread::Get().Unregister(this);
  timer_.Stop();

  net_client_.Reset();
}

// 이 메서드는 일정 시간마다 반드시 호출되어야 한다. 그것도 owner의 networker thread에서!
void VizAgent::Heartbeat()
{
  net_client_->Tick();

  CScopedLock2 guard(mutex_);

  switch (state_) {
    case State::Disconnected:
      Heartbeat_DisconnectedCase();
      break;

    case State::Connecting:
      Heartbeat_ConnectingCase();
      break;

    case State::Connected:
      Heartbeat_ConnectedCase();
      break;
  }
}

void VizAgent::Heartbeat_DisconnectedCase()
{
  // 시간이 되면 서버로의 연결을 시도한다.
  if (delegate_->GetAbsoluteTime() > try_connect_begin_time_) {
    NetConnectionArgs args;
    args.server_ip = server_ip_;
    args.server_port = server_port_;
    args.protocol_version = PROTOCOL_VERSION;

    net_client_->Connect(args);

    try_connect_begin_time_ = NetConfig::viz_reconnect_try_interval_sec + delegate_->GetAbsoluteTime();

    state_ = State::Connecting;
  }
}

void VizAgent::Heartbeat_ConnectingCase()
{
}

void VizAgent::Heartbeat_ConnectedCase()
{
}

void VizAgent::OnJoinedToServer(const ResultInfo* info, const ByteArray& reply_from_server)
{
  CScopedLock2 guard(mutex_);

  // 서버 연결 결과에 따라 상태 천이
  if (info->result_code == ResultCode::Ok) {
    state_ = State::Connected;

    // 인증
    c2s_proxy_.RequestLogin(HostId_Server, GSecureReliableSend_INTERNAL, login_key_, GetOwnerHostId());
  }
  else {
    state_ = State::Disconnected;

    // try_connect_begin_time_ = NetConfig::viz_reconnect_try_interval_sec + delegate_->GetAbsoluteTime(); 불필요. 이미 충분히 늘려놨으므로.
  }
}

void VizAgent::OnLeftFromServer(const ResultInfo* result_info)
{
  CScopedLock2 guard(mutex_);

  state_ = State::Disconnected;

  // try_connect_begin_time_ = NetConfig::viz_reconnect_try_interval_sec + delegate_->GetAbsoluteTime(); 불필요. 이미 충분히 늘려놨으므로.
}

DEFRPC_VizS2C_NotifyLoginOk(VizAgent)
{
  CScopedLock2 guard(mutex_);

  // 자기 상태 보내기
  NotifyInitState();

  return true;
}

DEFRPC_VizS2C_NotifyLoginFailed(VizAgent)
{
  CScopedLock2 guard(mutex_);

  // 서버와의 연결을 끊는다. 그리고 한참 뒤에 다시 재시작.
  net_client_->Disconnect();

  try_connect_begin_time_ = NetConfig::viz_reconnect_try_interval_sec + delegate_->GetAbsoluteTime();

  state_ = State::Disconnected;

  return true;
}

void VizAgent::NotifyInitState()
{
  CScopedLock2 guard(mutex_);

  if (auto client = delegate_->QueryNetClient()) {
    // 서버로의 연결 진행 상황
    ServerConnectionState dummy;
    c2s_proxy_.NotifyClient_ConnectionState(HostId_Server, GReliableSend_INTERNAL, client->GetServerConnectionState(dummy));

    // P2P 연결 갯수
    c2s_proxy_.NotifyClient_Peers_Clear(HostId_Server, GReliableSend_INTERNAL);

    for (const auto& pair : client->remote_peers_) {
      c2s_proxy_.NotifyClient_Peers_AddOrEdit(HostId_Server, GReliableSend_INTERNAL, pair.value->host_id_);
    }
  }
  else if (auto server = delegate_->QueryNetServer()) {
    c2s_proxy_.NotifySrv_ClientEmpty(HostId_Server, GReliableSend_INTERNAL);

    for (const auto& pair : server->authed_remote_clients_) {
      c2s_proxy_.NotifySrv_Clients_AddOrEdit(HostId_Server, GReliableSend_INTERNAL, pair.value->host_id_);
    }
  }
}

HostId VizAgent::GetOwnerHostId()
{
  if (auto client = delegate_->QueryNetClient()) {
    return client->local_host_id_;
  }
  else if (auto server = delegate_->QueryNetServer()) {
    return HostId_Server;
  }
  else {
    fun_check(0);
    return HostId_None;
  }
}

} // namespace net
} // namespace fun
