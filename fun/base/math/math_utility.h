#pragma once

#include "fun/base/types.h"
#include "fun/base/math/math_base.h"

namespace fun {

//#define IMPLEMENT_ASSIGNMENT_OPERATOR_MANUALLY

// Assert on non finite numbers. Used to track NaNs.
#ifndef FUN_ENABLE_NAN_DIAGNOSTIC
#define FUN_ENABLE_NAN_DIAGNOSTIC  0
#endif

//
// Definitions.
//

// Forward declarations.
struct Vector;
struct Vector4;
struct Plane;
struct Box;
struct Rotator;
struct Matrix;
struct Quat;
struct TwoVectors;
struct Transform;
struct Sphere;
struct Vector2;
struct LinearColor;


//
// Floating point constants.
//

#undef  PI
#define PI                  (3.1415926535897932f)
#define SMALL_NUMBER        (1.e-8f)
#define KINDA_SMALL_NUMBER  (1.e-4f)
#define BIG_NUMBER          (3.4e+38f)
#define EULERS_NUMBER       (2.71828182845904523536f)

// Copied from float.h
#define MAX_FLT             (3.402823466e+38F)

// Aux constants.
#define INV_PI              (0.31830988618f)
#define HALF_PI             (1.57079632679f)
#define TWO_PI              (6.28318530717f)

#define RADIANS_TO_DEGREES  (180.f / PI)
#define DEGREES_TO_RADIANS  (PI / 180.f)

// Magic numbers for numerical precision.
#define DELTA               (0.00001f)

/**
 * Lengths of normalized vectors (These are half their maximum values
 * to assure that dot products with normalized vectors don't overflow).
 */
#define FLOAT_NORMAL_THRESH             (0.0001f)

//
// Magic numbers for numerical precision.
//
#define THRESH_POINT_ON_PLANE           (0.10f)     /* Thickness of plane for front/back/inside test */
#define THRESH_POINT_ON_SIDE            (0.20f)     /* Thickness of polygon side's side-plane for point-inside/outside/on side test */
#define THRESH_POINTS_ARE_SAME          (0.00002f)  /* Two points are same if within this distance */
#define THRESH_POINTS_ARE_NEAR          (0.015f)    /* Two points are near if within this distance and can be combined if imprecise math is ok */
#define THRESH_NORMALS_ARE_SAME         (0.00002f)  /* Two normal points are same if within this distance */
                          /* Making this too large results in incorrect CSG classification and disaster */
#define THRESH_VECTORS_ARE_NEAR         (0.0004f)   /* Two vectors are near if within this distance and can be combined if imprecise math is ok */
                          /* Making this too large results in lighting problems due to inaccurate texture coordinates */
#define THRESH_SPLIT_POLY_WITH_PLANE    (0.25f)     /* a plane splits a polygon in half */
#define THRESH_SPLIT_POLY_PRECISELY     (0.01f)     /* a plane exactly splits a polygon */
#define THRESH_ZERO_NORM_SQUARED        (0.0001f)   /* Size of a unit normal that is considered "zero", squared */
#define THRESH_NORMALS_ARE_PARALLEL     (0.999845f) /* Two unit vectors are parallel if abs(a dot b) is greater than or equal to this. This is roughly cosine(1.0 degrees). */
#define THRESH_NORMALS_ARE_ORTHOGONAL   (0.017455f) /* Two unit vectors are orthogonal (perpendicular) if abs(a dot b) is less than or equal this. This is roughly cosine(89.0 degrees). */

#define THRESH_VECTOR_NORMALIZED        (0.01f)     /** Allowed error for a normalized vector (against squared magnitude) */
#define THRESH_QUAT_NORMALIZED          (0.01f)     /** Allowed error for a normalized quaternion (against squared magnitude) */


//
// Global functions.
//

/**
 * Structure for all math helper functions, inherits from platform math to pick up platform-specific implementations
 * Check math_base_generic.h for additional math functions
 */
struct Math : public MathBase {
  // Random Number Functions

  ///** Helper function for rand implementations. Returns a random number in [0..a) */
  //static FUN_ALWAYS_INLINE int32 RandHelper(int32 a) {
  //  // RAND_MAX+1 give interval [0..a) with even distribution.
  //  return a > 0 ? TruncToInt(Rand() / (float)((uint32)RAND_MAX + 1) * a) : 0;
  //}
  //
  ///** Helper function for rand implementations. Returns a random number >= Min and <= Max */
  //static FUN_ALWAYS_INLINE int32 RandRange(int32 min, int32 max) {
  //  const int32 range = (max - min) + 1;
  //  return min + RandHelper(range);
  //}
  //
  ///** Util to generate a random number in a range. Overloaded to distinguish from int32 version, where passing a float is typically a mistake. */
  //static FUN_ALWAYS_INLINE float RandRange(float min, float max) {
  //  return FRandRange(min, max);
  //}
  //
  ///** Util to generate a random number in a range. */
  //static FUN_ALWAYS_INLINE float FRandRange(float min, float max) {
  //  return min + (max - min) * FRand();
  //}
  //
  ///** Util to generate a random boolean. */
  //static FUN_ALWAYS_INLINE bool RandBool() {
  //  return (RandRange(0, 1) == 1) ? true : false;
  //}

  /**
   * Return a uniformly distributed random unit length vector = point on the unit sphere surface.
   */
  static Vector VRand();

  /**
   * Returns a random unit vector, uniformly distributed, within the specified cone
   * cos_half_angle_rad is the half-angle of cone, in radians.  Returns a normalized vector.
   */
  static FUN_BASE_API Vector VRandCone(Vector const& dir, float cos_half_angle_rad);

  /**
   * This is a version of VRandCone that handles "squished" cones, i.e. with different angle limits in the y and z axes.
   * Assumes world y and z, although this could be extended to handle arbitrary rotations.
   */
  static FUN_BASE_API Vector VRandCone(Vector const& Dir, float horizontal_cone_half_angle_rad, float vertical_cone_half_angle_rad);

  /**
   * Returns a random point within the passed in bounding box
   */
  static FUN_BASE_API Vector RandPointInBox(const Box& box);

