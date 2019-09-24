#pragma once

#include "fun/base/base.h"
#include "fun/base/path.h"
#include "fun/base/string/string.h"
#include "fun/base/container/array.h"

#ifdef _MSC_VER
#undef SetPort //#define SetPort SetPortW in winspool.h
#endif

namespace fun {

/**
 * A Uniform Resource Identifier, as specified in RFC 3986.
 *
 * The URI class provides methods for building URIs from their
 * parts, as well as for splitting URIs into their parts.
 * Furthermore, the class provides methods for resolving
 * relative URIs against base URIs.
 *
 * The class automatically performs a few normalizations on
 * all URIs and URI parts passed to it:
 *   * scheme identifiers are converted to lower case
 *   * percent-encoded characters are decoded (except for the query string)
 *   * optionally, dot segments are removed from paths (see Normalize())
 *
 * Note that dealing with query strings requires some precautions, as, internally,
 * query strings are stored in percent-encoded form, while all other parts of the URI
 * are stored in decoded form. While parsing query strings from properly encoded URLs
 * generally works, explicitly setting query strings with SetQuery() or extracting
 * query strings with GetQuery() may lead to ambiguities. See the descriptions of
 * SetQuery(), SetRawQuery(), GetQuery() and GetRawQuery() for more information.
 */
class FUN_BASE_API Uri {
 public:
  struct QueryParameter {
    String name;
    String value;

    QueryParameter() = default;

    QueryParameter(const String& name, const String& value)
      : name(name), value(value) {}
  };

  typedef Array<QueryParameter> QueryParameters;


  /**
   * Creates an empty URI.
   */
  Uri();

  /**
   * Parses an URI from the given string. Throws a
   * UriSyntaxException if the uri is not valid.
   */
  explicit Uri(const String& other);

  /**
   * Parses an URI from the given string. Throws a
   * UriSyntaxException if the uri is not valid.
   */
  explicit Uri(const char* uri);

  /**
   * Creates an URI from its parts.
   */
  Uri(const String& scheme, const String& path_etc);

  /**
   * Creates an URI from its parts.
   */
  Uri(const String& scheme, const String& authority, const String& path_etc);

  /**
   * Creates an URI from its parts.
   */
  Uri(const String& scheme,
      const String& authority,
      const String& path,
      const String& query);

  /**
   * Creates an URI from its parts.
   */
  Uri(const String& scheme,
      const String& authority,
      const String& path,
      const String& query,
      const String& fragment);

  /**
   * Copy constructor. Creates an URI from another one.
   */
  Uri(const Uri& other);

  /**
   * Move constructor.
   */
  Uri(Uri&& other);

  /**
   * Creates an URI from a base URI and a relative URI, according to
   * the algorithm in section 5.2 of RFC 3986.
   */
  Uri(const Uri& base_uri, const String& relative_uri);

  /**
   * Create from Path.
   */
  explicit Uri(const Path& path);

  /**
   * Destroys the URI.
   */
  ~Uri();

  /**
   * Assignment operator.
   */
  Uri& operator = (const Uri& other);

  /**
   * Move operator.
   */
  Uri& operator = (Uri&& other);

  /**
   * Parses and assigns an URI from the given string. Throws a
   * UriSyntaxException if the uri is not valid.
   */
  Uri& operator = (const String& uri);

  /**
   * Parses and assigns an URI from the given string. Throws a
   * UriSyntaxException if the uri is not valid.
   */
  Uri& operator = (const char* uri);

  /**
   * Swaps the URI with another one.
   */
  void Swap(Uri& other);

  /**
   * Clears all parts of the URI.
   */
  void Clear();

  /**
   * Returns a string representation of the URI.
   *
   * Characters in the path, query and fragment parts will be
   * percent-encoded as necessary.
   */
  String ToString() const;

  /**
   * Returns the scheme part of the URI.
   */
  const String& GetScheme() const;

  /**
   * Sets the scheme part of the URI. The given scheme
   * is converted to lower-case.
   *
   * A list of registered URI schemes can be found
   *      at <http://www.iana.org/assignments/uri-schemes>.
   */
  void SetScheme(const String& scheme);

