#pragma once

namespace fun {
namespace net {

class CryptoRC4Key {
 public:
  ByteArray key;
  bool key_exists;

  CryptoRC4Key() : key_exists(false) {}

  bool KeyExists() const { return key_exists; }

  void Reset() {
    key.Clear();
    key_exists = false;
  }
};

class CryptoRC4 {
 public:
  static bool ExpandFrom(CryptoRC4Key& out_key, const uint8* input_key,
                         int32 key_length);
  static int32 GetEncryptSize(const int32 data_length);
  static bool Encrypt(const CryptoRC4Key& key, const uint8* input,
                      const int32 input_length, uint8* output,
                      int32& output_length);
  static bool Decrypt(const CryptoRC4Key& key, const uint8* input,
                      const int32 input_length, uint8* output,
                      int32& output_length);
  static bool Encrypt(const CryptoRC4Key& key, ByteStringView input,
                      ByteArray& output);
  static bool Decrypt(const CryptoRC4Key& key, ByteStringView input,
                      ByteArray& output);

 private:
  static const int32 MAX_KEY_LENGTH = 256;

  static bool InternalEncrypt(const CryptoRC4Key& key, uint8* output,
                              int32 length);
};

}  // namespace net
}  // namespace fun