  /**
   * Given a direction vector and a surface normal, returns the vector reflected across the surface normal.
   * Produces a result like shining a laser at a mirror!
   *
   * \param direction - direction vector the ray is coming from.
   * \param surface_normal - a normal of the surface the ray should be reflected on.
   *
   * \returns Reflected vector.
   */
  static FUN_BASE_API Vector GetReflectionVector(const Vector& direction, const Vector& surface_normal);


  /**
   * Clamps an arbitrary angle to be between the given angles.
   * Will clamp to nearest boundary.
   *
   * \param min_angle_degrees - "from" angle that defines the beginning of the range of valid angles (sweeping clockwise)
   * \param max_angle_degrees - "to" angle that defines the end of the range of valid angles
   *
   * \return Returns clamped angle in the range -180..180.
   */
  static float FUN_BASE_API ClampAngle(float angle_degrees, float min_angle_degrees, float max_angle_degrees);

  /**
   * Find the smallest angle between two headings (in radians)
   */
  static float FindDeltaAngle(float a1, float a2) {
    // Find the difference
    float delta = a2 - a1;

    // If change is larger than PI
    if (delta > PI) {
      // Flip to negative equivalent
      delta = delta - TWO_PI;
    } else if (delta < -PI) {
      // Otherwise, if change is smaller than -PI
      // Flip to positive equivalent
      delta = delta + TWO_PI;
    }

    // Return delta in [-PI, PI] range
    return delta;
  }

  /**
   * Given a heading which may be outside the +/- PI range, 'unwind' it back into that range.
   */
  static float UnwindRadians(float radians) {
    while (radians > PI) {
      radians -= (float)TWO_PI;
    }

    while (radians < -PI) {
      radians += (float)TWO_PI;
    }

    return radians;
  }

  /**
   * Utility to ensure angle is between +/- 180 degrees by unwinding.
   */
  static float UnwindDegrees(float degrees) {
    while (degrees > 180.f) {
      degrees -= 360.f;
    }

    while (a < -180.f) {
      degrees += 360.f;
    }

    return a;
  }

  /**
   * Returns a new rotation component value
   *
   * \param current - is the current rotation value
   * \param desired - is the desired rotation value
   * \param delta_rate - is the rotation amount to apply
   *
   * \return a new rotation component value
   */
  static FUN_BASE_API float FixedTurn(float current, float desired, float delta_rate);

  /**
   * Converts given cartesian coordinate pair to polar coordinate system.
   */
  static FUN_ALWAYS_INLINE void CartesianToPolar(const float x, const float y, float& out_radius, float& out_radians) {
    out_radius = Sqrt(Square(x) + Square(y));
    out_radians = Atan2(y, x);
  }

  /**
   * Converts given cartesian coordinate pair to polar coordinate system.
   */
  static FUN_ALWAYS_INLINE void CartesianToPolar(const Vector2& cartesian, Vector2& out_polar);

  /**
   * Converts given polar coordinate pair to cartesian coordinate system.
   */
  static FUN_ALWAYS_INLINE void PolarToCartesian(const float radius, const float radians, float& out_x, float& out_y) {
    out_x = radius * Cos(radians);
    out_y = radius * Sin(radians);
  }

  /**
   * Converts given polar coordinate pair to cartesian coordinate system.
   */
  static FUN_ALWAYS_INLINE void PolarToCartesian(const Vector2& polar, Vector2& out_cartesian);

  /**
   * Calculates the dotted distance of vector 'direction' to coordinate system O(axis_x, axis_y, axis_z).
   *
   * Orientation: (consider 'O' the first person view of the player, and 'direction' a vector pointing to an enemy)
   * - positive azimuth means enemy is on the right of crosshair. (negative means left).
   * - positive elevation means enemy is on top of crosshair, negative means below.
   *
   * @Note: 'Azimuth' (.x) sign is changed to represent left/right and not front/behind. front/behind is the funtion's return value.
   *
   * \param out_dot_dist - .x = 'direction' dot axis_x relative to plane (axis_x, axis_z). (== Cos(Azimuth))
   *                       .y = 'direction' dot axis_x relative to plane (axis_x, axis_y). (== Sin(Elevation))
   * \param direction - direction of target.
   * \param axis_x - x component of reference system.
   * \param axis_y - y component of reference system.
   * \param axis_z - z component of reference system.
   *
   * \return true if 'direction' is facing axis_x (direction dot axis_x >= 0.f)
   */
  static FUN_BASE_API bool GetDotDistance(Vector2 &out_dot_dist, const Vector& direction, const Vector& axis_x, const Vector& axis_y, const Vector& axis_z);

  /**
   * Returns Azimuth and Elevation of vector 'direction' in coordinate system O(axis_x, axis_y, axis_z).
   *
   * Orientation: (consider 'O' the first person view of the player, and 'direction' a vector pointing to an enemy)
   * - positive azimuth means enemy is on the right of crosshair. (negative means left).
   * - positive elevation means enemy is on top of crosshair, negative means below.
   *
   * \param direction - direction of target.
   * \param axis_x - x component of reference system.
   * \param axis_y - y component of reference system.
   * \param axis_z - z component of reference system.
   *
   * \return Vector2 x = Azimuth angle (in radians) (-PI, +PI)
   *                 y = Elevation angle (in radians) (-PI/2, +PI/2)
   */
  static FUN_BASE_API Vector2 GetAzimuthAndElevation(const Vector& direction, const Vector& axis_x, const Vector& axis_y, const Vector& axis_z);


  //
  // Interpolation Functions
  //

  /**
   * Calculates the percentage along a line from min_value to max_value that value is.
   */
  static FUN_ALWAYS_INLINE float GetRangePct(float min_value, float max_value, float value) {
    return (value - min_value) / (max_value - min_value);
  }

  /**
   * Same as above, but taking a 2d vector as the range.
   */
  static float GetRangePct(Vector2 const& range, float value);

  /**
   * Basically a Vector2 version of Lerp.
   */
  static float GetRangeValue(Vector2 const& range, float Pct);

