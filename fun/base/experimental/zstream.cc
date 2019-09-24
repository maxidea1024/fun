
ZStream::ZStream(std::istream& istr, StreamType type, int level)
  : istr_(&istr)
  , ostr_(nullptr)
  , eof_(false)
{
  zstr_.next_in   = 0;
  zstr_.avail_in  = 0;
  zstr_.total_in  = 0;
  zstr_.next_out  = 0;
  zstr_.avail_out = 0;
  zstr_.total_out = 0;
  zstr_.msg       = 0;
  zstr_.state     = 0;
  zstr_.zalloc    = Z_NULL;
  zstr_.zfree     = Z_NULL;
  zstr_.opaque    = Z_NULL;
  zstr_.data_type = 0;
  zstr_.adler     = 0;
  zstr_.reserved  = 0;

  buffer_ = new char[DEFLATE_BUFFER_SIZE];

  int rc = deflateInit2(&zstr_, level, Z_DEFLATED, 15 + (type == STREAM_GZIP ? 16 : 0), 8, Z_DEFAULT_STRATEGY);
  if (rc != Z_OK) {
    delete [] buffer_;
    throw IoException(zError(rc));
  }
}

ZStream::ZStream(std::istream& istr, int windows_bits, int level)
  : istr_(&istr)
  , ostr_(nullptr)
  , eof_(false)
{
  zstr_.zalloc    = Z_NULL;
  zstr_.zfree     = Z_NULL;
  zstr_.opaque    = Z_NULL;
  zstr_.next_in   = 0;
  zstr_.avail_in  = 0;
  zstr_.next_out  = 0;
  zstr_.avail_out = 0;

  buffer_ = new char[DEFLATE_BUFFER_SIZE];

  int rc = deflateInit2(&zstr_, level, Z_DEFLATED, windows_bits, 8, Z_DEFAULT_STRATEGY);
  if (rc != Z_OK) {
    delete [] buffer_;
    throw IoException(zError(rc));
  }
}

ZStream::ZStream(std::ostream& ostr, StreamType type, int level)
  : istr_(nullptr)
  , ostr_(&ostr)
  , eof_(false)
{
  zstr_.zalloc    = Z_NULL;
  zstr_.zfree     = Z_NULL;
  zstr_.opaque    = Z_NULL;
  zstr_.next_in   = 0;
  zstr_.avail_in  = 0;
  zstr_.next_out  = 0;
  zstr_.avail_out = 0;

  buffer_ = new char[DEFLATE_BUFFER_SIZE];

  int rc = deflateInit2(&zstr_, level, Z_DEFLATED, 15 + (type == STREAM_GZIP ? 16 : 0), 8, Z_DEFAULT_STRATEGY);
  if (rc != Z_OK) {
    delete [] buffer_;
    throw IoException(zError(rc));
  }
}

ZStream::ZStream(std::ostream& ostr, int windows_bits, int level)
  : istr_(nullptr)
  , ostr_(&ostr)
  , eof_(false)
{
  zstr_.zalloc    = Z_NULL;
  zstr_.zfree     = Z_NULL;
  zstr_.opaque    = Z_NULL;
  zstr_.next_in   = 0;
  zstr_.avail_in  = 0;
  zstr_.next_out  = 0;
  zstr_.avail_out = 0;

  buffer_ = new char[DEFLATE_BUFFER_SIZE];

  int rc = deflateInit2(&zstr_, level, Z_DEFLATED, windows_bits, 8, Z_DEFAULT_STRATEGY);
  if (rc != Z_OK) {
    delete [] buffer_;
    throw IoException(zError(rc));
  }
}

ZStream::~ZStream()
{
  try {
    Close();
  }
  catch (...) {
  }

  delete[] buffer_;
  deflateEnd(&zstr_);
}

int ZStream::Close()
{
  BufferedStreamBuf::Sync();

  istr_ = nullptr;

  if (ostr_) {
    if (zstr_.next_out) {
      int rc = deflate(&zstr_, Z_FINISH);
      if (rc != Z_OK && rc != Z_STREAM_END) {
        throw IoException(zError(rc));
      }
      ostr_->write(buffer_, DEFLATE_BUFFER_SIZE - zstr_.avail_out);
      if (!ostr_->good()) {
        throw IoException(zError(rc));
      }
      zstr_.next_out  = (unsigned char*)buffer_;
      zstr_.avail_out = DEFLATE_BUFFER_SIZE;
      while (rc != Z_STREAM_END) {
        rc = deflate(&zstr_, Z_FINISH);
        if (rc != Z_OK && rc != Z_STREAM_END) {
          throw IoException(zError(rc));
        }
        ostr_->write(buffer_, DEFLATE_BUFFER_SIZE - zstr_.avail_out);
        if (!ostr_->good()) {
          throw IoException(zError(rc));
        }
        zstr_.next_out  = (unsigned char*)buffer_;
        zstr_.avail_out = DEFLATE_BUFFER_SIZE;
      }
    }
    ostr_->flush();
    ostr_ = nullptr;
  }
  return 0;
}

