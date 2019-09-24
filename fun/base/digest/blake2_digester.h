#pragma once

#include "fun/base/base.h"
#include "fun/base/digest/digester.h"

namespace fun {

/**
 * This class implements the BLAKE2 hashing algorithm.
 * (RFC 7693, see https://tools.ietf.org/html/rfc7693)
 */
class FUN_BASE_API BLAKE2Disgester : public Digester {
 public:
  enum Algorithm {
    BLAKE2b_224 = 224,
    BLAKE2b_256 = 256,
    BLAKE2b_384 = 384,
    BLAKE2b_512 = 512
  };

  BLAKE2Disgester(Algorithm algorithm = BLAKE2b_512);
  ~BLAKE2Disgester();

  // Digester interface
  int32 GetDigestLength() override;
  void Reset() override;
  const Digest& GetDigest() override;

  // Disable copy
  BLAKE2Disgester(const BLAKE2Disgester&) = delete;
  BLAKE2Disgester& operator = (const BLAKE2Disgester&) = delete;

 protected:
  // Digester interface
  void UpdateImpl(const void* data, size_t length) override;

 private:
  void* context_;
  Algorithm algorithm_;
  Digester::Digest digest_;
};

} // namespace fun
