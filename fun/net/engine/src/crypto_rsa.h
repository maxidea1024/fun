#pragma once

typedef struct Rsa_key Rsa_key;  //@note libtom에 정의 되어 있음!

namespace fun {
namespace net {

class CryptoRSAKey {
 public:
  /** RSA Key */
  Rsa_key* key;

  CryptoRSAKey();

  ~CryptoRSAKey();

  bool FromBlob(const ByteArray& blob);

  bool ToBlob(ByteArray& To);

  operator Rsa_key*() { return key; }
};

typedef SharedPtr<CryptoRSAKey> RSAKeyPtr;

class CryptoRSA {
 public:
  static bool CreateRandomBlock(ByteArray& output, int32 length);
  static bool CreatePublicAndPrivateKey(CryptoRSAKey& out_xchg_key,
                                        ByteArray& out_public_key_blob);
  static bool EncryptSessionKeyByPublicKey(ByteArray& out_encrypted_session_key,
                                           const ByteArray& random_block,
                                           const ByteArray& public_key_blob_);
  static ResultInfoPtr DecryptSessionKeyByPrivateKey(
      ByteArray& out_random_block, const ByteArray& encrypted_session_key,
      const CryptoRSAKey& private_key);
};

}  // namespace net
}  // namespace fun
