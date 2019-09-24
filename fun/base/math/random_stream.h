#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Implements a thread-safe SRand based RNG.
 * 
 * Very bad quality in the lower bits. Don't use the modulus (%) operator.
 */
class CRandomStream {
 public:
  /**
   * Default constructor.
   * 
   * The seed should be set prior to use.
   */
  CRandomStream() : InitialSeed(0), Seed(0) {}

  /**
   * Creates and initializes a new random stream from the specified seed value.
   * 
   * \param InSeed - The seed value.
   */
  CRandomStream(int32 InSeed)
    : InitialSeed(InSeed), Seed(InSeed) {}

 public:
  /**
  Initializes this random stream with the specified seed value.

  \param InSeed - The seed value.
  */
  void Initialize(int32 InSeed) {
    InitialSeed = InSeed;
    Seed = InSeed;
  }

  /**
  Resets this random stream to the initial seed value.
  */
  void Reset() const {
    Seed = InitialSeed;
  }

  int32 GetInitialSeed() const {
    return InitialSeed;
  }

  /**
  Generates a new random seed.
  */
  void GenerateNewSeed() {
    Initialize(Math::Rand());
  }

  /**
  Returns a random number between 0 and 1.

  \return Random number.
  */
  float GetFraction() const {
    MutateSeed();

    const float SRandTemp = 1.f;
    float result;

    *(int32*)&result = (*(int32*)&SRandTemp & 0xFF800000) | (Seed & 0x007FFFFF);

    return Math::Fractional(result);
  }

  /**
  Returns a random number between 0 and MAXUINT.

  \return Random number.
  */
  uint32 GetUnsignedInt() const {
    MutateSeed();

    return *(uint32*)&Seed;
  }

  /**
  Returns a random vector of unit size.

  \return Random unit vector.
  */
  Vector GetUnitVector() const {
    Vector result;
    float L;

    do {
      // Check random vectors in the unit sphere so result is statistically uniform.
      result.x = GetFraction() * 2.f - 1.f;
      result.y = GetFraction() * 2.f - 1.f;
      result.z = GetFraction() * 2.f - 1.f;
      L = result.SizeSquared();
    } while (L > 1.f || L < KINDA_SMALL_NUMBER);

    return result.GetUnsafeNormal();
  }

  /**
  Gets the current seed.

  \return current seed.
  */
  int32 GetCurrentSeed() const {
    return Seed;
  }

  /**
  Mirrors the random number API in Math

  \return Random number.
  */
  FUN_ALWAYS_INLINE float FRand() const {
    return GetFraction();
  }

  /**
  Helper function for rand implementations.

  \return A random number in [0..A)
  */
  FUN_ALWAYS_INLINE int32 RandHelper(int32 A) const {
    // Can't just multiply GetFraction by A, as GetFraction could be == 1.f
    return ((A > 0) ? Math::TruncToInt(GetFraction() * ((float)A - DELTA)) : 0);
  }

  /**
  Helper function for rand implementations.

  \return A random number >= Min and <= Max
  */
  FUN_ALWAYS_INLINE int32 RandRange(int32 Min, int32 Max) const {
    const int32 Range = (Max - Min) + 1;

    return Min + RandHelper(Range);
  }

  /**
  Helper function for rand implementations.

  \return A random number >= Min and <= Max
  */
  FUN_ALWAYS_INLINE float FRandRange(float Min, float Max) const {
    return Min + (Max - Min) * FRand();
  }

  /**
  Returns a random vector of unit size.

  \return Random unit vector.
  */
  FUN_ALWAYS_INLINE Vector VRand() const {
    return GetUnitVector();
  }

