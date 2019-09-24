//TODO code bloating을 줄이는 방법에 대해서 연구해야함..

#pragma once

#include "fun/base/base.h"
#include "fun/base/string/string.h"
#include "fun/base/container/map.h"
#include "fun/base/container/array.h"

namespace fun {

/**
 * A class for working with regular expressions.
 * Implemented using PCRE, the Perl Compatible
 * Regular Expressions library by Philip Hazel
 * (see http://www.pcre.org).
 */
class FUN_BASE_API Regex {
 public:
  /**
   * Some of the following options can only be passed to the constructor;
   * some can be passed only to matching functions, and some can be used
   * everywhere.
   *
   *   * Options marked [ctor] can be passed to the constructor.
   *   * Options marked [match] can be passed to match, Extract, Split and subst.
   *   * Options marked [subst] can be passed to subst.
   *
   * See the PCRE documentation for more information.
   */
  enum Options { // These must match the corresponding options in pcre.h!
    RE_CASELESS        = 0x00000001, /// case insensitive matching (/i) [ctor]
    RE_MULTILINE       = 0x00000002, /// enable multi-line mode; affects ^ and $ (/m) [ctor]
    RE_DOTALL          = 0x00000004, /// dot matches all characters, including newline (/s) [ctor]
    RE_EXTENDED        = 0x00000008, /// totally ignore whitespace (/x) [ctor]
    RE_ANCHORED        = 0x00000010, /// treat pattern as if it starts with a ^ [ctor, match]
    RE_DOLLAR_ENDONLY  = 0x00000020, /// dollar matches end-of-string only, not last newline in string [ctor]
    RE_EXTRA           = 0x00000040, /// enable optional PCRE functionality [ctor]
    RE_NOTBOL          = 0x00000080, /// circumflex does not match beginning of string [match]
    RE_NOTEOL          = 0x00000100, /// $ does not match end of string [match]
    RE_UNGREEDY        = 0x00000200, /// make quantifiers ungreedy [ctor]
    RE_NOTEMPTY        = 0x00000400, /// empty string never matches [match]
    RE_UTF8            = 0x00000800, /// assume pattern and subject is UTF-8 encoded [ctor]
    RE_NO_AUTO_CAPTURE = 0x00001000, /// disable numbered capturing parentheses [ctor, match]
    RE_NO_UTF8_CHECK   = 0x00002000, /// do not check validity of UTF-8 code sequences [match]
    RE_FIRSTLINE       = 0x00040000, /// an  unanchored  pattern  is  required  to  match
                                     /// before  or  at  the  first  newline  in  the subject string,
                                     /// though the matched text may continue over the newline [ctor]
    RE_DUPNAMES        = 0x00080000, /// names used to identify capturing  subpatterns  need not be unique [ctor]
    RE_NEWLINE_CR      = 0x00100000, /// assume newline is CR ('\r'), the default [ctor]
    RE_NEWLINE_LF      = 0x00200000, /// assume newline is LF ('\n') [ctor]
    RE_NEWLINE_CRLF    = 0x00300000, /// assume newline is CRLF ("\r\n") [ctor]
    RE_NEWLINE_ANY     = 0x00400000, /// assume newline is any valid Unicode newline character [ctor]
    RE_NEWLINE_ANYCRLF = 0x00500000, /// assume newline is any of CR, LF, CRLF [ctor]
    RE_GLOBAL          = 0x10000000, /// replace all occurrences (/g) [subst]
    RE_NO_VARS         = 0x20000000  /// treat dollar in replacement string as ordinary character [subst]
  };

  struct MatchInfo {
    int32 offset; /// zero based offset (INVALID_INDEX if subexpr does not match)
    int32 length; /// length of substring
    String name;  /// name of group
  };
  typedef Array<MatchInfo> MatchList;
  typedef Map<int, String> GroupMap;