int ZStream::ReadFromDevice(char* buff, size_t length)
{
  if (!istr_) {
    return 0;
  }

  if (zstr_.avail_in == 0 && !eof_) {
    int n = 0;
    if (istr_->good()) {
      istr_->read(buffer_, DEFLATE_BUFFER_SIZE);
      n = static_cast<int>(istr_->gcount());
    }
    if (n > 0) {
      zstr_.next_in  = (unsigned char*)buffer_;
      zstr_.avail_in = n;
    }
    else {
      zstr_.next_in  = 0;
      zstr_.avail_in = 0;
      eof_ = true;
    }
  }
  zstr_.next_out  = (unsigned char*)buffer;
  zstr_.avail_out = static_cast<unsigned>(length);
  for (;;) {
    int rc = deflate(&zstr_, eof_ ? Z_FINISH : Z_NO_FLUSH);
    if (eof_ && rc == Z_STREAM_END) {
      istr_ = 0;
      return static_cast<int>(length) - zstr_.avail_out;
    }
    if (rc != Z_OK) {
      throw IoException(zError(rc));
    }
    if (zstr_.avail_out == 0) {
      return static_cast<int>(length);
    }
    if (zstr_.avail_in == 0) {
      int n = 0;
      if (istr_->good()) {
        istr_->read(buffer_, DEFLATE_BUFFER_SIZE);
        n = static_cast<int>(istr_->gcount());
      }
      if (n > 0) {
        zstr_.next_in  = (unsigned char*)buffer_;
        zstr_.avail_in = n;
      }
      else {
        zstr_.next_in  = 0;
        zstr_.avail_in = 0;
        eof_ = true;
      }
    }
  }
}

int ZStream::WriteToDevice(const char* data, size_t length)
{
  if (length == 0 || !ostr_) {
    return 0;
  }

  zstr_.next_in   = (unsigned char*)data;
  zstr_.avail_in  = static_cast<unsigned>(length);
  zstr_.next_out  = (unsigned char*)buffer_;
  zstr_.avail_out = DEFLATE_BUFFER_SIZE;
  for (;;) {
    int rc = deflate(&zstr_, Z_NO_FLUSH);
    if (rc != Z_OK) {
      throw IoException(zError(rc));
    }
    if (zstr_.avail_out == 0) {
      ostr_->write(buffer_, DEFLATE_BUFFER_SIZE);
      if (!ostr_->good()) {
        throw IoException(zError(rc));
      }
      zstr_.next_out  = (unsigned char*)buffer_;
      zstr_.avail_out = DEFLATE_BUFFER_SIZE;
    }
    if (zstr_.avail_in == 0) {
      ostr_->write(buffer_, DEFLATE_BUFFER_SIZE - zstr_.avail_out);
      if (!ostr_->good()) {
        throw IoException(zError(rc));
      }
      zstr_.next_out  = (unsigned char*)buffer_;
      zstr_.avail_out = DEFLATE_BUFFER_SIZE;
      break;
    }
  }
  return static_cast<int>(length);
}

int ZStream::Sync()
{
  if (BufferedStreamBuf::Sync()) {
    return -1;
  }

  if (ostr_) {
    if (zstr_.next_out) {
      int rc = deflate(&zstr_, Z_SYNC_FLUSH);
      if (rc != Z_OK) {
        throw IoException(zError(rc));
      }
      ostr_->write(buffer_, DEFLATE_BUFFER_SIZE - zstr_.avail_out);
      if (!ostr_->good()) {
        throw IoException(zError(rc));
      }
      while (zstr_.avail_out == 0) {
        zstr_.next_out  = (unsigned char*)buffer_;
        zstr_.avail_out = DEFLATE_BUFFER_SIZE;
        rc = deflate(&zstr_, Z_SYNC_FLUSH);
        if (rc != Z_OK) {
          throw IoException(zError(rc));
        }
        ostr_->write(buffer_, DEFLATE_BUFFER_SIZE - zstr_.avail_out);
        if (!ostr_->good()) {
          throw IoException(zError(rc));
        }
      };
      zstr_.next_out  = (unsigned char*)buffer_;
      zstr_.avail_out = DEFLATE_BUFFER_SIZE;
    }
    // NOTE: This breaks the Zip library and causes corruption in some files.
    // See GH #1828
    // ostr_->flush();
  }
}
