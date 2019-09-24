//TODO path 구분시 path 앞의 '/' 문자가 생략되는 부분 확인해야함.

#include "fun/base/uri.h"
#include "fun/base/exception.h"
#include "fun/base/string/string.h"

namespace fun {

/*! \class uri

URI(Uniform Resource Identifier) - RFC 3986

  scheme:[//[user:password@]host[:port]][/]path[?query][#fragment]

https://en.wikipedia.org/wiki/Uniform_Resource_Identifier



[EXAMPLE]

The URL literal is:

    http://tester:pwd123@localhost:8080/game/api/v1/sessions/10000000/?offset=0&limit=100


The result of parsing is as follows.

  schema         : http
  user_info      : tester:pwd123
  host           : localhost
  port           : 8080
  authority      : tester:pwd123@localhost:8080
  path           : /game/api/v1/sessions/10000000/
  query          : offset=0&limit=100
  fragment       :
  path_etc       : /game/api/v1/sessions/10000000/?offset=0&limit=100
  path_and_query : /game/api/v1/sessions/10000000/?offset=0&limit=100
  Path_Segments  :
    game
    api
    v1
    sessions
    10000000
  QueryParams   :
    offset=0
    limit=100

*/

const String Uri::RESERVED_PATH = "?#";
const String Uri::RESERVED_QUERY = "?#/";
const String Uri::RESERVED_FRAGMENT = "";
const String Uri::ILLEGAL = "%<>{}|\\\"^`";

Uri::Uri() : port_(0) {}

Uri::Uri(const String& uri) : port_(0) {
  Parse(uri);
}

Uri::Uri(const char* uri) : port_(0) {
  Parse(String(uri));
}

Uri::Uri(const String& scheme, const String& path_etc)
  : scheme_(scheme),
    port_(0) {
  scheme_.MakeLower();

  port_ = GetWellKnownPort();

  const char* b = path_etc.cbegin();
  const char* e = path_etc.cend();
  ParsePathEtc(b, e);
}

Uri::Uri(const String& scheme, const String& authority, const String& path_etc)
  : scheme_(scheme) {
  scheme_.MakeLower();

  const char* b = authority.cbegin();
  const char* e = authority.cend();
  ParseAuthority(b, e);

  b = *path_etc;
  e = *path_etc + path_etc.Len();
  ParsePathEtc(b, e);
}

Uri::Uri( const String& scheme,
          const String& authority,
          const String& path,
          const String& query)
  : scheme_(scheme),
    path_(path),
    query_(query) {
  scheme_.MakeLower();

  const char* b = authority.cbegin();
  const char* e = authority.cend();
  ParseAuthority(b, e);
}

Uri::Uri( const String& scheme,
          const String& authority,
          const String& path,
          const String& query,
          const String& fragment)
  : scheme_(scheme),
    path_(path),
    query_(query),
    fragment_(fragment) {
  scheme_.MakeLower();

  const char* b = authority.cbegin();
  const char* e = authority.cend();
  ParseAuthority(b, e);
}

Uri::Uri(const Uri& other)
  : scheme_(other.scheme_),
    user_info_(other.user_info_),
    host_(other.host_),
    port_(other.port_),
    path_(other.path_),
    query_(other.query_),
    fragment_(other.fragment_) {
}

Uri::Uri(Uri&& other)
  : scheme_(MoveTemp(other.scheme_)),
    user_info_(MoveTemp(other.user_info_)),
    host_(MoveTemp(other.host_)),
    port_(MoveTemp(other.port_)),
    path_(MoveTemp(other.path_)),
    query_(MoveTemp(other.query_)),
    fragment_(MoveTemp(other.fragment_)) {
}

Uri::Uri(const Uri& base_uri, const String& relative_uri)
  : scheme_(base_uri.scheme_),
    user_info_(base_uri.user_info_),
    host_(base_uri.host_),
    port_(base_uri.port_),
    path_(base_uri.path_),
    query_(base_uri.query_),
    fragment_(base_uri.fragment_) {
  Resolve(relative_uri);
}

Uri::Uri(const Path& path)
  : scheme_("file"), port_(0) {
  Path absolute_path(path);
  absolute_path.MakeAbsolute();
  path_ = absolute_path.ToString(Path::PATH_UNIX);
}

Uri::~Uri() {}

Uri& Uri::operator = (const Uri& other) {
  if (FUN_LIKELY(&other != this)) {
    scheme_ = other.scheme_;
    user_info_ = other.user_info_;
    host_ = other.host_;
    port_ = other.port_;
    path_ = other.path_;
    query_ = other.query_;
    fragment_ = other.fragment_;
  }
  return *this;
}

Uri& Uri::operator = (Uri&& other) {
  if (FUN_LIKELY(&other != this)) {
    scheme_ = MoveTemp(other.scheme_);
    user_info_ = MoveTemp(other.user_info_);
    host_ = MoveTemp(other.host_);
    port_ = MoveTemp(other.port_);
    path_ = MoveTemp(other.path_);
    query_ = MoveTemp(other.query_);
    fragment_ = MoveTemp(other.fragment_);
  }
  return *this;
}

Uri& Uri::operator = (const String& uri) {
  Clear();
  Parse(uri);
  return *this;
}

Uri& Uri::operator = (const char* uri) {
  Clear();
  Parse(String(uri));
  return *this;
}

void Uri::Swap(Uri& other) {
  fun::Swap(scheme_, other.scheme_);
  fun::Swap(user_info_, other.user_info_);
  fun::Swap(host_, other.host_);
  fun::Swap(port_, other.port_);
  fun::Swap(path_, other.path_);
  fun::Swap(query_, other.query_);
  fun::Swap(fragment_, other.fragment_);
}

void Uri::Clear() {
  scheme_.Clear();
  user_info_.Clear();
  host_.Clear();
  port_ = 0;
  path_.Clear();
  query_.Clear();
  fragment_.Clear();
}

String Uri::ToString() const {
  String uri;
  if (IsRelative()) {
    uri = Encode(path_, RESERVED_PATH);
  } else {
    uri = scheme_; // http, https
    uri << ':';

    const String auth = GetAuthority();
    if (!auth.IsEmpty() || scheme_ == "file") {
      uri << "//";
      uri << auth;
    }

    if (!path_.IsEmpty()) {
      if (!auth.IsEmpty() && path_[0] != '/') {
        uri << '/';
      }

      uri << Encode(path_, RESERVED_PATH);
    } else if (!query_.IsEmpty() || !fragment_.IsEmpty()) {
      uri << '/';
    }
  }

  if (!query_.IsEmpty()) {
    uri << '?';
    uri << query_;
  }

  if (!fragment_.IsEmpty()) {
    uri << '#';
    uri << Encode(fragment_, RESERVED_FRAGMENT);
  }

  return uri;
}

void Uri::SetScheme(const String& scheme) {
  scheme_ = scheme;
  scheme_.MakeLower();

  if (port_ == 0) {
    port_ = GetWellKnownPort();
  }
}

void Uri::SetUserInfo(const String& user_info) {
  user_info_.Clear();
  user_info_ = Decode(user_info);
}

void Uri::SetHost(const String& host) {
  host_ = host;
}

int32 Uri::GetPort() const {
  return port_ == 0 ? GetWellKnownPort() : port_;
}

void Uri::SetPort(int32 port) {
  fun_check(port >= 0 && port <= 65535);
  port_ = port;
}

String Uri::GetAuthority() const {
  String auth;

  if (!user_info_.IsEmpty()) {
    auth << user_info_;
    auth << '@';
  }

  if (host_.Contains(':')) {
    auth << '[';
    auth << host_;
    auth << ']';
  } else {
    auth << host_;
  }

  if (port_ && !IsWellKnownPort()) {
    auth << ':';
    auth << String::FromNumber(port_);
  }

  return auth;
}

void Uri::SetAuthority(const String& authority) {
  user_info_.Clear();
  host_.Clear();
  port_ = 0;

  const char* b = authority.cbegin();
  const char* e = authority.cend();
  ParseAuthority(b, e);
}

void Uri::SetPath(const String& path) {
  path_.Clear();
  path_ = Decode(path);
}

void Uri::SetRawQuery(const String& query) {
  query_ = query;
}

void Uri::SetQuery(const String& query) {
  query_.Clear();
  query_ = Encode(query, RESERVED_QUERY);
}

void Uri::AddQueryParameter(const String& param, const String& value) {
  String reserved(RESERVED_QUERY);

  reserved << "=&";

  if (!query_.IsEmpty()) {
    query_ << '&';
  }

  //FIXME 이게 왜 안되는걸까?
  // -> 연산자 정의시 const 한정자를 붙인게 있고 아닌게 있어서 컴파일 오류가 있었음.
  //query_ += Encode(param, reserved) + '=' + Encode(value, reserved);

  query_ << Encode(param, reserved);
  query_ << '=';
  query_ << Encode(value, reserved);
}

String Uri::GetQuery() const {
  String query;
  query = Decode(query_);
  return query;
}

Uri::QueryParameters Uri::GetQueryParameters() const {
  QueryParameters result;

  const char* cur = query_.cbegin();
  const char* end = query_.cend();
  while (cur != end) {
    String name;
    String value;
    while (cur != end && *cur != '=' && *cur != '&') {
      if (*cur == '+') {
        name << ' ';
      } else {
        name << *cur;
      }
      ++cur;
    }

    if (cur != end && *cur == '=') {
      ++cur;
      while (cur != end && *cur != '&') {
        if (*cur == '+') {
          value << ' ';
        } else {
          value << *cur;
        }
        ++cur;
      }
    }

    String decoded_name = Uri::Decode(name);
    String decoded_value = Uri::Decode(value);
    result.Add(QueryParameter(decoded_name, decoded_value));

    if (cur != end && *cur == '&') {
      ++cur;
    }
  }

  return result;
}

void Uri::SetQueryParameters(const QueryParameters& params) {
  query_.Clear();

  for (const auto& param : params) {
    AddQueryParameter(param.name, param.value);
  }
}

void Uri::SetFragment(const String& fragment) {
  fragment_.Clear();
  fragment_ = Decode(fragment);
}

void Uri::SetPathEtc(const String& path_etc) {
  path_.Clear();
  query_.Clear();
  fragment_.Clear();

  const char* b = path_etc.cbegin();
  const char* e = path_etc.cend();
  ParsePathEtc(b, e);
}

String Uri::GetPathEtc() const {
  String path_etc;
  path_etc = Encode(path_, RESERVED_PATH);

  if (!query_.IsEmpty()) {
    path_etc << '?';
    path_etc << query_;
  }

  if (!fragment_.IsEmpty()) {
    path_etc << '#';
    path_etc = Encode(fragment_, RESERVED_FRAGMENT);
  }

  return path_etc;
}

String Uri::GetPathAndQuery() const
{
  String path_and_query;
  path_and_query = Encode(path_, RESERVED_PATH);

  if (!query_.IsEmpty()) {
    path_and_query << '?';
    path_and_query << query_;
  }

  return path_and_query;
}

void Uri::Resolve(const String& relative_uri) {
  Uri parsed_uri(relative_uri);
  Resolve(parsed_uri);
}

void Uri::Resolve(const Uri& relative_uri) {
  if (!relative_uri.scheme_.IsEmpty()) {
    scheme_ = relative_uri.scheme_;
    user_info_ = relative_uri.user_info_;
    host_ = relative_uri.host_;
    port_ = relative_uri.port_;
    path_ = relative_uri.path_;
    query_ = relative_uri.query_;
    RemoveDotSegments();
  } else {
    if (!relative_uri.host_.IsEmpty()) {
      user_info_ = relative_uri.user_info_;
      host_ = relative_uri.host_;
      port_ = relative_uri.port_;
      path_ = relative_uri.path_;
      query_ = relative_uri.query_;
      RemoveDotSegments();
    } else {
      if (relative_uri.path_.IsEmpty()) {
        if (!relative_uri.query_.IsEmpty()) {
          query_ = relative_uri.query_;
        }
      } else {
        if (relative_uri.path_[0] == '/') {
          path_ = relative_uri.path_;
          RemoveDotSegments();
        } else {
          MergePath(relative_uri.path_);
        }

        query_ = relative_uri.query_;
      }
    }
  }

  fragment_ = relative_uri.fragment_;
}

bool Uri::IsRelative() const {
  return scheme_.IsEmpty();
}

bool Uri::IsAbsolute() const {
  return !scheme_.IsEmpty();
}

bool Uri::IsEmpty() const {
  return  scheme_.IsEmpty() &&
          host_.IsEmpty() &&
          path_.IsEmpty() &&
          query_.IsEmpty() &&
          fragment_.IsEmpty();
}

bool Uri::operator == (const Uri& other) const {
  return Equals(other);
}

bool Uri::operator == (const String& uri) const {
  Uri parsed_uri(uri);
  return Equals(parsed_uri);
}

bool Uri::operator != (const Uri& other) const {
  return !Equals(other);
}

bool Uri::operator != (const String& uri) const {
  Uri parsed_uri(uri);
  return !Equals(parsed_uri);
}

bool Uri::Equals(const Uri& other) const {
  return  scheme_ == other.scheme_ &&
          user_info_ == other.user_info_ &&
          host_ == other.host_ &&
          port_ == other.port_ &&
          path_ == other.path_ &&
          query_ == other.query_ &&
          fragment_ == other.fragment_;
}

void Uri::Normalize() {
  RemoveDotSegments(!IsRelative());
}

void Uri::RemoveDotSegments(bool remove_leading) {
  if (path_.IsEmpty()) {
    return;
  }

  const bool leading_slash = path_.LastOr() == '/';
  const bool trailing_slash = path_.LastOr() == '/';
  Array<String> segments;
  Array<String> normalized_segments;
  GetPathSegments(segments);
  for (const auto& segment : segments) {
    if (segment == "..") {
      if (normalized_segments.Count() > 0) {
        if (normalized_segments.Last() == "..") {
          normalized_segments.Add(segment);
        } else {
          normalized_segments.RemoveLast();
        }
      } else if (!remove_leading) {
        normalized_segments.Add(segment);
      }
    } else if (segment != ".") {
      normalized_segments.Add(segment);
    }
  }

  BuildPath(normalized_segments, leading_slash, trailing_slash);
}

void Uri::GetPathSegments(Array<String>& segments) {
  GetPathSegments(path_, segments);
}

void Uri::GetPathSegments(const String& path, Array<String>& segments) {
  const char* cur = path.cbegin();
  const char* end = path.cend();
  String segment;
  while (cur != end) {
    if (*cur == '/') {
      if (!segment.IsEmpty()) {
        segments.Add(segment);
        segment.Clear();
      }
    } else {
      segment << *cur;
    }
    ++cur;
  }

  if (!segment.IsEmpty()) {
    segments.Add(segment);
  }
}

String Uri::Encode(const String& str, const String& reserved) {
  String encoded_str;

  const char* cur = str.cbegin();
  const char* end = str.cend();
  for (; cur != end; ++cur) {
    const int32 c = *cur;

    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '-' || c == '_' ||
        c == '.' || c == '~') {
      encoded_str << char(c);
    } else if (c <= 0x20 || c >= 0x7F || ILLEGAL.Contains(char(c)) || reserved.Contains(char(c))) {
      encoded_str << '%';
      encoded_str << String::Format("%02X", (uint32)(uint8)c);
    } else {
      encoded_str << char(c);
    }
  }