  /**
   * Creates a regular expression and parses the given pattern.
   * If study is true, the pattern is analyzed and optimized. This
   * is mainly useful if the pattern is used more than once.
   * For a description of the options, please see the PCRE documentation.
   * Throws a RegularExpressionException if the patter cannot be compiled.
   */
  Regex(const String& pattern, int options = 0, bool study = true);

  /**
  Destroys the regular expression.
  */
  ~Regex();

 public:
  /**
  Matches the given subject string against the pattern. Returns the position
  of the first captured substring in match.
  If no part of the subject matches the pattern, match.offset is INVALID_INDEX and
  match.length is 0.
  Throws a RegularExpressionException in case of an error.
  Returns the number of matches.
  */
  int Match(const String& subject, MatchInfo& match, int options = 0) const;

  /**
  Matches the given subject string, starting at offset, against the pattern.
  Returns the position of the captured substring in match.
  If no part of the subject matches the pattern, match.offset is INVALID_INDEX and
  match.length is 0.
  Throws a RegularExpressionException in case of an error.
  Returns the number of matches.
  */
  int Match(const String& subject, int32 offset, MatchInfo& match, int options = 0) const;

  /**
  Matches the given subject string against the pattern.
  The first entry in matches contains the position of the captured substring.
  The following entries identify matching subpatterns. See the PCRE documentation
  for a more detailed explanation.
  If no part of the subject matches the pattern, matches is empty.
  Throws a RegularExpressionException in case of an error.
  Returns the number of matches.
  */
  int Match(const String& subject, int32 offset, MatchList& matches, int options = 0) const;

  /**
  Returns true if and only if the subject matches the regular expression.

  Internally, this method sets the RE_ANCHORED and RE_NOTEMPTY options for
  matching, which means that the empty string will never match and
  the pattern is treated as if it starts with a ^.
  */
  bool Match(const String& subject, int32 offset = 0) const;

  /**
  Returns true if and only if the subject matches the regular expression.
  */
  bool Match(const String& subject, int32 offset, int options) const;

  /**
  Returns true if and only if the subject matches the regular expression.

  Internally, this method sets the RE_ANCHORED and RE_NOTEMPTY options for
  matching, which means that the empty string will never match and
  the pattern is treated as if it starts with a ^.
  */
  bool operator == (const String& subject) const;

  /**
  Returns true if and only if the subject does not match the regular expression.

  Internally, this method sets the RE_ANCHORED and RE_NOTEMPTY options for
  matching, which means that the empty string will never match and
  the pattern is treated as if it starts with a ^.
  */
  bool operator != (const String& subject) const;

  /**
  Matches the given subject string against the pattern.
  Returns the captured string.
  Throws a RegularExpressionException in case of an error.
  Returns the number of matches.
  */
  int Extract(const String& subject, String& str, int options = 0) const;

  /**
  Matches the given subject string, starting at offset, against the pattern.
  Returns the captured string.
  Throws a RegularExpressionException in case of an error.
  Returns the number of matches.
  */
  int Extract(const String& subject, int32 offset, String& str, int options = 0) const;

  /**
  Matches the given subject string against the pattern.
  The first entry in captured is the captured substring.
  The following entries contain substrings matching subpatterns. See the PCRE documentation
  for a more detailed explanation.
  If no part of the subject matches the pattern, captured is empty.
  Throws a RegularExpressionException in case of an error.
  Returns the number of matches.
  */
  int Split(const String& subject, Array<String>& strings, int options = 0) const;

  /**
  Matches the given subject string against the pattern.
  The first entry in captured is the captured substring.
  The following entries contain substrings matching subpatterns. See the PCRE documentation
  for a more detailed explanation.
  If no part of the subject matches the pattern, captured is empty.
  Throws a RegularExpressionException in case of an error.
  Returns the number of matches.
  */
  int Split(const String& subject, int32 offset, Array<String>& strings, int options = 0) const;

