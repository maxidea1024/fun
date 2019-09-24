#include "fun/base/cryptographic_hash.h"

#include "fun/base/digest/md4_digester.h"
#include "fun/base/digest/md5_digester.h"
#include "fun/base/digest/sha1_digester.h"
#include "fun/base/digest/sha2_digester.h"
#include "fun/base/digest/sha3_digester.h"

namespace fun {

CryptographicHash::CryptographicHash(Algorithm algorithm)
  : algorithm_(algorithm) {
  switch (algorithm_) {
    case MD4:
      digester_ = new MD4Disgester();
      break;

    case MD5:
      digester_ = new MD5Disgester();
      break;

    case SHA1:
      digester_ = new SHA1Digester();
      break;

    case SHA2_224:
      digester_ = new SHA2Digester(SHA2Digester::SHA2_224);
      break;
    case SHA2_256:
      digester_ = new SHA2Digester(SHA2Digester::SHA2_256);
      break;
    case SHA2_384:
      digester_ = new SHA2Digester(SHA2Digester::SHA2_384);
      break;
    case SHA2_512:
      digester_ = new SHA2Digester(SHA2Digester::SHA2_512);
      break;

    case SHA3_224:
      digester_ = new SHA3Digester(SHA3Digester::SHA3_224);
      break;
    case SHA3_256:
      digester_ = new SHA3Digester(SHA3Digester::SHA3_256);
      break;
    case SHA3_384:
      digester_ = new SHA3Digester(SHA3Digester::SHA3_384);
      break;
    case SHA3_512:
      digester_ = new SHA3Digester(SHA3Digester::SHA3_512);
      break;

    default:
      fun_unexpected();
      break;
  }
}

CryptographicHash::~CryptographicHash() {
  delete digester_;
}

void CryptographicHash::Reset() {
  digester_->Reset();
}

void CryptographicHash::AddData(const char* data, int32 len) {
  digester_->Update(data, len);
}

void CryptographicHash::AddData(const ByteArray& data) {
  AddData(data.ConstData(), data.Len());
}

bool CryptographicHash::AddData(IFile* file) {
  //TODO
  fun_check(0);
  return false;
}

CryptographicHash::Algorithm CryptographicHash::GetAlgorithm() const {
  return algorithm_;
}

ByteArray CryptographicHash::GetResult() const {
  return digester_->GetDigest();
}

ByteArray CryptographicHash::Hash(const ByteArray& data, Algorithm algorithm) {
  CryptographicHash hash(algorithm);
  hash.AddData(data);
  return hash.GetResult();
}

} // namespace fun
