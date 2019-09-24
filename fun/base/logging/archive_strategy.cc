#include "fun/base/logging/archive_strategy.h"

#if FUN_WITH_ARCHIVE_STRATEGY

#include "fun/base/file.h"
#include "fun/base/path.h"
#include "fun/base/deflating_stream.h"
#include "fun/base/stream_copier.h"
#include "fun/base/exception.h"
#include "fun/base/async_dispatcher.h"
#include "fun/base/async_method.h"
//#include "fun/base/void.h"
#include "fun/base/file_stream.h"

namespace fun {

//
// ArchiveCompressor
//

class ArchiveCompressor : public AsyncDispatcher {
 public:
  ArchiveCompressor()
    : compress_(this, &ArchiveCompressor::CompressImpl) {}

  ~ArchiveCompressor() {}

  AsyncMethod<void, String, ArchiveCompressor, AsyncStarter<AsyncDispatcher> > compress_;

 protected:
  void CompressImpl(const String& path) {
    String gz_path(path);
    gz_path.Append(".gz");

    FileInputStream istr(path);
    FileOutputStream ostr(gz_path);
    try {
      DeflatingOutputStream deflater(ostr, DeflatingStreamBuf::STREAM_GZIP);
      StreamCopier::CopyStream(istr, deflater);
      if (!deflater.good() || !ostr.good()) {
        throw WriteFileException(gz_path);
      }
      deflater.close();
      ostr.close();
      istr.close();
    } catch (fun::Exception&) {
      // deflating failed - remove gz file and leave uncompressed log file
      ostr.close();
      fun::File gzf(gz_path);
      gzf.remove();
      return;
    }
    File f(path);
    f.Remove();
    return;
  }
};


//
// ArchiveStrategy
//

ArchiveStrategy::ArchiveStrategy()
  : compress_(false), compressor_(nullptr) {}

ArchiveStrategy::~ArchiveStrategy() {
  delete compressor_;
}

void ArchiveStrategy::Compress(bool flag) {
  compress_ = flag;
}

void ArchiveStrategy::MoveFile( const String& old_path,
                                const String& new_path) {
  bool compressed = false;

  Path p(old_path);
  File f(old_path);
  if (!f.Exists()) {
    f = old_path + ".gz";
    compressed = true;
  }

  String mv_path(new_path);
  if (compress_ || compressed) {
    mv_path.Append(".gz");
  }

  if (!compress_ || compressed) {
    f.RenameTo(mv_path);
  } else {
    f.RenameTo(new_path);
    if (!compressor_) {
      compressor_ = new ArchiveCompressor;
    }
    compressor_->Compress(new_path);
  }
}

bool ArchiveStrategy::Exists(const String& name) {
  File f(name);
  if (f.Exists()) {
    return true;
  } else if (compress_) {
    String gz_name(name);
    gz_name.Append(".gz");
    File gzf(gz_name);
    return gzf.Exists();
  } else {
    return false;
  }
}


//
// ArchiveByNumberStrategy
//

ArchiveByNumberStrategy::ArchiveByNumberStrategy() {}

ArchiveByNumberStrategy::~ArchiveByNumberStrategy() {}

LogFile* ArchiveByNumberStrategy::Archive(LogFile* file) {
  String base_path = file->GetPath();
  delete file;
  int32 n = -1;
  String path;
  do {
    path = base_path;
    path.Append(".");
    NumberFormatter::Append(path, ++n);
  }
  while (Exists(path));

  while (n >= 0) {
    String old_path = base_path;
    if (n > 0) {
      old_path.Append(".");
      NumberFormatter::Append(old_path, n - 1);
    }
    String new_path = base_path;
    new_path.Append(".");
    NumberFormatter::Append(new_path, n);
    moveFile(old_path, new_path);
    --n;
  }
  return new LogFile(base_path);
}

} // namespace fun

#endif // FUN_WITH_ARCHIVE_STRATEGY
