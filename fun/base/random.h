#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * A better random number generator.
 * Random implements a pseudo random number generator
 * (PRNG). The PRNG is a nonlinear additive
 * feedback random number generator using 256 bytes
 * of state information and a period of up to 2^69.
 */
class FUN_BASE_API Random {
 public:
  enum Type {
    /** linear congruential */
    RND_STATE_0 = 8,
    /** x**7 + x**3 + 1 */
    RND_STATE_32 = 32,
    /** x**15 + x + 1 */
    RND_STATE_64 = 64,
    /** x**31 + x**3 + 1 */
    RND_STATE_128 = 128,
    /** x**63 + x + 1 */
    RND_STATE_256 = 256
  };

  /**
   * Creates and initializes the PRNG.
   * Specify either a state buffer size
   * (8 to 256 bytes) or one of the Type values.
   */
  Random(int32 state_size = 256);

  /**
   * Destroys the PRNG.
   */
  ~Random();

  /**
   * Seeds the pseudo random generator with the given seed.
   */
  void SetSeed(uint32 seed);

  /**
   * Seeds the pseudo random generator with a random seed
   * obtained from a RandomInputStream.
   */
  void Randomize();

  /**
   * Returns the next 31-bit pseudo random number.
   */
  uint32 Next() const;

  /**
   * Returns the next 31-bit pseudo random number modulo n.
   */
  uint32 Next(uint32 n) const;

  /**
   * Returns the next pseudo random byte(between 0 and 255).
   */
  uint8 NextByte() const;

  /**
   * Returns the next boolean pseudo random value.
   */
  bool NextBool() const;

  /**
   * Returns the next float pseudo random number between 0.0 and 1.0.
   */
  float NextFloat() const;

  /**
   * Returns the next double pseudo random number between 0.0 and 1.0.
   */
  double NextDouble() const;

  /**
   * Returns the next integer pseudo random number between min and
   * max(inclusive).
   */
  int32 NextRangeI(int32 min, int32 max) const;

  /**
   * Returns the next float pseudo random number between min and max(inclusive).
   */
  float NextRangeF(float min, float max) const;

  /**
   * Returns the next double pseudo random number between min and
   * max(inclusive).
   */
  double NextRangeD(double min, double max) const;

  /**
   * Generate cryptographical random block data.
   *
   * \warning This functions is very slow.
   */
  static int32 GenerateCryptRandomData(char* buffer, size_t length);

 protected:
  void InitState(uint32 seed, char* arg_state, int32 n);
  static uint32 GoodRand(int32 x);

 private:
  enum { MAX_TYPES = 5, NSHUFF = 50 };

  mutable uint32* fptr_;
  mutable uint32* rptr_;
  mutable uint32* state_;
  mutable uint32* end_ptr_;
  mutable char* buffer_;

  int32 rand_type_;
  int32 rand_deg_;
  int32 rand_sep_;
};

//
// inlines
//

FUN_ALWAYS_INLINE uint32 Random::Next(uint32 n) const {
  if (n == 0) {
    return 0;
  }
  return Next() % n;
}

FUN_ALWAYS_INLINE uint8 Random::NextByte() const {
  // return uint8((Next() >> 3) & 0xFF);
  return (uint8)NextRangeI(0, 255);
}

FUN_ALWAYS_INLINE bool Random::NextBool() const {
  return (Next() & 0x1000) != 0;
}

FUN_ALWAYS_INLINE float Random::NextFloat() const {
  return float(Next()) / 0x7FFFFFFF;
}

FUN_ALWAYS_INLINE double Random::NextDouble() const {
  return double(Next()) / 0x7FFFFFFF;
}

FUN_ALWAYS_INLINE int32 Random::NextRangeI(int32 min, int32 max) const {
  // Never use the modulo(%) operation to take the distribution evenly.
  // Instead, it creates a value between 0.0 and 1.0 and scales it.
  int32 range = (max - min) + 1;
  if (range > 0) {
    range = (int32)(NextFloat() *
                    ((float)range - 0.00001f));  // 0.00001f is float error
  } else {
    range = 0;
  }

  return min + range;
}

FUN_ALWAYS_INLINE float Random::NextRangeF(float min, float max) const {
  return min + (max - min) * NextFloat();
}

FUN_ALWAYS_INLINE double Random::NextRangeD(double min, double max) const {
  return min + (max - min) * NextDouble();
}

FUN_ALWAYS_INLINE uint32 Random::GoodRand(int32 x) {
  if (x == 0) {
    x = 123459876;
  }

  const int32 hi = x / 127773;
  const int32 lo = x % 127773;
  x = 16807 * lo - 2836 * hi;

  if (x < 0) {
    x += 0x7FFFFFFF;
  }

  return x;
}

}  // namespace fun
