#pragma once

#include "fun/http/error.h"
#include "fun/http/types.h"

#include "fun/http/options/cookies.h"

namespace fun {
namespace http {

class Response {
 public:
  int32 status_code;
  Uri uri;
  Headers headers;
  String Body;
  double elapsed;
  Cookies cookies;
  Error error;

  Response() = default;

  inline Response(const int32 status_code)
      : status_code{status_code},
        uri(),
        headers(),
        body(),
        elapsed(0),
        cookies(),
        error() {}

  template <typename BodyType, typename HeadersType, typename UriType,
            typename CookiesType, typename ErrorType>
  inline Response(const int32 status_code, BodyType&& body,
                  HeadersType&& headers, UriType&& uri, const double elapsed,
                  CookiesType&& cookies = Cookies{},
                  ErrorType&& error = Error{})
      : status_code{status_code},
        uri{FORWARD_VA_ARGS(uri)},
        headers{FORWARD_VA_ARGS(headers)},
        body{FORWARD_VA_ARGS(body)},
        elapsed{elapsed},
        cookies{FORWARD_VA_ARGS(cookies)},
        error{FORWARD_VA_ARGS(error)} {}

  /*

  void AddCookie(const String& Key, const String& Value);
  void AddDateHeader(

  inline void SetHeader(const String& Key, const String& Value)
  {
  }



  Response();
  Response(StatusCode status_code, const String& reason);
  Response(Version version, StatusCode status_code, const String& reason);
  Response(StatusCode status_code);
  Response(Version version, StatusCode status_code);

  void SetStatus(StatusCode status_code);
  StatusCode GetStatus() const;

  void SetReason(const String& reason);
  const String& GetReason() const;

  void SetStatusAndReason(StatusCode status_code, const String& reason);
  void SetStatusAndReason(StatusCode status_code);

  void SetDate(const DateTime& date);
  DateTime GetData() const;

  void AddCookie(const Cookie& cookie);
  void GetCookies(Array<Cookie>& out_cookies) const;



  */
};

}  // namespace http
}  // namespace fun
