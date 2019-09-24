#pragma once

namespace fun {

/**
Template for interpolation curves.

@see InterpCurvePoint

@todo Docs: InterpCurve needs template and function documentation
*/
template <typename T>
class InterpCurve
{
 public:
  /** Holds the collection of interpolation points. */
  Array<InterpCurvePoint<T>> points;

  /** Specify whether the curve is looped or not */
  bool is_looped;

  /** Specify the offset from the last point's input key corresponding to the loop point */
  float loop_key_offset;

 public:
  /** default_value constructor. */
  InterpCurve()
    : is_looped(false) {}

 public:
  /**
   * adds a new keypoint to the InterpCurve with the supplied In and Out value.
   *
   * \param in_val
   * \param out_val
   * \return The index of the new key.
   */
  int32 AddPoint(const float in_val, const T& out_val);

  /**
   * Moves a keypoint to a new In value.
   *
   * This may change the index of the keypoint, so the new key index is returned.
   *
   * \param point_index
   * \param new_in_val
   * \return
   */
  int32 MovePoint(int32 point_index, float new_in_val);

  /**
   * Clears all keypoints from InterpCurve.
   */
  void Reset();

  /**
   * Set loop key for curve
   */
  void SetLoopKey(float loop_key);

  /**
   * Clear loop key for curve
   */
  void ClearLoopKey();

  /**
   * Evaluate the output for an arbitary input value.
   * For inputs outside the range of the keys, the first/last key value is assumed.
   */
  T Eval(const float in_val, const T& default_value = T(ForceInit)) const;

  /**
   * Evaluate the derivative at a point on the curve.
   */
  T EvalDerivative(const float in_val, const T& default_value = T(ForceInit)) const;

  /**
   * Evaluate the second derivative at a point on the curve.
   */
  T EvalSecondDerivative(const float in_val, const T& default_value = T(ForceInit)) const;

  /**
   * Find the nearest point on spline to the given point.
   *
   * \param point_in_space - the given point
   * \param out_distance_sqr - output - the squared distance between the given point and the closest found point.
   *
   * \return The key (the 't' parameter) of the nearest point.
   */
  float InaccurateFindNearest(const T& point_in_space, float& out_distance_sqr) const;

  /**
   * Find the nearest point (to the given point) on segment between points[PtIdx] and points[PtIdx+1]
   *
   * \param point_in_space - the given point
   *
   * \return The key (the 't' parameter) of the found point.
   */
  float InaccurateFindNearestOnSegment(const T& point_in_space, int32 PtIdx, float& out_distance_sqr) const;

  /**
   * Automatically set the tangents on the curve based on surrounding points
   */
  void AutoSetTangents(float tension = 0.0f, bool bStationaryEndpoints = true);

  /**
   * Calculate the min/max out value that can be returned by this InterpCurve.
   */
  void CalcBounds(T& out_min, T& out_max, const T& default_value = T(ForceInit)) const;

 public:
  /**
   * Serializes the interp curve.
   *
   * \param ar - Reference to the serialization archive.
   * \param curve - Reference to the interp curve being serialized.
   *
   * \return Reference to the Archive after serialization.
   */
  friend Archive& operator & (Archive& ar, InterpCurve& curve)
  {
    // NOTE: This is not used often for InterpCurves.  Most of the time these are serialized
    //   as inline struct properties in UnClass.cpp!

    ar & curve.points;
    ar & curve.is_looped;
    ar & curve.loop_key_offset;
    return ar;
  }

  /**
   * Compare equality of two InterpCurves
   */
  friend bool operator == (const InterpCurve& curve1, const InterpCurve& curve2)
  {
    return (curve1.points == curve2.points &&
            curve1.is_looped == curve2.is_looped &&
            (!curve1.is_looped || curve1.loop_key_offset == curve2.loop_key_offset));
  }

  /**
   * Compare inequality of two InterpCurves
   */
  friend bool operator != (const InterpCurve& curve1, const InterpCurve& curve2)
  {
    return !(curve1 == curve2);
  }

