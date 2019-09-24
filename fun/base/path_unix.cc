#include "fun/base/path_unix.h"
#include "fun/base/environment_unix.h"
#include "fun/base/exception.h"
//#include "fun/base/ascii.h"

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#if FUN_PLATFORM != FUN_PLATFORM_VXWORKS
#include <pwd.h>
#endif

#if FUN_PLATFORM == FUN_PLATFORM_MAC_OS_X
#include <mach-o/dyld.h>
#endif

#include <climits>

#ifndef PATH_MAX
#define PATH_MAX 1024  // fallback
#endif

namespace fun {

namespace {

// 패스는 반듯이 "/"로 끝나야함!
FUN_ALWAYS_INLINE String& EnsurePathTerm(String& str) {
  if (str.Len() > 0 && str[str.Len() - 1] != '/') {
    str << "/";
  }
  return str;
}

}  // namespace

String PathImpl::GetCurrentImpl() {
  String path;
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd))) {
    path = cwd;
  } else {
    throw SystemException("cannot get current directory");
  }

  return EnsurePathTerm(path);
}

String PathImpl::GetHomeImpl() {
#if FUN_PLATFORM == FUN_PLATFORM_VXWORKS
  if (EnvironmentImpl::HasImpl("HOME")) {
    return EnvironmentImpl::GetImpl("HOME");
  } else {
    return "/";
  }
#else
  String path;
  struct passwd* pwd = getpwuid(getuid());
  if (pwd) {
    path = pwd->pw_dir;
  } else {
    pwd = getpwuid(geteuid());
    if (pwd) {
      path = pwd->pw_dir;
    } else {
      if (EnvironmentImpl::HasImpl("HOME")) {
        path = EnvironmentImpl::GetImpl("HOME");
      }
    }
  }

  return EnsurePathTerm(path);
#endif
}

String PathImpl::GetConfigHomeImpl() {
#if FUN_PLATFORM == FUN_PLATFORM_VXWORKS
  return PathImpl::GetHomeImpl();
#elif FUN_PLATFORM == FUN_PLATFORM_MAC_OS_X
  String path = PathImpl::GetHomeImpl();
  String::size_type n = path.size();
  if (n > 0 && path[n - 1] == '/') {
    path.Append("Library/Preferences/");
  }
  return path;
#else
  String path;
  if (EnvironmentImpl::HasImpl("XDG_CONFIG_HOME")) {
    path = EnvironmentImpl::GetImpl("XDG_CONFIG_HOME");
  }
  if (!path.IsEmpty()) {
    return path;
  }

  path = PathImpl::GetHomeImpl();
  String::size_type n = path.size();
  if (n > 0 && path[n - 1] == '/') {
    path.Append(".config/");
  }

  return path;
#endif
}

String PathImpl::GetDataHomeImpl() {
#if FUN_PLATFORM == FUN_PLATFORM_VXWORKS
  return PathImpl::GetHomeImpl();
#elif FUN_PLATFORM == FUN_PLATFORM_MAC_OS_X
  String path = PathImpl::GetHomeImpl();
  String::size_type n = path.size();
  if (n > 0 && path[n - 1] == '/') {
    path.Append("Library/Application Support/");
  }
  return path;
#else
  String path;
  if (EnvironmentImpl::HasImpl("XDG_DATA_HOME")) {
    path = EnvironmentImpl::GetImpl("XDG_DATA_HOME");
  }
  if (!path.IsEmpty()) {
    return path;
  }

  path = PathImpl::GetHomeImpl();
  String::size_type n = path.size();
  if (n > 0 && path[n - 1] == '/') {
    path.Append(".local/share/");
  }
  return path;
#endif
}

