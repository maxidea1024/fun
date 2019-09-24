#pragma once

namespace fun {
namespace net {

class CryptoAESKey;
class MessageIn;
class MessageOut;

class CryptoAES {
 public:
  enum EncryptionMode {
    ECB = 0,
    CBC = 1,
    CFB = 2
  };

  enum { DEFAULT_BLOCK_SIZE = 16 };
  enum { MAX_BLOCK_SIZE = 32, MAX_ROUNDS = 14, MAX_KC = 8, MAX_BC = 8 };

  static bool ExpandFrom(CryptoAESKey& out_key, const uint8* input_key, int32 key_length = DEFAULT_BLOCK_SIZE, int32 block_size = DEFAULT_BLOCK_SIZE);
  static int GetEncryptSize(const CryptoAESKey& key, int32 input_length);
  static bool Encrypt(const CryptoAESKey& key, const uint8* input, int32 input_length, uint8* output, int32& output_length, EncryptionMode encryption_mode = ECB);
  static bool Decrypt(const CryptoAESKey& key, const uint8* input, int32 input_length, uint8* output, int32& output_length, EncryptionMode encryption_mode = ECB);
  static bool Encrypt(const CryptoAESKey& key, ByteStringView input, ByteArray& output, EncryptionMode encryption_mode = ECB);
  static bool Decrypt(const CryptoAESKey& key, ByteStringView input, ByteArray& output, EncryptionMode encryption_mode = ECB);

 private:
  static bool DefaultEncryptBlock(const CryptoAESKey& key, const uint8* input, uint8* output);
  static bool DefaultDecryptBlock(const CryptoAESKey& key, const uint8* input, uint8* output);
  static bool EncryptBlock(const CryptoAESKey& key, const uint8* input, uint8* output);
  static bool DecryptBlock(const CryptoAESKey& key, const uint8* input, uint8* output);
  static int Mul(int32 a, int32 b);
  static int Mul4(int32 a, char b[]);
  static void Xor(uint8* buff, const uint8* chain, const int32 block_size);

  static const int32 _ALog[256];
  static const int32 _Log[256];

  static const int8 _S[256];

  static const int8 _SI[256];

  static const int32 _T1[256];
  static const int32 _T2[256];
  static const int32 _T3[256];
  static const int32 _T4[256];

  static const int32 _T5[256];
  static const int32 _T6[256];
  static const int32 _T7[256];
  static const int32 _T8[256];

  static const int32 _U1[256];
  static const int32 _U2[256];
  static const int32 _U3[256];
  static const int32 _U4[256];
  static const int8 _RCon[30];
  static const int32 _Shifts[3][4][2];
};


class CryptoAESKey {
  friend class CryptoAES;

 public:
  CryptoAESKey() : key_length(0) {}
  int32 GetKeyLength() const;
  int32 GetBlockSize() const;
  int32 GetRounds() const;
  bool KeyExists() const;
  void Reset();

 private:
  int32 ke[CryptoAES::MAX_ROUNDS+1][CryptoAES::MAX_BC];
  int32 kd[CryptoAES::MAX_ROUNDS+1][CryptoAES::MAX_BC];
  int32 key_length;
  int32 block_size;
  int32 rounds;
};

} // namespace net
} // namespace fun
