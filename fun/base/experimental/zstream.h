
class ZStream {
 public:
  enum StreamType {
    STREAM_ZLIB,
    STREAM_GZIP,
  };

  ZStream(std::istream& istr, StreamType type, int level);
  ZStream(std::istream& istr, int windows_bits, int level);

  ZStream(std::ostream& ostr, StreamType type, int level);
  ZStream(std::ostream& ostr, int windows_bits, int level);

  ~ZStream();

  int Close();

 protected:
  int ReadFromDevice(char* buff, size_t length);
  int WriteToDevice(const char* data, size_t length);
  int Sync();

 private:
  enum {
    STREAM_BUFFER_SIZE = 1024,
    DEFLATE_BUFFER_SIZE = 32768,
  };

  std::istream* istr_;
  std::ostream* ostr_;
  char* buffer_;
  z_stream zstr_;
  bool eof_;
};
