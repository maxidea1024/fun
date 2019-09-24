#include "fun/base/glob_matcher.h"
#include "fun/base/path.h"
#include "fun/base/exception.h"
#include "fun/base/directory_iterator.h"
#include "fun/base/file.h"
#include "fun/base/encoding/utf8_encoding.h"
#include "fun/base/text_iterator.h"
#include "fun/base/string/string.h"

namespace fun {

GlobMatcher::GlobMatcher(const String& pattern, int options)
  : pattern_(pattern), options_(options) {
}

GlobMatcher::~GlobMatcher() {}

bool GlobMatcher::Match(const String& subject) {
  Utf8Encoding utf8_encoding;
  TextIterator itp(pattern_, utf8_encoding);
  TextIterator endp(pattern_);
  TextIterator its(subject, utf8_encoding);
  TextIterator ends(subject);

  if ((options_ & GLOB_DOT_SPECIAL) && its != ends && *its == '.' && (*itp == '?' || *itp == '*')) {
    return false;
  } else {
    return Match(itp, endp, its, ends);
  }
}

void GlobMatcher::Glob(const String& path_pattern, Set<String>& files, int options) {
  Glob(Path(Path::Expand(path_pattern), Path::PATH_GUESS), files, options);
}

void GlobMatcher::Glob(const char* path_pattern, Set<String>& files, int options) {
  Glob(Path(Path::Expand(path_pattern), Path::PATH_GUESS), files, options);
}

void GlobMatcher::Glob(const Path& path_pattern, Set<String>& files, int options) {
  Path pattern(path_pattern);
  pattern.MakeDirectory(); // to simplify pattern handling later on
  Path base(pattern);
  Path abs_path(base);
  abs_path.MakeAbsolute();

  // In case of UNC paths we must not pop the topmost directory
  // (which must not contain wildcards), otherwise Collect() will fail
  // as one cannot create a DirectoryIterator with only a node name ("\\srv\").
  int min_depth = base.GetNode().IsEmpty() ? 0 : 1;
  while (base.GetDepth() > min_depth && base[base.GetDepth() - 1] != "..") {
    base.PopDirectory();
    abs_path.PopDirectory();
  }

  if (path_pattern.IsDirectory()) {
    options |= GLOB_DIRS_ONLY;
  }

  Collect(pattern, abs_path, base, path_pattern[base.GetDepth()], files, options);
}

void GlobMatcher::Glob( const Path& path_pattern,
                        const Path& base_path,
                        Set<String>& files,
                        int options) {
  Path pattern(path_pattern);
  pattern.MakeDirectory(); // to simplify pattern handling later on
  Path abs_path(base_path);
  abs_path.MakeAbsolute();

  if (path_pattern.IsDirectory()) {
    options |= GLOB_DIRS_ONLY;
  }

  Collect(pattern, abs_path, base_path, path_pattern[base_path.GetDepth()], files, options);
}

bool GlobMatcher::Match(TextIterator& itp,
                        const TextIterator& endp,
                        TextIterator& its,
                        const TextIterator& ends) {
  while (itp != endp) {
    if (its == ends) {
      while (itp != endp && *itp == '*') ++itp;
      break;
    }

    switch (*itp) {
      case '?':
        ++itp;
        ++its;
        break;

      case '*':
        if (++itp != endp) {
          while (its != ends && !MatchAfterAsterisk(itp, endp, its, ends)) {
            ++its;
          }
          return its != ends;
        }
        return true;

      case '[':
        if (++itp != endp) {
          bool invert = *itp == '!';
          if (invert) {
            ++itp;
          }

          if (itp != endp) {
            bool mtch = MatchSet(itp, endp, *its++);
            if ((invert && mtch) || (!invert && !mtch)) {
              return false;
            }
            break;
          }
        }
        throw SyntaxException("bad range syntax in glob pattern");

      case '\\':
        if (++itp == endp) {
          throw SyntaxException("backslash must be followed by character in glob pattern");
        }
        FUN_FALLTHROUGH

      default:
        if (options_ & GLOB_CASELESS) {
          if (CharTraitsU::ToLower(*itp) != CharTraitsU::ToLower(*its)) return false;
        } else {
          if (*itp != *its) return false;
        }
        ++itp; ++its;
      }
  }
  return itp == endp && its == ends;
}

bool GlobMatcher::MatchAfterAsterisk( TextIterator itp,
                                      const TextIterator& endp,
                                      TextIterator its,
                                      const TextIterator& ends) {
  return Match(itp, endp, its, ends);
}

bool GlobMatcher::MatchSet(TextIterator& itp, const TextIterator& endp, int c) {
  if (options_ & GLOB_CASELESS) {
    c = CharTraitsU::ToLower(c);
  }

  while (itp != endp) {
    switch (*itp) {
      case ']':
        ++itp;
        return false;

      case '\\':
        if (++itp == endp) {
          throw SyntaxException("backslash must be followed by character in glob pattern");
        }
    }

    int first = *itp;
    int last  = first;
    if (++itp != endp && *itp == '-') {
      if (++itp != endp) {
        last = *itp++;
      } else {
        throw SyntaxException("bad range syntax in glob pattern");
      }
    }

    if (options_ & GLOB_CASELESS) {
      first = CharTraitsU::ToLower(first);
      last  = CharTraitsU::ToLower(last);
    }

    if (first <= c && c <= last) {
      while (itp != endp) {
        switch (*itp) {
          case ']':
            ++itp;
            return true;

          case '\\':
            if (++itp == endp) break;

          default:
            ++itp;
        }
      }

      throw SyntaxException("range must be terminated by closing bracket in glob pattern");
    }
  }
  return false;
}

void GlobMatcher::Collect(const Path& path_pattern,
                          const Path& base,
                          const Path& current,
                          const String& pattern,
                          Set<String>& files,
                          int options) {
  try {
    String pp = path_pattern.ToString();
    String basep = base.ToString();
    String curp  = current.ToString();
    GlobMatcher g(pattern, options);
    DirectoryIterator it(base);
    DirectoryIterator end;
    while (it != end) {
      const String& name = it.GetName();
      if (g.Match(name)) {
        Path p(current);
        if (p.GetDepth() < path_pattern.GetDepth() - 1) {
          p.PushDirectory(name);
          Collect(path_pattern, it.GetPath(), p, path_pattern[p.GetDepth()], files, options);
        } else {
          p.SetFileName(name);
          if (IsDirectory(p, (options & GLOB_FOLLOW_SYMLINKS) != 0)) {
            p.MakeDirectory();
            files.Add(p.ToString());
          } else if (!(options & GLOB_DIRS_ONLY)) {
            files.Add(p.ToString());
          }
        }
      }
      ++it;
    }
  } catch (Exception&) {
  }
}

bool GlobMatcher::IsDirectory(const Path& path, bool follow_symlink) {
  File f(path);
  bool is_dir = false;
  try {
    is_dir = f.IsDirectory();
  } catch (Exception&) {
    return false;
  }

  if (is_dir) {
    return true;
  } else if (follow_symlink && f.IsLink()) {
    try {
      // Test if link resolves to a directory.
      DirectoryIterator it(f);
      return true;
    } catch (Exception&) {}
  }

  return false;
}

} // namespace fun
