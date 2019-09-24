#pragma once

#include "fun/base/base.h"
#include "fun/base/digest/digester.h"

namespace fun {

template <typename DisgesterTy>
class HMACDigester : public Digester {
 public:
  enum {
    BLOCK_SIZE  = DisgesterTy::BLOCK_SIZE,
    DIGEST_SIZE = DisgesterTy::DIGEST_SIZE
  };

  HMACDigester(const String& pass_phrase) {
    Init(pass_phrase.ConstData(), pass_phrase.Len());
  }

  HMACDigester(const char* pass_phrase, int32 length) {
    Init(pass_phrase, length);
  }

  ~HMACDigester() {
    UnsafeMemory::Memzero(ipad_, BLOCK_SIZE);
    UnsafeMemory::Memzero(opad_, BLOCK_SIZE);
    delete[] ipad_;
    delete[] opad_;
  }


  //
  // Digester interface
  //

  int32 GetDigestLength() override {
    return DIGEST_SIZE;
  }

  void Reset() override {
    inner_digester_.Reset();
    inner_digester_.Update(ipad_, BLOCK_SIZE);
  }

  const Digest& GetDigest() override {
    const auto& digest = inner_digester_.GetDigest();
    char digest_buffer[DIGEST_SIZE];
    char* ptr = digest_buffer;
    for (const auto& data : digest) {
      *ptr++ = data;
    }
    inner_digester_.Reset();
    inner_digester_.Update(opad_, BLOCK_SIZE);
    inner_digester_.update(digest_buffer, DIGEST_SIZE);
    const auto& result = inner_digester_.GetDigest();
    Reset();
    return result;
  }

  // Disable copy
  HMACDigester(const HMACDigester&) = delete;
  HMACDigester& operator = (const HMACDigester&) = delete;

 protected:
  void Init(const char* pass_phrase, int32 length) {
    ipad_ = new char[BLOCK_SIZE];
    opad_ = new char[BLOCK_SIZE];
    UnsafeMemory::Memzero(ipad_, BLOCK_SIZE);
    UnsafeMemory::Memzero(opad_, BLOCK_SIZE);
    if (length > BLOCK_SIZE) {
      inner_digester_.Reset();
      inner_digester_.Update(pass_phrase, length);
      const auto& d = inner_digester_.GetDigest();
      char* iptr = ipad_;
      char* optr = opad_;
      int32 N = BLOCK_SIZE;
      for (int32 i = 0; i < d.Len() && N > 0; ++i, --N) {
        *iptr++ = d[i];
        *optr++ = d[i];
      }
    } else {
      UnsafeMemory::Memcpy(ipad_, pass_phrase, length);
      UnsafeMemory::Memcpy(opad_, pass_phrase, length);
    }

    for (int32 i = 0; i < BLOCK_SIZE; ++i) {
      ipad_[i] ^= 0x36;
      opad_[i] ^= 0x5c;
    }

    Reset();
  }

  // Digester interface
  void UpdateImpl(const void* data, size_t length) override {
    inner_digester_.Update(data, length);
  }

 private:
  DisgesterTy inner_digester_;
  char* ipad_;
  char* opad_;
};

} // namespace fun
