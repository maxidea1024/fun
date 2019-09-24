#pragma once

#include "fun/base/base.h"
#include "fun/base/digest/digester.h"
#include "fun/base/byte_order.h"

namespace fun {

/**
 * https://en.wikipedia.org/wiki/PBKDF2
 * 
 * DK = PBKDF2(PRF, Password, salt, c, dkLen)
 * 
 * - PRF is a pseudorandom function of two parameters with output length hLen (e.g. a keyed HMAC)
 * - Password is the master password from which a derived key is generated
 * - salt is a sequence of bits, known as a cryptographic salt
 * - c is the number of iterations desired
 * - dkLen is the desired length of the derived key
 * - DK is the generated derived key
 * 
 * 
 * This class implements the Password-Based Key Derivation Function 2,
 * as specified in RFC 2898. The underlying Digester (THMACDigester, etc.),
 * which must accept the passphrase as constructor argument (String),
 * must be given as template argument.
 * 
 * PBKDF2 (Password-Based Key Derivation Function 2) is a key derivation function
 * that is part of RSA Laboratories' Public-Key Cryptography Standards (PKCS) series,
 * specifically PKCS #5 v2.0, also published as Internet Engineering Task Force's
 * RFC 2898. It replaces an earlier standard, PBKDF1, which could only produce
 * derived keys up to 160 bits long.
 * 
 * PBKDF2 applies a pseudorandom function, such as a cryptographic hash, cipher, or
 * HMAC to the input password or passphrase along with a salt value and repeats the
 * process many times to produce a derived key, which can then be used as a
 * cryptographic key in subsequent operations. The added computational work makes
 * password cracking much more difficult, and is known as key stretching.
 * When the standard was written in 2000, the recommended minimum number of
 * iterations was 1000, but the parameter is intended to be increased over time as
 * CPU speeds increase. Having a salt added to the password reduces the ability to
 * use precomputed hashes (rainbow tables) for attacks, and means that multiple
 * passwords have to be tested individually, not all at once. The standard
 * recommends a salt length of at least 64 bits. [Wikipedia]
 * 
 * The PBKDF2 algorithm is implemented as a Digester. The passphrase is specified
 * by calling Update().
 * 
 * Example (WPA2):
 *   TPBKDF2Digester<HMACDisgester<SHA1Digester> > PBKDF2(SSID, 4096, 256);
 *   PBKDF2.Update(pass_phrase);
 *   Digester::Digest digest = PBKDF2.GetDigest();
 */
template <typename PRF>
class FUN_BASE_API PBKDF2Digester : public Digester {
 public:
  enum { PRF_DIGEST_SIZE = PRF::DIGEST_SIZE };

  PBKDF2Digester(const String& salt, uint32 c = 4096, uint32 dk_len = PRF_DIGEST_SIZE)
    : s_(salt), c_(c), dk_len_(dk_len) {
    result_.Reserve(dk_len + PRF_DIGEST_SIZE);
  }

  ~PBKDF2Digester() {}

  // Digester interface
  int32 GetDigestLength() override {
    return dk_len_;
  }

  // Digester interface
  void Reset() override {
    p_.Clear();
    result_.Clear();
  }

  // Digester interface
  const Digest& GetDigest() override {
    uint32 i = 1;
    while (result_.Count() < dk_len_) {
      Func(i++);
    }
    result_.Truncate(dk_len_);
    return result_;
  }

  // Disable copy
  PBKDF2Digester(const PBKDF2Digester&) = delete;
  PBKDF2Digester& operator = (const PBKDF2Digester&) = delete;

 protected:
  // Digester interface
  void UpdateImpl(const void* data, size_t length) override {
    p_.Append(reinterpret_cast<const char*>(data), length);
  }

  void Func(uint32 i) {
    PRF inner_digester(p_);
    inner_digester.Update(s_);
    uint32 i_be = ByteOrder::ToBigEndian(i);
    inner_digester.Update(&i_be, sizeof(i_be));
    auto up = inner_digester.GetDigest();
    auto ux = up;
    fun_check(ux.Len() == PRF_DIGEST_SIZE);
    for (uint32 k = 1; k < c_; ++k) {
      inner_digester.Reset();
      inner_digester.Update(up);
      auto u = inner_digester.GetDigest();
      fun_check(u.Len() == PRF_DIGEST_SIZE);
      for (int32 j = 0; j < PRF_DIGEST_SIZE; ++j) {
        ux[j] ^= u[j];
      }
      Swap(up, u);
    }
    result_.Append(ux);
  }

 private:
  String p_;
  String s_;
  uint32 c_;
  uint32 dk_len_;
  Digester::Digest result_;
};

} // namespace fun
