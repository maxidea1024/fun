#pragma once

#include "fun/base/base.h"
#include "fun/base/string/byte_array.h"

namespace fun {

class IFile;
class Digester;

class FUN_BASE_API CryptographicHash {
 public:
  enum Algorithm {
    MD4,
    MD5,
    SHA1,
    SHA2_224,
    SHA2_256,
    SHA2_384,
    SHA2_512,
    SHA3_224,
    SHA3_256,
    SHA3_384,
    SHA3_512
  };

  explicit CryptographicHash(Algorithm algorithm);
  ~CryptographicHash();

  void Reset();

  template <typename T>
  void AddData(const T* data, int32 len)
  {
    AddData((const char*)data, len * sizeof(T));
  }
  void AddData(const char* data, int32 len);
  void AddData(const ByteArray& data);
  bool AddData(IFile* file);

  Algorithm GetAlgorithm() const;
  ByteArray GetResult() const;

  static ByteArray Hash(const ByteArray& data, Algorithm algorithm);

 private:
  Algorithm algorithm_;
  Digester* digester_;
};

} // namespace fun
