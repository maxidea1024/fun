#pragma once

#include "CryptoAES.h"
#include "CryptoRC4.h"
#include "CryptoRSA.h"

namespace fun {
namespace net {

/**
 * Cryptography counter type.
 */
typedef uint16 CryptoCountType;


/**
 * Session security key.
 */
class SessionKey
{
public:
  CryptoAESKey aes_key;
  CryptoRC4Key rc4_key;

  /** Clear keys. */
  inline void Reset()
  {
    aes_key.Reset();
    rc4_key.Reset();
  }

  /** Returns true if keys are exists. */
  inline bool KeyExists() const
  {
    return aes_key.KeyExists() && rc4_key.KeyExists();
  }
};

} // namespace net
} // namespace fun
