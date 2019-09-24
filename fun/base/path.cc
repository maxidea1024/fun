#include "fun/base/path.h"
#include "fun/base/file.h"
#include "fun/base/exception.h"

#if FUN_PLATFORM_UNIX_FAMILY
#include "fun/base/path_unix.cc"
#elif FUN_PLATFORM_WINDOWS_FAMILY
#if defined(_WIN32_WCE)
#include "fun/base/path_wince.cc"
#else
#include "fun/base/path_win32.cc"
#endif
#endif

namespace fun {

Path::Path() : absolute_(false) {}

Path::Path(bool absolute) : absolute_(absolute) {}

Path::Path(const String& path) {
  fun_check(path.IsNulTerm());
  Assign(path);
}

Path::Path(const String& path, Style style) {
  fun_check(path.IsNulTerm());
  Assign(path, style);
}

Path::Path(const char* path) {
  fun_check_ptr(path);

  Assign(path);
}

Path::Path(const char* path, Style style) {
  fun_check_ptr(path);

  Assign(path, style);
}

Path::Path(const Path& path)
  : node_(path.node_),
    device_(path.device_),
    name_(path.name_),
    version_(path.version_),
    dirs_(path.dirs_),
    absolute_(path.absolute_) {}

Path::Path(const Path& parent, const String& filename)
  : node_(parent.node_),
    device_(parent.device_),
    name_(parent.name_),
    version_(parent.version_),
    dirs_(parent.dirs_),
    absolute_(parent.absolute_) {
  fun_check(filename.IsNulTerm());
  MakeDirectory();
  name_ = filename;
}

Path::Path(const Path& parent, const char* filename)
  : node_(parent.node_),
    device_(parent.device_),
    name_(parent.name_),
    version_(parent.version_),
    dirs_(parent.dirs_),
    absolute_(parent.absolute_) {
  MakeDirectory();
  name_ = filename;
}

Path::Path(const Path& parent, const Path& relative)
  : node_(parent.node_),
    device_(parent.device_),
    name_(parent.name_),
    version_(parent.version_),
    dirs_(parent.dirs_),
    absolute_(parent.absolute_) {
  Resolve(relative);
}

Path::~Path() {}

Path& Path::operator = (const Path& path) {
  return Assign(path);
}

Path& Path::operator = (const String& path) {
  fun_check(CStringTraitsA::Strlen(path.c_str()) == path.Len());

  return Assign(path);
}

Path& Path::operator = (const char* path) {
  fun_check_ptr(path);

  return Assign(path);
}

void Path::Swap(Path& path) {
  fun::Swap(node_, path.node_);
  fun::Swap(device_, path.device_);
  fun::Swap(name_, path.name_);
  fun::Swap(version_, path.version_);
  fun::Swap(dirs_, path.dirs_);
  fun::Swap(absolute_, path.absolute_);
}

Path& Path::Assign(const Path& path) {
  if (FUN_LIKELY(&path != this)) {
    node_ = path.node_;
    device_ = path.device_;
    name_ = path.name_;
    version_ = path.version_;
    dirs_ = path.dirs_;
    absolute_ = path.absolute_;
  }
  return *this;
}

Path& Path::Assign(const String& path) {
#if FUN_PLATFORM_WINDOWS_FAMILY
  ParseWindows(path);
#else
  ParseUnix(path);
#endif
  return *this;
}

Path& Path::Assign(const String& path, Style style) {
  switch (style) {
    case PATH_UNIX:
      ParseUnix(path);
      break;
    case PATH_WINDOWS:
      ParseWindows(path);
      break;
    case PATH_VMS:
      ParseVMS(path);
      break;
    case PATH_NATIVE:
      Assign(path);
      break;
    case PATH_GUESS:
      ParseGuess(path);
      break;
    default:
      fun_bugcheck();
  }
  return *this;
}

Path& Path::Assign(const char* path) {
  return Assign(String(path));
}

String Path::ToString() const {
#if FUN_PLATFORM_WINDOWS_FAMILY
  return BuildWindows();
#else
  return BuildUnix();
#endif
}

String Path::ToString(Style style) const {
  switch (style) {
    case PATH_UNIX:
      return BuildUnix();
    case PATH_WINDOWS:
      return BuildWindows();
    case PATH_VMS:
      return BuildVMS();
    case PATH_NATIVE:
    case PATH_GUESS:
      return ToString();
    default:
      fun_bugcheck();
  }
  return String();
}

bool Path::TryParse(const String& path) {
  fun_check(path.IsNulTerm());
  try {
    Path p;
    p.Parse(path);
    Assign(p);
    return true;
  } catch (...) {
    return false;
  }
}

bool Path::TryParse(const String& path, Style style) {
  fun_check(path.IsNulTerm());
  try {
    Path p;
    p.Parse(path, style);
    Assign(p);
    return true;
  } catch (...) {
    return false;
  }
}

Path& Path::ParseDirectory(const String& path) {
  fun_check(path.IsNulTerm());
  Assign(path);
  return MakeDirectory();
}

Path& Path::ParseDirectory(const String& path, Style style) {
  fun_check(CStringTraitsA::Strlen(path.c_str()) == path.Len());
  Assign(path, style);
  return MakeDirectory();
}

Path& Path::MakeDirectory() {
  PushDirectory(name_);
  name_.Clear();
  version_.Clear();
  return *this;
}

Path& Path::MakeFile() {
  if (!dirs_.IsEmpty() && name_.IsEmpty()) {
    name_ = dirs_.Last();
    dirs_.PopBack();
  }
  return *this;
}

Path& Path::MakeAbsolute() {
  return MakeAbsolute(Current());
}

Path& Path::MakeAbsolute(const Path& base) {
  if (!absolute_) {
    Path tmp = base;
    tmp.MakeDirectory();
    for (const auto& dir : dirs_) {
      tmp.PushDirectory(dir);
    }
    node_ = tmp.node_;
    device_ = tmp.device_;
    dirs_ = tmp.dirs_;
    absolute_ = base.absolute_;
  }
  return *this;
}

Path Path::Absolute() const {
  Path result(*this);
  if (!result.absolute_) {
    result.MakeAbsolute();
  }
  return result;
}

Path Path::Absolute(const Path& base) const {
  Path result(*this);
  if (!result.absolute_) {
    result.MakeAbsolute(base);
  }
  return result;
}

Path Path::Parent() const {
  Path p(*this);
  return p.MakeParent();
}

Path& Path::MakeParent() {
  if (name_.IsEmpty()) {
    if (dirs_.IsEmpty()) {
      if (!absolute_) {
        dirs_.PushBack("..");
      }
    } else {
      if (dirs_.Last() == "..") {
        dirs_.PushBack("..");
      } else {
        dirs_.PopBack();
      }
    }
  } else {
    name_.Clear();
    version_.Clear();
  }
  return *this;
}

Path& Path::Append(const Path& path) {
  MakeDirectory();
  dirs_.Append(path.dirs_);
  name_ = path.name_;
  version_ = path.version_;
  return *this;
}

Path& Path::Resolve(const Path& path) {
  if (path.IsAbsolute()) {
    Assign(path);
  } else {
    for (int32 i = 0; i < path.GetDepth(); ++i) {
      PushDirectory(path[i]);
    }
    name_ = path.name_;
  }
  return *this;
}

Path& Path::SetNode(const String& node) {
  fun_check(node.IsNulTerm());
  node_ = node;
  absolute_ = absolute_ || !node.IsEmpty();
  return *this;
}

Path& Path::SetDevice(const String& device) {
  fun_check(device.IsNulTerm());
  device_ = device;
  absolute_ = absolute_ || !device.IsEmpty();
  return *this;
}

const String& Path::GetDirectory(int32 n) const {
  fun_check(0 <= n && n <= dirs_.Count());

  if (n < dirs_.Count()) {
    return dirs_[n];
  } else {
    return name_;
  }
}

const String& Path::operator [] (int32 n) const {
  fun_check(0 <= n && n <= dirs_.Count());

  if (n < dirs_.Count()) {
    return dirs_[n];
  } else {
    return name_;
  }
}

Path& Path::PushDirectory(const String& dir) {
  fun_check(dir.IsNulTerm());
  if (!dir.IsEmpty() && dir != ".") {
    if (dir == "..") {
      if (!dirs_.IsEmpty() && dirs_.Last() != "..") {
        dirs_.PopBack();
      } else if (!absolute_) {
        dirs_.PushBack(dir);
      }
    } else {
      dirs_.PushBack(dir);
    }
  }
  return *this;
}

Path& Path::PopDirectory() {
  fun_check(!dirs_.IsEmpty());

  dirs_.PopBack();
  return *this;
}

Path& Path::PopFrontDirectory() {
  fun_check(!dirs_.IsEmpty());

  dirs_.PopFront();
  return *this;
}

Path& Path::SetFileName(const String& name) {
  fun_check(name.IsNulTerm());
  name_ = name;
  return *this;
}

Path& Path::SetBaseName(const String& name) {
  fun_check(name.IsNulTerm());
  String ext = GetExtension();
  name_ = name;
  if (!ext.IsEmpty()) {
    name_ << ".";
    name_ << ext;
  }
  return *this;
}

String Path::GetBaseName() const {
  int32 pos = name_.LastIndexOf('.');
  if (pos != INVALID_INDEX) {
    return name_.Mid(0, pos);
  } else {
    return name_;
  }
}

Path& Path::SetExtension(const String& extension) {
  fun_check(extension.IsNulTerm());
  name_ = GetBaseName();
  if (!extension.IsEmpty()) {
    name_ << ".";
    name_ << extension;
  }
  return *this;
}

String Path::GetExtension() const {
  int32 pos = name_.LastIndexOf('.');
  if (pos != INVALID_INDEX) {
    return name_.Mid(pos + 1);
  } else {
    return String();
  }
}

Path& Path::Clear() {
  node_.Clear();
  device_.Clear();
  name_.Clear();
  dirs_.Clear();
  version_.Clear();
  absolute_ = false;
  return *this;
}

String Path::Current() {
  return PathImpl::GetCurrentImpl();
}

String Path::GetHome() {
  return PathImpl::GetHomeImpl();
}

String Path::GetConfigHome() {
#if FUN_PLATFORM_UNIX_FAMILY || FUN_PLATFORM_WINDOWS_FAMILY
  return PathImpl::GetConfigHomeImpl();
#else
  return PathImpl::GetHomeImpl();
#endif
}

String Path::GetDataHome() {
#if FUN_PLATFORM_UNIX_FAMILY || FUN_PLATFORM_WINDOWS_FAMILY
  return PathImpl::GetDataHomeImpl();
#else
  return PathImpl::GetHomeImpl();
#endif
}

String Path::GetCacheHome() {
#if FUN_PLATFORM_UNIX_FAMILY || FUN_PLATFORM_WINDOWS_FAMILY
  return PathImpl::GetCacheHomeImpl();
#else
  return PathImpl::GetHomeImpl();
#endif
}

String Path::GetSelf() {
  return PathImpl::GetSelfImpl();
}

String Path::GetTemp() {
  return PathImpl::GetTempImpl();
}

String Path::GetConfig() {
#if FUN_PLATFORM_UNIX_FAMILY || FUN_PLATFORM_WINDOWS_FAMILY
  return PathImpl::GetConfigImpl();
#else
  return PathImpl::GetConfigImpl();
#endif
}

String Path::GetNull() {
  return PathImpl::GetNullImpl();
}

String Path::Expand(const String& path) {
  fun_check(path.IsNulTerm());
  return PathImpl::ExpandImpl(path);
}

void Path::ListRoots(Array<String>& roots) {
  PathImpl::ListRootsImpl(roots);
}

bool Path::Find(const Array<String>& path_list, const String& name, Path& path) {
  fun_check(name.IsNulTerm());
  for (const auto& elem : path_list) {
#if FUN_PLATFORM_WINDOWS_FAMILY
    String clean_path(elem);
    if (clean_path.Len() > 1 && clean_path[0] == '"' && clean_path[clean_path.Len() - 1] == '"') {
      clean_path = clean_path.Mid(1, clean_path.Len() - 2);
    }

    Path p(clean_path);
#else
    Path p(elem);
#endif
    p.MakeDirectory();
    p.Resolve(Path(name));
    File f(p);
    if (f.Exists()) {
      path = p;
      return true;
    }
  }
  return false;
}

bool Path::Find(const Array<StringRef>& path_list, const String& name, Path& path) {
  fun_check(name.IsNulTerm());
  for (const auto& elem : path_list) {
#if FUN_PLATFORM_WINDOWS_FAMILY
    String clean_path(elem);
    if (clean_path.Len() > 1 && clean_path[0] == '"' && clean_path[clean_path.Len() - 1] == '"') {
      clean_path = clean_path.Mid(1, clean_path.Len() - 2);
    }

    Path p(clean_path);
#else
    Path p(elem);
#endif
    p.MakeDirectory();
    p.Resolve(Path(name));
    File f(p);
    if (f.Exists()) {
      path = p;
      return true;
    }
  }
  return false;
}

bool Path::Find(const String& path_list, const String& name, Path& path) {
  Array<String> st = path_list.Split(PathSeparator(), 0, StringSplitOption::TrimmingAndCullEmpty);
  return Find(st, name, path);
}

void Path::ParseUnix(const String& path) {
  Clear();

  String::ConstIterator cur = path.begin();
  String::ConstIterator end = path.end();

  if (cur != end) {
    if (*cur == '/') {
      absolute_ = true;
      ++cur;
    } else if (*cur == '~') {
      ++cur;
      if (cur == end || *cur == '/') {
        Path cwd(GetHome());
        dirs_ = cwd.dirs_;
        absolute_ = true;
      } else {
        --cur;
      }
    }

    while (cur != end) {
      String name;
      while (cur != end && *cur != '/') {
        name += *cur++;
      }

      if (cur != end) {
        if (dirs_.IsEmpty()) {
          if (!name.IsEmpty() && name.Last() == ':') {
            absolute_ = true;
            device_ = name.Mid(0, name.Len() - 1); // without last character
          } else {
            PushDirectory(name);
          }
        } else {
          PushDirectory(name);
        }
      } else {
        name_ = name;
      }

      if (cur != end) {
        ++cur;
      }
    }
  }
}

void Path::ParseWindows(const String& path) {
  Clear();

  String::ConstIterator cur  = path.begin();
  String::ConstIterator end = path.end();

  if (cur != end) {
    if (*cur == '\\' || *cur == '/') {
      absolute_ = true;
      ++cur;
    }

    if (absolute_ && cur != end && (*cur == '\\' || *cur == '/')) { // UNC
      ++cur;
      while (cur != end && *cur != '\\' && *cur != '/') {
        node_ += *cur++;
      }

      if (cur != end) {
        ++cur;
      }
    } else if (cur != end) {
      char d = *cur++;
      if (cur != end && *cur == ':') { // drive letter
        if (!((d >= 'a' && d <= 'z') || (d >= 'A' && d <= 'Z'))) {
          throw PathSyntaxException(path);
        }

        absolute_ = true;
        device_ += d;
        ++cur;
        if (cur == end || (*cur != '\\' && *cur != '/')) {
          throw PathSyntaxException(path);
        }

        ++cur;
      } else {
        --cur;
      }
    }

    while (cur != end) {
      String name;
      while (cur != end && *cur != '\\' && *cur != '/') {
        name += *cur++;
      }

      if (cur != end) {
        PushDirectory(name);
      } else {
        name_ = name;
      }

      if (cur != end) {
        ++cur;
      }
    }
  }

  if (!node_.IsEmpty() && dirs_.IsEmpty() && !name_.IsEmpty()) {
    MakeDirectory();
  }
}

void Path::ParseVMS(const String& path) {
  Clear();

  String::ConstIterator cur  = path.begin();
  String::ConstIterator end = path.end();

  if (cur != end) {
    String name;
    while (cur != end && *cur != ':' && *cur != '[' && *cur != ';') {
      name += *cur++;
    }
    if (cur != end) {
      if (*cur == ':') {
        ++cur;
        if (cur != end && *cur == ':') {
          node_ = name;
          ++cur;
        } else {
          device_ = name;
        }
        absolute_ = true;
        name.Clear();
      }
      if (cur != end) {
        if (device_.IsEmpty() && *cur != '[') {
          while (cur != end && *cur != ':' && *cur != ';') {
            name += *cur++;
          }
          if (cur != end) {
            if (*cur == ':') {
              device_ = name;
              absolute_ = true;
              name.Clear();
              ++cur;
            }
          }
        }
      }
      if (name.IsEmpty()) {
        if (cur != end && *cur == '[') {
          ++cur;
          if (cur != end) {
            absolute_ = true;
            if (*cur == '.') {
              absolute_ = false;
              ++cur;
            } else if (*cur == ']' || *cur == '-') {
              absolute_ = false;
            }
            while (cur != end && *cur != ']') {
              name.Clear();
              if (*cur == '-') {
                name = "-";
              } else {
                while (cur != end && *cur != '.' && *cur != ']') {
                  name += *cur++;
                }
              }
              if (!name.IsEmpty()) {
                if (name == "-") {
                  if (dirs_.IsEmpty() || dirs_.Back() == "..") {
                    dirs_.PushBack("..");
                  } else {
                    dirs_.PopBack();
                  }
                } else {
                  dirs_.PushBack(name);
                }
              }
              if (cur != end && *cur != ']') {
                ++cur;
              }
            }
            if (cur == end) {
              throw PathSyntaxException(path);
            }
            ++cur;
            if (cur != end && *cur == '[') {
              if (!absolute_) {
                throw PathSyntaxException(path);
              }
              ++cur;
              if (cur != end && *cur == '.') {
                throw PathSyntaxException(path);
              }
              int32 d = dirs_.Count();
              while (cur != end && *cur != ']') {
                name.Clear();
                if (*cur == '-') {
                  name = "-";
                } else {
                  while (cur != end && *cur != '.' && *cur != ']') {
                    name += *cur++;
                  }
                }
                if (!name.IsEmpty()) {
                  if (name == "-") {
                    if (dirs_.Count() > d) {
                      dirs_.PopBack();
                    }
                  } else {
                    dirs_.PushBack(name);
                  }
                }
                if (cur != end && *cur != ']') {
                  ++cur;
                }
              }
              if (cur == end) {
                throw PathSyntaxException(path);
              }
              ++cur;
            }
          }
          name_.Clear();
        }
        while (cur != end && *cur != ';') {
          name_ += *cur++;
        }
      } else {
        name_ = name;
      }
      if (cur != end && *cur == ';') {
        ++cur;
        while (cur != end) {
          version_ += *cur++;
        }
      }
    } else {
      name_ = name;
    }
  }
}

void Path::ParseGuess(const String& path) {
  bool has_blackslash = false;
  bool has_slash = false;
  bool hash_open_bracket = false;
  bool hash_close_bracket = false;
  bool is_windows = path.Len() > 2 && path[1] == ':' && (path[2] == '/' || path[2] == '\\');
  String::ConstIterator end = path.end();
  String::ConstIterator semi_it = end;
  if (!is_windows) {
    for (String::ConstIterator cur = path.begin(); cur != end; ++cur) {
      switch (*cur) {
        case '\\': has_blackslash = true; break;
        case '/':  has_slash = true; break;
        case '[':  hash_open_bracket = true;
        case ']':  hash_close_bracket = hash_open_bracket;
        case ';':  semi_it = cur; break;
      }
    }
  }
  if (has_blackslash || is_windows) {
    ParseWindows(path);
  } else if (has_slash) {
    ParseUnix(path);
  } else {
    bool is_vms = hash_close_bracket;
    if (!is_vms && semi_it != end) {
      is_vms = true;
      ++semi_it;
      while (semi_it != end) {
        if (*semi_it < '0' || *semi_it > '9') {
          is_vms = false; break;
        }
        ++semi_it;
      }
    }

    if (is_vms) {
      ParseVMS(path);
    } else {
      ParseUnix(path);
    }
  }
}

String Path::BuildUnix() const {
  String result;
  if (!device_.IsEmpty()) {
    result << "/";
    result << device_;
    result << ":/";
  } else if (absolute_) {
    result << "/";
  }
  for (const auto& dir : dirs_) {
    result << dir;
    result << "/";
  }
  result << name_;
  return result;
}

String Path::BuildWindows() const {
  String result;
  if (!node_.IsEmpty()) {
    result << "\\\\";
    result << node_;
    result << "\\";
  } else if (!device_.IsEmpty()) {
    result << device_;
    result << ":\\";
  } else if (absolute_) {
    result << "\\";
  }
  for (const auto& dir : dirs_) {
    result << dir;
    result << "\\";
  }
  result << name_;
  return result;
}

String Path::BuildVMS() const {
  String result;
  if (!node_.IsEmpty()) {
    result << node_;
    result << "::";
  }
  if (!device_.IsEmpty()) {
    result << device_;
    result << ":";
  }

  if (!dirs_.IsEmpty()) {
    result << "[";
    if (!absolute_ && dirs_[0] != "..") {
      result << ".";
    }

    for (int32 i = 0; i < dirs_.Count(); ++i) {
      const auto& dir = dirs_[i];
      if (i > 0 && dir != "..") {
        result << ".";
      }
      if (dir == "..") {
        result << "-";
      } else {
        result << dir;
      }
    }

    result << "]";
  }
  result << name_;
  if (!version_.IsEmpty()) {
    result << ";";
    result << version_;
  }
  return result;
}

String Path::Transcode(const String& path) {
#if FUN_PLATFORM_WINDOWS_FAMILY
  UString upath = UString::FromUtf8(path);
  DWORD len = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, upath.c_str(), upath.Len(), NULL, 0, NULL, NULL);
  if (len > 0) {
    Array<char> buffer(len, NoInit);
    DWORD rc = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, upath.c_str(), upath.Len(), buffer.MutableData(), buffer.Count(), NULL, NULL);
    if (rc) {
      return String(buffer.ConstData(), buffer.Count());
    }
  }
#endif
  return path;
}

} // namespace fun