  /**
   * Returns the user-info part of the URI.
   */
  const String& GetUserInfo() const;

  /**
   * Gets the user-info User and Password.
   */
  void GetUserInfo(String& out_user, String& out_password) const;

  /**
   * Sets the user-info part of the URI.
   */
  void SetUserInfo(const String& user_info);

  /**
   * Returns the host part of the URI.
   */
  const String& GetHost() const;

  /**
   * Sets the host part of the URI.
   */
  void SetHost(const String& host);

  /**
   * Returns the port number part of the URI.
   *
   * If no port number (0) has been specified, the
   * well-known port number (e.g., 80 for http) for
   * the given scheme is returned if it is known.
   * Otherwise, 0 is returned.
   */
  int32 GetPort() const;

  /**
   * Sets the port number part of the URI.
   */
  void SetPort(int32 port);

  /**
   * Returns the authority part (userInfo, host and port)
   * of the URI.
   *
   * If the port number is a well-known port
   * number for the given scheme (e.g., 80 for http), it
   * is not included in the authority.
   */
  String GetAuthority() const;

  /**
   * Parses the given authority part for the URI and sets
   * the user-info, host, port components accordingly.
   */
  void SetAuthority(const String& authority);

  /**
   * Returns the decoded path part of the URI.
   */
  const String& GetPath() const;

  /**
   * Sets the path part of the URI.
   */
  void SetPath(const String& path);

  /**
   * Returns the decoded query part of the URI.
   *
   * Note that encoded ampersand characters ('&', "%26")
   * will be decoded, which could cause ambiguities if the query
   * String contains multiple parameters and a parameter name
   * or value contains an ampersand as well.
   * In such a case it's better to use GetRawQuery() or
   * GetQueryParameters().
   */
  String GetQuery() const;

  /**
   * Sets the query part of the URI.
   *
   * The query string will be percent-encoded. If the query
   * already contains percent-encoded characters, these
   * will be double-encoded, which is probably not what's
   * intended by the caller. Furthermore, ampersand ('&')
   * characters in the query will not be encoded. This could
   * lead to ambiguity issues if the query string contains multiple
   * name-value parameters separated by ampersand, and if any
   * name or value also contains an ampersand. In such a
   * case, it's better to use SetRawQuery() with a properly
   * percent-encoded query string, or use AddQueryParameter()
   * or SetQueryParameters(), which take care of appropriate
   * percent encoding of parameter names and values.
   */
  void SetQuery(const String& query);

  /**
   * Adds "param=val" to the query; "param" may not be empty.
   * If val is empty, only '=' is appended to the parameter.
   *
   * In addition to regular encoding, function also encodes '&' and '=',
   * if found in param or val.
   */
  void AddQueryParameter(const String& key, const String& value = "");

  /**
   * Returns the query String in raw form, which usually means percent encoded.
   */
  const String& GetRawQuery() const;

  /**
   * Sets the query part of the URI.
   *
   * The given query string must be properly percent-encoded.
   */
  void SetRawQuery(const String& query);

  /**
   * Returns the decoded query string parameters as a vector of name-value pairs.
   */
  QueryParameters GetQueryParameters() const;

  /**
   * Sets the query part of the URI from a vector
   * of query parameters.
   *
   * Calls AddQueryParameter() for each parameter name and value.
   */
  void SetQueryParameters(const QueryParameters& params);

  /**
   * Returns the fragment part of the URI.
   */
  const String& GetFragment() const;

  /**
   * Sets the fragment part of the URI.
   */
  void SetFragment(const String& fragment);

  /**
   * Sets the path, query and fragment parts of the URI.
   */
  void SetPathEtc(const String& path_etc);

  /**
   * Returns the encoded path, query and fragment parts of the URI.
   */
  String GetPathEtc() const;

  /**
   * Returns the encoded path and query parts of the URI.
   */
  String GetPathAndQuery() const;

  /**
   * Resolves the given relative URI against the base URI.
   * See section 5.2 of RFC 3986 for the algorithm used.
   */
  void Resolve(const String& relative_uri);