  /**
   * For the given value clamped to the input range, returns the corresponding value in the output range.
   */
  static FUN_ALWAYS_INLINE float GetMappedRangeValueClamped(const Vector2& input_range, const Vector2& output_range, const float value) {
    const float clamped_pct = Clamp01<float>(GetRangePct(input_range, value));
    return GetRangeValue(output_range, clamped_pct);
  }

  /**
   * Transform the given value relative to the input range to the output range.
   */
  static FUN_ALWAYS_INLINE float GetMappedRangeValueUnclamped(const Vector2& input_range, const Vector2& output_range, const float value) {
    return GetRangeValue(output_range, GetRangePct(input_range, value));
  }

  /**
   * Performs a linear interpolation between two values, alpha ranges from 0-1
   */
  template <typename T, typename U>
  static FUN_ALWAYS_INLINE_DEBUGGABLE T Lerp(const T& a, const T& b, const U& alpha) {
    return (T)(a + alpha * (b - a));
  }

  /**
   * Performs a linear interpolation between two values, alpha ranges from 0-1.
   * Handles full numeric range of T
   */
  template <typename T>
  static FUN_ALWAYS_INLINE_DEBUGGABLE T LerpStable(const T& a, const T& b, double alpha) {
    return (T)((a * (1.0 - alpha)) + (b * alpha));
  }

  /**
   * Performs a linear interpolation between two values, alpha ranges from 0-1.
   * Handles full numeric range of T
   */
  template <typename T>
  static FUN_ALWAYS_INLINE_DEBUGGABLE T LerpStable(const T& a, const T& b, float alpha) {
    return (T)((a * (1.f - alpha)) + (b * alpha));
  }

  /**
   * Performs a 2D linear interpolation between four values values, frac_x, frac_y ranges from 0-1
   */
  template <typename T, typename U>
  static FUN_ALWAYS_INLINE_DEBUGGABLE T BiLerp(const T& p00, const T& p10, const T& p01, const T& p11, const U& frac_x, const U& frac_y) {
    return Lerp(Lerp(p00, p10, frac_x), Lerp(p01, p11, frac_x), frac_y);
  }

  /**
   * Performs a cubic interpolation
   *
   * \param P - end points
   * \param T - tangent directions at end points
   * \param alpha - distance along spline
   *
   * \return Interpolated value
   */
  template <typename T, typename U>
  static FUN_ALWAYS_INLINE_DEBUGGABLE T CubicInterp(const T& p0, const T& t0, const T& p1, const T& t1, const U& a) {
    const float a2 = a * a;
    const float a3 = a2 * a;
    return (T)(((2*a3)-(3*a2)+1) * p0) + ((a3-(2*a2)+a) * t0) + ((a3-a2) * t1) + (((-2*a3)+(3*a2)) * p1);
  }

  /**
   * Performs a first derivative cubic interpolation
   *
   * \param p - end points
   * \param t - tangent directions at end points
   * \param alpha - distance along spline
   *
   * \return Interpolated value
   */
  template <typename T, typename U>
  static FUN_ALWAYS_INLINE_DEBUGGABLE T CubicInterpDerivative(const T& p0, const T& t0, const T& p1, const T& t1, const U& a) {
    const T a =  6.f*p0 + 3.f*t0 + 3.f*t1 - 6.f*p1;
    const T b = -6.f*p0 - 4.f*t0 - 2.f*t1 + 6.f*p1;
    const T c = t0;
    const float a2 = a * a;
    return (a * a2) + (b * a) + c;
  }

  /**
   * Performs a second derivative cubic interpolation
   *
   * \param p - end points
   * \param t - tangent directions at end points
   * \param alpha - distance along spline
   *
   * \return Interpolated value
   */
  template <typename T, typename U>
  static FUN_ALWAYS_INLINE_DEBUGGABLE T CubicInterpSecondDerivative(const T& p0, const T& t0, const T& p1, const T& t1, const U& a) {
    const T a = 12.f*p0 + 6.f*t0 + 6.f*t1 - 12.f*p1;
    const T b = -6.f*p0 - 4.f*t0 - 2.f*t1 +  6.f*p1;
    return (a * a) + b;
  }

  /**
   * Interpolate between a and b, applying an ease in function.
   * Exp controls the degree of the curve.
   */
  template <typename T>
  static FUN_ALWAYS_INLINE_DEBUGGABLE T InterpEaseIn(const T& a, const T& b, float alpha, float exp) {
    const float modified_alpha = Math::Pow(alpha, exp);
    return Lerp<T>(a, b, modified_alpha);
  }

  /**
   * Interpolate between a and b, applying an ease out function.
   * Exp controls the degree of the curve.
   */
  template <typename T>
  static FUN_ALWAYS_INLINE_DEBUGGABLE T InterpEaseOut(const T& a, const T& b, float alpha, float exp) {
    const float modified_alpha = 1.f - Math::Pow(1.f - alpha, exp);
    return Lerp<T>(a, b, modified_alpha);
  }

  /**
   * Interpolate between a and b, applying an ease in/out function.
   * Exp controls the degree of the curve.
   */
  template <typename T>
  static FUN_ALWAYS_INLINE_DEBUGGABLE T InterpEaseInOut(const T& a, const T& b, float alpha, float exp) {
    float const modified_alpha = (alpha < 0.5f) ?
      0.5f * Pow(2.f * alpha, exp) :
      1.f - 0.5f * Pow(2.f * (1.f - alpha), exp);

    return Lerp<T>(a, b, modified_alpha);
  }

  /**
   * Interpolation between a and b, applying a step function.
   */
  template <typename T>
  static FUN_ALWAYS_INLINE_DEBUGGABLE T InterpStep(const T& a, const T& b, float alpha, int32 step_count) {
    if (step_count <= 1) {
      return a;
    }

    const float step_count_as_float = static_cast<float>(step_count);
    const float interval_count = step_count_as_float - 1.f;
    float const modified_alpha = FloorToFloat(alpha * step_count_as_float) / interval_count;
    return Lerp<T>(a, b, modified_alpha);
  }

