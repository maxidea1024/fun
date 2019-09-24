#pragma once

#include "fun/base/base.h"
#include "fun/base/uuid.h"
#include "fun/base/random.h"
#include "fun/base/cryptographic_hash.h"
#include "fun/base/mutex.h"

namespace fun {

/**
 * This class implements a generator for Universal Unique Identifiers,
 * as specified in Appendix A of the DCE 1.1 Remote Procedure
 * Call Specification (http://www.opengroup.org/onlinepubs/9629399/),
 * RFC 2518 (WebDAV), section 6.4.1 and the UUIDs and GUIDs internet
 * draft by Leach/Salz from February, 1998
 * (http://ftp.ics.uci.edu/pub/ietf/webdav/uuid-guid/draft-leach-uuids-guids-01.txt)
 */
class FUN_BASE_API UuidGenerator {
 public:
  /**
   * Creates the UUIDGenerator.
   */
  UuidGenerator();

  /**
   * Destroys the UUIDGenerator.
   */
  ~UuidGenerator();

  /**
   * Creates a new time-based UUID, using the MAC address of
   * one of the system's ethernet adapters.
   * 
   * Throws a SystemException if no MAC address can be
   * obtained.
   */
  Uuid NewUuid();

  /**
   * Creates a name-based UUID.
   */
  Uuid NewUuidFromName(const Uuid& ns_id, const String& name);

  /**
   * Creates a name-based UUID, using the given digest engine.
   * 
   * Note: in order to create a standard-compliant UUID, the given DigestEngine
   * must be either an instance of MD5Engine or SHA1Engine. The version field of
   * the UUID will be set accordingly.
   */
  Uuid NewUuidFromName( const Uuid& ns_id,
                        const String& name,
                        CryptographicHash& hasher);

  /**
   * Creates a name-based UUID, using the given digest engine and version.
   */
  Uuid NewUuidFromName( const Uuid& ns_id,
                        const String& name,
                        CryptographicHash& hasher,
                        UuidVersion version);

  /**
   * Creates a random UUID.
   */
  Uuid NewSecuredRandomUuid();

  /**
   * Creates a random UUID. (fast version: not provide security)
   */
  Uuid NewRandomUuid();

  /**
   * Returns a reference to the default UUIDGenerator.
   */
  static UuidGenerator& DefaultGenerator();

 protected:
  int64 GetTimestamp();

 private:
  FastMutex mutex_;
  Random random_;
  int64 last_time_;
  int32 ticks_;
  uint8 node_[6]; // mac-address
  bool have_node_;
};

} // namespace fun
