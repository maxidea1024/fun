#pragma once

#include "fun/http/http.h"

namespace fun {
namespace http {

/**
HTTP status codes

1xx = Informational (리퀘스트를 받고, 처리중에 있음)
2xx = Success       (리퀘스트를 정상적으로 완료함)
3xx = Redirection   (Redirection되었으므로, 완료를 위해서는 추가적인 동작이
필요함) 4xx = Client Error  (클라이언트의 요청을 처리할 수 없어서 발생하는 오류)
5xx = Server Error  (서버에서 처리를 하지 못하여 발생하는 오류)

https://ko.wikipedia.org/wiki/HTTP_%EC%83%81%ED%83%9C_%EC%BD%94%EB%93%9C
*/
enum class StatusCode {
  // 1×× Informational
  // 요청을 받았으며 작업을 계속한다.

  /// 요청자는 요청을 계속해야 한다. 서버는 이 코드를 제공하여 요청의 첫 번째
  /// 부분을 받았으며 나머지를 기다리고 있음을 나타낸다.
  Continue = 100,
  /// 요청자가 서버에 프로토콜 전환을 요청했으며 서버는 이를 승인하는 중이다.
  SwitchingProtocols = 101,
  /// RFC 2518
  Processing = 102,

  // 2×× Success
  // 이 클래스의 상태 코드는 클라이언트가 요청한 동작을 수신하여 이해했고
  // 승낙했으며 성공적으로 처리했음을 가리킨다.

  /// (성공): 서버가 요청을 제대로 처리했다는 뜻이다. 이는 주로 서버가 요청한
  /// 페이지를 제공했다는 의미로 쓰인다.
  OK = 200,
  /// (작성됨): 성공적으로 요청되었으며 서버가 새 리소스를 작성했다.
  Created = 201,
  /// (허용됨): 서버가 요청을 접수했지만 아직 처리하지 않았다.
  Accepted = 202,
  /// (신뢰할 수 없는 정보): 서버가 요청을 성공적으로 처리했지만 다른 소스에서
  /// 수신된 정보를 제공하고 있다.
  NonAuthoritativeInformation = 203,
  /// (콘텐츠 없음): 서버가 요청을 성공적으로 처리했지만 콘텐츠를 제공하지
  /// 않는다.
  NoContent = 204,
  /// (콘텐츠 재설정): 서버가 요청을 성공적으로 처리했지만 콘텐츠를 표시하지
  /// 않는다. 204 응답과 달리 이 응답은 요청자가 문서 보기를 재설정할 것을
  /// 요구한다(예: 새 입력을 위한 양식 비우기).
  ResetContent = 205,
  /// (일부 콘텐츠): 서버가 GET 요청의 일부만 성공적으로 처리했다.
  PartialContent = 206,
  /// (다중 상태): RFC 4918
  MultiStatus = 207,
  /// (이미 보고됨): RFC 5842
  AlreadyReported = 208,
  /// RFC 3229
  IMUsed = 226,

  // 3×× Redirection
  // 클라이언트는 요청을 마치기 위해 추가 동작을 취해야 한다.

