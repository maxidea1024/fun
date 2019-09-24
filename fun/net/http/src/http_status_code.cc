#include "fun/http/status_code.h"

namespace fun {
namespace http {

String GetStatusCodeText(int32 code) {
  // See IANA HTTP status code assignment:
  // http://www.iana.org/assignments/http-status-codes/http-status-codes.xhtml

  switch (code) {
    // RFC2616 Section 10.1 - Informational 1xx
    case 100:
      return StringLiteral("Continue");  // RFC2616 Section 10.1.1
    case 101:
      return StringLiteral("Switching Protocols");  // RFC2616 Section 10.1.2
    case 102:
      return StringLiteral("Processing");  // RFC2518 Section 10.1

    // RFC2616 Section 10.2 - Successful 2xx
    case 200:
      return StringLiteral("OK");  // RFC2616 Section 10.2.1
    case 201:
      return StringLiteral("Created");  // RFC2616 Section 10.2.2
    case 202:
      return StringLiteral("Accepted");  // RFC2616 Section 10.2.3
    case 203:
      return StringLiteral(
          "Non-Authoritative Information");  // RFC2616 Section 10.2.4
    case 204:
      return StringLiteral("No Content");  // RFC2616 Section 10.2.5
    case 205:
      return StringLiteral("Reset Content");  // RFC2616 Section 10.2.6
    case 206:
      return StringLiteral("Partial Content");  // RFC2616 Section 10.2.7
    case 207:
      return StringLiteral(
          "Multi-Status");  // RFC2518 Section 10.2, RFC4918 Section 11.1
    case 208:
      return StringLiteral("Already Reported");  // RFC5842 Section 7.1

    case 226:
      return StringLiteral("IM Used");  // RFC3229 Section 10.4.1

    // RFC2616 Section 10.3 - Redirection 3xx
    case 300:
      return StringLiteral("Multiple choices");  // RFC2616 Section 10.3.1
    case 301:
      return StringLiteral("Moved Permanently");  // RFC2616 Section 10.3.2
    case 302:
      return StringLiteral("Found");  // RFC2616 Section 10.3.3
    case 303:
      return StringLiteral("See Other");  // RFC2616 Section 10.3.4
    case 304:
      return StringLiteral("Not Modified");  // RFC2616 Section 10.3.5
    case 305:
      return StringLiteral("Use Proxy");  // RFC2616 Section 10.3.6
    case 307:
      return StringLiteral("Temporary Redirect");  // RFC2616 Section 10.3.8
    case 308:
      return StringLiteral("Permanent Redirect");  // RFC7238 Section 3

    // RFC2616 Section 10.4 - Client Error 4xx
    case 400:
      return StringLiteral("Bad request");  // RFC2616 Section 10.4.1
    case 401:
      return StringLiteral("Unauthorized");  // RFC2616 Section 10.4.2
    case 402:
      return StringLiteral("Payment Required");  // RFC2616 Section 10.4.3
    case 403:
      return StringLiteral("Forbidden");  // RFC2616 Section 10.4.4
    case 404:
      return StringLiteral("Not Found");  // RFC2616 Section 10.4.5
    case 405:
      return StringLiteral("method Not Allowed");  // RFC2616 Section 10.4.6
    case 406:
      return StringLiteral("Not Acceptable");  // RFC2616 Section 10.4.7
    case 407:
      return StringLiteral(
          "Proxy Authentication Required");  // RFC2616 Section 10.4.8
    case 408:
      return StringLiteral("Request Time-out");  // RFC2616 Section 10.4.9
    case 409:
      return StringLiteral("Conflict");  // RFC2616 Section 10.4.10
    case 410:
      return StringLiteral("Gone");  // RFC2616 Section 10.4.11
    case 411:
      return StringLiteral("Length Required");  // RFC2616 Section 10.4.12
    case 412:
      return StringLiteral("Precondition Failed");  // RFC2616 Section 10.4.13
    case 413:
      return StringLiteral(
          "Request Entity Too Large");  // RFC2616 Section 10.4.14
    case 414:
      return StringLiteral("Request-URI Too Large");  // RFC2616 Section 10.4.15
    case 415:
      return StringLiteral(
          "Unsupported Media Type");  // RFC2616 Section 10.4.16
    case 416:
      return StringLiteral(
          "Requested Range Not Satisfiable");  // RFC2616 Section 10.4.17
    case 417:
      return StringLiteral("Expectation Failed");  // RFC2616 Section 10.4.18

    case 421:
      return StringLiteral("Misdirected Request");  // RFC7540 Section 9.1.2
    case 422:
      return StringLiteral("Unproccessable Entity");  // RFC2518 Section 10.3,
                                                      // RFC4918 Section 11.2
    case 423:
      return StringLiteral(
          "Locked");  // RFC2518 Section 10.4, RFC4918 Section 11.3
    case 424:
      return StringLiteral(
          "Failed Dependency");  // RFC2518 Section 10.5, RFC4918 Section 11.4

    case 426:
      return StringLiteral("Upgrade Required");  // RFC 2817 Section 4

    case 428:
      return StringLiteral("Precondition Required");  // RFC 6585, Section 3
    case 429:
      return StringLiteral("Too Many Requests");  // RFC 6585, Section 4

    case 431:
      return StringLiteral(
          "Request Header Fields Too Large");  // RFC 6585, Section 5

    case 451:
      return StringLiteral("Unavailable For Legal Reasons");  // draft-tbray-http-legally-restricted-status-05,
                                                              // Section 3

    // RFC2616 Section 10.5 - Server Error 5xx
    case 500:
      return StringLiteral("Internal Server Error");  // RFC2616 Section 10.5.1
    case 501:
      return StringLiteral("Not Implemented");  // RFC2616 Section 10.5.2
    case 502:
      return StringLiteral("Bad Gateway");  // RFC2616 Section 10.5.3
    case 503:
      return StringLiteral("Service Unavailable");  // RFC2616 Section 10.5.4
    case 504:
      return StringLiteral("Gateway Time-out");  // RFC2616 Section 10.5.5
    case 505:
      return StringLiteral(
          "HTTP Version Not Supported");  // RFC2616 Section 10.5.6
    case 506:
      return StringLiteral("Variant Also Negotiates");  // RFC 2295, Section 8.1
    case 507:
      return StringLiteral("Insufficient Storage");  // RFC2518 Section 10.6,
                                                     // RFC4918 Section 11.5
    case 508:
      return StringLiteral("Loop Detected");  // RFC5842 Section 7.1

    case 510:
      return StringLiteral("Not Extended");  // RFC 2774, Section 7
    case 511:
      return StringLiteral(
          "Network Authentication Required");  // RFC 6585, Section 6

    // Other status codes, not shown in the IANA HTTP status code assignment.
    // E.g., "de facto" standards due to common use, ...
    case 418:
      return StringLiteral("I Am A Teapot");  // RFC2324 Section 2.3.2
    case 419:
      return StringLiteral("Authentication Timedout");  // common use
    case 420:
      return StringLiteral("Enhance Your Calm");  // common use
    case 440:
      return StringLiteral("Login Timeout");  // common use
    case 509:
      return StringLiteral("Bandwidth Limit Exceeded");  // common use

    default:
      // Return at least a category according to RFC 2616 Section 10.

      if (code >= 100 && code < 200) { // unknown informational status code
        return StringLiteral("Information");
      } else if (code >= 200 && code < 300) { // unknown success code
        return StringLiteral("Success");
      } else if (code >= 300 && code < 400) { // unknown redirection code
        return StringLiteral("Redirection");
      } else if (code >= 400 && code < 500) {  // unknown request error code
        return StringLiteral("Client Error");
      } else if (code >= 500 && code < 600) { // unknown server error code
        return StringLiteral("Server Error");
      }

      // response code not even within reasonable range
      return StringLiteral("");
  }
}

}  // namespace http
}  // namespace fun