  /**
   * Interpolation between a and b, applying a sinusoidal in function.
   */
  template <typename T>
  static FUN_ALWAYS_INLINE_DEBUGGABLE T InterpSinIn(const T& a, const T& b, float alpha) {
    float const modified_alpha = -1.f * Cos(alpha * HALF_PI) + 1.f;
    return Lerp<T>(a, b, modified_alpha);
  }

  /**
   * Interpolation between a and b, applying a sinusoidal out function.
   */
  template <typename T>
  static FUN_ALWAYS_INLINE_DEBUGGABLE T InterpSinOut(const T& a, const T& b, float alpha) {
    float const modified_alpha = Sin(alpha * HALF_PI);
    return Lerp<T>(a, b, modified_alpha);
  }

  /**
   * Interpolation between a and b, applying a sinusoidal in/out function.
   */
  template <typename T>
  static FUN_ALWAYS_INLINE_DEBUGGABLE T InterpSinInOut(const T& a, const T& b, float alpha) {
    return (alpha < 0.5f) ?
      InterpSinIn(a, b, alpha * 2.f) * 0.5f :
      InterpSinOut(a, b, alpha * 2.f - 1.f) * 0.5f + 0.5f;
  }

  /**
   * Interpolation between a and b, applying an exponential in function.
   */
  template <typename T>
  static FUN_ALWAYS_INLINE_DEBUGGABLE T InterpExpoIn(const T& a, const T& b, float alpha) {
    float const modified_alpha = (alpha == 0.f) ? 0.f : Math::Pow(2.f, 10.f * (alpha - 1.f));
    return Lerp<T>(a, b, modified_alpha);
  }

  /**
   * Interpolation between a and b, applying an exponential out function.
   */
  template <typename T>
  static FUN_ALWAYS_INLINE_DEBUGGABLE T InterpExpoOut(const T& a, const T& b, float alpha) {
    float const modified_alpha = (alpha == 1.f) ? 1.f : -Math::Pow(2.f, -10.f * alpha) + 1.f;
    return Lerp<T>(a, b, modified_alpha);
  }

  /**
   * Interpolation between a and b, applying an exponential in/out function.
   */
  template <typename T>
  static FUN_ALWAYS_INLINE_DEBUGGABLE T InterpExpoInOut(const T& a, const T& b, float alpha) {
    return (alpha < 0.5f) ?
      InterpExpoIn(a, b, alpha * 2.f) * 0.5f :
      InterpExpoOut(a, b, alpha * 2.f - 1.f) * 0.5f + 0.5f;
  }

  /**
   * Interpolation between a and b, applying a circular in function.
   */
  template <typename T>
  static FUN_ALWAYS_INLINE_DEBUGGABLE T InterpCircularIn(const T& a, const T& b, float alpha) {
    float const modified_alpha = -1.f * (Sqrt(1.f - alpha * alpha) - 1.f);
    return Lerp<T>(a, b, modified_alpha);
  }

  /**
   * Interpolation between a and b, applying a circular out function.
   */
  template <typename T>
  static FUN_ALWAYS_INLINE_DEBUGGABLE T InterpCircularOut(const T& a, const T& b, float alpha) {
    alpha -= 1.f;
    float const modified_alpha = Sqrt(1.f - alpha * alpha);
    return Lerp<T>(a, b, modified_alpha);
  }

  /**
   * Interpolation between a and b, applying a circular in/out function.
   */
  template <typename T>
  static FUN_ALWAYS_INLINE_DEBUGGABLE T InterpCircularInOut(const T& a, const T& b, float alpha) {
    return (alpha < 0.5f) ?
      InterpCircularIn(a, b, alpha * 2.f) * 0.5f :
      InterpCircularOut(a, b, alpha * 2.f - 1.f) * 0.5f + 0.5f;
  }

  // rotator specific interpolation
  template <typename U>
  static Rotator Lerp(const Rotator& a, const Rotator& b, const U& alpha);

  // quat-specific interpolation

  template <typename U>
  static Quat Lerp(const Quat& a, const Quat& b, const U& alpha);

  template <typename U>
  static Quat BiLerp(const Quat& p00, const Quat& p10, const Quat& p01, const Quat& p11, const U& frac_x, const U& frac_y);

  /**
   * In the case of quaternions, we use a bezier like approach.
   * T - Actual 'control' orientations.
   */
  template <typename U>
  static Quat CubicInterp(const Quat& p0, const Quat& t0, const Quat& p1, const Quat& t1, const U& a);

  /**
   * Cubic Catmull-Rom Spline interpolation. Based on http://www.cemyuksel.com/research/catmullrom_param/catmullrom.pdf
   * Curves are guaranteed to pass through the control points and are easily chained together.
   * Equation supports abitrary parameterization. eg. Uniform=0, 1, 2, 3 ; chordal= |Pn - Pn-1| ; centripetal = |Pn - Pn-1|^0.5
   * p0 - The control point preceding the interpolation range.
   * p1 - The control point starting the interpolation range.
   * p2 - The control point ending the interpolation range.
   * p3 - The control point following the interpolation range.
   * t0-3 - The interpolation parameters for the corresponding control points.
   * T - The interpolation factor in the range 0 to 1. 0 returns p1. 1 returns p2.
   */
  template <typename U>
  static U CubicCRSplineInterp(const U& p0, const U& p1, const U& p2, const U& p3, const float t0, const float t1, const float t2, const float t3, const float t);

  /**
   * Same as CubicCRSplineInterp but with additional saftey checks.
   * If the checks fail p1 is returned.
   */
  template <typename U>
  static U CubicCRSplineInterpSafe(const U& p0, const U& p1, const U& p2, const U& p3, const float t0, const float t1, const float t2, const float t3, const float t);

  // Special-case interpolation

  /**
   * Interpolate a normal vector current to target, by interpolating the angle
   * between those vectors with constant step.
   */
  static FUN_BASE_API Vector VInterpNormalRotationTo(const Vector& current, const Vector& target, float delta_time, float rotation_speed_degrees);

