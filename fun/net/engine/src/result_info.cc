#include "fun/net/net.h"

namespace fun {
namespace net {

ResultInfo::ResultInfo()
  : last_received_msg() {
  result_code = ResultCode::Ok; // unexpected로 바꾸지 말것! 생성자 값을 그대로 쓰는데가 여기저기 있으니!
  detail_code = ResultCode::Ok;
  socket_error = SocketErrorCode::Ok;
  remote = HostId_None;
  remote_addr = InetAddress::None;
  //HResult = S_OK;
}

SharedPtr<ResultInfo> ResultInfo::FromSocketError(ResultCode result_code, SocketErrorCode socket_error) {
  SharedPtr<ResultInfo> ret(new ResultInfo());
  ret->result_code = result_code;
  ret->socket_error = socket_error;
  return ret;
}

SharedPtr<ResultInfo> ResultInfo::From(ResultCode result_code, HostId remote, const String& comment, const ByteArray& last_received_msg) {
  SharedPtr<ResultInfo> ret(new ResultInfo());
  ret->result_code = result_code;
  ret->remote = remote;
  ret->comment = comment;
  ret->last_received_msg = last_received_msg;
  return ret;
}

String ResultInfo::ToString() const {
  String result;

  result = String::Format("result: (%s", (const char*)TypeToString(result_code));

  if (result_code != detail_code && detail_code != ResultCode::Ok) {
    result << String::Format("), detail: (%s)", (const char*)TypeToString(detail_code));;
  }

  if (socket_error != SocketErrorCode::Ok) {
    result << String::Format("), socket_error: (%d", (int32)socket_error);
  }

  if (remote != HostId_None) {
    if (remote == HostId_Server) {
      result << "), host_id: (Server";
    } else {
      result << String::Format("), host_id: (%d", (int32)remote);
    }

    if (remote_addr.IsUnicast()) {
      result << String::Format("), remote_addr: (%s", *remote_addr.ToString());
    }
  }

  if (!comment.IsEmpty()) {
    result << "), (";
    result << comment;
  }

  //if (HResult != S_OK) {
  //  result << String::Format(", HRESULT: (%d", (long)HResult);
  //}

  //if (!source.IsEmpty()) {
  //  result << "), (";
  //  result << source;
  //}

  result << ")";

  return result;
}


//TODO localization
const char* ResultInfo::TypeToString(ResultCode result_code) {
  switch (result_code) {
    case ResultCode::Ok:
      return "OK";
    case ResultCode::Unexpected:
      return "unexpected error.";
    case ResultCode::AlreadyConnected:
      return "already connected.";
    case ResultCode::TCPConnectFailure:
      return "TCP connection failure.";
    case ResultCode::InvalidSessionKey:
      return "invalid session key.";
    case ResultCode::EncryptFail:
      return "encryption failed.";
    case ResultCode::DecryptFail:
      return "decryption failed or hack suspected.";
    case ResultCode::ConnectServerTimeout:
      return "connect to server timed out.";
    case ResultCode::ProtocolVersionMismatch:
      return "mismatched protocol between hosts.";
    case ResultCode::NotifyServerDeniedConnection:
      return "server denied connection attempt.";
    case ResultCode::ConnectServerSuccessful:
      return "connecting to server successful.";
    case ResultCode::DisconnectFromRemote:
      return "remote host disconnected.";
    case ResultCode::DisconnectFromLocal:
      return "local host disconnected.";
    case ResultCode::DangerousArgumentWarning:
      return "dangerous parameters are detected.";
    case ResultCode::UnknownEndPoint:
      return "unknown endpoint.";
    case ResultCode::ServerNotReady:
      return "server is not ready.";
    case ResultCode::ServerPortListenFailure:
      return "server socket listen failure.  make sure that the TCP or UDP listening port is not already in use.";
    case ResultCode::AlreadyExists:
      return "object already exists.";
    case ResultCode::PermissionDenied:
      return "permission denied.";
    case ResultCode::BadSessionUuid:
      return "bad session GUID.";
    case ResultCode::InvalidCredential:
      return "invalid credential.";
    case ResultCode::InvalidHeroName:
      return "invalid player character name.";
    case ResultCode::LoadDataPreceded:
      return "corruption occurred while unlocked loading and locking.";
    case ResultCode::AdjustedGamerIDNotFilled:
      return "output parameter AdjustedGamerIDNotFilled is not filled.";
    case ResultCode::NoHero:
      return "no player character(hero) found.";
    case ResultCode::UnitTestFailed:
      return "UnittestFailed";
    case ResultCode::P2PUdpFailed:
      return "P2P UDP comm is blocked.";
    case ResultCode::RUdpFailed:
      return "P2P reliable UDP failed.";
    case ResultCode::ServerUdpFailed:
      return "client-server UDP comm is blocked.";
    case ResultCode::NoP2PGroupRelation:
      return "no common P2P group exists anymore.";
    case ResultCode::UserRequested:
      return "by user request.";
    case ResultCode::InvalidPacketFormat:
      return "invalid packet format. remote host is hacked or has a bug.";
    case ResultCode::TooLargeMessageDetected:
      return "too large message is detected.  contact technical supports.";
    case ResultCode::CannotEncryptUnreliableMessage:
      return "an unreliable message cannot be encrypted.";
    case ResultCode::ValueNotExist:
      return "not exist value.";
    case ResultCode::TimeOut:
      return "working is timedout.";
    case ResultCode::LoadedDataNotFound:
      return "can not found loaddata.";
    case ResultCode::SendQueueIsHeavy:
      return "send-queue has accumulated too much.";
    case ResultCode::TooSlowHeartbeatWarning:
      return "heartbeat call in too slow.suspected starvation";
    case ResultCode::CompressFail:
      return "message compress/decompress fail.";
    case ResultCode::LocalSocketCreationFailed:
      return "unable to start listening of client socket.  must check if either TCP or UDP socket is already in use.";
    case ResultCode::NoneAvailableInPortPool:
      return "failed binding to local port that defined in port pool.  please check number of values in port pool are sufficient.";
    case ResultCode::InvalidPortPool:
      return "range of user defined port is wrong.  set port to 0(random port binding) or check if it is overlapped.";
  }
  return "<undefined>";
}

} // namespace net
} // namespace fun
