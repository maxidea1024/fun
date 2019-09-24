#include "fun/base/directory_iterator.h"

#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/directory_iterator_win32.cc"
#elif FUN_PLATFORM_UNIX_FAMILY
#include "fun/base/directory_iterator_unix.cc"
#endif

namespace fun {

DirectoryIterator::DirectoryIterator()
  : impl_(nullptr) {}

DirectoryIterator::DirectoryIterator(const String& path)
  : path_(path),
    impl_(new DirectoryIteratorImpl(path)) {
  path_.MakeDirectory();
  path_.SetFileName(impl_->Get());
  file_ = path_;
}

DirectoryIterator::DirectoryIterator(const DirectoryIterator& iterator)
  : path_(iterator.path_),
    impl_(iterator.impl_) {
  if (impl_) {
    impl_->AddRef();
    file_ = path_;
  }
}

DirectoryIterator::DirectoryIterator(const File& file)
  : path_(file.GetPath()),
    impl_(new DirectoryIteratorImpl(file.GetPath())) {
  path_.MakeDirectory();
  path_.SetFileName(impl_->Get());
  file_ = path_;
}

DirectoryIterator::DirectoryIterator(const Path& path)
  : path_(path),
    impl_(new DirectoryIteratorImpl(path.ToString())) {
  path_.MakeDirectory();
  path_.SetFileName(impl_->Get());
  file_ = path_;
}

DirectoryIterator::~DirectoryIterator() {
  if (impl_) {
    impl_->Release();
  }
}

DirectoryIterator& DirectoryIterator::operator = (const DirectoryIterator& it) {
  if (impl_) {
    impl_->Release();
  }

  impl_ = it.impl_;
  if (impl_) {
    impl_->AddRef();
    path_ = it.path_;
    file_ = path_;
  }
  return *this;
}

DirectoryIterator& DirectoryIterator::operator = (const File& file) {
  if (impl_) {
    impl_->Release();
  }

  impl_ = new DirectoryIteratorImpl(file.GetPath());
  path_.ParseDirectory(file.GetPath());
  path_.SetFileName(impl_->Get());
  file_ = path_;
  return *this;
}

DirectoryIterator& DirectoryIterator::operator = (const Path& path) {
  if (impl_) {
    impl_->Release();
  }

  impl_ = new DirectoryIteratorImpl(path.ToString());
  path_ = path;
  path_.MakeDirectory();
  path_.SetFileName(impl_->Get());
  file_ = path_;
  return *this;
}

DirectoryIterator& DirectoryIterator::operator = (const String& path) {
  if (impl_) {
    impl_->Release();
  }

  impl_ = new DirectoryIteratorImpl(path);
  path_.ParseDirectory(path);
  path_.SetFileName(impl_->Get());
  file_ = path_;
  return *this;
}

DirectoryIterator& DirectoryIterator::operator ++ () {
  if (impl_) {
    path_.SetFileName(impl_->Next());
    file_ = path_;
  }
  return *this;
}

DirectoryIterator DirectoryIterator::operator ++ (int /*dummy*/) {
  if (impl_) {
    path_.SetFileName(impl_->Next());
    file_ = path_;
  }
  return *this;
}

} // namespace fun