  /**
   * Interpolate vector from current to target with constant step
   */
  static FUN_BASE_API Vector VInterpConstantTo(const Vector& current, const Vector& target, float delta_time, float interp_speed);

  /**
   * Interpolate vector from current to target. Scaled by distance to target,
   * so it has a strong start speed and ease out.
   */
  static FUN_BASE_API Vector VInterpTo(const Vector& current, const Vector& target, float delta_time, float interp_speed);

  /**
   * Interpolate Vector2 from current to target with constant step
   */
  static FUN_BASE_API Vector2 Vector2DInterpConstantTo(const Vector2& current, const Vector2& target, float delta_time, float interp_speed);

  /**
   * Interpolate Vector2 from current to target.
   * Scaled by distance to target, so it has a strong start speed and ease out.
   */
  static FUN_BASE_API Vector2 Vector2DInterpTo(const Vector2& current, const Vector2& target, float delta_time, float interp_speed);

  /**
   * Interpolate rotator from current to target with constant step
   */
  static FUN_BASE_API Rotator RInterpConstantTo(const Rotator& current, const Rotator& target, float delta_time, float interp_speed);

  /**
   * Interpolate rotator from current to target.
   * Scaled by distance to target, so it has a strong start speed and ease out.
   */
  static FUN_BASE_API Rotator RInterpTo(const Rotator& current, const Rotator& target, float delta_time, float interp_speed);

  /**
   * Interpolate float from current to target with constant step
   */
  static FUN_BASE_API float FInterpConstantTo(float current, float target, float delta_time, float interp_speed);

  /**
   * Interpolate float from current to target.
   * Scaled by distance to target, so it has a strong start speed and ease out.
   */
  static FUN_BASE_API float FInterpTo(float current, float target, float delta_time, float interp_speed);

  /**
   * Interpolate Linear color from current to target.
   * Scaled by distance to target, so it has a strong start speed and ease out.
   */
  static FUN_BASE_API LinearColor CInterpTo(const LinearColor& current, const LinearColor& target, float delta_time, float interp_speed);

  /**
   * Simple function to create a pulsating scalar value
   *
   * \param current_time - current absolute time
   * \param pulses_per_second - How many full pulses per second?
   * \param phase - Optional phase amount, between 0.0 and 1.0 (to synchronize pulses)
   *
   * \return Pulsating value (0.0-1.0)
   */
  static float MakePulsatingValue(const double current_time, const float pulses_per_second, const float phase = 0.f) {
    return 0.5f + 0.5f * Math::Sin(((0.25f + phase) * TWO_PI) + (current_time * TWO_PI) * pulses_per_second);
  }

  // Geometry intersection

  /**
   * Find the intersection of an infinite line (defined by two points) and
   * a plane.  Assumes that the line and plane do indeed intersect; you must
   * make sure they're not parallel before calling.
   */
  static Vector LinePlaneIntersection(const Vector& point1, const Vector& point2, const Vector& plane_origin, const Vector& plane_normal);
  static Vector LinePlaneIntersection(const Vector& point1, const Vector& point2, const Plane& plane);

  /**
   * @parma ref_scissor_rect - should be set to View.ViewRect before the call
   *
   * \return 0: light is not visible, 1:use scissor rect, 2: no scissor rect needed
   */
  static FUN_BASE_API uint32 ComputeProjectedSphereScissorRect(
                          struct IntRect& ref_scissor_rect,
                          const Vector& sphre_origin,
                          float radius,
                          const Vector& view_origin,
                          const Matrix& view_matrix,
                          const Matrix& proj_matrix);

  /**
   * Determine if a plane and an aabb intersect
   *
   * \param p - the plane to test
   * \param aabb - the axis aligned bounding box to test
   *
   * \return if collision occurs
   */
  static FUN_BASE_API bool PlaneAABBIntersection(const Plane& p, const Box& aabb);

  /**
   * Performs a sphere vs box intersection test using Arvo's algorithm:
   *
   *  for each i in (x, y, z)
   *      if (sphere_center(i) < BoxMin(i)) d2 += (sphere_center(i) - BoxMin(i)) ^ 2
   *      else if (sphere_center(i) > BoxMax(i)) d2 += (sphere_center(i) - BoxMax(i)) ^ 2
   *
   * \param sphere - the center of the sphere being tested against the aabb
   * \param radius_sqr - the size of the sphere being tested
   * \param aabb - the box being tested against
   *
   * \return Whether the sphere/box intersect or not.
   */
  static bool SphereAABBIntersection(const Vector& sphere_center, const float radius_sqr, const Box& aabb);

  /**
   * Converts a sphere into a point plus radius squared for the test above
   */
  static bool SphereAABBIntersection(const Sphere& sphere, const Box& aabb);

  /**
   * Determines whether a point is inside a box.
   */
  static bool PointBoxIntersection(const Vector& point, const Box& box);

  /**
   * Determines whether a line intersects a box.
   */
  static bool LineBoxIntersection(const Box& box, const Vector& start, const Vector& end, const Vector& direction);

  /**
   * Determines whether a line intersects a box.
   * This overload avoids the need to do the reciprocal every time.
   */
  static bool LineBoxIntersection(const Box& box, const Vector& start, const Vector& end, const Vector& direction, const Vector& one_over_direction);

  /**
   * Swept-box vs box test
   */
  static FUN_BASE_API bool LineExtentBoxIntersection(const Box& box, const Vector& start, const Vector& end, const Vector& extent, Vector& hit_location, Vector& hit_normal, float& hit_time);

  /**
   * Determines whether a line intersects a sphere.
   */
  static bool LineSphereIntersection(const Vector& start, const Vector& dir, float length, const Vector& origin, float radius);

  /**
   * Assumes the cone tip is at 0, 0, 0 (means the sphere_center is relative to the cone tip)
   *
   * \return true: cone and sphere do intersect, false otherwise
   */
  static FUN_BASE_API bool SphereConeIntersection(const Vector& sphere_center, float sphere_radius, const Vector& cone_axis, float cone_angle_sin, float cone_angle_cos);

  /**
   * Find the point on line segment from line_start to line_end which is closest to point
   */
  static FUN_BASE_API Vector ClosestPointOnLine(const Vector& line_start, const Vector& line_end, const Vector& point);