  /**
   * Resolves the given relative URI against the base URI.
   * See section 5.2 of RFC 3986 for the algorithm used.
   */
  void Resolve(const Uri& relative_uri);

  /**
   * Returns true if the URI is a relative reference, false otherwise.
   *
   * A relative reference does not contain a scheme identifier.
   * Relative references are usually resolved against an absolute
   * base reference.
   */
  bool IsRelative() const;

  /**
   * Returns true if the URI is a absolute reference, false otherwise.
   */
  bool IsAbsolute() const;

  /**
   * Returns true if the URI is empty, false otherwise.
   */
  bool IsEmpty() const;

  /**
   * A loopback URI is one which refers to a hostname or ip address with meaning only on the local machine.
   */
  bool IsHostLoopback() const;

  /**
   * A wildcard URI is one which refers to all hostnames that Resolve to the local machine (using the * or +)
   */
  bool IsHostWildcard() const;

  /**
   * A portable URI is one with a hostname that can be resolved globally (used from another machine).
   */
  bool IsHostPortable() const;

  /**
   * A default port is one where the port is unspecified, and will be determined by the operating system.
   * The choice of default port may be dictated by the scheme (http -> 80) or not.
   */
  bool IsPortDefault() const;

  /**
   * An "authority" URI is one with only a scheme, optional userinfo, hostname, and (optional) port.
   */
  bool IsAuthority() const;

  /**
   * Returns whether the other URI has the same authority as this one.
   */
  bool HasSameAuthority(const Uri& other) const;

  /**
   * Returns whether the path portion of this URI is empty
   */
  bool IsPathEmpty() const;

  /**
   * Returns true if both URIs are identical, false otherwise.
   *
   * Two URIs are identical if their scheme, authority,
   * path, query and fragment part are identical.
   */
  bool operator == (const Uri& other) const;

  /**
   * Parses the given URI and returns true if both URIs are identical, false otherwise.
   */
  bool operator == (const String& other) const;

  /**
   * Returns true if both URIs are identical, false otherwise.
   */
  bool operator != (const Uri& other) const;

  /**
   * Parses the given URI and returns true if both URIs are identical, false otherwise.
   */
  bool operator != (const String& other) const;

  /**
   * Normalizes the URI by removing all but leading . and .. segments from the path.
   *
   * If the first path segment in a relative path contains a colon (:),
   * such as in a Windows path containing a drive letter, a dot segment (./)
   * is prepended in accordance with section 3.3 of RFC 3986.
   */
  void Normalize();

  /**
   * Places the single path segments (delimited by slashes) into the given vector.
   */
  void GetPathSegments(Array<String>& out_segments);

  /**
   * URI-encodes the given string by escaping reserved and non-ASCII
   * characters. The encoded string is appended to encodedStr.
   */
  static String Encode(const String& str, const String& reserved = "");

  /**
   * URI-decodes the given string by replacing percent-encoded
   * characters with the actual character. The decoded string
   * is appended to decodedStr.
   *
   * When plus_as_space is true, non-encoded plus signs in the query are decoded as spaces.
   * (http://www.w3.org/TR/html401/interact/forms.html#h-17.13.4.1)
   */
  static String Decode(const String& str, bool plus_as_space = false);

 protected:
  /**
   * Returns true if both uri's are equivalent.
   */
  bool Equals(const Uri& other) const;

  /**
   * Returns true if the URI's port number is a well-known one (for example, 80, if the scheme is http).
   */
  bool IsWellKnownPort() const;

  /**
   * Returns the well-known port number for the URI's scheme, or 0 if the port number is not known.
   */
  int32 GetWellKnownPort() const;

  /**
   * Parses and assigns an URI from the given string. Throws a SyntaxException if the uri is not valid.
   */
  void Parse(const String& uri);

  /**
   * Parses and sets the user-info, host and port from the given data.
   */
  void ParseAuthority(const char*& cur, const char* end);

  /**
   * Parses and sets the host and port from the given data.
   */
  void ParseHostAndPort(const char*& cur, const char* end);

