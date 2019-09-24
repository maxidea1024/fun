// NOTE 주석 중간에 /* 이 있어서 javadoc스타일로하면 문제가 생김..

#pragma once

#include "fun/base/base.h"
#include "fun/base/container/set.h"
#include "fun/base/string/string.h"

namespace fun {

class Path;
class TextIterator;

/**
 * This class implements glob-style pattern matching
 * as known from Unix shells.
 *
 * In the pattern string, '*' matches any sequence of characters,
 * '?' matches any single character, [SET] matches any single character
 * in the specified set, [!SET] matches any character not in the
 * specified set.
 *
 * A set is composed of characters or ranges; a range looks like
 * character hyphen character (as in 0-9 or A-Z).
 * [0-9a-zA-Z_] is the set of characters allowed in C identifiers.
 * Any other character in the pattern must be matched exactly.
 *
 * To suppress the special syntactic significance of any of '[]*?!-\',
 * and match the character exactly, precede it with a backslash.
 *
 * All strings are assumed to be UTF-8 encoded.
 */
class FUN_BASE_API GlobMatcher {
 public:
  /**
   * Flags that modify the matching behavior.
   */
  enum Options {
    GLOB_DEFAULT = 0x00,  /// default behavior
    GLOB_DOT_SPECIAL =
        0x01,  /// '*' and '?' do not match '.' at beginning of subject
    GLOB_FOLLOW_SYMLINKS = 0x02,  /// follow symbolic links
    GLOB_CASELESS = 0x04,         /// ignore case when comparing characters
    GLOB_DIRS_ONLY = 0x80  /// only glob for directories (for internal use only)
  };

  /// Creates the GlobMatcher, using the given pattern. The pattern
  /// must not be an empty string.
  ///
  /// If the GLOB_DOT_SPECIAL option is specified, '*' and '?' do
  /// not match '.' at the beginning of a matched subject. This is useful for
  /// making dot-files invisible in good old Unix-style.
  GlobMatcher(const String& pattern, int options = 0);

  /// Destroys the GlobMatcher.
  ~GlobMatcher();

  /// Matches the given subject against the glob pattern.
  /// Returns true if the subject matches the pattern, false
  /// otherwise.
  bool Match(const String& subject);

  /// Creates a set of files that match the given path_pattern.
  ///
  /// The path may be give in either Unix or Windows syntax and
  /// is automatically expanded by calling Path::Expand().
  ///
  /// The pattern may contain wildcard expressions even in intermediate
  /// directory names (e.g. /usr/include/*/*.h).
  ///
  /// Note that, for obvious reasons, escaping characters in a pattern
  /// with a backslash does not work in Windows-style paths.
  ///
  /// Directories that for whatever reason cannot be traversed are
  /// ignored.
  static void Glob(const String& path_pattern, Set<String>& files,
                   int options = 0);

  /// Creates a set of files that match the given path_pattern.
  ///
  /// The path may be give in either Unix or Windows syntax and
  /// is automatically expanded by calling Path::Expand().
  ///
  /// The pattern may contain wildcard expressions even in intermediate
  /// directory names (e.g. /usr/include/*/*.h).
  ///
  /// Note that, for obvious reasons, escaping characters in a pattern
  /// with a backslash does not work in Windows-style paths.
  ///
  /// Directories that for whatever reason cannot be traversed are
  /// ignored.
  static void Glob(const char* path_pattern, Set<String>& files,
                   int options = 0);

  /// Creates a set of files that match the given path_pattern.
  ///
  /// The pattern may contain wildcard expressions even in intermediate
  /// directory names (e.g. /usr/include/*/*.h).
  ///
  /// Note that, for obvious reasons, escaping characters in a pattern
  /// with a backslash does not work in Windows-style paths.
  ///
  /// Directories that for whatever reason cannot be traversed are
  /// ignored.
  static void Glob(const Path& path_pattern, Set<String>& files,
                   int options = 0);

  /// Creates a set of files that match the given path_pattern, starting from
  /// base_path.
  ///
  /// The pattern may contain wildcard expressions even in intermediate
  /// directory names (e.g. /usr/include/*/*.h).
  ///
  /// Note that, for obvious reasons, escaping characters in a pattern
  /// with a backslash does not work in Windows-style paths.
  ///
  /// Directories that for whatever reason cannot be traversed are
  /// ignored.
  static void Glob(const Path& path_pattern, const Path& base_path,
                   Set<String>& files, int options = 0);

  GlobMatcher() = delete;
  GlobMatcher(const GlobMatcher&) = delete;
  GlobMatcher& operator=(const GlobMatcher&) = delete;

 protected:
  bool Match(TextIterator& itp, const TextIterator& endp, TextIterator& its,
             const TextIterator& ends);

  bool MatchAfterAsterisk(TextIterator itp, const TextIterator& endp,
                          TextIterator its, const TextIterator& ends);

  bool MatchSet(TextIterator& itp, const TextIterator& endp, int c);

  static void Collect(const Path& path_pattern, const Path& base,
                      const Path& current, const String& pattern,
                      Set<String>& files, int options);

  static bool IsDirectory(const Path& path, bool follow_symlink);

 private:
  String pattern_;
  int options_;
};

}  // namespace fun
