#include "fun/http/error.h"

//#define CURL_STATICLIB
#include "ThirdParty/curl-7.52.1/include/curl/curl.h"

namespace fun {
namespace http {

ErrorCode Error::GetErrorCodeForCurlError(int32 cur_code) {
  switch (cur_code) {
    case CURLE_OK:
      return ErrorCode::Ok;
    case CURLE_UNSUPPORTED_PROTOCOL:
      return ErrorCode::UnsupportedProtocol;
    case CURLE_URL_MALFORMAT:
      return ErrorCode::InvalidUrlFormat;
    case CURLE_COULDNT_RESOLVE_PROXY:
      return ErrorCode::ProxyResolutionFailure;
    case CURLE_COULDNT_RESOLVE_HOST:
      return ErrorCode::HostResolutionFailure;
    case CURLE_COULDNT_CONNECT:
      return ErrorCode::ConnectionFailure;
    case CURLE_OPERATION_TIMEDOUT:
      return ErrorCode::OperationTimedout;
    case CURLE_SSL_CONNECT_ERROR:
      return ErrorCode::SslConnectError;
    case CURLE_PEER_FAILED_VERIFICATION:
      return ErrorCode::SslRemoteCertificateError;
    case CURLE_GOT_NOTHING:
      return ErrorCode::EmptyResponse;
    case CURLE_SSL_ENGINE_NOTFOUND:
      return ErrorCode::GenericSslError;
    case CURLE_SSL_ENGINE_SETFAILED:
      return ErrorCode::GenericSslError;
    case CURLE_SEND_ERROR:
      return ErrorCode::NetworkSendFailure;
    case CURLE_RECV_ERROR:
      return ErrorCode::NetworkReceiveError;
    case CURLE_SSL_CERTPROBLEM:
      return ErrorCode::SslLocalCertificateError;
    case CURLE_SSL_CIPHER:
      return ErrorCode::GenericSslError;
    case CURLE_SSL_CACERT:
      return ErrorCode::SslCacertError;
    case CURLE_USE_SSL_FAILED:
      return ErrorCode::GenericSslError;
    case CURLE_SSL_ENGINE_INITFAILED:
      return ErrorCode::GenericSslError;
    case CURLE_SSL_CACERT_BADFILE:
      return ErrorCode::SslCacertError;
    case CURLE_SSL_SHUTDOWN_FAILED:
      return ErrorCode::GenericSslError;
    case CURLE_SSL_CRL_BADFILE:
      return ErrorCode::SslCacertError;
    case CURLE_SSL_ISSUER_ERROR:
      return ErrorCode::SslCacertError;
    case CURLE_TOO_MANY_REDIRECTS:
      return ErrorCode::Ok;
    default:
      return ErrorCode::InternalError;
  }
}

}  // namespace http
}  // namespace fun