  /**
  Substitute in subject all matches of the pattern with replacement.
  If RE_GLOBAL is specified as option, all matches are replaced. Otherwise,
  only the first match is replaced.
  Occurrences of $<n> (for example, $1, $2, ...) in replacement are replaced
  with the corresponding captured string. $0 is the original subject string.
  Returns the number of replaced occurrences.
  */
  int Substitute(String& subject, const String& replacement, int options = 0) const;

  /**
  Substitute in subject all matches of the pattern with replacement,
  starting at offset.
  If RE_GLOBAL is specified as option, all matches are replaced. Otherwise,
  only the first match is replaced.
  Unless RE_NO_VARS is specified, occurrences of $<n> (for example, $0, $1, $2, ... $9)
  in replacement are replaced with the corresponding captured string.
  $0 is the captured substring. $1 ... $n are the substrings matching the subpatterns.
  Returns the number of replaced occurrences.
  */
  int Substitute(String& subject, int32 offset, const String& replacement, int options = 0) const;

  /**
  Matches the given subject string against the regular expression given in pattern,
  using the given options.
  */
  static bool Match(const String& subject, const String& pattern, int options = 0);

 protected:
  int32 SubstituteOne(String& subject, int32 offset, const String& replacement, int options) const;

 private:
  // Note: to avoid a dependency on the pcre.h header the following are
  // declared as void* and casted to the correct type in the implementation file.
  void* pcre_;  // Actual type is pcre*
  void* extra_; // Actual type is struct pcre_extra*

  GroupMap groups_;

  static const int OVEC_SIZE;

  Regex();
  Regex(const Regex&);
  Regex& operator = (const Regex&);
};


//
// inlines
//

inline int Regex::Match(const String& subject, MatchInfo& match, int options) const {
  return Match(subject, 0, match, options);
}

inline int Regex::Split(const String& subject, Array<String>& strings, int options) const {
  return Split(subject, 0, strings, options);
}

inline int Regex::Substitute(String& subject, const String& replacement, int options) const {
  return Substitute(subject, 0, replacement, options);
}

inline bool Regex::operator == (const String& subject) const {
  return Match(subject);
}

inline bool Regex::operator != (const String& subject) const {
  return !Match(subject);
}



//
// Unicode version
//


/**
A class for working with regular expressions.
Implemented using PCRE, the Perl Compatible
Regular Expressions library by Philip Hazel
(see http://www.pcre.org).
*/
class FUN_BASE_API URegex {
 public:
  /**
  Some of the following options can only be passed to the constructor;
  some can be passed only to matching functions, and some can be used
  everywhere.

    * Options marked [ctor] can be passed to the constructor.
    * Options marked [match] can be passed to match, Extract, Split and subst.
    * Options marked [subst] can be passed to subst.

  See the PCRE documentation for more information.
  */
  enum Options // These must match the corresponding options in pcre.h! {
    RE_CASELESS        = 0x00000001, /// case insensitive matching (/i) [ctor]
    RE_MULTILINE       = 0x00000002, /// enable multi-line mode; affects ^ and $ (/m) [ctor]
    RE_DOTALL          = 0x00000004, /// dot matches all characters, including newline (/s) [ctor]
    RE_EXTENDED        = 0x00000008, /// totally ignore whitespace (/x) [ctor]
    RE_ANCHORED        = 0x00000010, /// treat pattern as if it starts with a ^ [ctor, match]
    RE_DOLLAR_ENDONLY  = 0x00000020, /// dollar matches end-of-string only, not last newline in string [ctor]
    RE_EXTRA           = 0x00000040, /// enable optional PCRE functionality [ctor]
    RE_NOTBOL          = 0x00000080, /// circumflex does not match beginning of string [match]
    RE_NOTEOL          = 0x00000100, /// $ does not match end of string [match]
    RE_UNGREEDY        = 0x00000200, /// make quantifiers ungreedy [ctor]
    RE_NOTEMPTY        = 0x00000400, /// empty string never matches [match]
    RE_UTF8            = 0x00000800, /// assume pattern and subject is UTF-8 encoded [ctor]
    RE_NO_AUTO_CAPTURE = 0x00001000, /// disable numbered capturing parentheses [ctor, match]
    RE_NO_UTF8_CHECK   = 0x00002000, /// do not check validity of UTF-8 code sequences [match]
    RE_FIRSTLINE       = 0x00040000, /// an  unanchored  pattern  is  required  to  match
                                     /// before  or  at  the  first  newline  in  the subject string,
                                     /// though the matched text may continue over the newline [ctor]
    RE_DUPNAMES        = 0x00080000, /// names used to identify capturing  subpatterns  need not be unique [ctor]
    RE_NEWLINE_CR      = 0x00100000, /// assume newline is CR ('\r'), the default [ctor]
    RE_NEWLINE_LF      = 0x00200000, /// assume newline is LF ('\n') [ctor]
    RE_NEWLINE_CRLF    = 0x00300000, /// assume newline is CRLF ("\r\n") [ctor]
    RE_NEWLINE_ANY     = 0x00400000, /// assume newline is any valid Unicode newline character [ctor]
    RE_NEWLINE_ANYCRLF = 0x00500000, /// assume newline is any of CR, LF, CRLF [ctor]
    RE_GLOBAL          = 0x10000000, /// replace all occurrences (/g) [subst]
    RE_NO_VARS         = 0x20000000  /// treat dollar in replacement string as ordinary character [subst]
  };

