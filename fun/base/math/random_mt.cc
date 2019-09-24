//original source code from:
//  https://github.com/cslarsen/mersenne-twister/blob/master/mersenne-twister.cpp

//TODO 테스팅이 좀 필요해보임..

#include "fun/base/math/random_mt.h"

namespace fun {

RandomMt::RandomMt() : Index(0) {
  //한번은 불러줘야하는데 이게흠...
  Randomize();
}

void RandomMt::Randomize() const {
  SetSeed(Clock::Cycles());
}

void RandomMt::SetSeed(uint32 Seed) const {
  MT[0] = Seed;
  Index = 0;

  for (register uint32 i = 1; i < SIZE; ++i) {
    MT[i] = 0x6c078965*(MT[i-1] ^ MT[i-1]>>30) + i;
  }
}

int32 RandomMt::NextInt32() const {
  return static_cast<int32>(0x7FFFFFFF & NextUInt32());
}

uint32 RandomMt::NextUInt32() const {
  if (Index == 0) {
    GenerateNumbers();
  }

  register uint32 y = MT[Index];

  // Tempering
  y ^= y>>11;
  y ^= y<< 7 & 0x9d2c5680;
  y ^= y<<15 & 0xefc60000;
  y ^= y>>18;

  if (++Index == SIZE) {
    Index = 0;
  }

  return y;
}

uint64 RandomMt::NextUInt64() const {
  return static_cast<uint64>(NextUInt32())<<32 | NextUInt32();
}

float RandomMt::NextFloat() const {
  return float(NextUInt32()) / float(uint32_MAX);
  //return float(NextUInt32()) / (uint32_MAX+1.f);
}

double RandomMt::NextDouble() const {
  return double(NextUInt32()) / double(uint32_MAX);
  //return double(NextUInt32()) / (uint32_MAX+1.);
}

int32 RandomMt::RandHelper(int32 A) const {
  return ((A > 0) ? Math::TruncToInt(NextFloat() * ((float)A - DELTA)) : 0);
}

int32 RandomMt::RangedInt(int32 Min, int32 Max) const {
  const int32 Range = Max - Min + 1;
  return Min + RandHelper(Range);
}

float RandomMt::RangedFloat(float Min, float Max) const {
  return Min + (Max - Min) * NextFloat();
}

double RandomMt::RangedDouble(double Min, double Max) const {
  return Min + (Max - Min) * NextDouble();
}

CGuid RandomMt::NextGuid() const {
  CGuid Guid;
  double* Doubles = (double*)&Guid;
  Doubles[0] = NextDouble();
  Doubles[1] = NextDouble();
  return Guid;
}


#define M32(x) (0x80000000 & x) // 32nd Most Significant Bit
#define L31(x) (0x7FFFFFFF & x) // 31 Least Significant Bits
#define ODD(x) (x & 1) // Check if number is odd

#define UNROLL(expr) \
  y = M32(MT[i]) | L31(MT[i+1]); \
  MT[i] = MT[expr] ^ (y>>1) ^ MATRIX[ODD(y)]; \
  ++i;

void RandomMt::GenerateNumbers() const {
  static const uint32 MATRIX[2] = {0, 0x9908b0df};
  register uint32 y, i = 0;

  // i = [0 ... 225]
  while (i < (DIFF-1)) {
    UNROLL(i+PERIOD);
    UNROLL(i+PERIOD);
  }

  // i = 226
  UNROLL((i+PERIOD) % SIZE);

  // i = [227 ... 622]
  while (i < (SIZE-1)) {
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
  }

  // i = 623
  y = M32(MT[SIZE-1]) | L31(MT[0]);
  MT[SIZE-1] = MT[PERIOD-1] ^ (y>>1) ^ MATRIX[ODD(y)];
}

} // namespace fun
