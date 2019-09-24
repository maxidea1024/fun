#include "CryptoRC4.h"
#include "fun/net/net.h"
#include "libtom.h"

namespace fun {
namespace net {

bool CryptoRC4::ExpandFrom(CryptoRC4Key& out_key, const uint8* input_key,
                           int32 key_length) {
  if (key_length == 0) {
    TRACE_SOURCE_LOCATION();

    // RC4 키의 길이가 0이면, 사용하지 않을 경우이므로 키교환이 제대로
    // 이루어졌음을 세팅합니다.
    out_key.key_exists = true;
    return true;
  }

  if (input_key == nullptr) {
    // RC4 ExpandFrom error. input_key is nullptr.
    TRACE_SOURCE_LOCATION();
    return false;
  }

  uint8 vector_t[MaxKeyLength];
  out_key.key.ResizeUninitialized(key_length);

  // initialization
  // Key값 & 임시 벡터 T생성
  for (int32 i = 0; i < key_length; ++i) {
    out_key.key[i] = i;
    vector_t[i] = input_key[i % key_length];
  }

  char* data = out_key.key.MutableData();

  // 최초 치환 key 생성
  // Initial permutation of s
  int32 j = 0;
  for (int32 i = 0; i < key_length; ++i) {
    j = ((j + data[i] + vector_t[i]) % key_length);
    Swap(data[i], data[j % key_length]);
  }

  out_key.key_exists = true;

  return true;
}

int32 CryptoRC4::GetEncryptSize(const int32 data_length) {
  return data_length + sizeof(uint32);
}

bool CryptoRC4::Encrypt(const CryptoRC4Key& key, const uint8* input,
                        const int32 input_length, uint8* output,
                        int32& output_length) {
  if (!key.KeyExists()) {
    TRACE_SOURCE_LOCATION();

    // 대칭키의 키값이 없습니다.
    return false;
  }

  if (key.key.IsEmpty()) {
    TRACE_SOURCE_LOCATION();

    // ServerStartParameter::FastEncryptedMessageKeyLength 를 세팅하지 않고
    // 사용하는 경우
    return false;
  }

  if (input_length == 0) {
    TRACE_SOURCE_LOCATION();

    // 암호화할 데이터가 없음
    return true;
  }

  if (output_length < GetEncryptSize(input_length)) {
    TRACE_SOURCE_LOCATION();

    // output 사이즈가 작습니다.
    return false;
  }

  // 암호화할 데이터를 복사합니다.
  UnsafeMemory::Memcpy(output, input, input_length);

  // Crc를 뒤에 붙입니다.
  const uint32 crc = Crc::Crc32((uint8*)input, input_length);
  //@todo endian issue는 없으려나??
  UnsafeMemory::Memcpy(output + input_length, &crc, sizeof(crc));

  if (!InternalEncrypt(key, output, GetEncryptSize(input_length))) {
    TRACE_SOURCE_LOCATION();
    return false;
  }

  output_length = input_length + sizeof(crc);
  return true;
}

bool CryptoRC4::Decrypt(const CryptoRC4Key& key, const uint8* input,
                        const int32 input_length, uint8* output,
                        int32& output_length) {
  if (!key.KeyExists()) {
    TRACE_SOURCE_LOCATION();

    // RC4 : 대칭키의 키값이 없습니다.
    return false;
  }

  // WeakEncrypt를 사용하지 않는 경우
  if (key.key.IsEmpty()) {
    TRACE_SOURCE_LOCATION();

    // ServerStartParameter::weak_encrypted_message_key_length 를 세팅하지 않고
    // 사용하는 경우
    return false;
  }

  if (input_length == 0) {
    TRACE_SOURCE_LOCATION();

    // 복호화할 데이터가 없음
    return true;
  }

  // 복호화 후 데이터의 길이가 충분한지 확인합니다.
  if (output_length < input_length) {
    TRACE_SOURCE_LOCATION();

    // 실제 복호화후 사이즈는 작아지지만 복호화시 InputLength사이즈가
    // 필요합니다.
    return false;
  }

  UnsafeMemory::Memcpy(output, input, input_length);

  // 실제 복호화를 수행합니다.
  if (!InternalEncrypt(key, output, input_length)) {
    TRACE_SOURCE_LOCATION();
    return false;
  }

  // Crc값을 체크합니다.
  const uint32 calculated_crc =
      Crc::Crc32(output, input_length - sizeof(uint32));
  //@todo endian issue를 처리해야할듯함!!
  uint32 tossed_crc;
  UnsafeMemory::Memcpy(&tossed_crc, output + input_length - sizeof(uint32),
                       sizeof(tossed_crc));
  if (calculated_crc != tossed_crc) {
    TRACE_SOURCE_LOCATION();
    return false;
  }
  output_length = input_length - sizeof(uint32);

  return true;
}

bool CryptoRC4::Encrypt(const CryptoRC4Key& key, ByteStringView input,
                        ByteArray& output) {
  int32 output_length = GetEncryptSize(input.Len());
  output.ResizeUninitialized(output_length);
  return Encrypt(key, (const uint8*)input.ConstData(), input.Len(),
                 (uint8*)output.MutableData(), output_length);
}

bool CryptoRC4::Decrypt(const CryptoRC4Key& key, ByteStringView input,
                        ByteArray& output) {
  int32 output_length = input.Len();
  output.ResizeUninitialized(output_length);

  const bool decryption_ok =
      Decrypt(key, (const uint8*)input.ConstData(), output_length,
              (uint8*)output.MutableData(), output_length);
  if (!decryption_ok) {
    TRACE_SOURCE_LOCATION();
    return false;
  }

  output.ResizeUninitialized(output_length);
  return true;
}

template <size_t KnownLength>
void OptimizeEncrypt(const CryptoRC4Key& key, uint8* output, int32 length) {
  uint8 key_copy[KnownLength];
  UnsafeMemory::Memcpy(key_copy, key.key.ConstData(), KnownLength);

  // stream generation
  int32 i = 0, j = 0;
  for (int32 counter = 0; counter < length; ++counter) {
    i = (i + 1) % KnownLength;
    j = (j + key_copy[i]) % KnownLength;

    const int32 sum = key_copy[i] + key_copy[j];
    Swap(key_copy[i], key_copy[j]);

    const int32 t = sum % KnownLength;
    output[counter] ^= key_copy[t];
  }
}

bool CryptoRC4::InternalEncrypt(const CryptoRC4Key& key, uint8* output,
                                int32 length) {
  // 키 길이가 2의 승수일 경우 컴파일러에 의해 상수일 경우 컴파일 타임에
  // 최적화가 이루어집니다.
  switch (key.key.Len() * 8) {
    case (int32)WeakEncryptionLevel::Low:
      OptimizeEncrypt<64>(key, output, length);
      break;

    case (int32)WeakEncryptionLevel::Middle:
      OptimizeEncrypt<128>(key, output, length);
      break;

    case (int32)WeakEncryptionLevel::High:
      OptimizeEncrypt<256>(key, output, length);
      break;

    default:
      TRACE_SOURCE_LOCATION();
      return false;
  }

  return true;
}

}  // namespace net
}  // namespace fun