  /**
   * Compute intersection point of three planes. Return 1 if valid, 0 if infinite.
   */
  static bool IntersectPlanes3(Vector& i, const Plane& p1, const Plane& p2, const Plane& p3);

  /**
   * Compute intersection point and direction of line joining two planes.
   * Return 1 if valid, 0 if infinite.
   */
  static bool IntersectPlanes2(Vector& i, Vector& d, const Plane& p1, const Plane& p2);

  /**
   * Calculates the distance of a given point in world space to a given line,
   * defined by the vector couple (origin, direction).
   *
   * \param point - point to check distance to axis
   * \param direction - unit vector indicating the direction to check against
   * \param origin - point of reference used to calculate distance
   * \param out_closest_point - optional point that represents the closest point projected onto axis
   *
   * \return distance of point from line defined by (origin, direction)
   */
  static FUN_BASE_API float PointDistToLine(const Vector& point, const Vector& line, const Vector& origin, Vector& out_closest_point);
  static FUN_BASE_API float PointDistToLine(const Vector& point, const Vector& line, const Vector& origin);

  /**
   * Returns closest point on a segment to a given point.
   * The idea is to project point on line formed by segment.
   * Then we see if the closest point on the line is outside of segment or inside.
   *
   * \param point - point for which we find the closest point on the segment
   * \param start_point - start_point of segment
   * \param end_point - end_point of segment
   *
   * \return point on the segment defined by (start_point, end_point) that is closest to point.
   */
  static FUN_BASE_API Vector ClosestPointOnSegment(const Vector& point, const Vector& start_point, const Vector& end_point);

  /**
   * Vector2 version of ClosestPointOnSegment.
   * Returns closest point on a segment to a given 2D point.
   * The idea is to project point on line formed by segment.
   * Then we see if the closest point on the line is outside of segment or inside.
   *
   * \param point - point for which we find the closest point on the segment
   * \param start_point - start_point of segment
   * \param end_point - end_point of segment
   *
   * \return point on the segment defined by (start_point, end_point) that is closest to point.
   */
  static FUN_BASE_API Vector2 ClosestPointOnSegment2D(const Vector2& point, const Vector2& start_point, const Vector2& end_point);

  /**
   * Returns distance from a point to the closest point on a segment.
   *
   * \param point - point to check distance for
   * \param start_point - start_point of segment
   * \param end_point - end_point of segment
   *
   * \return closest distance from point to segment defined by (start_point, end_point).
   */
  static FUN_BASE_API float PointDistToSegment(const Vector& point, const Vector& start_point, const Vector& end_point);

  /**
   * Returns square of the distance from a point to the closest point on a segment.
   *
   * \param point - point to check distance for
   * \param start_point - start_point of segment
   * \param end_point - end_point of segment
   *
   * \return square of the closest distance from point to segment defined by (start_point, end_point).
   */
  static FUN_BASE_API float PointDistToSegmentSquared(const Vector& point, const Vector& start_point, const Vector& end_point);

  /**
   * Find closest points between 2 segments.
   *
   * \param (a1, b1) - defines the first segment.
   * \param (a2, b2) - defines the second segment.
   * \param out_p1 - Closest point on segment 1 to segment 2.
   * \param out_p2 - Closest point on segment 2 to segment 1.
   */
  static FUN_BASE_API void SegmentDistToSegment(const Vector& a1, const Vector& b1, const Vector& a2, const Vector& b2, Vector& out_p1, Vector& out_p2);

  /**
   * Find closest points between 2 segments.
   *
   * \param (a1, b1) - defines the first segment.
   * \param (a2, b2) - defines the second segment.
   * \param out_p1 - Closest point on segment 1 to segment 2.
   * \param out_p2 - Closest point on segment 2 to segment 1.
   */
  static FUN_BASE_API void SegmentDistToSegmentSafe(const Vector& a1, const Vector& b1, const Vector& a2, const Vector& b2, Vector& out_p1, Vector& out_p2);

  /**
   * returns the time (t) of the intersection of the passed segment and a plane (could be <0 or >1)
   *
   * \param start_point - start point of segment
   * \param end_point - end point of segment
   * \param plane - plane to intersect with
   *
   * \return time(T) of intersection
   */
  static FUN_BASE_API float GetTForSegmentPlaneIntersect(const Vector& start_point, const Vector& end_point, const Plane& plane);

  /**
   * Returns true if there is an intersection between the segment specified by start_point and Endpoint, and
   * the plane on which polygon plane lies. If there is an intersection, the point is placed in out_intersection_point
   *
   * \param start_point - start point of segment
   * \param end_point - end point of segment
   * \param plane - plane to intersect with
   * \param out_intersection_point - out var for the point on the segment that intersects the mesh (if any)
   *
   * \return true if intersection occurred
   */
  static FUN_BASE_API bool SegmentPlaneIntersection(const Vector& start_point, const Vector& end_point, const Plane& plane, Vector& out_intersection_point);

  /**
   * Returns true if there is an intersection between the segment specified by segment_start_a and segment_end_a, and
   * the segment specified by segment_start_b and segment_end_b, in 2D space. If there is an intersection, the point is placed in out_intersection_point
   *
   * \param segment_start_a - start point of first segment
   * \param segment_end_a - end point of first segment
   * \param segment_start_b - start point of second segment
   * \param segment_end_b - end point of second segment
   * \param out_intersection_point - out var for the intersection point (if any)
   *
   * \return true if intersection occurred
   */
  static FUN_BASE_API bool SegmentIntersection2D( const Vector& segment_start_a,
                                                  const Vector& segment_end_a,
                                                  const Vector& segment_start_b,
                                                  const Vector& segment_end_b,
                                                  Vector& out_intersection_point);

  /**
   * Returns closest point on a triangle to a point.
   * The idea is to identify the halfplanes that the point is
   * in relative to each triangle segment "plane"
   *
   * \param point - point to check distance for
   * \param a, b, c - counter clockwise ordering of points defining a triangle
   *
   * \return point on triangle abc closest to given point
   */
  static FUN_BASE_API Vector ClosestPointOnTriangleToPoint(const Vector& point, const Vector& a, const Vector& b, const Vector& c);

