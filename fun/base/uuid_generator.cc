#include "fun/base/uuid_generator.h"
#include "fun/base/singleton.h"

// TODO 왜 개별적으로 포함해주어야... platform_win32.h에서 빠진듯...
#if FUN_PLATFORM_WINDOWS_FAMILY
#include <combaseapi.h>
#include "fun/base/windows_less.h"
#endif

#include "fun/base/singleton.h"

namespace fun {

UuidGenerator::UuidGenerator() : ticks_(0), have_node_(false) {
  UnsafeMemory::Memzero(node_, sizeof(node_));
}

UuidGenerator::~UuidGenerator() {}

Uuid UuidGenerator::NewUuid() {
// TODO 눈에 읽힐 정도로 변화가 없어서, 일단은 윈도우즈에서 지원하는것을 사용함.
#if 0
  //TODO Environment::GetNodeId로 대체해야함.
  ScopedLock<FastMutex> guard(mutex_); //fixme: lock된 상태에서 GetTimestamp() 함수가 대기를 할수도 있으므로, 불필요한 stall이 발생할 수 있을듯 싶음.

  if (!have_node_) {
    auto m = PlatformMisc::GetMacAddress();
    if (m.Count() == 6) {
      UnsafeMemory::Memcpy(node_, m.ConstData(), 6);
      have_node_ = true;
    }
  }

  /*
  RFC4122 layout

  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | time_low |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | time_mid | time_hi_and_version |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | clk_seq_hi_res | clk_seq_low | node(0 - 1) |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | node(2 - 5) |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  */

  const int64  time_value = GetTimestamp();
  const uint32 time_low = uint32(time_value & 0xFFFFFFFF);
  const uint16 time_mid = uint16((time_value >> 32) & 0xFFFF);
  const uint16 time_hi_and_version = uint16((time_value >> 48) & 0x0FFF) + ((uint16)UuidVersion::TimeBased << 12);
  const uint16 clock_seq = (uint16(random_.GetUnsignedInt() >> 4) & 0x3FFF) | 0x8000;

  return Uuid(time_low, time_mid, time_hi_and_version, clock_seq, node_);
#else
  GUID guid;
  CoCreateGuid(&guid);
  return Uuid((const uint8*)&guid.Data1);
#endif
}

Uuid UuidGenerator::NewUuidFromName(const Uuid& ns_id, const String& name) {
  CryptographicHash hasher(CryptographicHash::MD5);
  return NewUuidFromName(ns_id, name, hasher);
}

Uuid UuidGenerator::NewUuidFromName(const Uuid& ns_id, const String& name,
                                    CryptographicHash& hasher) {
  UuidVersion version = UuidVersion::NameBased;
  if (hasher.GetAlgorithm() == CryptographicHash::SHA1) {
    version = UuidVersion::NameBasedSHA1;
  }

  return NewUuidFromName(ns_id, name, hasher, version);
}

Uuid UuidGenerator::NewUuidFromName(const Uuid& ns_id, const String& name,
                                    CryptographicHash& hasher,
                                    UuidVersion version) {
  Uuid net_ns_id = ns_id;

  hasher.Reset();
  hasher.AddData(&net_ns_id.u.time_low, sizeof(net_ns_id.u.time_low));
  hasher.AddData(&net_ns_id.u.time_mid, sizeof(net_ns_id.u.time_mid));
  hasher.AddData(&net_ns_id.u.time_hi_and_version,
                 sizeof(net_ns_id.u.time_hi_and_version));
  hasher.AddData(&net_ns_id.u.clock_seq, sizeof(net_ns_id.u.clock_seq));
  hasher.AddData(&net_ns_id.u.node[0], sizeof(net_ns_id.u.node));
  hasher.AddData(name);

  uint8 buffer[16];
  const String hash = hasher.GetResult();
  fun_check(hash.Len() >= 16);
  for (int32 i = 0; i < 16; ++i) {
    buffer[i] = hash[i];
  }

  return Uuid(buffer, version);
}

Uuid UuidGenerator::NewSecuredRandomUuid() {
  uint8 buffer[Uuid::BYTE_LENGTH];
  Random::GenerateCryptRandomData((char*)buffer, sizeof(buffer));
  return Uuid(buffer, UuidVersion::Random);
}

Uuid UuidGenerator::NewRandomUuid() {
  // TODO 현재 문제가 있는지 체크중이라 금지시킴.
  // ScopedLock<FastMutex> guard(mutex_); //random_ context를 보호하기 위해서
  // lock을 사용해야함.
  //
  // union {
  //  struct { uint32 a, b, c, d; };
  //  uint8 buffer[16];
  //} blk;
  //
  // blk.a = random_.GetUnsignedInt();
  // blk.b = random_.GetUnsignedInt();
  // blk.c = random_.GetUnsignedInt();
  // blk.d = random_.GetUnsignedInt();
  //
  // return Uuid(blk.buffer, UuidVersion::random_);
  return NewUuid();
}

// NOTE 정밀도는 1000000임. 윈도우 클럭의 해상도는 10000000이므로 나누기 10을
// 해야함.
static int64 GetTicks() {
  // TODO SystemTime::Cycles()로 대체해도 될듯한데...
#if FUN_PLATFORM_WINDOWS_FAMILY
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);

  ULARGE_INTEGER epoch;  // UNIX epoch (1970-01-01 00:00:00) expressed in
                         // Windows NT FILETIME
  epoch.LowPart = 0xD53E8000;
  epoch.HighPart = 0x019DB1DE;

  ULARGE_INTEGER ts;
  ts.LowPart = ft.dwLowDateTime;
  ts.HighPart = ft.dwHighDateTime;
  ts.QuadPart -= epoch.QuadPart;
  return ts.QuadPart / 10;
#else
  // TODO
  fun_check(0);
#endif
}

int64 UuidGenerator::GetTimestamp() {
  ScopedLock<FastMutex> guard(mutex_);

  int64 now = GetTicks();
  for (;;) {
    if (now != last_time_) {
      last_time_ = now;
      ticks_ = 0;
      break;
    }

    if (ticks_ < 100) {
      ++ticks_;
      break;
    }

    now = GetTicks();
  }

  return now + ticks_;
}

UuidGenerator& UuidGenerator::DefaultGenerator() {
  Singleton<UuidGenerator>::Holder sh;
  return *sh.Get();
}

}  // namespace fun
