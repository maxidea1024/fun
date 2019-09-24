#pragma once

#include "fun/base/base.h"
#include "fun/base/digest/digester.h"

namespace fun {

/**
 * This class implements the SHA-3 message digest algorithm.
 * (FIPS 202, see http://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.202.pdf)
 */
class FUN_BASE_API SHA3Digester : public Digester {
 public:
  enum Algorithm {
    SHA3_224 = 224,
    SHA3_256 = 256,
    SHA3_384 = 384,
    SHA3_512 = 512,
  };

  SHA3Digester(Algorithm algorithm = SHA3_512);
  virtual ~SHA3Digester();

  // Digester interface
  int32 GetDigestLength() override;
  void Reset() override;
  const Digest& GetDigest() override;

  // Disable copy
  SHA3Digester(const SHA3Digester&) = delete;
  SHA3Digester& operator = (const SHA3Digester&) = delete;

 protected:
  // Digester interface
  void UpdateImpl(const void* data, size_t length) override;

 private:
  void Transform();

  void* context_;
  Algorithm algorithm_;
  Digester::Digest digest_;
};

} // namespace fun