  return encoded_str;
}

String Uri::Decode(const String& str, bool plus_as_space) {
  String decoded_str;

  bool is_in_query = false;
  const char* cur = str.begin();
  const char* end = str.cend();
  while (cur != end) {
    int32 c = *cur++;

    if (c == '?') {
      is_in_query = true;
    }

    // spaces may be encoded as plus signs in the query
    if (is_in_query && plus_as_space && c == '+') {
      c = ' ';
    } else if (c == '%') {
      if (cur == end) {
        throw UriSyntaxException(AsciiString("uri encoding: no hex digit following percent sign"), str);
      }

      const int32 hi = *cur++;
      if (cur == end) {
        throw UriSyntaxException(AsciiString("uri encoding: two hex digits must follow percent sign"), str);
      }

      const int32 lo = *cur++;
      if (hi >= '0' && hi <= '9') {
        c = hi - '0';
      } else if (hi >= 'A' && hi <= 'F') {
        c = hi - 'A' + 10;
      } else if (hi >= 'a' && hi <= 'f') {
        c = hi - 'a' + 10;
      } else {
        throw UriSyntaxException(AsciiString("uri encoding: not a hex digit"));
      }
      c *= 16;
      if (lo >= '0' && lo <= '9') {
        c += lo - '0';
      } else if (lo >= 'A' && lo <= 'F') {
        c += lo - 'A' + 10;
      } else if (lo >= 'a' && lo <= 'f') {
        c += lo - 'a' + 10;
      } else {
        throw UriSyntaxException(AsciiString("uri encoding: not a hex digit"));
      }
    }

    decoded_str << char(c);
  }

  return decoded_str;
}

