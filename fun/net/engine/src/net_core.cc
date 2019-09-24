// TODO 중복 rpc-id 체크.
#include "fun/net/net.h"

//#include "Apps/viz_agent_.h"

//#include "Compressor.h"
#include "Crypto.h"

namespace fun {
namespace net {

using lf = LiteFormat;

// const char* DuplicatedRpcIdErrorText = "Duplicated RPC ID is found. Review
// RPC ID declaration in .IDL files."; const char* BadRpcIdErrorText = "Wrong RPC
// ID is found. RPC ID should be >=1000 or <65530."; const char*
// AsyncCallbackMayOccurErrorText = "Already async callback may occur! server
// start or client connection should have not been done before here.";

NetCoreImpl::NetCoreImpl() {}

NetCoreImpl::~NetCoreImpl() {
  // 이미 상속 클래스에서 제거해둔 상태이어야 한다.
  fun_check(proxies_nolock_.IsEmpty());
  fun_check(stubs_nolock_.IsEmpty());

  // TODO
  // fun_check(!viz_agent);
}

void NetCoreImpl::CleanupEveryProxyAndStub() {
  for (auto& proxy : proxies_nolock_) {
    proxy->core_ = nullptr;
  }
  proxies_nolock_.Clear();

  for (auto& stub : stubs_nolock_) {
    stub->core_ = nullptr;
  }
  stubs_nolock_.Clear();
}

void NetCoreImpl::CheckDefaultTimeoutTimeValidation(double timeout_sec) {
  // if (timeout_sec < NetConfig::GetDefaultNoPingTimeoutTime()) {
  //  throw Exception("No ping timeout value is too small.");
  //}
}

//정렬되어 있다고 가정하면?
//그냥 겹치는지 확인하는게 좋을듯...?

static bool IsOverlapped(const RpcId* a, int32 a_count, const RpcId* b,
                         int32 b_count) {
  //겹치는지 여부만 알려주는게 아니라, 어떤것이 겹치는지 알려주는것이 좀더
  //좋을듯함. 단, 이때 번호만 알려주는게 아니라 이름까지 알려줄 수 있다면 좋을듯
  //한데...
}

void NetCoreImpl::AttachProxy(RpcProxy* proxy_to_attach) {
  fun_check_ptr(proxy_to_attach);

  // 이미 클라이언트 내지 서버가 실행 중이라면, 설정할 수 없음.
  // 즉, 클라이언트/서버 실행전에 설정해야함.
  if (AsyncCallbackMayOccur()) {
    // TODO Exception 클래스를 특수화하는게 좋을듯함.
    // CAsyncCallbackOccurException() 같은...?  이름은 좀더 생각을 해봐야할듯..
    throw Exception(
        "Already async callback may occur! server start or client connection "
        "should have not been done before here.");
  }

  //이미 attach된 경우에도 체크를 해야함.
  //이미 등록된 경우에는 무시하지 말고 예외를 던지도록 하자.
  //이게 좀더 합리적인게, 겹친다는것은 외부에서 의도치 않게
  //같은 작업을 여러번 했다는 얘기일테니...

  //@todo 이놈은 선형적이지 않은 목록을 추려내어서 개별로 충돌여부를 확인하도록
  //하자.
  // 만약 중복되는 ID가 있으면 에러 처리한다.
  for (int32 proxy_index = 0; proxy_index < proxies_nolock_.Count();
       ++proxy_index) {
    RpcProxy* proxy = proxies_nolock_[proxy_index];

    // TODO
    // if (proxy->GetFirstRpcId() >= 65530 || proxy->GetLastRpcId() < 1) {
    //  throw Exception(BadRpcIdErrorText);
    //}
    //
    // if (!IsCombinationEmpty((uint16)proxy->GetFirstRpcId(),
    // (uint16)(proxy->GetLastRpcId() + 1),
    // (uint16)proxy_to_attach->GetFirstRpcId(),
    // (uint16)(proxy_to_attach->GetLastRpcId() + 1))) {
    //  throw Exception(DuplicatedRpcIdErrorText);
    //}
  }

  proxy_to_attach->core_ = this;

  proxies_nolock_.Add(proxy_to_attach);
}

void NetCoreImpl::AttachStub(RpcStub* stub_to_attach) {
  fun_check_ptr(stub_to_attach);

  // TODO 이미 등록되어 있는것을 재차 시도한 경우 오류로 처리.
  // TODO ID 대역이 겹치는것을 체크하도록...

  //@todo 이놈은 선형적이지 않은 목록을 추려내어서 개별로 충돌여부를 확인하도록
  //하자.
  // 만약 중복되는 ID가 있으면 에러 처리한다.
  for (int32 stub_index = 0; stub_index < stubs_nolock_.Count(); ++stub_index) {
    RpcStub* stub = stubs_nolock_[stub_index];

    // TODO
    // if (!IsCombinationEmpty((uint16)stub->GetFirstRpcId(),
    // (uint16)(stub->GetLastRpcId() + 1),
    // (uint16)stub_to_attach->GetFirstRpcId(),
    // (uint16)(stub_to_attach->GetLastRpcId() + 1))) {
    //  throw Exception(DuplicatedRpcIdErrorText);
    //}
  }

  stub_to_attach->core_ = this;

  stubs_nolock_.Add(stub_to_attach);
}

void NetCoreImpl::ShowError_NOLOCK(ptr<ResultInfo> result_info) {
  GetMutex().AssertIsNotLockedByCurrentThread();

  if (GetCallbacks_NOLOCK()) {
    GetCallbacks_NOLOCK()->OnError(result_info.Get());
  }
}

void NetCoreImpl::ShowNotImplementedRpcWarning(RpcId rpc_id,
                                               const char* rpc_name) {
  LOG(LogNetEngine, Warning, "unimplemented RPC function is called. %s(%d)",
      rpc_name, (int32)rpc_id);
}

void NetCoreImpl::PostCheckReadMessage(IMessageIn& msg, RpcId rpc_id,
                                       const char* rpc_name) {
  if (!msg.AtEnd()) {
    LOG(LogNetEngine, Warning, "failed to read RPC incoming message %s",
        rpc_name);
  }

  // TEMP 임시로 어떤 RPC메시지를 받은건지 체크해보기 위한 용도임.
  // LOG(LogNetEngine,Warning,"  RPC: %s(%d)", rpc_name, (int32)rpc_id);

  // FIXME
  //예외처리를 해주어야할듯 싶은데..
  //여기서 로그만 출력하면, 실제 stub 함수가 엉망인상태로 호출되는 불상사가
  //발생하므로, 예외를 던지던지 하는 형태로 처리를 해야한다.
}

void NetCoreImpl::DetachProxy(RpcProxy* proxy_to_detach) {
  fun_check(proxy_to_detach);

  if (AsyncCallbackMayOccur()) {
    // TODO Exception 클래스를 특수화하는게 좋을듯함.
    // CAsyncCallbackOccurException() 같은...?  이름은 좀더 생각을 해봐야할듯..
    throw Exception(
        "Already async callback may occur! server start or client connection "
        "should have not been done before here.");
  }

  for (int32 proxy_index = 0; proxy_index < proxies_nolock_.Count();
       ++proxy_index) {
    RpcProxy* proxy = proxies_nolock_[proxy_index];

    if (proxy == proxy_to_detach) {
      proxies_nolock_.RemoveAt(proxy_index);
      proxy_to_detach->core_ = nullptr;
      return;
    }
  }

  // TODO Detach에 실패했을 경우, 오류로 처리하는것이 좋지 아니한가??
}

void NetCoreImpl::DetachStub(RpcStub* stub_to_detach) {
  fun_check_ptr(stub_to_detach);

  for (int32 stub_index = 0; stub_index < stubs_nolock_.Count(); ++stub_index) {
    RpcStub* stub = stubs_nolock_[stub_index];

    if (stub == stub_to_detach) {
      stubs_nolock_.RemoveAt(stub_index);
      stub_to_detach->core_ = nullptr;
      return;
    }
  }

  // TODO Detach에 실패했을 경우, 오류로 처리하는것이 좋지 아니한가??
}

// Compressed -> Secure -> Broadcast

//
// Compressed layer
//

#define COMPRESSION_ATLEAST_LENGTH (50 + 1)

bool NetCoreImpl::Send_CompressLayer(const SendFragRefs& payload,
                                     const SendOption& send_opt,
                                     const HostId* sendto_list,
                                     int32 sendto_count) {
  if (sendto_count <= 0) {
    return false;
  }

  // Because it has 12 bytes header, it is rather easy to increase the size
  // below 50 bytes.
  if (send_opt.compression_mode == CompressionMode::None ||
      payload.GetTotalLength() <= COMPRESSION_ATLEAST_LENGTH) {
    // Send in uncompressed.
    return Send_SecureLayer(payload, send_opt, sendto_list, sendto_count);
  }

  // Attach fragments to a single piece of data. That's how it gets compressed
  // note: 압축을 위해서는 어쩔수 없이 복사를 해야하나? 스트림 형태로 처리한다면
  // 복사 비용을 줄일수도 있을듯 싶은데...
  ByteArray payload_bytes = payload.ToBytes();  // allocation and copy

  // Prepare a space for messages to be compressed
  MessageOut compressed_msg;
  int32 actual_compressed_len = Compressor::GetMaxCompressedLength(
      send_opt.compression_mode, payload_bytes.Len());
  compressed_msg.SetLength(actual_compressed_len);

  // Let's rolling.
  String compression_error;
  const bool compression_ok = Compressor::Compress(
      send_opt.compression_mode, compressed_msg.MutableData(),
      &actual_compressed_len, (const uint8*)payload_bytes.ConstData(),
      payload_bytes.Len(), &compression_error);
  if (!compression_ok) {
    // Compression is failed.
    const String text = String::Format("packet compression failed. error: %s",
                                       *compression_error);
    EnqueueError(
        ResultInfo::From(ResultCode::CompressFail, sendto_list[0], text));

    // Send in uncompressed.
    return Send_SecureLayer(payload, send_opt, sendto_list, sendto_count);
  }

  // If compression efficiency is not available, it is a waste to
  // send it unnecessarily.
  if ((actual_compressed_len + 10) < payload.GetTotalLength()) {
    // Re-adjust to compressed actual size.
    compressed_msg.SetLength(actual_compressed_len);

    // Put the header before the compressed message.
    MessageOut header;
    lf::Write(header, MessageType::Compressed);
    lf::Write(header, send_opt.compression_mode);
    lf::Write(header, OptimalCounter32(compressed_msg.GetLength()));
    lf::Write(header, OptimalCounter32(payload_bytes.Len()));

    // Now add the compressed body
    SendFragRefs compressed_payload;
    compressed_payload.Add(header);
    compressed_payload.Add(compressed_msg);

    // Send it compressed(compressed -> secure)
    return Send_SecureLayer(compressed_payload, send_opt, send_to,
                            sendto_count);
  } else {
    // note: 실제로 압축을 해봤더니, 효율이 좋지 않았음.  CPU만 낭비한 상황임...

    // Send uncompressed
    return Send_SecureLayer(payload, send_opt, send_to, sendto_count);
  }
}

bool NetCoreImpl::DecompressMessage(ReceivedMessage& received_msg,
                                    MessageIn& decompressed_output) {
  auto& compressed_msg = received_msg.unsafe_message;
  const int32 saved_read_pos = compressed_msg.Tell();

  CompressionMode compression_mode;
  OptimalCounter32 compressed_payload_len;
  OptimalCounter32 decompressed_payload_len;

  // Read header
  if (!lf::Reads(compressed_msg, compression_mode, compressed_payload_len,
                 decompressed_payload_len)) {
    TRACE_SOURCE_LOCATION();

    // TODO 오류 트래킹을 해주는게 좋을듯 싶은데...??
    compressed_msg.Seek(saved_read_pos);
    return false;
  }

  // Check the length first because it is difficult to overkill memory due to
  // bug size or hacked size. 실제로는 0미만이 아닌 최소값 이상이어야함.
  // 왜냐하면, 최소값 이하는 압축을 하지 않았을테니...
  if (decompressed_payload_len < COMPRESSION_ATLEAST_LENGTH ||
      decompressed_payload_len > GetMessageMaxLength()) {
    // TRACE_SOURCE_LOCATION();
    // LOG(LogNetEngine,Warning,"decompressed_payload_len=%d,
    // message_max_length=%d", decompressed_payload_len, GetMessageMaxLength());

    // TODO 오류 트래킹을 해주는게 좋을듯 싶은데...??
    compressed_msg.Seek(saved_read_pos);
    return false;
  }

  // 넘겨받은 길이만큼 데이터를 읽을 수 있는지 체크.
  //
  // (정확히 비교하는게 좋을런지? 호출부 하단에서 모든 메시지를 읽었는지
  // 여부를 확인하므로 여기서는 딱히 더 확인할 필요는 없음.)
  if (!compressed_msg.CanRead(compressed_payload_len)) {
    TRACE_SOURCE_LOCATION();

    // TODO 오류 트래킹을 해주는게 좋을듯 싶은데...??
    compressed_msg.Seek(saved_read_pos);
    return false;
  }

  // Prepare the buffer for the uncompressed output
  ByteArray decompressed_payload(decompressed_payload_len, NoInit);

  // Let's decompression
  String decompression_error;
  int32 actual_decompressed_len = decompressed_payload_len;
  if (!Compressor::Decompress(
          compression_mode, (uint8*)decompressed_payload.MutableData(),
          &actual_decompressed_len, compressed_msg.GetReadableData(),
          compressed_payload_len, &decompression_error) ||
      actual_decompressed_len != decompressed_payload_len) {
    const String text = String::Format("packet decompression failed. error: %s",
                                       *decompression_error);
    EnqueueError(ResultInfo::From(ResultCode::CompressFail, HostId_None, text));

    compressed_msg.Seek(saved_read_pos);
    return false;
  }

  // OK!
  decompressed_output = MessageIn(decompressed_payload);
  return true;
}

//
// Secure layer
//

bool NetCoreImpl::Send_SecureLayer(const SendFragRefs& payload,
                                   const SendOption& send_opt,
                                   const HostId* sendto_list,
                                   int32 sendto_count) {
  if (send_opt.encryption_mode == EncryptionMode::None ||
      send_opt.reliability == MessageReliability::Last) {
    return Send_BroadcastLayer(payload, send_opt, sendto_list, sendto_count);
  }

  // ungroup send dst 후 각각에 대해 crypt_count 증가, unicast를 한다.
  // reliable 과 unreliable 메시지 모두를 지원함.
  HostIdArray flatten_send_dest_list;
  {
    // ConvertGroupToIndividualsAndUnion() 함수가 락을 걸고 호출하는 것을 전재로
    // 하였기 때문에 호출전에 락을 걸어주어야 한다.
    CScopedLock2 main_guard(GetMutex());
    CheckCriticalSectionDeadLock(__FUNCTION__);

    ConvertGroupToIndividualsAndUnion(sendto_count, sendto_list,
                                      flatten_send_dest_list);
  }

  String out_error;
  bool ok = false;
  for (int32 send_dest_index = 0;
       send_dest_index < flatten_send_dest_list.Count(); ++send_dest_index) {
    const HostId send_dest = flatten_send_dest_list[send_dest_index];
    SessionKey* session_key_ = nullptr;

    // 자기자신에게 보내는 경우에는, encryption할 필요 없음.
    if (GetLocalHostId() == send_dest) {
      ok = Send_BroadcastLayer(payload, send_opt, &send_dest, 1);
    }

    // NextEncryptCount는 나중에 실행해야 하므로 &&의 우변항에 있어야 한다!
    // 안그러면 GetCryptSessionKey가 실패시 NextEncryptCount는 +1 된 상태이므로
    // 상대측에서 패킷 해킹으로 오인함!
    else if ((session_key_ = GetCryptSessionKey(send_dest, out_error))) {
      // NextEncrytCount의 순서를 보장하기위해 여기서 main lock필요.
      CScopedLock2 main_guard(GetMutex());
      CheckCriticalSectionDeadLock(__FUNCTION__);

      MessageOut input_payload;

      // Reliable일 경우에만, 페이로드 앞쪽에 sequencial number를 추가함.
      // (패킬 리플레이 공격 방어차원)
      if (send_opt.reliability == MessageReliability::Reliable) {
        CryptoCountType encrypt_count;
        if (!NextEncryptCount(send_dest, encrypt_count)) {
          EnqueueError(ResultInfo::From(ResultCode::EncryptFail, send_dest,
                                        "NextEncryptCount is failed."));
          return false;
        }

        lf::Write(input_payload, encrypt_count);
      }

      // FIXED 의도치 않게 frag count가 삽입되는 문제가 있어서 수동으로 처리함.
      // 실제 payload 추가 (copy 수반됨. 개선할 방법이 없을까..??)
      // lf::Write(input_payload, payload);
      for (int32 frag_index = 0; frag_index < payload.Count(); ++frag_index) {
        input_payload.WriteRawBytes(payload[frag_index].data,
                                    payload[frag_index].length);
      }

      ByteArray output_payload;
      bool encryption_ok = false;
      switch (send_opt.encryption_mode) {
        case EncryptionMode::Strong:
          encryption_ok =
              CryptoAES::Encrypt(session_key_->aes_key,
                                 ByteStringView(input_payload.ConstData(),
                                                input_payload.GetLength()),
                                 output_payload);
          break;
        case EncryptionMode::Weak:
          encryption_ok =
              CryptoRC4::Encrypt(session_key_->rc4_key,
                                 ByteStringView(input_payload.ConstData(),
                                                input_payload.GetLength()),
                                 output_payload);
          break;
        // case EncryptionMode::Fast:
        //  fun_check(0); //TODO
        //  break;
        case EncryptionMode::None:
          // do nothing
          fun_check(0);  // 위에서 체크하고 들어왔으므로, 도달할 수 없는 경우임.
          break;
        default:
          EnqueueError(ResultInfo::From(
              ResultCode::EncryptFail, send_dest,
              String::Format("unsupported encryption method: %d",
                             (int32)send_opt.encryption_mode)));
          break;
      }

      if (encryption_ok) {
        MessageOut header;

        // Reliable/Unreliable을 각각 구분해서 전송해야함.
        // 위에서, Reliable일 경우에만 페이로드 앞쪽에 카운터 값을 추가
        // 했으므로..
        const MessageType msg_type =
            send_opt.reliability == MessageReliability::Reliable
                ? MessageType::Encrypted_Reliable
                : MessageType::Encrypted_Unreliable;
        lf::Write(header, msg_type);
        lf::Write(header, send_opt.encryption_mode);
        lf::Write(header, OptimalCounter32(output_payload.Len()));

        // LOG(LogNetEngine, Warning, "<< ENCRYPTION:  plain=%d bytes, cipher=%d
        // bytes", EncryptedPayload.GetLength(), output_payload.Len());

        SendFragRefs data_to_send;
        data_to_send.Add(header);
        data_to_send.Add(output_payload);

        // Send_BroadcastLayer이 실패할 상황이라면, 이미 NextEncryptCount에서
        // 실패. 그러므로, 여기서는 PrevEncryptCount를 하지 않는다.
        ok = Send_BroadcastLayer(data_to_send, send_opt, &send_dest, 1);
      } else {
        EnqueueError(ResultInfo::From(ResultCode::EncryptFail, send_dest,
                                      "encryption is failed."));

        // 암호화가 실패한 경우에는 EncryptCount를 원래대로 돌려주어야함.
        // (Rollback)
        if (send_opt.reliability == MessageReliability::Reliable) {
          PrevEncryptCount(send_dest);
        }
      }
    } else {
      CScopedLock2 main_guard(GetMutex());
      CheckCriticalSectionDeadLock(__FUNCTION__);

      // StartServerArgs.p2p_encrypted_messaging_enabled = false;

      if (!out_error.IsEmpty()) {
        EnqueueError(
            ResultInfo::From(ResultCode::EncryptFail, send_dest, out_error));
      } else {
        EnqueueError(
            ResultInfo::From(ResultCode::EncryptFail, send_dest,
                             "StartServerArgs.p2p_encrypted_messaging_enabled "
                             "= false. P2P Messaging can not encrypted."));
      }
    }
  }

  return ok;
}

bool NetCoreImpl::DecryptMessage(MessageType msg_type,
                                 ReceivedMessage& received_msg,
                                 MessageIn& decrypted_output) {
  // NetClient에서도 사용한다.
  // GetMutex().AssertIsNotLockedByCurrentThread();

  auto& msg = received_msg.unsafe_message;
  const int32 saved_read_pos = msg.Tell();

  EncryptionMode encryption_mode;
  OptimalCounter32 encrypted_payload_len;
  if (!lf::Reads(msg, encryption_mode, encrypted_payload_len)) {
    msg.Seek(saved_read_pos);
    return false;
  }

  // 넘겨진 페이로드 길이만큼 정확하게 데이터가 있는지 확인(더도 말고 덜도말고
  // 정확히 일치해야함)
  // -> 암호화된 경우에는 길이의 왜곡이 있을 수 있으므로, 정확히 비교하는대신
  // 최소 길이만 체크함.
  // if (encrypted_payload_len != msg.GetReadableLength()) {
  if (!msg.CanRead(encrypted_payload_len)) {
    msg.Seek(saved_read_pos);
    return false;
  }

  // 이제 decrypt를 할때 MainLock이 필요치 않음.

  // 복호화를 한다.

  bool decryption_ok = false;
  ByteArray decryption_payload;
  String out_error;
  SessionKey* session_key =
      GetCryptSessionKey(received_msg.remote_id, out_error);
  if (session_key) {  //세션키가 있어야지만, 복호화를 수행할 수 있음.
    switch (encryption_mode) {
      case EncryptionMode::Strong:
        decryption_ok = CryptoAES::Decrypt(
            session_key->aes_key,
            ByteStringView(msg.GetReadableData(), msg.GetReadableLength()),
            decryption_payload);
        break;
      case EncryptionMode::Weak:
        decryption_ok = CryptoRC4::Decrypt(
            session_key->rc4_key,
            ByteStringView(msg.GetReadableData(), msg.GetReadableLength()),
            decryption_payload);
        break;
      // case EncryptionMode::Fast:
      //  fun_check(0); //TODO
      //  break;
      case EncryptionMode::None:  //발생할 수 없는 경우임.
        LOG(LogNetEngine, Trace,
            "%s: encryption_mode.None is not allowed at here", __FUNCTION__);
        break;
      default:
        LOG(LogNetEngine, Trace, "%s: unsupported encryption method: %d",
            __FUNCTION__, (int32)encryption_mode);
        break;
    }
  }

  if (!decryption_ok) {
    CScopedLock2 main_guard(GetMutex());
    CheckCriticalSectionDeadLock(__FUNCTION__);

    if (!out_error.IsEmpty()) out_error << " ";
    out_error << "decryption failed";

    EnqueueError(ResultInfo::From(ResultCode::DecryptFail,
                                  received_msg.remote_id, out_error));
    msg.Seek(saved_read_pos);
    return false;
  }

// debugging
#if 1
  // LOG(LogNetEngine,Warning,"DecryptedMessageLength: %d",
  // decryption_payload.Len()); LOG(LogNetEngine, Warning, ">> DECRYPTION:
  // plain=%d bytes, cipher=%d bytes", decryption_payload.Len(),
  // encrypted_payload_len);
#endif

  decrypted_output = MessageIn(decryption_payload);

  // Reliable일 경우에 한해서, 앞쪽에 카운터가 있으므로, 카운터 값이 유효한지
  // 체크함.
  if (msg_type == MessageType::Encrypted_Reliable) {
    // Check whether the serial value is normal.

    CryptoCountType tossed_decrypt_count;
    CryptoCountType expected_decrypt_count;

    if (!lf::Read(decrypted_output, tossed_decrypt_count)) {
      CScopedLock2 main_guard(GetMutex());
      CheckCriticalSectionDeadLock(__FUNCTION__);

      EnqueueError(ResultInfo::From(ResultCode::DecryptFail,
                                    received_msg.remote_id,
                                    "tossed_decrypt_count read failed."));
      msg.Seek(saved_read_pos);
      return false;
    }

    if (!GetExpectedDecryptCount(received_msg.remote_id,
                                 expected_decrypt_count)) {
      CScopedLock2 main_guard(GetMutex());
      CheckCriticalSectionDeadLock(__FUNCTION__);

      EnqueueError(ResultInfo::From(ResultCode::DecryptFail,
                                    received_msg.remote_id,
                                    "GetExpectedDecryptCount failed."));
      msg.Seek(saved_read_pos);
      return false;
    }

    if (tossed_decrypt_count != expected_decrypt_count) {
      CScopedLock2 main_guard(GetMutex());
      CheckCriticalSectionDeadLock(__FUNCTION__);

      const String text = String::Format(
          "tossed_decrypt_count(%d) != expected_decrypt_count(%d)",
          tossed_decrypt_count, expected_decrypt_count);
      EnqueueError(ResultInfo::From(ResultCode::DecryptFail,
                                    received_msg.remote_id, text));
      msg.Seek(saved_read_pos);
      return false;
    }

    // 시리얼 값 이동 (내부에서 MainLock검)
    NextDecryptCount(received_msg.remote_id);
  }

  // debugging
#if 0
  //decrypted_output
  const uint8* ptr = decrypted_output.GetReadableData();
  String str;
  str << String::Format("DECRYPTION OK >>  output:%d, read:%d : ", decrypted_output.GetReadableLength(), decrypted_output.Tell());
  for (int32 i = 0; i < decrypted_output.GetReadableLength(); ++i) {
    if (i) str << " ";
    str << String::Format("%02X", ptr[i]);
  }
  LOG(LogNetEngine,Warning,"\n%s\n\n",*str);
#endif

  return true;
}

bool NetCoreImpl::SendFreeform(const HostId* sendto_list, int32 sendto_count,
                               const RpcCallOption& rpc_call_opt,
                               const uint8* payload, int32 payload_length) {
  rpc_call_opt.AssureValidation();

  MessageOut header;
  lf::Write(header, MessageType::FreeformMessage);
  lf::Write(header, OptimalCounter32(payload_length));

  SendFragRefs frag_refs;
  frag_refs.Add(header);
  frag_refs.Add(payload, payload_length);

  const bool ok =
      Send(frag_refs, SendOption(rpc_call_opt), sendto_list, sendto_count);

  // Call back the event that invoked the custom message.
  //@todo 이게 실제로 요구되는지 판단해서 필요없다면, 아예 안하는게 바람직할듯..
  //
  // if (RequiredViz()) 요런 함수하나 추가하면 될듯함..

  // TODO
  // if (viz_agent_) {
  //  MessageSummary summary;
  //  summary.payload_length = frag_refs.GetTotalLength();
  //  summary.rpc_id = RpcId_None;
  //  summary.rpc_name = "<User Defined Message>";
  //  summary.encryption_mode = rpc_call_opt.encryption_mode;
  //  summary.compression_mode = rpc_call_opt.compression_mode;
  //
  //  Viz_NotifySendByProxy(sendto_list, sendto_count, summary, rpc_call_opt);
  //}

  return ok;
}

}  // namespace net
}  // namespace fun
