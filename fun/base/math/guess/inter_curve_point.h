#pragma once

namespace fun {

enum InterpCurveMode {
  /** A straight line between two keypoint values. */
  CIM_Linear,

  /**
   * A cubic-hermite curve between two keypoints, using Arrive/Leave tangents. These tangents will be automatically
   * updated when points are moved, etc.  Tangents are unclamped and will plateau at curve start and end points.
   */
  CIM_CurveAuto,

  /**
   * The out value is held constant until the next key, then will jump to that value.
   */
  CIM_Constant,

  /**
   * A smooth curve just like CIM_Curve, but tangents are not automatically updated so you can have manual control over them (eg. in Curve Editor).
   */
  CIM_CurveUser,

  /**
   * A curve like CIM_Curve, but the arrive and leave tangents are not forced to be the same, so you can create a 'corner' at this key.
   */
  CIM_CurveBreak,

  /**
   * A cubic-hermite curve between two keypoints, using Arrive/Leave tangents. These tangents will be automatically
   * updated when points are moved, etc.  Tangents are clamped and will plateau at curve start and end points.
   */
  CIM_CurveAutoClamped,

  /**
   * Invalid or unknown curve type.
   */
  CIM_Unknown
};

/**
 * Template for interpolation points.
 *
 * Interpolation points are used for describing the shape of interpolation curves.
 *
 * @see InterpCurve
 * @todo Docs: InterpCurvePoint needs better function documentation.
 */
template <typename T> class InterpCurvePoint
{
 public:
  /** Float input value that corresponds to this key (eg. time). */
  float in_val;

  /** Output value of templated type when input is equal to in_val. */
  T out_val;

  /** Tangent of curve arrive this point. */
  T arrive_tangent;

  /** Tangent of curve leaving this point. */
  T leave_tangent;

  /** Interpolation mode between this point and the next one. @see InterpCurveMode */
  EnumAsByte<InterpCurveMode> InterpMode;

 public:
  /**
   * Default constructor (no initialization).
   */
  InterpCurvePoint() {};

  /**
   * Constructor
   *
   * \param in - input value that corresponds to this key
   * \param out - Output value of templated type
   *
   * @note uses linear interpolation
   */
  InterpCurvePoint(const float in, const T& out);

  /**
   * Constructor
   *
   * \param in - input value that corresponds to this key
   * \param out - Output value of templated type
   * \param InArriveTangent - Tangent of curve arriving at this point.
   * \param InLeaveTangent - Tangent of curve leaving from this point.
   * \param InInterpMode - interpolation mode to use
   */
  InterpCurvePoint(const float in, const T& out, const T& InArriveTangent, const T& InLeaveTangent, const InterpCurveMode InInterpMode);

 public:
  /** \return true if the key value is using a curve interp mode, otherwise false */
  inline bool IsCurveKey() const;

 public:
  /**
   * Serializes the Curve point.
   *
   * \param ar - Reference to the serialization archive.
   * \param point - Reference to the curve point being serialized.
   *
   * \return Reference to the Archive after serialization.
   */
  friend Archive& operator & (Archive& ar, InterpCurvePoint& point)
  {
    ar & point.in_val & point.out_val;
    ar & point.arrive_tangent & point.leave_tangent;
    ar & point.InterpMode;
    return ar;
  }

  /**
   * Compare equality of two Curve points
   */
  friend bool operator == (const InterpCurvePoint& point1, const InterpCurvePoint& point2)
  {
    return (point1.in_val == point2.in_val &&
            point1.out_val == point2.out_val &&
            point1.arrive_tangent == point2.arrive_tangent &&
            point1.leave_tangent == point2.leave_tangent &&
            point1.InterpMode == point2.InterpMode);
  }

  /**
   * Compare inequality of two Curve points
   */
  friend bool operator != (const InterpCurvePoint& point1, const InterpCurvePoint& point2)
  {
    return !(point1 == point2);
  }
};


template <typename T>
inline InterpCurvePoint<T>::InterpCurvePoint(const float in, const T& out)
  : in_val(in)
  , out_val(out)
{
  UnsafeMemory::Memzero(&arrive_tangent, sizeof(T));
  UnsafeMemory::Memzero(&leave_tangent, sizeof(T));

  InterpMode = CIM_Linear;
}

template <typename T>
inline InterpCurvePoint<T>::InterpCurvePoint(const float in, const T& out, const T& InArriveTangent, const T& InLeaveTangent, const InterpCurveMode InInterpMode)
  : in_val(in)
  , out_val(out)
  , arrive_tangent(InArriveTangent)
  , leave_tangent(InLeaveTangent)
  , InterpMode(InInterpMode)
{}

template <typename T>
inline bool InterpCurvePoint<T>::IsCurveKey() const
{
  return ((InterpMode == CIM_CurveAuto) || (InterpMode == CIM_CurveAutoClamped) || (InterpMode == CIM_CurveUser) || (InterpMode == CIM_CurveBreak));
}

/**
Clamps a tangent formed by the specified control point values
*/
FUN_BASE_API float ClampFloatTangent(float prev_point_val, float prev_time, float cur_point_val, float cur_time, float next_point_val, float next_time);

/** Computes Tangent for a curve segment */
template <typename T, typename U>
inline void AutoCalcTangent(const T& prev_p, const T& P, const T& next_p, const U& tension, T& out_tan)
{
  out_tan = (1.f - tension) * ((P - prev_p) + (next_p - P));
}

/**
This actually returns the control point not a tangent. This is expected by the CubicInterp function for Quaternions
*/
template <typename U>
inline void AutoCalcTangent(const Quat& prev_p, const Quat& P, const Quat& next_p, const U& tension, Quat& out_tan)
{
  Quat::CalcTangents(prev_p, P, next_p, tension, out_tan);
}

/** Computes a tangent for the specified control point.  General case, doesn't support clamping. */
template <typename T>
inline void ComputeCurveTangent(float prev_time, const T& prev_point,
              float cur_time, const T& curr_point,
              float next_time, const T& next_point,
              float tension,
              bool with_clamping,
              T& out_tangent)
{
  // NOTE: Clamping not supported for non-float vector types (with_clamping is ignored)

  AutoCalcTangent(prev_point, curr_point, next_point, tension, out_tangent);

  const float prev_to_next_time_diff = Math::Max< double >(KINDA_SMALL_NUMBER, next_time - prev_time);

  out_tangent /= prev_to_next_time_diff;
}

/**
Computes a tangent for the specified control point; supports clamping, but only works
with floats or contiguous arrays of floats.
*/
template <typename T>
inline void ComputeClampableFloatVectorCurveTangent(float prev_time, const T& prev_point,
                        float cur_time, const T& curr_point,
                        float next_time, const T& next_point,
                        float tension,
                        bool with_clamping,
                        T& out_tangent)
{
  // Clamp the tangents if we need to do that
  if (with_clamping)
  {
    // NOTE: We always treat the type as an array of floats
    float* prev_point_val = (float*)&prev_point;
    float* cur_point_val = (float*)&curr_point;
    float* next_point_val = (float*)&next_point;
    float* out_tangent_val = (float*)&out_tangent;
    for (int32 cur_val_pos = 0; cur_val_pos < sizeof(T); cur_val_pos += sizeof(float)) {
      // Clamp it!
      const float clamped_tangent =
        ClampFloatTangent(
          *prev_point_val, prev_time,
          *cur_point_val, cur_time,
          *next_point_val, next_time);

      // Apply tension value
      *out_tangent_val = (1.0f - tension) * clamped_tangent;

      // Advance pointers
      ++out_tangent_val;
      ++prev_point_val;
      ++cur_point_val;
      ++next_point_val;
    }
  }
  else
  {
    // No clamping needed
    AutoCalcTangent(prev_point, curr_point, next_point, tension, out_tangent);

    const float prev_to_next_time_diff = Math::Max< double >(KINDA_SMALL_NUMBER, next_time - prev_time);
    out_tangent /= prev_to_next_time_diff;
  }
}

/** Computes a tangent for the specified control point.  Special case for float types; supports clamping. */
inline void ComputeCurveTangent(float prev_time, const float& prev_point,
                  float cur_time, const float& curr_point,
                  float next_time, const float& next_point,
                  float tension,
                  bool with_clamping,
                  float& out_tangent)
{
  ComputeClampableFloatVectorCurveTangent(
    prev_time, prev_point,
    cur_time, curr_point,
    next_time, next_point,
    tension, with_clamping, out_tangent);
}

/** Computes a tangent for the specified control point.  Special case for Vector types; supports clamping. */
inline void ComputeCurveTangent(float prev_time, const Vector& prev_point,
                  float cur_time, const Vector& curr_point,
                  float next_time, const Vector& next_point,
                  float tension,
                  bool with_clamping,
                  Vector& out_tangent)
{
  ComputeClampableFloatVectorCurveTangent(
    prev_time, prev_point,
    cur_time, curr_point,
    next_time, next_point,
    tension, with_clamping, out_tangent);
}

/** Computes a tangent for the specified control point.  Special case for Vector2 types; supports clamping. */
inline void ComputeCurveTangent(float prev_time, const Vector2& prev_point,
                  float cur_time, const Vector2& curr_point,
                  float next_time, const Vector2& next_point,
                  float tension,
                  bool with_clamping,
                  Vector2& out_tangent)
{
  ComputeClampableFloatVectorCurveTangent(
    prev_time, prev_point,
    cur_time, curr_point,
    next_time, next_point,
    tension, with_clamping, out_tangent);
}

/** Computes a tangent for the specified control point.  Special case for TwoVectors types; supports clamping. */
inline void ComputeCurveTangent(float prev_time, const TwoVectors& prev_point,
                  float cur_time, const TwoVectors& curr_point,
                  float next_time, const TwoVectors& next_point,
                  float tension,
                  bool with_clamping,
                  TwoVectors& out_tangent)
{
  ComputeClampableFloatVectorCurveTangent(
    prev_time, prev_point,
    cur_time, curr_point,
    next_time, next_point,
    tension, with_clamping, out_tangent);
}

/**
 * Calculate bounds of float inervals
 *
 * \param start - interp curve point at start
 * \param end - interp curve point at end
 * \param current_min - Input and Output could be updated if needs new interval minimum bound
 * \param current_max - Input and Output could be updated if needs new interval maximmum bound
 */
void FUN_BASE_API CurveFloatFindIntervalBounds(const InterpCurvePoint<float>& start, const InterpCurvePoint<float>& end, float& current_min, float& current_max);

/**
 * alculate bounds of 2D vector intervals
 *
 * param start - interp curve point at start
 * param end - interp curve point at end
 * param current_min - Input and Output could be updated if needs new interval minimum bound
 * param current_max - Input and Output could be updated if needs new interval maximmum bound
 */
void FUN_BASE_API CurveVector2DFindIntervalBounds(const InterpCurvePoint<Vector2>& start, const InterpCurvePoint<Vector2>& end, Vector2& current_min, Vector2& current_max);

/**
 * Calculate bounds of vector intervals
 *
 * \param start - interp curve point at start
 * \param end - interp curve point at end
 * \param current_min - Input and Output could be updated if needs new interval minimum bound
 * \param current_max - Input and Output could be updated if needs new interval maximmum bound
 */
void FUN_BASE_API CurveVectorFindIntervalBounds(const InterpCurvePoint<Vector>& start, const InterpCurvePoint<Vector>& end, Vector& current_min, Vector& current_max);

/**
 * Calculate bounds of twovector intervals
 *
 * \param start - interp curve point at start
 * \param end - interp curve point at end
 * \param current_min - Input and Output could be updated if needs new interval minimum bound
 * \param current_max - Input and Output could be updated if needs new interval maximmum bound
 */
void FUN_BASE_API CurveTwoVectorsFindIntervalBounds(const InterpCurvePoint<TwoVectors>& start, const InterpCurvePoint<TwoVectors>& end, TwoVectors& current_min, TwoVectors& current_max);

/**
 * Calculate bounds of color intervals
 *
 * \param start - interp curve point at start
 * \param end - interp curve point at end
 * \param current_min - Input and Output could be updated if needs new interval minimum bound
 * \param current_max - Input and Output could be updated if needs new interval maximmum bound
 */
void FUN_BASE_API CurveLinearColorFindIntervalBounds(const InterpCurvePoint<LinearColor>& start, const InterpCurvePoint<LinearColor>& end, LinearColor& current_min, LinearColor& current_max);

template <typename T, typename U>
inline void CurveFindIntervalBounds(const InterpCurvePoint<T>& start, const InterpCurvePoint<T>& end, T& current_min, T& current_max, const U& dummy)
{}

template <typename U>
inline void CurveFindIntervalBounds(const InterpCurvePoint<float>& start, const InterpCurvePoint<float>& end, float& current_min, float& current_max, const U& dummy)
{
  CurveFloatFindIntervalBounds(start, end, current_min, current_max);
}

template <typename U>
void CurveFindIntervalBounds(const InterpCurvePoint<Vector2>& start, const InterpCurvePoint<Vector2>& end, Vector2& current_min, Vector2& current_max, const U& dummy)
{
  CurveVector2DFindIntervalBounds(start, end, current_min, current_max);
}

template <typename U>
inline void CurveFindIntervalBounds(const InterpCurvePoint<Vector>& start, const InterpCurvePoint<Vector>& end, Vector& current_min, Vector& current_max, const U& dummy)
{
  CurveVectorFindIntervalBounds(start, end, current_min, current_max);
}

template <typename U>
inline void CurveFindIntervalBounds(const InterpCurvePoint<TwoVectors>& start, const InterpCurvePoint<TwoVectors>& end, TwoVectors& current_min, TwoVectors& current_max, const U& dummy)
{
  CurveTwoVectorsFindIntervalBounds(start, end, current_min, current_max);
}

template <typename U>
inline void CurveFindIntervalBounds(const InterpCurvePoint<LinearColor>& start, const InterpCurvePoint<LinearColor>& end, LinearColor& current_min, LinearColor& current_max, const U& dummy)
{
  CurveLinearColorFindIntervalBounds(start, end, current_min, current_max);
}

} // namespace fun