  /**
   * Returns closest point on a tetrahedron to a point.
   * The idea is to identify the halfplanes that the point is
   * in relative to each face of the tetrahedron
   *
   * \param point - point to check distance for
   * \param a, b, c, d - four points defining a tetrahedron
   *
   * \return point on tetrahedron abcd closest to given point
   */
  static FUN_BASE_API Vector ClosestPointOnTetrahedronToPoint(const Vector& point, const Vector& a, const Vector& b, const Vector& c, const Vector& d);

  /**
   * Find closest point on a sphere to a Line.
   *
   * When line intersects - sphere, then closest point to line_origin is returned.
   *
   * \param sphre_origin - origin of sphere
   * \param sphere_radius - Radius of sphere
   * \param line_origin - origin of line
   * \param line_dir - direction of line. Needs to be normalized!!
   * \param out_closest_point - Closest point on sphere to given line.
   */
  static FUN_BASE_API void SphereDistToLine(const Vector& sphre_origin,
                                            float sphere_radius,
                                            const Vector& line_origin,
                                            const Vector& line_dir,
                                            Vector& out_closest_point);

  /**
   * Calculates whether a point is within a cone segment, and also what percentage within the cone
   * (100% is along the center line, whereas 0% is along the edge)
   *
   * \param point - The point in question
   * \param cone_start_point - the beginning of the cone (with the smallest radius)
   * \param cone_line - the line out from the start point that ends at the largest radius point of the cone
   * \param radius_at_start - the radius at the cone_start_point (0 for a 'proper' cone)
   * \param radius_at_end - the largest radius of the cone
   * \param out_percentage - output variable the holds how much within the cone the point is (1 = on center line, 0 = on exact edge or outside cone).
   *
   * \return true if the point is within the cone, false otherwise.
   */
  static FUN_BASE_API bool GetDistanceWithinConeSegment(const Vector& point,
                                                        const Vector& cone_start_point,
                                                        const Vector& cone_line,
                                                        float radius_at_start,
                                                        float radius_at_end,
                                                        float& out_percentage);
  /**
   * Determines whether a given set of points are coplanar, with a tolerance.
   * Any three points or less are always coplanar.
   *
   * \param points - The set of points to determine coplanarity for.
   * \param tolerance - Larger numbers means more variance is allowed.
   *
   * \return Whether the points are relatively coplanar, based on the tolerance
   */
  static FUN_BASE_API bool PointsAreCoplanar(const Array<Vector>& points, const float tolerance = 0.1f);


  /**
   * Converts a floating point number to the nearest integer, equidistant ties go to
   * the value which is closest to an even value: 1.5 becomes 2, 0.5 becomes 0
   *
   * \param x - Floating point value to convert
   *
   * \return The rounded integer
   */
  static FUN_BASE_API float RoundHalfToEven(float x);
  static FUN_BASE_API double RoundHalfToEven(double x);

  /**
   * Converts a floating point number to the nearest integer, equidistant ties go to
   * the value which is further from zero: -0.5 becomes -1.0, 0.5 becomes 1.0
   *
   * \param x - Floating point value to convert
   *
   * \return The rounded integer
   */
  static FUN_BASE_API float RoundHalfFromZero(float x);
  static FUN_BASE_API double RoundHalfFromZero(double x);

  /**
   * Converts a floating point number to the nearest integer, equidistant ties go to
   * the value which is closer to zero: -0.5 becomes 0, 0.5 becomes 0
   *
   * \param x - Floating point value to convert
   *
   * \return The rounded integer
   */
  static FUN_BASE_API float RoundHalfToZero(float x);
  static FUN_BASE_API double RoundHalfToZero(double x);

  /**
   * Converts a floating point number to an integer which is further from zero,
   * "larger" in absolute value: 0.1 becomes 1, -0.1 becomes -1
   *
   * \param x - Floating point value to convert
   *
   * \return The rounded integer
   */
  static float RoundFromZero(float x);
  static double RoundFromZero(double x);

  /**
   * Converts a floating point number to an integer which is closer to zero,
   * "smaller" in absolute value: 0.1 becomes 0, -0.1 becomes 0
   *
   * \param x - Floating point value to convert
   *
   * \return The rounded integer
   */
  static float RoundToZero(float x);
  static double RoundToZero(double x);

  /**
   * Converts a floating point number to an integer which is more negative: 0.1 becomes 0, -0.1 becomes -1
   *
   * \param x - Floating point value to convert
   *
   * \return The rounded integer
   */
  static float RoundToNegativeInfinity(float x);
  static double RoundToNegativeInfinity(double x);

  /**
   * Converts a floating point number to an integer which is more positive: 0.1 becomes 1, -0.1 becomes 0
   *
   * \param x - Floating point value to convert
   *
   * \return The rounded integer
   */
  static float RoundToPositiveInfinity(float x);
  static double RoundToPositiveInfinity(double x);

  // Formatting functions

  /**
   * Formats an integer value into a human readable string (i.e. 12345 becomes "12, 345")
   *
   * \param x - The value to use
   *
   * \return string The human readable string
   */
  static String FormatIntToHumanReadable(int32 x);


  //
  // Utilities
  //

  ///**
  // * Tests a memory region to see that it's working properly.
  // *
  // * \param BaseAddress - Starting address
  // * \param NumBytes - Number of bytes to test (will be rounded down to a multiple of 4)
  // *
  // * \return true if the memory region passed the test
  // */
  //static FUN_BASE_API bool MemoryTest(void* BaseAddress, uint32 NumBytes);
  //
  ///**
  // * Evaluates a numerical equation.
  // *
  // * Operators and precedence: 1:+- 2:/% 3:* 4:^ 5:&|
  // * Unary: -
  // * Types: Numbers (0-9.), Hex ($0-$f)
  // * Grouping: ()
  // *
  // * \param str - String containing the equation.
  // * \param out_value - Pointer to storage for the result.
  // *
  // * \return 1 if successful, 0 if equation fails.
  // */
  //static FUN_BASE_API bool Eval(String str, float& out_value);