bool Uri::IsWellKnownPort() const {
  return port_ == GetWellKnownPort();
}

int32 Uri::GetWellKnownPort() const {
  if (scheme_ == "ftp") {
    return 21;
  } else if (scheme_ == "ssh") {
    return 22;
  } else if (scheme_ == "telnet") {
    return 23;
  } else if (scheme_ == "http" || scheme_ == "ws") {
    return 80;
  } else if (scheme_ == "nntp") {
    return 119;
  } else if (scheme_ == "ldap") {
    return 389;
  } else if (scheme_ == "https" || scheme_ == "wss") {
    return 443;
  } else if (scheme_ == "rtsp") {
    return 554;
  } else if (scheme_ == "sip") {
    return 5060;
  } else if (scheme_ == "sips") {
    return 5061;
  } else if (scheme_ == "xmpp") {
    return 5222;
  } else {
    return 0;
  }
}

void Uri::Parse(const String& uri) {
  const char* cur = uri.cbegin();
  const char* end = uri.cend();
  if (cur == end) {
    return;
  }

  if (*cur != '/' && *cur != '.' && *cur != '?' && *cur != '#') {
    String scheme;
    while (cur != end && *cur != ':' && *cur != '?' && *cur != '#' && *cur != '/') {
      scheme << *cur++;
    }

    if (cur != end && *cur == ':') {
      ++cur;
      if (cur == end) {
        throw UriSyntaxException(AsciiString("uri scheme must be followed by authority or path"), uri);
      }

      SetScheme(scheme);

      if (*cur == '/') {
        ++cur;
        if (cur != end && *cur == '/') {
          ++cur;
          ParseAuthority(cur, end);
        } else {
          --cur;
        }
      }

      ParsePathEtc(cur, end);
    } else {
      cur = *uri;
      ParsePathEtc(cur, end);
    }
  } else {
    ParsePathEtc(cur, end);
  }
}