  /// (여러 선택항목): 서버가 요청에 따라 여러 조치를 선택할 수 있다. 서버가
  /// 사용자 에이전트에 따라 수행할 작업을 선택하거나, 요청자가 선택할 수 있는
  /// 작업 목록을 제공한다.
  MultipleChoices = 300,
  /// (영구 이동): 요청한 페이지를 새 위치로 영구적으로 이동했다. GET 또는 HEAD
  /// 요청에 대한 응답으로 이 응답을 표시하면 요청자가 자동으로 새 위치로
  /// 전달된다.
  MovedPermanently = 301,
  /// (임시 이동): 현재 서버가 다른 위치의 페이지로 요청에 응답하고 있지만
  /// 요청자는 향후 요청 시 원래 위치를 계속 사용해야 한다.
  Found = 302,
  /// (기타 위치 보기): 요청자가 다른 위치에 별도의 GET 요청을 하여 응답을
  /// 검색할 경우 서버는 이 코드를 표시한다. HEAD 요청 이외의 모든 요청을 다른
  /// 위치로 자동으로 전달한다.
  SeeOther = 303,
  /// (수정되지 않음): 마지막 요청 이후 요청한 페이지는 수정되지 않았다. 서버가
  /// 이 응답을 표시하면 페이지의 콘텐츠를 표시하지 않는다. 요청자가 마지막으로
  /// 페이지를 요청한 후 페이지가 변경되지 않으면 이 응답(If-Modified-Since HTTP
  /// 헤더라고 함)을 표시하도록 서버를 구성해야 한다.
  NotModified = 304,
  /// (프록시 사용): 요청자는 프록시를 사용하여 요청한 페이지만 액세스할 수
  /// 있다. 서버가 이 응답을 표시하면 요청자가 사용할 프록시를 가리키는 것이기도
  /// 하다.
  UseProxy = 305,
  /// (임시 리다이렉션): 현재 서버가 다른 위치의 페이지로 요청에 응답하고 있지만
  /// 요청자는 향후 요청 시 원래 위치를 계속 사용해야 한다.
  TemporaryRedirect = 307,
  /// (영구 리다이렉션, RFC에서 실험적으로 승인됨)
  PermanentRedirect = 308,

  // 4×× Client Error
  // 클래스의 상태 코드는 클라이언트에 오류가 있음을 나타낸다.

  /// (잘못된 요청): 서버가 요청의 구문을 인식하지 못했다.
  BadRequest = 400,
  /// (권한 없음): 이 요청은 인증이 필요하다. 서버는 로그인이 필요한 페이지에
  /// 대해 이 요청을 제공할 수 있다.
  Unauthorized = 401,
  /// (결제 필요): 이 요청은 결제가 필요합니다.
  PaymentRequired = 402,
  /// (금지됨): 서버가 요청을 거부하고 있다.
  Forbidden = 403,
  /// (찾을 수 없음): 서버가 요청한 페이지를 찾을 수 없다. 예를 들어 서버에
  /// 존재하지 않는 페이지에 대한 요청이 있을 경우 서버는 이 코드를 제공한다.
  NotFound = 404,
  /// (허용되지 않는 방법): 요청에 지정된 방법을 사용할 수 없다. (POST, GET,
  /// PUT, DELETE, ...)
  MethodNotAllowed = 405,
  /// (허용되지 않음): 요청한 페이지가 요청한 콘텐츠 특성으로 응답할 수 없다.
  NotAcceptable = 406,
  /// (프록시 인증 필요): 이 상태 코드는 401(권한 없음)과 비슷하지만 요청자가
  /// 프록시를 사용하여 인증해야 한다. 서버가 이 응답을 표시하면 요청자가 사용할
  /// 프록시를 가리키는 것이기도 한다.
  ProxyAuthenticationRequired = 407,
  /// (요청 시간초과): 서버의 요청 대기가 시간을 초과하였다.
  RequestTimeout = 408,
  /// (충돌): 서버가 요청을 수행하는 중에 충돌이 발생했다. 서버는 응답할 때
  /// 충돌에 대한 정보를 포함해야 한다. 서버는 PUT 요청과 충돌하는 PUT 요청에
  /// 대한 응답으로 이 코드를 요청 간 차이점 목록과 함께 표시해야 한다.
  Conflict = 409,
  /// (사라짐): 서버는 요청한 리소스가 영구적으로 삭제되었을 때 이 응답을
  /// 표시한다. 404(찾을 수 없음) 코드와 비슷하며 이전에 있었지만 더 이상
  /// 존재하지 않는 리소스에 대해 404 대신 사용하기도 한다. 리소스가 영구적으로
  /// 이동된 경우 301을 사용하여 리소스의 새 위치를 지정해야 한다.
  Gone = 410,
  /// (길이 필요): 서버는 유효한 콘텐츠 길이 헤더 입력란 없이는 요청을 수락하지
  /// 않는다.
  LengthRequired = 411,
  /// (사전조건 실패): 서버가 요청자가 요청 시 부과한 사전조건을 만족하지
  /// 않는다.
  PreconditionFailed = 412,
  /// (요청 속성이 너무 큼): 요청이 너무 커서 서버가 처리할 수 없다.
  PayloadTooLarge = 413,
  /// (요청 URI가 너무 긺): 요청 URI(일반적으로 URL)가 너무 길어 서버가 처리할
  /// 수 없다.
  RequestURITooLong = 414,
  /// (지원되지 않는 미디어 유형): 요청이 요청한 페이지에서 지원하지 않는
  /// 형식으로 되어 있다.
  UnsupportedMediaType = 415,
  /// (처리할 수 없는 요청범위): 요청이 페이지에서 처리할 수 없는 범위에
  /// 해당되는 경우 서버는 이 상태 코드를 표시한다.
  RequestedRangeNotSatisfiable = 416,
  /// (예상 실패): 서버는 Expect 요청 헤더 입력란의 요구사항을 만족할 수 없다.
  ExpectationFailed = 417,
  /// RFC 2324
  ImATeapot = 418,
  /// ???
  MisdirectedRequest = 421,
  /// (처리할 수 없는 엔티티, WebDAV; RFC 4918)
  UnprocessableEntity = 422,
  /// (잠김,WebDAV; RFC 4918)
  Locked = 423,
  /// (실패된 의존성, WebDAV; RFC 4918)
  FailedDependency = 424,
  /// (실패된 의존성, WebDAV; RFC 4918)
  UpgradeRequired = 426,
  /// (전제조건 필요, RFC 6585)
  PreconditionRequired = 428,
  /// (너무 많은 요청, RFC 6585)
  TooManyRequests = 429,
  /// (요청 헤더 필드가 너무 큼, RFC 6585)
  RequestHeaderFieldsTooLarge = 431,
  /// (응답 없음, Nginx)
  ConnectionClosedWithoutResponse = 444,
  /// (법적인 이유로 이용 불가, 인터넷 초안)
  UnavailableForLegalReasons = 451,
  /// (클라이언트가 요청을 닫음, Nginx)
  ClientClosedRequest = 499,