  /**
  Returns a random unit vector, uniformly distributed, within the specified cone.

  \param Dir - The center direction of the cone
  \param cos_half_angle_rad - Half-angle of cone, in radians.

  \return Normalized - vector within the specified cone.
  */
  FUN_ALWAYS_INLINE Vector VRandCone(Vector const& Dir, float cos_half_angle_rad) {
    if (cos_half_angle_rad > 0.f) {
      float const RandU = FRand();
      float const RandV = FRand();

      // Get spherical coords that have an even distribution over the unit sphere
      // Method described at http://mathworld.wolfram.com/SpherePointPicking.html
      float Theta = 2.f * PI * RandU;
      float Phi = Math::Acos((2.f * RandV) - 1.f);

      // restrict phi to [0, cos_half_angle_rad]
      // this gives an even distribution of points on the surface of the cone
      // centered at the origin, pointing upward (z), with the desired angle
      Phi = Math::Fmod(Phi, cos_half_angle_rad);

      // get axes we need to rotate around
      Matrix const DirMat = CRotationMatrix(Dir.Rotation());
      // note the axis translation, since we want the variation to be around x
      Vector const DirZ = DirMat.GetUnitAxis(Axis::X);
      Vector const DirY = DirMat.GetUnitAxis(Axis::Y);

      Vector result = Dir.RotateAngleAxis(Phi * 180.f / PI, DirY);
      result = result.RotateAngleAxis(Theta * 180.f / PI, DirZ);

      // ensure it's a unit vector (might not have been passed in that way)
      result = result.GetSafeNormal();

      return result;
    } else {
      return Dir.GetSafeNormal();
    }
  }

  /**
  Returns a random unit vector, uniformly distributed, within the specified cone.

  \param Dir - The center direction of the cone
  \param horizontal_cone_half_angle_rad - Horizontal half-angle of cone, in radians.
  \param vertical_cone_half_angle_rad - Vertical half-angle of cone, in radians.

  \return Normalized vector within the specified cone.
  */
  FUN_ALWAYS_INLINE Vector VRandCone(Vector const& Dir, float horizontal_cone_half_angle_rad, float vertical_cone_half_angle_rad) {
    if ((vertical_cone_half_angle_rad > 0.f) && (horizontal_cone_half_angle_rad > 0.f)) {
      float const RandU = FRand();
      float const RandV = FRand();

      // Get spherical coords that have an even distribution over the unit sphere
      // Method described at http://mathworld.wolfram.com/SpherePointPicking.html
      float Theta = 2.f * PI * RandU;
      float Phi = Math::Acos((2.f * RandV) - 1.f);

      // restrict phi to [0, cos_half_angle_rad]
      // where cos_half_angle_rad is now a function of Theta
      // (specifically, radius of an ellipse as a function of angle)
      // function is ellipse function (x/a)^2 + (y/b)^2 = 1, converted to polar coords
      float cos_half_angle_rad = Math::Square(Math::Cos(Theta) / vertical_cone_half_angle_rad) + Math::Square(Math::Sin(Theta) / horizontal_cone_half_angle_rad);
      cos_half_angle_rad = Math::Sqrt(1.f / cos_half_angle_rad);

      // clamp to make a cone instead of a sphere
      Phi = Math::Fmod(Phi, cos_half_angle_rad);

      // get axes we need to rotate around
      Matrix const DirMat = CRotationMatrix(Dir.Rotation());
      // note the axis translation, since we want the variation to be around x
      Vector const DirZ = DirMat.GetUnitAxis(Axis::X);
      Vector const DirY = DirMat.GetUnitAxis(Axis::Y);

      Vector result = Dir.RotateAngleAxis(Phi * 180.f / PI, DirY);
      result = result.RotateAngleAxis(Theta * 180.f / PI, DirZ);

      // ensure it's a unit vector (might not have been passed in that way)
      result = result.GetSafeNormal();

      return result;
    } else {
      return Dir.GetSafeNormal();
    }
  }

 protected:
  /**
   * Mutates the current seed into the next seed.
   */
  void MutateSeed() const {
    Seed = (Seed * 196314165) + 907633515;
  }

 private:
  /** Holds the initial seed. */
  int32 InitialSeed;

  /** Holds the current seed. */
  mutable int32 Seed;
};

} // namespace fun
