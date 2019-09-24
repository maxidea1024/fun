#pragma once

#include "fun/http/http.h"
#include "fun/http/options/auth.h"
#include "fun/http/options/body.h"
#include "fun/http/options/cookies.h"
#include "fun/http/options/digest.h"
#include "fun/http/options/low_speed.h"
#include "fun/http/options/max_redirects.h"
#include "fun/http/options/multipart.h"
#include "fun/http/options/parameters.h"
#include "fun/http/options/payload.h"
#include "fun/http/options/proxies.h"
#include "fun/http/options/redirect.h"
#include "fun/http/options/timeout.h"
#include "fun/http/options/uri.h"
#include "fun/http/options/verify_ssl.h"
#include "fun/http/types.h"

#include "fun/http/response.h"

namespace fun {
namespace http {

class FUN_HTTP_API Session : public Noncopyable {
 public:
  Session();
  virtual ~Session();

  // Set options
  void SetUri(const Uri& uri);
  void SetUri(const ByteArray& uri);
  void SetParameters(const Parameters& prameters);
  void SetParameters(Parameters&& prameters);
  void SetHeaders(const Headers& headers);
  void SetTimeout(const Timeout& timeout);
  void SetAuth(const Auth& auth);
  void SetDigest(const Digest& auth);
  void SetPayload(Payload&& payload);
  void SetPayload(const Payload& payload);
  void SetProxies(Proxies&& proxies);
  void SetProxies(const Proxies& proxies);
  void SetMultipart(Multipart&& multipart);
  void SetMultipart(const Multipart& multipart);
  void SetRedirect(const Redirect& redirect);
  void SetMaxRedirects(const MaxRedirects& max_redirects);
  void SetCookies(const Cookies& cookies);
  void SetBody(Body&& body);
  void SetBody(const Body& body);
  void SetBody(const ByteArray& body);
  void SetBody(ByteArray&& body);
  void SetLowSpeed(const LowSpeed& low_speed);
  void SetVerifySsl(const VerifySsl& verify_ssl);

  // Used in templated functions
  void SetOption_INTERNAL(const Uri& uri);
  void SetOption_INTERNAL(const Parameters& prameters);
  void SetOption_INTERNAL(Parameters&& prameters);
  void SetOption_INTERNAL(const Headers& headers);
  void SetOption_INTERNAL(const Timeout& timeout);
  void SetOption_INTERNAL(const Auth& auth);
  void SetOption_INTERNAL(const Digest& digest);
  void SetOption_INTERNAL(Payload&& payload);
  void SetOption_INTERNAL(const Payload& payload);
  void SetOption_INTERNAL(Proxies&& proxies);
  void SetOption_INTERNAL(const Proxies& proxies);
  void SetOption_INTERNAL(Multipart&& multipart);
  void SetOption_INTERNAL(const Multipart& multipart);
  void SetOption_INTERNAL(const Redirect& redirect);
  void SetOption_INTERNAL(const MaxRedirects& max_redirects);
  void SetOption_INTERNAL(const Cookies& cookies);
  void SetOption_INTERNAL(Body&& body);
  void SetOption_INTERNAL(const Body& body);
  void SetOption_INTERNAL(const LowSpeed& low_speed);
  void SetOption_INTERNAL(const VerifySsl& verify_ssl);

  // Methods

  Response Request(Method method);

  Response Delete();
  Response Get();
  Response Head();
  Response Options();
  Response Patch();
  Response Post();
  Response Put();

 private:
  class SessionImpl* impl_;
};

}  // namespace http
}  // namespace fun