void Uri::ParseAuthority(const char*& cur, const char* end) {
  String user_info;
  String part;
  while (cur != end && *cur != '/' && *cur != '?' && *cur != '#') {
    if (*cur == '@') {
      user_info = part;
      part.Clear();
    } else {
      part << *cur;
    }
    ++cur;
  }

  const char* part_begin = part.cbegin();
  const char* part_end = part.cend();
  ParseHostAndPort(part_begin, part_end);
  user_info_ = user_info;
}

void Uri::ParseHostAndPort(const char*& cur, const char* end) {
  if (cur == end) {
    return;
  }

  String host;
  if (*cur == '[') {
    // IPv6 address
    ++cur;
    while (cur != end && *cur != ']') {
      host << *cur++;
    }
    if (cur == end) {
      throw UriSyntaxException(AsciiString("unterminated IPv6 address"));
    }
    ++cur;
  } else {
    while (cur != end && *cur != ':') {
      host << *cur++;
    }
  }

  if (cur != end && *cur == ':') {
    ++cur;

    String port_str;
    while (cur != end) {
      port_str << *cur++;
    }

    if (!port_str.IsEmpty()) {
      bool ok = false;
      const int32 port_no = port_str.ToInt32(&ok);
      if (ok && port_no > 0 && port_no < 65536) {
        port_ = port_no;
      } else {
        throw UriSyntaxException(AsciiString("bad or invalid port number"), port_str);
      }
    } else {
      port_ = GetWellKnownPort();
    }
  } else {
    port_ = GetWellKnownPort();
  }

  host_ = host;
  host_.MakeLower();
}

