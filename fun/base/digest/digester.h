#pragma once

#include "fun/base/base.h"
#include "fun/base/container/array.h"
#include "fun/base/string/string.h"

namespace fun {

/**
 * This class is an abstract base class for all classes implementing a message
 * digest algorithm, like MD5Digester and SHA1Digester.
 *
 * Call Update() repeatedly with data to compute the digest from. When done,
 * call GetDigest() to obtain the message digest.
 */
class FUN_BASE_API Digester {
 public:
  typedef ByteString Digest;

  Digester();
  virtual ~Digester();

  void Update(const void* data, size_t length);
  void Update(char data);
  void Update(const String& data);

  // Disable copy
  Digester(const Digester&) = delete;
  Digester& operator=(const Digester&) = delete;

  /** Returns the length of the digest in bytes. */
  virtual int32 GetDigestLength() = 0;

  /** Resets the engine so that a new digest can be computed. */
  virtual void Reset() = 0;

  /**
   * Finishes the computation of the digest and
   * returns the message digest. Resets the engine
   * and can thus only be called once for every digest.
   * The returned reference is valid until the next
   * time digest() is called, or the engine object is destroyed.
   */
  virtual const Digest& GetDigest() = 0;

  /** Converts a message digest into a string of hexadecimal numbers. */
  static String DigestToHex(const Digest& digest);

  /** Converts a string created by digestToHex back to its Digest presentation
   */
  static Digest DigestFromHex(const String& digest);

  /**
   * Compares two Digest values using a constant-time comparison
   * algorithm. This can be used to prevent timing attacks
   * (as discussed in <https://codahale.com/a-lesson-in-timing-attacks/>).
   */
  static bool ConstantTimeEquals(const Digest& digest1, const Digest& digest2);

 protected:
  /**
   * Updates the digest with the given data. Must be implemented
   * by subclasses.
   */
  virtual void UpdateImpl(const void* data, size_t length) = 0;
};

//
// inlines
//

FUN_ALWAYS_INLINE void Digester::Update(const void* data, size_t length) {
  UpdateImpl(data, length);
}

FUN_ALWAYS_INLINE void Digester::Update(char data) { UpdateImpl(&data, 1); }

FUN_ALWAYS_INLINE void Digester::Update(const String& data) {
  UpdateImpl(data.ConstData(), data.Len());
}

}  // namespace fun
