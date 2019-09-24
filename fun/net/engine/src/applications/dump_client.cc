#include "../NetEnginePrivate.h"
#include "Net/Engine/App/DumpClient.h"

#include "GeneratedRPCs/Dump_DumpC2S_proxy.cc"
#include "GeneratedRPCs/Dump_DumpS2C_stub.cc"

namespace fun {
namespace net {

DumpClientImpl::DumpClientImpl(IDumpClientDelegate* delegate)
{
  delegate_ = delegate;

  send_progress_ = 0;
  send_total_ = 1;

  state_ = State::Connecting;
  should_stop_ = false;

  client_.Attach(NetClient::New());
  client_->AttachProxy(&c2s_proxy_);
  client_->AttachStub(this);
  client_->SetCallbacks(this);
}

DumpClientImpl::~DumpClientImpl()
{
}

void DumpClientImpl::Start(const String& server_ip, int32 server_port, const String& filename)
{
  NetConnectionArgs args;
  args.protocol_version = DUMP_PROTOCOL_VERSION;
  args.server_ip = server_ip;
  args.server_port = server_port;

  filename_ = filename;

  file_ = TRefCount<IFile>(IPlatformFS::GetPlatformPhysical().OpenRead(filename_));
  if (file_ == nullptr) {
    delegate_->OnException(HostId_None, Exception(*String::Format("cannot open dump-file '%s' for uploading.", *filename_)));
    return;
  }

  if (!client_->Connect(args)) {
    delegate_->OnException(Exception("cannot connect to dump-server."));
  }
}

void DumpClientImpl::Tick()
{
  client_->Tick();
}

void DumpClientImpl::OnJoinedToServer(const ResultInfo* result_info, const ByteArray& reply_from_server)
{
  if (result_info->result_code != ResultCode::Ok) {
    delegate_->OnException(HostId_None, Exception("cannot logon to dump-server."));
  }
  else {
    state_ = State::Sending;

    //@todo 파일 크기를 서버에 알려서, 서버에서 크기를 가늠할 수 있도록 힌트를 제공하도록 하자!
    //      클라이언트를 무조건 신뢰할 수는 없을듯..
    //      checksum을 같이 보내면 더 좋지 않을까???
    c2s_proxy_.Dump_Start(HostId_Server, GReliableSend_INTERNAL);

    //코멘트, 파일크기, 콜스택등을 같이 보내주면 좋을듯..
    //zip파일로 묶어서 보내주는게 좋을듯 싶음.
    //zip파일로 묶어서 보내면 프로토콜은 수정안해도 될듯.

    send_progress_ = 0;
    send_total_ = (int32)file_->Size(); //@todo 기가 단위의 전송은 아닐테니.. 그냥 64비트로 바꿀까??

    SendNextChunk();
  }
}

// 서버에서 데이터를 받을 준비가 됐음을 알려주었으므로, 다음 데이터를 읽어서 보냄.
bool DumpClientImpl::Dump_ChunkAck(HostId remote, const RpcHint& rpc_hint)
{
  SendNextChunk();
  return true;
}

bool DumpClientImpl::SendNextChunk()
{
  ByteArray chunk;
  const int32 chunk_len = MathBase::Max(client_->GetMessageMaxLength() - 100, 4096);
  chunk.ResizeUninitialized(chunk_len);

  const int32 readed_len = (int32)file_->TryRead(chunk.GetData(), chunk.Count()); //@todo 기가 단위의 전송은 아닐테니.. 그냥 64비트로 바꿀까??
  if (readed_len < 0) {
    delegate_->OnException(GetLocalHostId(), Exception(*String::Format("I/O error while reading '%s' file.", *filename_)));
    return false;
  }
  chunk.ResizeUninitialized(readed_len);

  if (readed_len == 0) {
    // Completed!

    c2s_proxy_.Dump_End(HostId_Server, GReliableSend_INTERNAL);

    state_ = State::Closing;
    client_->Disconnect();
    state_ = State::Stopped;

    delegate_->OnComplete();
    return false;
  }
  else {
    c2s_proxy_.Dump_Chunk(HostId_Server, GReliableSend_INTERNAL, chunk);

    send_progress_ += readed_len;
    return true;
  }
}

void DumpClientImpl::OnError(HostId host_id, const ResultInfo* result_info)
{
  delegate_->OnException(Exception("%s", result_info->ToString()));
}

void DumpClientImpl::OnLeftFromServer(const ResultInfo* result_info)
{
  state_ = State::Stopped;
}

void DumpClientImpl::OnP2PMemberJoined( HostId member_id,
                                        HostId group_id,
                                        int32 member_count,
                                        const ByteArray& custom_field)
{
  // NOOP
}

void DumpClientImpl::OnP2PMemberLeft(HostId member_id, HostId group_id, int32 member_count)
{
  // NOOP
}

DumpClient::State DumpClientImpl::GetState()
{
  return state_;
}

int32 DumpClientImpl::GetSendProgress()
{
  return send_progress_;
}

int32 DumpClientImpl::GetSendTotal()
{
  return send_total_;
}

DumpClient* DumpClient::New(IDumpClientDelegate* delegate)
{
  return new DumpClientImpl(delegate);
}

} // namespace net
} // namespace fun
