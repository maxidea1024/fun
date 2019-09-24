#pragma once

#include "fun/base/base.h"
#include "fun/base/digest/digester.h"

namespace fun {

/**
 * This class implements the SHA-1 message digest algorithm.
 * (FIPS 180-1, see http://www.itl.nist.gov/fipspubs/fip180-1.htm)
 */
class FUN_BASE_API SHA1Digester : public Digester {
 public:
  enum { BLOCK_SIZE = 64, DIGEST_SIZE = 20 };

  SHA1Digester();
  virtual ~SHA1Digester();

  // Digester interface
  int32 GetDigestLength() override;
  void Reset() override;
  const Digest& GetDigest() override;

  // Disable copy
  SHA1Digester(const SHA1Digester&) = delete;
  SHA1Digester& operator=(const SHA1Digester&) = delete;

 protected:
  // Digester interface
  void UpdateImpl(const void* data, size_t length) override;

 private:
  void Transform();
  static void ByteReverse(uint32* buffer, int32 byte_count);

  struct Context {
    uint32 digest[5];  // Message digest
    uint32 count_lo;   // 64-bit bit count
    uint32 count_hi;
    uint32 data[16];  // SHA data buffer
    uint32 slop;      // # of bytes saved in data[]
  };

  Context context_;
  Digester::Digest digest_;
};

}  // namespace fun