  struct MatchInfo {
    int32 offset; /// zero based offset (INVALID_INDEX if subexpr does not match)
    int32 length; /// length of substring
    UString name; /// name of group
  };
  typedef Array<MatchInfo> MatchList;
  typedef Map<int, UString> GroupMap;

  /**
  Creates a regular expression and parses the given pattern.
  If study is true, the pattern is analyzed and optimized. This
  is mainly useful if the pattern is used more than once.
  For a description of the options, please see the PCRE documentation.
  Throws a RegularExpressionException if the patter cannot be compiled.
  */
  URegex(const UString& pattern, int options = 0, bool study = true);

  /**
  Destroys the regular expression.
  */
  ~URegex();

 public:
/**
  Matches the given subject string against the pattern. Returns the position
  of the first captured substring in match.
  If no part of the subject matches the pattern, match.offset is INVALID_INDEX and
  match.length is 0.
  Throws a RegularExpressionException in case of an error.
  Returns the number of matches.
  */
  int Match(const UString& subject, MatchInfo& match, int options = 0) const;

  /**
  Matches the given subject string, starting at offset, against the pattern.
  Returns the position of the captured substring in match.
  If no part of the subject matches the pattern, match.offset is INVALID_INDEX and
  match.length is 0.
  Throws a RegularExpressionException in case of an error.
  Returns the number of matches.
  */
  int Match(const UString& subject, int32 offset, MatchInfo& match, int options = 0) const;

  /**
  Matches the given subject string against the pattern.
  The first entry in matches contains the position of the captured substring.
  The following entries identify matching subpatterns. See the PCRE documentation
  for a more detailed explanation.
  If no part of the subject matches the pattern, matches is empty.
  Throws a RegularExpressionException in case of an error.
  Returns the number of matches.
  */
  int Match(const UString& subject, int32 offset, MatchList& matches, int options = 0) const;

  /**
  Returns true if and only if the subject matches the regular expression.

  Internally, this method sets the RE_ANCHORED and RE_NOTEMPTY options for
  matching, which means that the empty string will never match and
  the pattern is treated as if it starts with a ^.
  */
  bool Match(const UString& subject, int32 offset = 0) const;

  /**
  Returns true if and only if the subject matches the regular expression.
  */
  bool Match(const UString& subject, int32 offset, int options) const;

  /**
  Returns true if and only if the subject matches the regular expression.

  Internally, this method sets the RE_ANCHORED and RE_NOTEMPTY options for
  matching, which means that the empty string will never match and
  the pattern is treated as if it starts with a ^.
  */
  bool operator == (const UString& subject) const;