  // 5×× Server Error

  /// (내부 서버 오류): 서버에 오류가 발생하여 요청을 수행할 수 없다.
  InternalServerError = 500,
  /// (구현되지 않음): 서버에 요청을 수행할 수 있는 기능이 없다. 예를 들어
  /// 서버가 요청 메소드를 인식하지 못할 때 이 코드를 표시한다.
  NotImplemented = 501,
  /// (불량 게이트웨이): 서버가 게이트웨이나 프록시 역할을 하고 있거나 또는
  /// 업스트림 서버에서 잘못된 응답을 받았다.
  BadGateway = 502,
  /// (서비스를 사용할 수 없음): 서버가 오버로드되었거나 유지관리를 위해
  /// 다운되었기 때문에 현재 서버를 사용할 수 없다. 이는 대개 일시적인 상태이다.
  ServiceUnavailable = 503,
  /// (게이트웨이 시간초과): 서버가 게이트웨이나 프록시 역할을 하고 있거나 또는
  /// 업스트림 서버에서 제때 요청을 받지 못했다.
  GatewayTimeout = 504,
  /// (HTTP 버전이 지원되지 않음): 서버가 요청에 사용된 HTTP 프로토콜 버전을
  /// 지원하지 않는다.
  HTTPVersionNotSupported = 505,
  /// (Variant Also Negotiates, RFC 2295)
  VariantAlsoNegotiates = 506,
  /// (용량 부족, WebDAV; RFC 4918)
  InsufficientStorage = 507,
  /// (루프 감지됨, WebDAV; RFC 5842)
  LoopDetected = 508,
  /// (확장되지 않음, RFC 2774)
  NotExtended = 510,
  /// (네트워크 인증 필요, RFC 6585)
  NetworkAuthenticationRequired = 511,
  /// (네트워크 읽기 시간초과 오류, 알 수 없음)
  NetworkConnectTimeoutError = 599,
};

FUN_HTTP_API String GetStatusCodeText(int32 code);

inline String GetStatusCodeText(StatusCode code) {
  return GetStatusCodeText(int32(code));
}

}  // namespace http
}  // namespace fun
