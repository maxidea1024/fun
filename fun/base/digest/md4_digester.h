#pragma once

#include "fun/base/base.h"
#include "fun/base/digest/digester.h"

namespace fun {

/**
 * This class implements the MD4 message digest algorithm,
 * described in RFC 1320.
 */
class FUN_BASE_API MD4Disgester : public Digester {
 public:
  enum {
    BLOCK_SIZE = 64,
    DIGEST_SIZE = 16
  };

  MD4Disgester();
  ~MD4Disgester();

  // Digester interface
  int32 GetDigestLength() override;
  void Reset() override;
  const Digest& GetDigest() override;

  // Disable copy
  MD4Disgester(const MD4Disgester&) = delete;
  MD4Disgester& operator = (const MD4Disgester&) = delete;

 protected:
  // Digester interface
  void UpdateImpl(const void* data, size_t length) override;

 private:
  void Transform(uint32 state[4], const uint8 block[64]);

  static void Encode(uint8* output, const uint32* input, int32 len);
  static void Decode(uint32* output, const uint8* input, int32 len);

  struct Context {
    uint32 state[4];  // state (ABCD)
    uint32 count[2];  // number of bits, modulo 2^64 (lsb first)
    uint8 buffer[64]; // input buffer
  };

  Context context_;
  Digester::Digest digest_;
};

} // namespace fun
