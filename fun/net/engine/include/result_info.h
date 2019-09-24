#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

/**
 * 처리에 대한 결과에 대한 정보를 담은 구조체입니다.
 */
class ResultInfo {
 public:
  /**
   * 처리결과 코드입니다.  성공일 경우에는 ResultCode::Ok입니다.
   */
  ResultCode result_code;
  /**
   * 처리결과에 대한 세부 코드입니다. (구체적인 이유에 해당합니다.)
   */
  ResultCode detail_code;
  /**
   * 발생한 소켓 에러코드입니다.
   */
  SocketErrorCode socket_error;
  /**
   * 오류가 발생한 Remote의 ID입니다.
   */
  HostId remote;
  /**
   * 연결 생대방의 주소입니다.
   */
  InetAddress remote_addr;
  /**
   * 추가 상세설명입니다.
   */
  String comment;
  /**
   * 오류를 발생시킨 마지막 메시지의 전체 내용입니다.
   */
  ByteArray last_received_msg;

  //HRESULT HResult; //TODO 윈도우즈 전용이므로 제거 고려...
  //String source; //TODO DB sql statement string이므로, 제거 고려...

 public:
  FUN_NETX_API ResultInfo();

  inline bool IsOk() const { return result_code == ResultCode::Ok; }

  FUN_NETX_API static SharedPtr<ResultInfo> FromSocketError(ResultCode result_code, SocketErrorCode socket_error);
  FUN_NETX_API static SharedPtr<ResultInfo> From(ResultCode result_code,
                                            HostId remote = HostId_None,
                                            const String& comment = String(),
                                            const ByteArray& last_received_msg = ByteArray());

  FUN_NETX_API String ToString() const;

  FUN_NETX_API static const char* TypeToString(ResultCode result_code);
};

} // namespace net
} // namespace fun
