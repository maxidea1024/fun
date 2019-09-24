#include "fun/base/regex.h"
#include <sstream>
#include "fun/base/exception.h"

#if defined(FUN_UNBUNDLED)
#include <pcre.h>
#else
#include "fun/base/bundle/pcre/pcre.h"
#include "fun/base/bundle/pcre/pcre_config.h"
#endif

namespace fun {

const int Regex::OVEC_SIZE = 126;  // must be multiple of 3

Regex::Regex(const String& pattern, int options, bool study)
    : pcre_(0), extra_(0) {
  const char* error;
  int offs;
  unsigned nmcount;
  unsigned nmentrysz;
  unsigned char* nmtbl;

  pcre_ = pcre_compile(pattern.c_str(), options, &error, &offs, 0);
  if (!pcre_) {
    std::ostringstream msg;
    msg << error << " (at offset " << offs << ")";
    throw RegularExpressionException(msg.str().c_str());
  }

  if (study) {
    extra_ = pcre_study(reinterpret_cast<pcre*>(pcre_), 0, &error);
  }

  pcre_fullinfo(reinterpret_cast<const pcre*>(pcre_),
                reinterpret_cast<const pcre_extra*>(extra_),
                PCRE_INFO_NAMECOUNT, &nmcount);
  pcre_fullinfo(reinterpret_cast<const pcre*>(pcre_),
                reinterpret_cast<const pcre_extra*>(extra_),
                PCRE_INFO_NAMEENTRYSIZE, &nmentrysz);
  pcre_fullinfo(reinterpret_cast<const pcre*>(pcre_),
                reinterpret_cast<const pcre_extra*>(extra_),
                PCRE_INFO_NAMETABLE, &nmtbl);

  for (int i = 0; i < nmcount; i++) {
    unsigned char* group = nmtbl + 2 + (nmentrysz * i);
    int n = pcre_get_stringnumber(reinterpret_cast<const pcre*>(pcre_),
                                  (char*)group);
    groups_[n] = String((char*)group);
  }
}

Regex::~Regex() {
  if (pcre_) {
    pcre_free(reinterpret_cast<pcre*>(pcre_));
  }

  if (extra_) {
    pcre_free(reinterpret_cast<struct pcre_extra*>(extra_));
  }
}

int Regex::Match(const String& subject, int32 offset, MatchInfo& match,
                 int options) const {
  fun_check(offset <= subject.Len());

  int ovec[OVEC_SIZE];
  int rc = pcre_exec(reinterpret_cast<pcre*>(pcre_),
                     reinterpret_cast<struct pcre_extra*>(extra_),
                     subject.c_str(), int(subject.Len()), int(offset),
                     options & 0xFFFF, ovec, OVEC_SIZE);
  if (rc == PCRE_ERROR_NOMATCH) {
    match.offset = INVALID_INDEX;
    match.length = 0;
    return 0;
  } else if (rc == PCRE_ERROR_BADOPTION) {
    throw RegularExpressionException("bad option");
  } else if (rc == 0) {
    throw RegularExpressionException("too many captured substrings");
  } else if (rc < 0) {
    std::ostringstream msg;
    msg << "PCRE error " << rc;
    throw RegularExpressionException(msg.str().c_str());
  }
  match.offset = ovec[0] < 0 ? INVALID_INDEX : ovec[0];
  match.length = ovec[1] - match.offset;
  return rc;
}

int Regex::Match(const String& subject, int32 offset, MatchList& matches,
                 int options) const {
  fun_check(offset <= subject.Len());

  matches.Clear();

  int ovec[OVEC_SIZE];
  int rc = pcre_exec(reinterpret_cast<pcre*>(pcre_),
                     reinterpret_cast<struct pcre_extra*>(extra_),
                     subject.c_str(), int(subject.Len()), int(offset),
                     options & 0xFFFF, ovec, OVEC_SIZE);
  if (rc == PCRE_ERROR_NOMATCH) {
    return 0;
  } else if (rc == PCRE_ERROR_BADOPTION) {
    throw RegularExpressionException("bad option");
  } else if (rc == 0) {
    throw RegularExpressionException("too many captured substrings");
  } else if (rc < 0) {
    std::ostringstream msg;
    msg << "PCRE error " << rc;
    throw RegularExpressionException(msg.str().c_str());
  }
  matches.Reserve(rc);
  for (int i = 0; i < rc; ++i) {
    MatchInfo m;
    GroupMap::const_iterator it;

    m.offset = ovec[i * 2] < 0 ? INVALID_INDEX : ovec[i * 2];
    m.length = ovec[i * 2 + 1] - m.offset;

    it = groups_.find(i);
    if (it != groups_.end()) {
      m.name = (*it).second;
    }

    matches.push_back(m);
  }
  return rc;
}

bool Regex::Match(const String& subject, int32 offset) const {
  MatchInfo match;
  Match(subject, offset, match, RE_ANCHORED | RE_NOTEMPTY);
  return match.offset == offset && match.length == subject.Len() - offset;
}

bool Regex::Match(const String& subject, int32 offset, int options) const {
  MatchInfo match;
  Match(subject, offset, match, options);
  return match.offset == offset && match.length == subject.Len() - offset;
}

int Regex::Extract(const String& subject, String& str, int options) const {
  MatchInfo match;
  int rc = Match(subject, 0, match, options);
  if (match.offset != INVALID_INDEX) {
    str.Assign(subject, match.offset, match.length);
  } else {
    str.Clear();
  }
  return rc;
}

int Regex::Extract(const String& subject, int32 offset, String& str,
                   int options) const {
  MatchInfo match;
  int rc = Match(subject, offset, match, options);
  if (match.offset != INVALID_INDEX) {
    str.Assign(subject, match.offset, match.length);
  } else {
    str.Clear();
  }
  return rc;
}

int Regex::Split(const String& subject, int32 offset, Array<String>& strings,
                 int options) const {
  MatchList matches;
  strings.Clear();
  int rc = Match(subject, offset, matches, options);
  strings.Reserve(matches.Count());
  for (MatchList::const_iterator it = matches.begin(); it != matches.end();
       ++it) {
    if (it->offset != INVALID_INDEX) {
      strings.push_back(subject.substr(it->offset, it->length));
    } else {
      strings.push_back(String());
    }
  }
  return rc;
}

int Regex::Substitute(String& subject, int32 offset, const String& replacement,
                      int options) const {
  if (options & RE_GLOBAL) {
    int rc = 0;
    int32 pos = SubstituteOne(subject, offset, replacement, options);
    while (pos != INVALID_INDEX) {
      ++rc;
      pos = SubstituteOne(subject, pos, replacement, options);
    }
    return rc;
  } else {
    return SubstituteOne(subject, offset, replacement, options) != INVALID_INDEX
               ? 1
               : 0;
  }
}

int32 Regex::SubstituteOne(String& subject, int32 offset,
                           const String& replacement, int options) const {
  if (offset >= subject.Len()) {
    return INVALID_INDEX;
  }

  int ovec[OVEC_SIZE];
  int rc = pcre_exec(reinterpret_cast<pcre*>(pcre_),
                     reinterpret_cast<struct pcre_extra*>(extra_),
                     subject.c_str(), int(subject.Len()), int(offset),
                     options & 0xFFFF, ovec, OVEC_SIZE);
  if (rc == PCRE_ERROR_NOMATCH) {
    return INVALID_INDEX;
  } else if (rc == PCRE_ERROR_BADOPTION) {
    throw RegularExpressionException("bad option");
  } else if (rc == 0) {
    throw RegularExpressionException("too many captured substrings");
  } else if (rc < 0) {
    std::ostringstream msg;
    msg << "PCRE error " << rc;
    throw RegularExpressionException(msg.str().c_str());
  }

  String result;
  int32 len = subject.Len();
  int32 pos = 0;
  int32 rp = INVALID_INDEX;
  while (pos < len) {
    if (ovec[0] == pos) {
      String::const_iterator it = replacement.begin();
      String::const_iterator end = replacement.end();
      while (it != end) {
        if (*it == '$' && !(options & RE_NO_VARS)) {
          ++it;
          if (it != end) {
            char d = *it;
            if (d >= '0' && d <= '9') {
              int c = d - '0';
              if (c < rc) {
                int o = ovec[c * 2];
                int l = ovec[c * 2 + 1] - o;
                result.Append(subject, o, l);
              }
            } else {
              result += '$';
              result += d;
            }
            ++it;
          } else {
            result += '$';
          }
        } else {
          result += *it++;
        }
      }
      pos = ovec[1];
      rp = result.Len();
    } else {
      result += subject[pos++];
    }
  }
  subject = result;
  return rp;
}

bool Regex::Match(const String& subject, const String& pattern, int options) {
  int ctor_options =
      options & (RE_CASELESS | RE_MULTILINE | RE_DOTALL | RE_EXTENDED |
                 RE_ANCHORED | RE_DOLLAR_ENDONLY | RE_EXTRA | RE_UNGREEDY |
                 RE_UTF8 | RE_NO_AUTO_CAPTURE);
  int match_options =
      options & (RE_ANCHORED | RE_NOTBOL | RE_NOTEOL | RE_NOTEMPTY |
                 RE_NO_AUTO_CAPTURE | RE_NO_UTF8_CHECK);
  Regex re(pattern, ctor_options, false);
  return re.Match(subject, 0, match_options);
}

//
// Unicode version
//

const int URegex::OVEC_SIZE = 126;  // must be multiple of 3

URegex::URegex(const StringU& pattern, int options, bool study)
    : pcre_(0), extra_(0) {
  const char* error;
  int offs;
  unsigned nmcount;
  unsigned nmentrysz;
  unsigned char* nmtbl;

  pcre_ = pcre_compile(pattern.c_str(), options, &error, &offs, 0);
  if (!pcre_) {
    std::ostringstream msg;
    msg << error << " (at offset " << offs << ")";
    throw RegularExpressionException(msg.str());
  }

  if (study) {
    extra_ = pcre_study(reinterpret_cast<pcre*>(pcre_), 0, &error);
  }

  pcre_fullinfo(reinterpret_cast<const pcre*>(pcre_),
                reinterpret_cast<const pcre_extra*>(extra_),
                PCRE_INFO_NAMECOUNT, &nmcount);
  pcre_fullinfo(reinterpret_cast<const pcre*>(pcre_),
                reinterpret_cast<const pcre_extra*>(extra_),
                PCRE_INFO_NAMEENTRYSIZE, &nmentrysz);
  pcre_fullinfo(reinterpret_cast<const pcre*>(pcre_),
                reinterpret_cast<const pcre_extra*>(extra_),
                PCRE_INFO_NAMETABLE, &nmtbl);

  for (int i = 0; i < nmcount; i++) {
    unsigned char* group = nmtbl + 2 + (nmentrysz * i);
    int n = pcre_get_stringnumber(reinterpret_cast<const pcre*>(pcre_),
                                  (char*)group);
    groups_[n] = StringU((char*)group);
  }
}

URegex::~URegex() {
  if (pcre_) {
    pcre_free(reinterpret_cast<pcre*>(pcre_));
  }

  if (extra_) {
    pcre_free(reinterpret_cast<struct pcre_extra*>(extra_));
  }
}

int URegex::Match(const StringU& subject, int32 offset, MatchInfo& match,
                  int options) const {
  fun_check(offset <= subject.Len());

  int ovec[OVEC_SIZE];
  int rc = pcre_exec(reinterpret_cast<pcre*>(pcre_),
                     reinterpret_cast<struct pcre_extra*>(extra_),
                     subject.c_str(), int(subject.Len()), int(offset),
                     options & 0xFFFF, ovec, OVEC_SIZE);
  if (rc == PCRE_ERROR_NOMATCH) {
    match.offset = INVALID_INDEX;
    match.length = 0;
    return 0;
  } else if (rc == PCRE_ERROR_BADOPTION) {
    throw RegularExpressionException("bad option");
  } else if (rc == 0) {
    throw RegularExpressionException("too many captured substrings");
  } else if (rc < 0) {
    std::ostringstream msg;
    msg << "PCRE error " << rc;
    throw RegularExpressionException(msg.str());
  }
  match.offset = ovec[0] < 0 ? INVALID_INDEX : ovec[0];
  match.length = ovec[1] - match.offset;
  return rc;
}

int URegex::Match(const StringU& subject, int32 offset, MatchList& matches,
                  int options) const {
  fun_check(offset <= subject.Len());

  matches.Clear();

  int ovec[OVEC_SIZE];
  int rc = pcre_exec(reinterpret_cast<pcre*>(pcre_),
                     reinterpret_cast<struct pcre_extra*>(extra_),
                     subject.c_str(), int(subject.Len()), int(offset),
                     options & 0xFFFF, ovec, OVEC_SIZE);
  if (rc == PCRE_ERROR_NOMATCH) {
    return 0;
  } else if (rc == PCRE_ERROR_BADOPTION) {
    throw RegularExpressionException("bad option");
  } else if (rc == 0) {
    throw RegularExpressionException("too many captured substrings");
  } else if (rc < 0) {
    std::ostringstream msg;
    msg << "PCRE error " << rc;
    throw RegularExpressionException(msg.str());
  }
  matches.Reserve(rc);
  for (int i = 0; i < rc; ++i) {
    MatchInfo m;
    GroupMap::const_iterator it;

    m.offset = ovec[i * 2] < 0 ? INVALID_INDEX : ovec[i * 2];
    m.length = ovec[i * 2 + 1] - m.offset;

    it = groups_.find(i);
    if (it != groups_.end()) {
      m.name = (*it).second;
    }

    matches.push_back(m);
  }
  return rc;
}

bool URegex::Match(const StringU& subject, int32 offset) const {
  MatchInfo match;
  Match(subject, offset, match, RE_ANCHORED | RE_NOTEMPTY);
  return match.offset == offset && match.length == subject.Len() - offset;
}

bool URegex::Match(const StringU& subject, int32 offset, int options) const {
  MatchInfo match;
  Match(subject, offset, match, options);
  return match.offset == offset && match.length == subject.Len() - offset;
}

int URegex::Extract(const StringU& subject, StringU& str, int options) const {
  MatchInfo match;
  int rc = Match(subject, 0, match, options);
  if (match.offset != INVALID_INDEX) {
    str.Assign(subject, match.offset, match.length);
  } else {
    str.Clear();
  }
  return rc;
}

int URegex::Extract(const StringU& subject, int32 offset, StringU& str,
                    int options) const {
  MatchInfo match;
  int rc = Match(subject, offset, match, options);
  if (match.offset != INVALID_INDEX) {
    str.Assign(subject, match.offset, match.length);
  } else {
    str.Clear();
  }
  return rc;
}

int URegex::Split(const StringU& subject, int32 offset, Array<StringU>& strings,
                  int options) const {
  MatchList matches;
  strings.Clear();
  int rc = Match(subject, offset, matches, options);
  strings.Reserve(matches.Count());
  for (MatchList::const_iterator it = matches.begin(); it != matches.end();
       ++it) {
    if (it->offset != INVALID_INDEX) {
      strings.push_back(subject.substr(it->offset, it->length));
    } else {
      strings.push_back(StringU());
    }
  }
  return rc;
}

int URegex::Substitute(StringU& subject, int32 offset,
                       const StringU& replacement, int options) const {
  if (options & RE_GLOBAL) {
    int rc = 0;
    int32 pos = SubstituteOne(subject, offset, replacement, options);
    while (pos != INVALID_INDEX) {
      ++rc;
      pos = SubstituteOne(subject, pos, replacement, options);
    }
    return rc;
  } else {
    return SubstituteOne(subject, offset, replacement, options) != INVALID_INDEX
               ? 1
               : 0;
  }
}

int32 URegex::SubstituteOne(StringU& subject, int32 offset,
                            const StringU& replacement, int options) const {
  if (offset >= subject.Len()) {
    return INVALID_INDEX;
  }

  int ovec[OVEC_SIZE];
  int rc = pcre_exec(reinterpret_cast<pcre*>(pcre_),
                     reinterpret_cast<struct pcre_extra*>(extra_),
                     subject.c_str(), int(subject.Len()), int(offset),
                     options & 0xFFFF, ovec, OVEC_SIZE);
  if (rc == PCRE_ERROR_NOMATCH) {
    return INVALID_INDEX;
  } else if (rc == PCRE_ERROR_BADOPTION) {
    throw RegularExpressionException("bad option");
  } else if (rc == 0) {
    throw RegularExpressionException("too many captured substrings");
  } else if (rc < 0) {
    std::ostringstream msg;
    msg << "PCRE error " << rc;
    throw RegularExpressionException(msg.str());
  }

  StringU result;
  int32 len = subject.Len();
  int32 pos = 0;
  int32 rp = INVALID_INDEX;
  while (pos < len) {
    if (ovec[0] == pos) {
      StringU::const_iterator it = replacement.begin();
      StringU::const_iterator end = replacement.end();
      while (it != end) {
        if (*it == '$' && !(options & RE_NO_VARS)) {
          ++it;
          if (it != end) {
            char d = *it;
            if (d >= '0' && d <= '9') {
              int c = d - '0';
              if (c < rc) {
                int o = ovec[c * 2];
                int l = ovec[c * 2 + 1] - o;
                result.append(subject, o, l);
              }
            } else {
              result += '$';
              result += d;
            }
            ++it;
          } else {
            result += '$';
          }
        } else {
          result += *it++;
        }
      }
      pos = ovec[1];
      rp = result.Len();
    } else {
      result += subject[pos++];
    }
  }
  subject = result;
  return rp;
}

bool URegex::Match(const StringU& subject, const StringU& pattern,
                   int options) {
  // TODO Utf8 옵션은 의미 없지 않으려나...??
  int ctor_options =
      options & (RE_CASELESS | RE_MULTILINE | RE_DOTALL | RE_EXTENDED |
                 RE_ANCHORED | RE_DOLLAR_ENDONLY | RE_EXTRA | RE_UNGREEDY |
                 RE_UTF8 | RE_NO_AUTO_CAPTURE);
  int match_options =
      options & (RE_ANCHORED | RE_NOTBOL | RE_NOTEOL | RE_NOTEMPTY |
                 RE_NO_AUTO_CAPTURE | RE_NO_UTF8_CHECK);
  URegex re(pattern, ctor_options, false);
  return re.match(subject, 0, match_options);
}

}  // namespace fun
