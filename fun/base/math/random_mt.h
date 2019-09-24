#pragma once

#include "fun/base/base.h"

namespace fun {

class FUN_BASE_API RandomMt {
 public:
  static const int32 MaxRandomValue = int32_MAX;

  RandomMt();

  void Randomize() const;

  void SetSeed(uint32 Seed) const;

  int32 NextInt32() const;
  uint32 NextUInt32() const;
  uint64 NextUInt64() const;
  float NextFloat() const;
  double NextDouble() const;

  int32 RangedInt(int32 Min, int32 Max) const;
  float RangedFloat(float Min, float Max) const;
  double RangedDouble(double Min, double Max) const;

  CGuid NextGuid() const;

 private:
  static const uint32 SIZE = 627;
  static const uint32 PERIOD = 397;
  static const uint32 DIFF = SIZE - PERIOD;

  mutable uint32 MT[SIZE];
  mutable uint32 Index;

  int32 RandHelper(int32 A) const;
  void GenerateNumbers() const;
};

} // namespace fun