String PathImpl::GetCacheHomeImpl() {
#if FUN_PLATFORM == FUN_PLATFORM_VXWORKS
  return PathImpl::GetTempImpl();
#elif FUN_PLATFORM == FUN_PLATFORM_MAC_OS_X
  String path = PathImpl::GetHomeImpl();
  String::size_type n = path.size();
  if (n > 0 && path[n - 1] == '/') {
    path.Append("Library/Caches/");
  }
  return path;
#else
  String path;
  if (EnvironmentImpl::HasImpl("XDG_CACHE_HOME")) {
    path = EnvironmentImpl::GetImpl("XDG_CACHE_HOME");
  }
  if (!path.IsEmpty()) {
    return path;
  }

  path = PathImpl::GetHomeImpl();
  String::size_type n = path.size();
  if (n > 0 && path[n - 1] == '/') {
    path.Append(".cache/");
  }
  return path;
#endif
}

String PathImpl::GetSelfImpl() {
#if FUN_PLATFORM == FUN_PLATFORM_MAC_OS_X
  char path[1024];
  uint32_t size = sizeof(path);
  if (_NSGetExecutablePath(path, &size) != 0) {
    throw SystemException("cannot obtain path for executable");
  }
  return path;
#elif FUN_PLATFORM == FUN_PLATFORM_LINUX || FUN_PLATFORM == FUN_PLATFORM_ANDROID
#ifdef PATH_MAX
  std::size_t sz = PATH_MAX;
#else
  std::size_t sz = 4096;
#endif
  char buf[sz];
  ssize_t ret = readlink("/proc/self/exe", buf, sz);
  if (-1 == ret) {
    throw SystemException("cannot obtain path for executable");
  }
  fun_check_dbg(ret < sz);
  buf[ret - 1] = '\0';
  return buf;
#endif
  // TODO (see https://stackoverflow.com/a/1024937/205386)
  // Solaris: getexecname()
  // FreeBSD: sysctl CTL_KERN KERN_PROC KERN_PROC_PATHNAME -1
  // FreeBSD if it has procfs: readlink /proc/curproc/file (FreeBSD doesn't have
  // procfs by default) NetBSD: readlink /proc/curproc/exe DragonFly BSD:
  // readlink /proc/curproc/file
  return "";
}

String PathImpl::GetTempImpl() {
  String path;
  char* tmp = getenv("TMPDIR");
  if (tmp) {
    path = tmp;
    return EnsurePathTerm(path);
  } else {
    path = "/tmp/";
  }

  return path;
}

String PathImpl::GetConfigImpl() {
  String path;

#if FUN_PLATFORM == FUN_PLATFORM_MAC_OS_X
  path = "/Library/Preferences/";
#else
  path = "/etc/";
#endif
  return path;
}

String PathImpl::GetNullImpl() {
#if FUN_PLATFORM == FUN_PLATFORM_VXWORKS
  return "/null";
#else
  return "/dev/null";
#endif
}

String PathImpl::ExpandImpl(const String& path) {
  String result;
  String::const_iterator it = path.begin();
  String::const_iterator end = path.end();
  if (it != end && *it == '~') {
    ++it;
    if (it != end && *it == '/') {
      const char* home_env = getenv("HOME");
      if (home_env) {
        result += home_env;
        String::size_type resultSize = result.size();
        if (resultSize > 0 && result[resultSize - 1] != '/') {
          result.Append("/");
        }
      } else {
        result += GetHomeImpl();
      }
      ++it;
    } else {
      result += '~';
    }
  }

  while (it != end) {
    if (*it == '\\') {
      ++it;
      if (*it == '$') {
        result += *it++;
      }
    } else if (*it == '$') {
      String var;
      ++it;
      if (it != end && *it == '{') {
        ++it;
        while (it != end && *it != '}') {
          var += *it++;
        }
        if (it != end) ++it;
      } else {
        while (it != end && (CharTraitsA::IsAlphaNumeric(*it) || *it == '_')) {
          var += *it++;
        }
      }

      // TODO 별도의 체계를 추가해줄수도 있을듯...
      char* val = getenv(var.c_str());
      if (val) {
        result += val;
      }
    } else {
      result += *it++;
    }
  }

  String::size_type found = result.find("//");
  while (found != String::npos) {
    result.replace(found, 2, "/");
    found = result.find("//", found + 1);
  }
  return result;
}

void PathImpl::ListRootsImpl(Array<String>& roots) {
  roots.Clear();
  roots.Add("/");
}

}  // namespace fun
