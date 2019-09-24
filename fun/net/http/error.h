#pragma once

#include "fun/http/types.h"


namespace fun {
namespace http {


enum class ErrorCode
{
  Ok = 0,
  ConnectionFailure,
  EmptyResponse,
  HostResolutionFailure,
  InternalError,
  InvalidUrlFormat,
  NetworkReceiveError,
  NetworkSendFailure,
  OperationTimedout,
  ProxyResolutionFailure,
  SslConnectError,
  SslLocalCertificateError,
  SslRemoteCertificateError,
  SslCacertError,
  GenericSslError,
  UnsupportedProtocol,

  UnknownError = 7777,
};


/// HTTP status code가 아니라, CURL에서 반환한 에러코드임.
class Error
{
public:
  Error() : code{ErrorCode::Ok} {}

  template <typename TextType>
  Error(const int32 curl_code, TextType&& message)
    : code{GetErrorCodeForCurlError(curl_code)}
    , message{FORWARD_VA_ARGS(message)}
  {
  }

  bool Ok() const
  {
    return code == ErrorCode::Ok;
  }

  /// Error code
  ErrorCode code;
  /// Error message
  String message;

private:
  FUN_HTTP_API static ErrorCode GetErrorCodeForCurlError(int32 curl_code);
};


} // namespace http
} // namespace fun
