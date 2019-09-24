//TODO 이게 실제로 필요할까??

#include "fun/framework/filesystem_configuration.h"
#include "fun/base/file.h"
#include "fun/base/path.h"
#include "fun/base/directory_iterator.h"
//#include "fun/base/file_stream.h"

namespace fun {
namespace framework {

FilesystemConfiguration::FilesystemConfiguration(const String& path)
  : path_(path) {
  path_.MakeDirectory();
}

FilesystemConfiguration::~FilesystemConfiguration() {}

void FilesystemConfiguration::Clear() {
  File reg_dir(path_);
  reg_dir.Remove(true);
}

bool FilesystemConfiguration::GetRaw(const String& key, String& value) const {
  Path p(KeyToPath(key));
  p.SetFileName("data");
  File f(p);
  if (f.Exists()) {
    value.Reserve((String::size_type)f.GetSize());
    //TOO
    fun_check(0);
    //fun::FileInputStream istr(p.ToString());
    //int32 c = istr.get();
    //while (c != std::char_traits<char>::eof()) {
    //  value += (char)c;
    //  c = istr.get();
    //}
    return true;
  } else {
    return false;
  }
}

void FilesystemConfiguration::SetRaw(const String& key, const String& value) {
  Path p(KeyToPath(key));
  File dir(p);
  dir.CreateDirectories();
  p.SetFileName("data");

  //TODO
  fun_check(0);
  //fun::FileOutputStream ostr(p.ToString());
  //ostr.write(value.c_str(), (std::streamsize)value.Len());
}

void FilesystemConfiguration::Enumerate(const String& key, Keys& range) const {
  Path p(KeyToPath(key));
  File dir(p);
  if (!dir.Exists()) {
    return;
  }

  DirectoryIterator it(p);
  DirectoryIterator end;
  while (it != end) {
    if (it->IsDirectory()) {
      range.PushBack(it.GetName());
    }
    ++it;
  }
}

void FilesystemConfiguration::RemoveRaw(const String& key) {
  Path p(KeyToPath(key));
  File dir(p);
  if (dir.Exists()) {
    dir.Remove(true);
  }
}

Path FilesystemConfiguration::KeyToPath(const String& key) const {
  Path result(path_);
  Array<String> dirs = key.Split(".", 0, StringSplitOption::TrimmingAndCullEmpty);
  for (const auto& dir : dirs) {
    result.PushDirectory(dir);
  }
  return result;
}

} // namespace framework
} // namespace fun