  /**
   * Finds the lower index of the two points whose input values bound the supplied input value.
   */
  int32 GetPointIndexForInputValue(const float value) const;
};


//
// inlines
//

template <typename T>
inline int32 InterpCurve<T>::AddPoint(const float in_val, const T& out_val)
{
  int32 i = 0; for (i = 0; i < points.Count() && points[i].in_val < in_val; i++);
  points.InsertUninitialized(i);
  points[i] = InterpCurvePoint<T>(in_val, out_val);
  return i;
}

template <typename T>
inline int32 InterpCurve<T>::MovePoint(int32 point_index, float new_in_val)
{
  if (point_index < 0 || point_index >= points.Count()) {
    return point_index;
  }

  const T out_val = points[point_index].out_val;
  const InterpCurveMode mode = points[point_index].interp_mode;
  const T arrive_tan = points[point_index].arrive_tangent;
  const T leave_tan = points[point_index].leave_tangent;

  points.RemoveAt(point_index);

  const int32 new_point_index = AddPoint(new_in_val, out_val);
  points[new_point_index].interp_mode = mode;
  points[new_point_index].arrive_tangent = arrive_tan;
  points[new_point_index].leave_tangent = leave_tan;

  return new_point_index;
}

template <typename T>
inline void InterpCurve<T>::Reset()
{
  points.Clear();
}

template <typename T>
inline void InterpCurve<T>::SetLoopKey(float loop_key)
{
  const float last_in_key = points.Last().in_val;
  if (loop_key > last_in_key) {
    // Calculate loop key offset from the input key of the final point
    is_looped = true;
    loop_key_offset = loop_key - last_in_key;
  }
  else {
    // Specified a loop key lower than the final point; turn off looping.
    is_looped = false;
  }
}

template <typename T>
inline void InterpCurve<T>::ClearLoopKey()
{
  is_looped = false;
}

template <typename T>
int32 InterpCurve<T>::GetPointIndexForInputValue(const float value) const
{
  const int32 point_count = points.Count();
  const int32 last_point = point_count - 1;

  fun_check(point_count > 0);

  if (value < points[0].in_val) {
    return -1;
  }

  if (value >= points[last_point].in_val) {
    return last_point;
  }

  int32 min_index = 0;
  int32 max_index = point_count;

  while (max_index - min_index > 1) {
    int32 mid_index = (min_index + max_index) / 2;

    if (points[mid_index].in_val <= value) {
      min_index = mid_index;
    }
    else {
      max_index = mid_index;
    }
  }

  return min_index;
}

template <typename T>
T InterpCurve<T>::Eval(const float in_val, const T& default_value) const
{
  const int32 point_count = points.Count();
  const int32 last_point = point_count - 1;

  // If no point in curve, return the default_value value we passed in.
  if (point_count == 0) {
    return default_value;
  }

  // Binary search to find index of lower bound of input value
  const int32 index = GetPointIndexForInputValue(in_val);

  // If before the first point, return its value
  if (index == -1) {
    return points[0].out_val;
  }

  // If on or beyond the last point, return its value.
  if (index == last_point) {
    if (!is_looped) {
      return points[last_point].out_val;
    }
    else if (in_val >= points[last_point].in_val + loop_key_offset) {
      // Looped spline: last point is the same as the first point
      return points[0].out_val;
    }
  }

  // Somewhere within curve range - interpolate.
  fun_check(index >= 0 && ((is_looped && index < point_count) || (!is_looped && index < last_point)));
  const bool is_loop_segment = (is_looped && index == last_point);
  const int32 next_index = is_loop_segment ? 0 : (index + 1);

  const auto& prev_point = points[index];
  const auto& next_point = points[next_index];

  const float diff = is_loop_segment ? loop_key_offset : (next_point.in_val - prev_point.in_val);

  if (diff > 0.0f && prev_point.interp_mode != CIM_Constant) {
    const float alpha = (in_val - prev_point.in_val) / diff;
    fun_check(alpha >= 0.0f && alpha <= 1.0f);

    if (prev_point.interp_mode == CIM_Linear) {
      return Math::Lerp(prev_point.out_val, next_point.out_val, alpha);
    }
    else {
      return Math::CubicInterp(prev_point.out_val, prev_point.leave_tangent * diff, next_point.out_val, next_point.arrive_tangent * diff, alpha);
    }
  }
  else {
    return points[index].out_val;
  }
}

template <typename T>
T InterpCurve<T>::EvalDerivative(const float in_val, const T& default_value) const
{
  const int32 point_count = points.Count();
  const int32 last_point = point_count - 1;

  // If no point in curve, return the default_value value we passed in.
  if (point_count == 0) {
    return default_value;
  }

  // Binary search to find index of lower bound of input value
  const int32 index = GetPointIndexForInputValue(in_val);

  // If before the first point, return its tangent value
  if (index == -1) {
    return points[0].leave_tangent;
  }

  // If on or beyond the last point, return its tangent value.
  if (index == last_point) {
    if (!is_looped) {
      return points[last_point].arrive_tangent;
    }
    else if (in_val >= points[last_point].in_val + loop_key_offset) {
      // Looped spline: last point is the same as the first point
      return points[0].arrive_tangent;
    }
  }

  // Somewhere within curve range - interpolate.
  fun_check(index >= 0 && ((is_looped && index < point_count) || (!is_looped && index < last_point)));
  const bool is_loop_segment = (is_looped && index == last_point);
  const int32 next_index = is_loop_segment ? 0 : (index + 1);

  const auto& prev_point = points[index];
  const auto& next_point = points[next_index];

  const float diff = is_loop_segment ? loop_key_offset : (next_point.in_val - prev_point.in_val);

  if (diff > 0.0f && prev_point.interp_mode != CIM_Constant) {
    if (prev_point.interp_mode == CIM_Linear) {
      return (next_point.out_val - prev_point.out_val) / diff;
    }
    else {
      const float alpha = (in_val - prev_point.in_val) / diff;
      fun_check(alpha >= 0.0f && alpha <= 1.0f);

      return Math::CubicInterpDerivative(prev_point.out_val, prev_point.leave_tangent * diff, next_point.out_val, next_point.arrive_tangent * diff, alpha) / diff;
    }
  }
  else {
    // Derivative of a constant is zero
    return T(ForceInit);
  }
}

template <typename T>
T InterpCurve<T>::EvalSecondDerivative(const float in_val, const T& default_value) const
{
  const int32 point_count = points.Count();
  const int32 last_point = point_count - 1;

  // If no point in curve, return the default_value value we passed in.
  if (point_count == 0) {
    return default_value;
  }

  // Binary search to find index of lower bound of input value
  const int32 index = GetPointIndexForInputValue(in_val);

  // If before the first point, return 0
  if (index == -1) {
    return T(ForceInit);
  }

  // If on or beyond the last point, return 0
  if (index == last_point) {
    if (!is_looped || (in_val >= points[last_point].in_val + loop_key_offset)) {
      return T(ForceInit);
    }
  }

  // Somewhere within curve range - interpolate.
  fun_check(index >= 0 && ((is_looped && index < point_count) || (!is_looped && index < last_point)));
  const bool is_loop_segment = (is_looped && index == last_point);
  const int32 next_index = is_loop_segment ? 0 : (index + 1);

  const auto& prev_point = points[index];
  const auto& next_point = points[next_index];

  const float diff = is_loop_segment ? loop_key_offset : (next_point.in_val - prev_point.in_val);

  if (diff > 0.0f && prev_point.interp_mode != CIM_Constant) {
    if (prev_point.interp_mode == CIM_Linear) {
      // No change in tangent, return 0.
      return T(ForceInit);
    }
    else {
      const float alpha = (in_val - prev_point.in_val) / diff;
      fun_check(alpha >= 0.0f && alpha <= 1.0f);

      return Math::CubicInterpSecondDerivative(prev_point.out_val, prev_point.leave_tangent * diff, next_point.out_val, next_point.arrive_tangent * diff, alpha) / (diff * diff);
    }
  }
  else {
    // Second derivative of a constant is zero
    return T(ForceInit);
  }
}

template <typename T>
float InterpCurve<T>::InaccurateFindNearest(const T& point_in_space, float& out_distance_sqr) const
{
  const int32 point_count = points.Count();
  const int32 segment_count = is_looped ? point_count : point_count - 1;

  if (point_count > 1) {
    float best_distance_sqr;
    float best_result = InaccurateFindNearestOnSegment(point_in_space, 0, best_distance_sqr);
    for (int32 segment = 1; segment < segment_count; ++segment) {
      float local_distance_sqr;
      float local_result = InaccurateFindNearestOnSegment(point_in_space, segment, local_distance_sqr);
      if (local_distance_sqr < best_distance_sqr) {
        best_distance_sqr = local_distance_sqr;
        best_result = local_result;
      }
    }
    out_distance_sqr = best_distance_sqr;
    return best_result;
  }

  if (point_count == 1) {
    out_distance_sqr = (point_in_space - points[0].out_val).SizeSquared();
    return points[0].in_val;
  }

  return 0.0f;
}

template <typename T>
float InterpCurve<T>::InaccurateFindNearestOnSegment(const T& point_in_space, int32 point_index, float& out_distance_sqr) const
{
  const int32 point_count = points.Count();
  const int32 last_point = point_count - 1;
  const int32 next_point_index = (is_looped && point_index == last_point) ? 0 : (point_index + 1);
  fun_check(point_index >= 0 && ((is_looped && point_index < point_count) || (!is_looped && point_index < last_point)));

  const float next_in_val = (is_looped && point_index == last_point) ? (points[last_point].in_val + loop_key_offset) : points[next_point_index].in_val;

  if (CIM_Constant == points[point_index].interp_mode) {
    const float distance1 = (points[point_index].out_val - point_in_space).SizeSquared();
    const float distance2 = (points[next_point_index].out_val - point_in_space).SizeSquared();
    if (distance1 < distance2) {
      out_distance_sqr = distance1;
      return points[point_index].in_val;
    }
    out_distance_sqr = distance2;
    return next_in_val;
  }

  const float diff = next_in_val - points[point_index].in_val;
  if (CIM_Linear == points[point_index].interp_mode) {
    // like in function: Math::ClosestPointOnLine
    const float a = (points[point_index].out_val - point_in_space) | (points[next_point_index].out_val - points[point_index].out_val);
    const float b = (points[next_point_index].out_val - points[point_index].out_val).SizeSquared();
    const float v = Math::Clamp01(-a / b);
    out_distance_sqr = (Math::Lerp(points[point_index].out_val, points[next_point_index].out_val, v) - point_in_space).SizeSquared();
    return v * diff + points[point_index].in_val;
  }
 {
    const int32 PointsChecked = 3;
    const int32 IterationNum = 3;
    const float Scale = 0.75;

    // Newton's methods is repeated 3 times, starting with t = 0, 0.5, 1.
    float ValuesT[PointsChecked];
    ValuesT[0] = 0.0f;
    ValuesT[1] = 0.5f;
    ValuesT[2] = 1.0f;

    T InitialPoints[PointsChecked];
    InitialPoints[0] = points[point_index].out_val;
    InitialPoints[1] = Math::CubicInterp(points[point_index].out_val, points[point_index].leave_tangent * diff, points[next_point_index].out_val, points[next_point_index].arrive_tangent * diff, ValuesT[1]);
    InitialPoints[2] = points[next_point_index].out_val;

    float DistancesSq[PointsChecked];

    for (int32 CheckPointIdx = 0; CheckPointIdx < PointsChecked; ++CheckPointIdx) {
      //Algorithm explanation: http://permalink.gmane.org/gmane.games.devel.sweng/8285
      T FoundPoint = InitialPoints[point];
      float LastMove = 1.0f;
      for (int32 Iter = 0; Iter < IterationNum; ++Iter) {
        const T LastBestTangent = Math::CubicInterpDerivative(points[point_index].out_val, points[point_index].leave_tangent * diff, points[next_point_index].out_val, points[next_point_index].arrive_tangent * diff, ValuesT[CheckPointIdx]);
        const T delta = (point_in_space - FoundPoint);
        float Move = (LastBestTangent | delta) / LastBestTangent.SizeSquared();
        Move = Math::Clamp(Move, -LastMove*Scale, LastMove*Scale);
        ValuesT[CheckPointIdx] += Move;
        ValuesT[CheckPointIdx] = Math::Clamp01(ValuesT[CheckPointIdx]);
        LastMove = Math::Abs(Move);
        FoundPoint = Math::CubicInterp(points[point_index].out_val, points[point_index].leave_tangent * diff, points[next_point_index].out_val, points[next_point_index].arrive_tangent * diff, ValuesT[CheckPointIdx]);
      }
      DistancesSq[CheckPointIdx] = (FoundPoint - point_in_space).SizeSquared();
      ValuesT[CheckPointIdx] = ValuesT[CheckPointIdx] * diff + points[point_index].in_val;
    }

    if (DistancesSq[0] <= DistancesSq[1] && DistancesSq[0] <= DistancesSq[2]) {
      out_distance_sqr = DistancesSq[0];
      return ValuesT[0];
    }
    if (DistancesSq[1] <= DistancesSq[2]) {
      out_distance_sqr = DistancesSq[1];
      return ValuesT[1];
    }
    out_distance_sqr = DistancesSq[2];
    return ValuesT[2];
  }
}

template <typename T>
void InterpCurve<T>::AutoSetTangents(float tension, bool bStationaryEndpoints)
{
  const int32 point_count = points.Count();
  const int32 last_point = point_count - 1;

  // Iterate over all points in this InterpCurve
  for (int32 point_index = 0; point_index < point_count; point_index++) {
    const int32 PrevIndex = (point_index == 0) ? (is_looped ? last_point : 0) : (point_index - 1);
    const int32 next_index = (point_index == last_point) ? (is_looped ? 0 : last_point) : (point_index + 1);

    auto& ThisPoint = points[point_index];
    const auto& prev_point = points[PrevIndex];
    const auto& next_point = points[next_index];

    if (ThisPoint.interp_mode == CIM_CurveAuto || ThisPoint.interp_mode == CIM_CurveAutoClamped) {
      if (bStationaryEndpoints && (point_index == 0 || (point_index == last_point && !is_looped))) {
        // start and end points get zero tangents if bStationaryEndpoints is true
        ThisPoint.arrive_tangent = T(ForceInit);
        ThisPoint.leave_tangent = T(ForceInit);
      }
      else if (prev_point.IsCurveKey()) {
        const bool bWantClamping = (ThisPoint.interp_mode == CIM_CurveAutoClamped);
        T Tangent;

        const float prev_time = (is_looped && point_index == 0) ? (ThisPoint.in_val - loop_key_offset) : prev_point.in_val;
        const float next_time = (is_looped && point_index == last_point) ? (ThisPoint.in_val + loop_key_offset) : next_point.in_val;

        ComputeCurveTangent(
          prev_time,     // Previous time
          prev_point.out_val, // Previous point
          ThisPoint.in_val,  // current time
          ThisPoint.out_val, // current point
          next_time,     // Next time
          next_point.out_val, // Next point
          tension,      // tension
          bWantClamping,    // Want clamping?
          Tangent);       // Out

        ThisPoint.arrive_tangent = Tangent;
        ThisPoint.leave_tangent = Tangent;
      }
      else {
        // Following on from a line or constant; set curve tangent equal to that so there are no discontinuities
        ThisPoint.arrive_tangent = prev_point.arrive_tangent;
        ThisPoint.leave_tangent = prev_point.leave_tangent;
      }
    }
    else if (ThisPoint.interp_mode == CIM_Linear) {
      T Tangent = next_point.out_val - ThisPoint.out_val;
      ThisPoint.arrive_tangent = Tangent;
      ThisPoint.leave_tangent = Tangent;
    }
    else if (ThisPoint.interp_mode == CIM_Constant) {
      ThisPoint.arrive_tangent = T(ForceInit);
      ThisPoint.leave_tangent = T(ForceInit);
    }
  }
}

template <typename T>
void InterpCurve<T>::CalcBounds(T& out_min, T& out_max, const T& default_value) const
{
  const int32 point_count = points.Count();

  if (point_count == 0) {
    out_min = default_value;
    out_max = default_value;
  }
  else if (point_count == 1) {
    out_min = points[0].out_val;
    out_max = points[0].out_val;
  }
  else {
    out_min = points[0].out_val;
    out_max = points[0].out_val;

    const int32 segment_count = is_looped ? point_count : (point_count - 1);

    for (int32 index = 0; index < segment_count; index++) {
      const int32 next_index = (index == point_count - 1) ? 0 : (index + 1);
      CurveFindIntervalBounds(points[index], points[next_index], out_min, out_max, 0.0f);
    }
  }
}


// Common type definitions

#define FUN_DEFINE_INTERPCURVE_WRAPPER_STRUCT(Name, ElementType) \
  struct Name : InterpCurve<ElementType> { \
  private: \
    typedef InterpCurve<ElementType> Super; \
   \
  public: \
    Name() \
      : Super() \ { \
    } \
     \
    Name(const Super& other) \
      : Super(other) \ { \
    } \
  }; \
   \
  template <> \
  struct IsBitwiseConstructible<Name, InterpCurve<ElementType>> { \
    enum { Value = true }; \
  }; \
   \
  template <> \
  struct IsBitwiseConstructible<InterpCurve<ElementType>, Name> { \
    enum { Value = true }; \
  };

FUN_DEFINE_INTERPCURVE_WRAPPER_STRUCT(InterpCurveFloat, float)
FUN_DEFINE_INTERPCURVE_WRAPPER_STRUCT(InterpCurveVector2, Vector2)
FUN_DEFINE_INTERPCURVE_WRAPPER_STRUCT(InterpCurveVector, Vector)
FUN_DEFINE_INTERPCURVE_WRAPPER_STRUCT(InterpCurveQuat, Quat)
FUN_DEFINE_INTERPCURVE_WRAPPER_STRUCT(InterpCurveTwoVectors, TwoVectors)
FUN_DEFINE_INTERPCURVE_WRAPPER_STRUCT(InterpCurveLinearColor, LinearColor)

} // namespace fun