  /**
  Returns true if and only if the subject does not match the regular expression.

  Internally, this method sets the RE_ANCHORED and RE_NOTEMPTY options for
  matching, which means that the empty string will never match and
  the pattern is treated as if it starts with a ^.
  */
  bool operator != (const UString& subject) const;

  /**
  Matches the given subject string against the pattern.
  Returns the captured string.
  Throws a RegularExpressionException in case of an error.
  Returns the number of matches.
  */
  int Extract(const UString& subject, UString& str, int options = 0) const;

  /**
  Matches the given subject string, starting at offset, against the pattern.
  Returns the captured string.
  Throws a RegularExpressionException in case of an error.
  Returns the number of matches.
  */
  int Extract(const UString& subject, int32 offset, UString& str, int options = 0) const;

  /**
  Matches the given subject string against the pattern.
  The first entry in captured is the captured substring.
  The following entries contain substrings matching subpatterns. See the PCRE documentation
  for a more detailed explanation.
  If no part of the subject matches the pattern, captured is empty.
  Throws a RegularExpressionException in case of an error.
  Returns the number of matches.
  */
  int Split(const UString& subject, Array<UString>& strings, int options = 0) const;

  /**
  Matches the given subject string against the pattern.
  The first entry in captured is the captured substring.
  The following entries contain substrings matching subpatterns. See the PCRE documentation
  for a more detailed explanation.
  If no part of the subject matches the pattern, captured is empty.
  Throws a RegularExpressionException in case of an error.
  Returns the number of matches.
  */
  int Split(const UString& subject, int32 offset, Array<UString>& strings, int options = 0) const;

  /**
  Substitute in subject all matches of the pattern with replacement.
  If RE_GLOBAL is specified as option, all matches are replaced. Otherwise,
  only the first match is replaced.
  Occurrences of $<n> (for example, $1, $2, ...) in replacement are replaced
  with the corresponding captured string. $0 is the original subject string.
  Returns the number of replaced occurrences.
  */
  int Substitute(UString& subject, const UString& replacement, int options = 0) const;

  /**
  Substitute in subject all matches of the pattern with replacement,
  starting at offset.
  If RE_GLOBAL is specified as option, all matches are replaced. Otherwise,
  only the first match is replaced.
  Unless RE_NO_VARS is specified, occurrences of $<n> (for example, $0, $1, $2, ... $9)
  in replacement are replaced with the corresponding captured string.
  $0 is the captured substring. $1 ... $n are the substrings matching the subpatterns.
  Returns the number of replaced occurrences.
  */
  int Substitute(UString& subject, int32 offset, const UString& replacement, int options = 0) const;

  /**
  Matches the given subject string against the regular expression given in pattern,
  using the given options.
  */
  static bool Match(const UString& subject, const UString& pattern, int options = 0);

 protected:
  int32 SubstituteOne(UString& subject, int32 offset, const UString& replacement, int options) const;

 private:
  // Note: to avoid a dependency on the pcre.h header the following are
  // declared as void* and casted to the correct type in the implementation file.
  void* pcre_;  // Actual type is pcre*
  void* extra_; // Actual type is struct pcre_extra*

  GroupMap groups_;

  static const int OVEC_SIZE;

  URegex();
  URegex(const URegex&);
  URegex& operator = (const URegex&);
};


//
// inlines
//

inline int URegex::Match(const UString& subject, MatchInfo& match, int options) const {
  return Match(subject, 0, match, options);
}

inline int URegex::Split(const UString& subject, Array<UString>& strings, int options) const {
  return Split(subject, 0, strings, options);
}

inline int URegex::Substitute(UString& subject, const UString& replacement, int options) const {
  return Substitute(subject, 0, replacement, options);
}

inline bool URegex::operator == (const UString& subject) const {
  return Match(subject);
}

inline bool URegex::operator != (const UString& subject) const {
  return !Match(subject);
}

} // namespace fun