  /**
   * Parses and sets the path from the given data.
   */
  void ParsePath(const char*& cur, const char* end);

  /**
   * Parses and sets the path, query and fragment from the given data.
   */
  void ParsePathEtc(const char*& cur, const char* end);

  /**
   * Parses and sets the query from the given data.
   */
  void ParseQuery(const char*& cur, const char* end);

  /**
   * Parses and sets the fragment from the given data.
   */
  void ParseFragment(const char*& cur, const char* end);

  /**
   * Appends a path to the URI's path.
   */
  void MergePath(const String& path);

  /**
   * Removes all dot segments from the path.
   */
  void RemoveDotSegments(bool remove_leading = true);

  /**
   * Places the single path segments (delimited by slashes) into the given vector.
   */
  static void GetPathSegments(const String& path, Array<String>& segments);

  /**
   * Builds the path from the given segments.
   */
  void BuildPath( const Array<String>& segments,
                  bool leading_slash,
                  bool trailing_slash);

  static const String RESERVED_PATH;
  static const String RESERVED_QUERY;
  static const String RESERVED_FRAGMENT;
  static const String ILLEGAL;

 private:
  /** scheme. "http", "https", "ftp" ... */
  String scheme_;
  /** User information(id, password) */
  String user_info_;
  /** Host(server address) */
  String host_;
  /** Host port number */
  int32 port_;
  /** path part */
  String path_;
  /** query part(without '?') */
  String query_;
  /** fragment part(without '#') */
  String fragment_;
};


//
// inlines
//

FUN_ALWAYS_INLINE const String& Uri::GetScheme() const {
  return scheme_;
}

FUN_ALWAYS_INLINE const String& Uri::GetUserInfo() const {
  return user_info_;
}

FUN_ALWAYS_INLINE void Uri::GetUserInfo(String& out_user, String& out_password) const {
  out_user = "";
  out_password = "";

  if (user_info_.Len() > 0) {
    const int32 colon_pos = user_info_.IndexOf(":");
    if (colon_pos != INVALID_INDEX) {
      out_user = user_info_.Mid(0, colon_pos);
      out_password = user_info_.Mid(colon_pos + 1);
    } else {
      out_user = user_info_;
    }
  }
}

FUN_ALWAYS_INLINE const String& Uri::GetHost() const {
  return host_;
}

FUN_ALWAYS_INLINE const String& Uri::GetPath() const {
  return path_;
}

FUN_ALWAYS_INLINE const String& Uri::GetRawQuery() const {
  return query_;
}

FUN_ALWAYS_INLINE const String& Uri::GetFragment() const {
  return fragment_;
}

FUN_ALWAYS_INLINE bool Uri::IsHostLoopback() const {
  // IPv6
  if (host_ == "::1") {
    return true;
  }

  return !IsEmpty() && ((host_ == "localhost") || (host_.Len() > 4 && host_.Mid(0, 4) == "127."));
}

FUN_ALWAYS_INLINE bool Uri::IsHostWildcard() const {
  return !IsEmpty() && (host_ == "*" || host_ == "+");
}

FUN_ALWAYS_INLINE bool Uri::IsHostPortable() const {
  return !(IsEmpty() || IsHostLoopback() || IsHostWildcard());
}

FUN_ALWAYS_INLINE bool Uri::IsPortDefault() const {
  return !IsEmpty() && port_ == 0;
}

FUN_ALWAYS_INLINE bool Uri::IsAuthority() const {
  return !IsEmpty() && IsPathEmpty() && query_.IsEmpty() && fragment_.IsEmpty();
}

FUN_ALWAYS_INLINE bool Uri::HasSameAuthority(const Uri& other) const {
  return !IsEmpty() && GetAuthority() == other.GetAuthority();
}

FUN_ALWAYS_INLINE bool Uri::IsPathEmpty() const {
  return path_.IsEmpty() || path_ == "/";
}

//TODO 없애는게 좋지 아니한가...
FUN_ALWAYS_INLINE void Swap(Uri& lhs, Uri& rhs) {
  lhs.Swap(rhs);
}

} // namespace fun