void Uri::ParsePath(const char*& cur, const char* end) {
  String path;
  while (cur != end && *cur != '?' && *cur != '#') {
    path << *cur++;
  }

  path_ = Decode(path);
}

void Uri::ParsePathEtc(const char*& cur, const char* end) {
  if (cur == end) {
    return;
  }

  if (*cur != '?' && *cur != '#') {
    ParsePath(cur, end);
  }

  if (cur != end && *cur == '?') {
    ++cur;
    ParseQuery(cur, end);
  }

  if (cur != end && *cur == '#') {
    ++cur;
    ParseFragment(cur, end);
  }
}

void Uri::ParseQuery(const char*& cur, const char* end) {
  query_.Clear();

  while (cur != end && *cur != '#') {
    query_ << *cur++;
  }
}

void Uri::ParseFragment(const char*& cur, const char* end) {
  String fragment;
  while (cur != end) {
    fragment << *cur++;
  }

  fragment_ = Decode(fragment);
}

void Uri::MergePath(const String& path) {
  Array<String> segments;
  Array<String> normalized_segments;
  bool add_leading_slash = false;
  if (!path_.IsEmpty()) {
    GetPathSegments(segments);
    const bool ends_with_slash = path_.LastOr() == '/';
    if (!ends_with_slash && segments.Count() > 0) {
      segments.RemoveLast();
    }
    add_leading_slash = path_[0] == '/';
  }
  GetPathSegments(path, segments);

  add_leading_slash = add_leading_slash || (!path.IsEmpty() && path.First() == '/');
  const bool has_trailing_slash = (!path.IsEmpty() && path.Last() == '/');
  bool add_trailing_slash = false;
  for (const auto& segment : segments) {
    if (segment == "..") {
      add_trailing_slash = true;
      if (normalized_segments.Count() > 0) {
        normalized_segments.RemoveLast();
      }
    } else if (segment != ".") {
      add_trailing_slash = false;
      normalized_segments.Add(segment);
    } else {
      add_trailing_slash = true;
    }
  }

  BuildPath(normalized_segments, add_leading_slash, has_trailing_slash || add_trailing_slash);
}

void Uri::BuildPath(const Array<String>& segments, bool leading_slash, bool trailing_slash) {
  path_.Clear();

  bool first = true;
  for (const auto& segment : segments) {
    if (first) {
      first = false;
      if (leading_slash) {
        path_ << '/';
      } else if (scheme_.IsEmpty() && segment.Contains(':')) {
        path_ << "./";
      }
    } else {
      path_ << '/';
    }

    path_ << segment;
  }

  if (trailing_slash) {
    path_ << '/';
  }
}

} // namespace fun