  /**
   * Computes the barycentric coordinates for a given point in a triangle - simpler version
   *
   * \param point - point to convert to barycentric coordinates (in plane of abc)
   * \param a, b, c - three non-colinear points defining a triangle in CCW
   *
   * \return vector containing the three weights a, b, c such that point = a*a + b*b + c*c
   *                                                            or point = a + b*(b-a) + c*(c-a) = (1-b-c)*a + b*b + c*c
   */
  static FUN_BASE_API Vector GetBaryCentric2D(const Vector& point, const Vector& a, const Vector& b, const Vector& c);

  /**
   * Computes the barycentric coordinates for a given point in a triangle
   *
   * \param point - point to convert to barycentric coordinates (in plane of abc)
   * \param a, b, c - three non-collinear points defining a triangle in CCW
   *
   * \return vector containing the three weights a, b, c such that point = a*a + b*b + c*c
   *                                                         or point = a + b*(b-a) + c*(c-a) = (1-b-c)*a + b*b + c*c
   */
  static FUN_BASE_API Vector ComputeBaryCentric2D(const Vector& point, const Vector& a, const Vector& b, const Vector& c);

  /**
   * Computes the barycentric coordinates for a given point on a tetrahedron (3D)
   *
   * \param point - point to convert to barycentric coordinates
   * \param a, b, c, d - four points defining a tetrahedron
   *
   * \return vector containing the four weights a, b, c, d such that point = a*a + b*b + c*c + d*d
   */
  static FUN_BASE_API Vector4 ComputeBaryCentric3D(const Vector& point, const Vector& a, const Vector& b, const Vector& c, const Vector& d);

  /** 32 bit values where BitFlag[x] == (1<<x) */
  static FUN_BASE_API const uint32 BitFlag[32];

  /**
   * Returns a smooth Hermite interpolation between 0 and 1 for the value x (where x ranges between a and b)
   * Clamped to 0 for x <= a and 1 for x >= b.
   *
   * \param a - Minimum value of x
   * \param b - Maximum value of x
   * \param x - Parameter
   *
   * \return Smoothed value between 0 and 1
   */
  static float SmoothStep(float a, float b, float x) {
    if (x < a) {
      return 0.f;
    } else if (x >= b) {
      return 1.f;
    }
    const float interp_fraction = (x - a) / (b - a);
    return interp_fraction * interp_fraction * (3.f - 2.f * interp_fraction);
  }

  /**
   * Get a bit in memory created from bitflags (uint32 value:1), used for EngineShowFlags,
   * TestBitFieldFunctions() tests the implementation
   */
  static FUN_ALWAYS_INLINE bool ExtractBoolFromBitfield(uint8* ptr, uint32 index) {
    uint8* byte_ptr = ptr + index / 8;
    const uint8 mask = 1 << (index & 0x7);

    return (*byte_ptr & mask) != 0;
  }

  /**
   * Set a bit in memory created from bitflags (uint32 value:1), used for EngineShowFlags,
   * TestBitFieldFunctions() tests the implementation
   */
  static FUN_ALWAYS_INLINE void SetBoolInBitField(uint8* ptr, uint32 index, bool set) {
    uint8* byte_ptr = ptr + index / 8;
    const uint8 mask = 1 << (index & 0x7);

    if (set) {
      *byte_ptr |= mask;
    } else {
      *byte_ptr &= ~mask;
    }
  }

  /**
   * Handy to apply scaling in the editor
   *
   * \param Dst - in and out
   */
  static FUN_BASE_API void ApplyScaleToFloat(float& dst, const Vector& delta_scale, float magnitude = 1.f);

  /**
   * \param x - assumed to be in this range: 0..1
   *
   * \return 0..255
   */
  static uint8 Quantize8UnsignedByte(float x) {
    // 0..1 -> 0..255
    const int32 ret = (int32)(x * 255.999f);

    fun_check(ret >= 0);
    fun_check(ret <= 255);

    return ret;
  }

  /**
   * \param x - assumed to be in this range: -1..1
   *
   * \return 0..255
   */
  static uint8 Quantize8SignedByte(float x) {
    // -1..1 -> 0..1
    const float y = x * 0.5f + 0.5f;
    return Quantize8UnsignedByte(y);
  }
};

} // namespace fun


// Platform specific vector intrinsics include.
#if FUN_WITH_DIRECTXMATH
#define SIMD_ALIGNMENT  16
#include "fun/base/math/vectorize_dx.h"
#elif FUN_PLATFORM_ENABLE_VECTORINTRINSICS
#define SIMD_ALIGNMENT  16
#include "fun/base/math/vectorize_sse.h"
#elif PLATFORM_ENABLE_VECTORINTRINSICS_NEON
#define SIMD_ALIGNMENT  16
#include "fun/base/math/vectorize_neon.h"
#else
#define SIMD_ALIGNMENT  4
#include "fun/base/math/vectorize_fpu.h"
#endif

// 'Cross-platform' vector intrinsics (built on the platform-specific ones defined above)
namespace fun {
  #include "fun/base/math/vectorize_common.h"
}

/** vector that represents (1/255, 1/255, 1/255, 1/255) */
namespace fun {
  extern FUN_BASE_API const VectorRegister VECTOR_INV_255;
}

/**
 * Below this weight threshold, animations won't be blended in.
 */
#define ZERO_ANIMWEIGHT_THRESH  (0.00001f)

namespace fun {
  namespace VectorizeConstants {
    static const VectorRegister AnimWeightThreshold = MakeVectorRegister(ZERO_ANIMWEIGHT_THRESH, ZERO_ANIMWEIGHT_THRESH, ZERO_ANIMWEIGHT_THRESH, ZERO_ANIMWEIGHT_THRESH);
    static const VectorRegister RotationSignificantThreshold = MakeVectorRegister(1.f - DELTA*DELTA, 1.f - DELTA*DELTA, 1.f - DELTA*DELTA, 1.f - DELTA*DELTA);
  }
}
