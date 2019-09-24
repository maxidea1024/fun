#pragma once

namespace fun {
namespace net {

class CryptoChaCha20Key
{
public:
  ByteArray key;
  bool key_exists;

  inline CryptoChaCha20Key()
    : key_exists(false)
  {}

  inline bool KeyExists() const
  {
    return key_exists;
  }

  inline void Reset()
  {
    key.Clear();
    key_exists = false;
  }
};


/**
*/
class CryptoChaCha20
{
public:
  static bool ExpandFrom(CryptoChaCha20Key& out_key, const uint8* input_key, int32 key_length);

  static int32 GetEncryptSize(const int32 data_length);

  static bool Encrypt(const CryptoChaCha20Key& key, const uint8* input, const int32 input_length, uint8* output, int32& output_length);

  static bool Decrypt(const CryptoChaCha20Key& key, const uint8* input, const int32 input_length, uint8* output, int32& output_length);

  static bool Encrypt(const CryptoChaCha20Key& key, ByteStringView input, ByteArray& output);

  static bool Decrypt(const CryptoChaCha20Key& key, ByteStringView input, ByteArray& output);

private:
  static const int32 MaxKeyLength = 256;

  static bool InternalEncrypt(const CryptoChaCha20Key& key, uint8* output, int32 length);
};

} // namespace net
} // namespace fun
