#include "fun/base/logging/log_file_std.h"
#include "fun/base/exception.h"
#include "fun/base/file.h"

namespace fun {

LogFileImpl::LogFileImpl(const String& path)
    : path_(path)
// TODO
//, str_(path_, std::ios::app)
//, size_((uint64)str_.tellp())
{
  if (size_ == 0) {
    creation_date_ = File(path).GetLastModified();
  } else {
    creation_date_ = File(path).GetCreated();
  }
}

LogFileImpl::~LogFileImpl() {
  // NOOP
}

void LogFileImpl::WriteImpl(const String& text, bool flush) {
  // TODO
  // if (!str_.good()) {
  //  str_.close();
  //  str_.open(path_, std::ios::app);
  //}
  //
  // if (!str_.good()) {
  //  throw WriteFileException(path_);
  //}
  //
  // str_ << text;
  //
  // if (flush) {
  //  str_ << std::endl;
  //} else {
  //  str_ << "\n";
  //}
  //
  // if (!str_.good()) {
  //  throw WriteFileException(path_);
  //}
  //
  // size_ = (uint64)str_.tellp();
}

uint64 LogFileImpl::GetSizeImpl() const { return size_; }

Timestamp LogFileImpl::GetCreationDateImpl() const { return creation_date_; }

const String& LogFileImpl::GetPathImpl() const { return path_; }

}  // namespace fun
