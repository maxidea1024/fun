#pragma once

#include "fun/base/base.h"
#include "fun/base/digest/digester.h"

namespace fun {

/**
 * This class implements the SHA-2 message digest algorithm.
 * (FIPS 180-4, see http://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf)
 */
class FUN_BASE_API SHA2Digester : public Digester {
 public:
  enum Algorithm {
    SHA2_224 = 224,
    SHA2_256 = 256,
    SHA2_384 = 384,
    SHA2_512 = 512,
  };

  SHA2Digester(Algorithm algorithm = SHA2_512);
  virtual ~SHA2Digester();

  // Digester interface
  int32 GetDigestLength() override;
  void Reset() override;
  const Digest& GetDigest() override;

  // Disable copy
  SHA2Digester(const SHA2Digester&) = delete;
  SHA2Digester& operator=(const SHA2Digester&) = delete;

 protected:
  // Digester interface
  void UpdateImpl(const void* data, size_t length) override;

 private:
  void Transform();

  void* context_;
  Algorithm algorithm_;
  Digester::Digest digest_;
};

}  // namespace fun
