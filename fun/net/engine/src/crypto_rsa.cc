#include "CryptoRSA.h"
#include "fun/net/net.h"
#include "libtom.h"

namespace fun {
namespace net {

namespace {

struct RSAProvider {
  prng_state prng;
  int32 prngIndex;
  int32 hashIndex;

  RSAProvider() {
    LTC_MUTEX_CREATE(&ltc_cipher_mutex);
    LTC_MUTEX_CREATE(&ltc_hash_mutex);
    LTC_MUTEX_CREATE(&ltc_prng_mutex);

    if (register_prng(&fortuna_desc) == -1) {
      throw Exception("REGISTER PRNG ERROR : SPRNG");
    }

    if (register_hash(&sha1_desc) == -1) {
      throw Exception("REGISTER HASH ERROR : SHA1");
    }

    ltc_mp = ltm_desc;
    prngIndex = find_prng("fortuna");
    hashIndex = find_hash("sha1");

    // Entropie=128bit
    if (rng_make_prng(128, prngIndex, &prng, nullptr) != CRYPT_OK) {
      throw Exception("PRNG MAKE ERROR");
    }
  }

  ~RSAProvider() {
    LTC_MUTEX_DELETE(&ltc_cipher_mutex);
    LTC_MUTEX_DELETE(&ltc_hash_mutex);
    LTC_MUTEX_DELETE(&ltc_prng_mutex);
    LTC_MUTEX_DELETE(&prng.fortuna.prng_lock);
  }
};

static RSAProvider g_rsa_provider;

}  // namespace

CryptoRSAKey::CryptoRSAKey() {
  key = new rsa_key;
  UnsafeMemory::Memset(key, 0, sizeof(rsa_key));
}

CryptoRSAKey::~CryptoRSAKey() { delete key; }

bool CryptoRSAKey::FromBlob(const ByteArray& blob) {
  if (rsa_import((const uint8*)blob.ConstData(), blob.Len(), key) != CRYPT_OK) {
    TRACE_SOURCE_LOCATION();
    return false;
  }

  return true;
}

bool CryptoRSAKey::ToBlob(ByteArray& out_blob) {
  // 1024bit : 140, 2048bit : 270
  out_blob.ResizeUninitialized(1024);

  unsigned long length = (unsigned long)out_blob.Len();
  if (rsa_export((uint8*)out_blob.MutableData(), &length, PK_PUBLIC, key) !=
      CRYPT_OK) {
    // PUBLIC KEY EXPORT ERROR
    TRACE_SOURCE_LOCATION();
    return false;
  }

  if (length > (unsigned long)out_blob.Len()) {
    // PublicKey to blob result is larger. memory corruption may occur.
    TRACE_SOURCE_LOCATION();
    return false;
  }

  out_blob.ResizeUninitialized(length);
  return true;
}

bool CryptoRSA::CreatePublicAndPrivateKey(CryptoRSAKey& out_xchg_key,
                                          ByteArray& out_public_key_blob) {
  if (rsa_make_key(&g_rsa_provider.prng, g_rsa_provider.prngIndex, 1024 / 8,
                   65537, out_xchg_key.key) != CRYPT_OK) {
    // RSA MAKE KEY ERROR
    TRACE_SOURCE_LOCATION();
    return false;
  }

  // 공개키를 BLOB으로 만든다.
  if (!out_xchg_key.ToBlob(out_public_key_blob)) {
    TRACE_SOURCE_LOCATION();
    return false;
  }

  return true;
}

bool CryptoRSA::EncryptSessionKeyByPublicKey(
    ByteArray& out_encrypted_session_key, const ByteArray& random_block,
    const ByteArray& public_key_blob) {
  CryptoRSAKey PublicKey;
  if (!PublicKey.FromBlob(public_key_blob)) {
    TRACE_SOURCE_LOCATION();
    return false;
  }

  unsigned long BlobLen = (unsigned long)random_block.Len();
  unsigned long DataSize = BlobLen;

  // outlen must be at least the size of the modulus.
  DataSize = mp_unsigned_bin_size(PublicKey.key->N);

  out_encrypted_session_key.ResizeUninitialized(DataSize);

  if (rsa_encrypt_key((const uint8*)random_block.ConstData(), BlobLen,
                      (uint8*)out_encrypted_session_key.MutableData(),
                      &DataSize, nullptr, 0, &g_rsa_provider.prng,
                      g_rsa_provider.prngIndex, g_rsa_provider.hashIndex,
                      PublicKey.key) != CRYPT_OK) {
    TRACE_SOURCE_LOCATION();
    return false;
  }

  if (DataSize > (unsigned long)out_encrypted_session_key.Len()) {
    // encryption result is larger. memory corruption may occur.
    TRACE_SOURCE_LOCATION();
    return false;
  }

  out_encrypted_session_key.ResizeUninitialized(DataSize);

  return true;
}

ResultInfoPtr CryptoRSA::DecryptSessionKeyByPrivateKey(
    ByteArray& out_random_block, const ByteArray& encrypted_session_key,
    const CryptoRSAKey& private_key) {
  int stat = 0;
  unsigned long len = (unsigned long)encrypted_session_key.Len();

  // decrypted msg 사이즈는 encrypted msg 보다 사이즈가 항상 작습니다.
  out_random_block.ResizeUninitialized(Len);

  if (rsa_decrypt_key((const uint8*)encrypted_session_key.ConstData(),
                      encrypted_session_key.Len(),
                      (uint8*)out_random_block.MutableData(), &len, nullptr, 0,
                      g_rsa_provider.hashIndex, &stat,
                      private_key.key) != CRYPT_OK) {
    return ResultInfo::From(ResultCode::DecryptFail, HostId_None, "error");
  }

  // 메세지가 변조가 되었는지 확인합니다.
  if (stat != 1) {
    TRACE_SOURCE_LOCATION();

    return ResultInfo::From(ResultCode::DecryptFail, HostId_None,
                            "incorrect packet");
  }

  if (len > (unsigned long)encrypted_session_key.Len()) {
    TRACE_SOURCE_LOCATION();

    const String text =
        "decryption result is larger. memory corruption may occur.";
    return ResultInfo::From(ResultCode::DecryptFail, HostId_None, text);
  }

  out_random_block.ResizeUninitialized(len);

  return SharedPtr<ResultInfo>();  // OK
}

bool CryptoRSA::CreateRandomBlock(ByteArray& output, int32 length) {
  length = length / 8;  // byte 로 변환
  output.ResizeUninitialized(length);

  if (fortuna_read((uint8*)output.MutableData(), length,
                   &g_rsa_provider.prng) <= 0) {
    // create random block read error.
    TRACE_SOURCE_LOCATION();
    return false;
  }

  return true;
}

}  // namespace net
}  // namespace fun
