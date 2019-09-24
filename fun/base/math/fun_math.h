#pragma once

#include "fun/base/math/color.h"
#include "fun/base/math/color_list.h"
#include "fun/base/math/int_point.h"
#include "fun/base/math/int_vector.h"
#include "fun/base/math/vector2.h"
#include "fun/base/math/int_rect.h"
#include "fun/base/math/vector.h"
#include "fun/base/math/vector4.h"
#include "fun/base/math/two_vectors.h"
#include "fun/base/math/edge.h"
#include "fun/base/math/plane.h"
#include "fun/base/math/sphere.h"
#include "fun/base/math/rotator.h"
//#include "fun/base/math/range_bound.h"
//#include "fun/base/math/range.h"
//#include "fun/base/math/range_set.h"
//#include "fun/base/math/interval.h"
#include "fun/base/math/box.h"
#include "fun/base/math/box2.h"
#include "fun/base/math/box_sphere_bounds.h"
#include "fun/base/math/oriented_box.h"
#include "fun/base/math/axis.h"
#include "fun/base/math/matrix.h"
#include "fun/base/math/rotation_translation_matrix.h"
#include "fun/base/math/rotation_about_point_matrix.h"
#include "fun/base/math/scale_rotation_translation_matrix.h"
#include "fun/base/math/rotation_matrix.h"
#include "fun/base/math/quat.h"
#include "fun/base/math/perspective_matrix.h"
#include "fun/base/math/ortho_matrix.h"
#include "fun/base/math/translation_matrix.h"
#include "fun/base/math/quat_rotation_translation_matrix.h"
#include "fun/base/math/inverse_rotation_matrix.h"
#include "fun/base/math/scale_matrix.h"
#include "fun/base/math/mirror_matrix.h"
#include "fun/base/math/clip_projection_matrix.h"
//#include "fun/base/math/interp_curve_point.h"
//#include "fun/base/math/interp_curve.h"
//#include "fun/base/math/curve_ed_interface.h"
#include "fun/base/math/float32.h"
#include "fun/base/math/float16.h"
#include "fun/base/math/float16_color.h"
#include "fun/base/math/vector2_half.h"
//#include "fun/base/math/alpha_blend_type.h"
#include "fun/base/math/scalar_register.h"
//#include "fun/base/math/convex_hull2.h"

#include "fun/base/math/random_mt.h"

#include "fun/base/math/matrix_inline.h"

namespace fun {

//
// Vector2 Implementation
//

FUN_ALWAYS_INLINE Vector2::Vector2(const Vector& v)
  : x(v.x), y(v.y) {}

FUN_ALWAYS_INLINE Vector Vector2::SphericalToUnitCartesian() const {
  const float sin_theta = Math::Sin(x);
  return Vector(Math::Cos(y) * sin_theta, Math::Sin(y) * sin_theta, Math::Cos(x));
}


//
// Vector Implementation
//

FUN_ALWAYS_INLINE Vector::Vector(const Vector4& v)
  : x(v.x), y(v.y), z(v.z) {
  DiagnosticCheckNaN();
}

FUN_ALWAYS_INLINE Vector Vector::MirrorByPlane(const Plane& plane) const {
  return *this - plane * (2.f * plane.PlaneDot(*this));
}

FUN_ALWAYS_INLINE Vector Vector::PointPlaneProject(const Vector& point, const Plane& plane) {
  //Find the distance of x from the plane
  //Add the distance back along the normal from the point
  return point - plane.PlaneDot(point) * plane;
}

FUN_ALWAYS_INLINE Vector Vector::PointPlaneProject(const Vector& point, const Vector& a, const Vector& b, const Vector& c) {
  //Compute the plane normal from ABC
  Plane plane(a, b, c);

  //Find the distance of x from the plane
  //Add the distance back along the normal from the point
  return point - plane.PlaneDot(point) * plane;
}

FUN_ALWAYS_INLINE_DEBUGGABLE Rotator Vector::Rotation() const {
  return ToOrientationRotator();
}

FUN_ALWAYS_INLINE_DEBUGGABLE Rotator Vector4::Rotation() const {
  return ToOrientationRotator();
}

FUN_ALWAYS_INLINE Plane Plane::TransformBy(const Matrix& m) const {
  const Matrix ta = m.TransposeAdjoint();
  const float det_m = m.Determinant();
  return this->TransformByUsingAdjointT(m, det_m, ta);
}

FUN_ALWAYS_INLINE Plane Plane::TransformByUsingAdjointT(const Matrix& m, float det_m, const Matrix& ta) const {
  Vector new_normal = ta.TransformVector(*this).GetSafeNormal();

  if (det_m < 0.f) {
    new_normal *= -1.f;
  }

  return Plane(m.TransformPosition(*this * w), new_normal);
}

FUN_ALWAYS_INLINE Vector Math::LinePlaneIntersection(const Vector& point1,
                                          const Vector& point2,
                                          const Vector& plane_origin,
                                          const Vector& plane_normal) {
  return
    point1
    + (point2 - point1)
    * (((plane_origin - point1) | plane_normal) / ((point2 - point1) | plane_normal));
}

FUN_ALWAYS_INLINE Vector Math::LinePlaneIntersection(const Vector& point1, const Vector& point2, const Plane& plane) {
  return
    point1
    + (point2 - point1)
    * ((plane.w - (point1 | plane)) / ((point2 - point1) | plane));
}

FUN_ALWAYS_INLINE bool Math::PointBoxIntersection(const Vector& point, const Box& box) {
  if (point.x >= box.min.x && point.x <= box.max.x &&
      point.y >= box.min.y && point.y <= box.max.y &&
      point.z >= box.min.z && point.z <= box.max.z) {
    return true;
  } else {
    return false;
  }
}

FUN_ALWAYS_INLINE bool Math::LineBoxIntersection(const Box& box,
                                      const Vector& start,
                                      const Vector& end,
                                      const Vector& direction) {
  return LineBoxIntersection(box, start, end, direction, direction.Reciprocal());
}

FUN_ALWAYS_INLINE bool Math::LineBoxIntersection(const Box& box,
                                      const Vector& start,
                                      const Vector& end,
                                      const Vector& direction,
                                      const Vector& one_over_direction) {
  Vector time;
  bool start_is_outside = false;

  if (start.x < box.min.x) {
    start_is_outside = true;

    if (end.x >= box.min.x) {
      time.x = (box.min.x - start.x) * one_over_direction.x;
    } else {
      return false;
    }
  } else if (start.x > box.max.x) {
    start_is_outside = true;

    if (end.x <= box.max.x) {
      time.x = (box.max.x - start.x) * one_over_direction.x;
    } else {
      return false;
    }
  } else {
    time.x = 0.f;
  }

  if (start.y < box.min.y) {
    start_is_outside = true;

    if (end.y >= box.min.y) {
      time.y = (box.min.y - start.y) * one_over_direction.y;
    } else {
      return false;
    }
  } else if (start.y > box.max.y) {
    start_is_outside = true;

    if (end.y <= box.max.y) {
      time.y = (box.max.y - start.y) * one_over_direction.y;
    } else {
      return false;
    }
  } else {
    time.y = 0.f;
  }

  if (start.z < box.min.z) {
    start_is_outside = true;

    if (end.z >= box.min.z) {
      time.z = (box.min.z - start.z) * one_over_direction.z;
    } else {
      return false;
    }
  } else if (start.z > box.max.z) {
    start_is_outside = true;

    if (end.z <= box.max.z) {
      time.z = (box.max.z - start.z) * one_over_direction.z;
    } else {
      return false;
    }
  } else {
    time.z = 0.f;
  }

  if (start_is_outside) {
    const float max_time = Max3(time.x, time.y, time.z);

    if (max_time >= 0.f && max_time <= 1.f) {
      const Vector hint = start + direction * max_time;
      const float BOX_SIDE_THRESHOLD = 0.1f;
      if (hint.x > (box.min.x - BOX_SIDE_THRESHOLD) && hint.x < (box.max.x + BOX_SIDE_THRESHOLD) &&
          hint.y > (box.min.y - BOX_SIDE_THRESHOLD) && hint.y < (box.max.y + BOX_SIDE_THRESHOLD) &&
          hint.z > (box.min.z - BOX_SIDE_THRESHOLD) && hint.z < (box.max.z + BOX_SIDE_THRESHOLD)) {
        return true;
      }
    }

    return false;
  } else {
    return true;
  }
}

FUN_ALWAYS_INLINE bool Math::LineSphereIntersection( const Vector& start,
                                          const Vector& dir,
                                          float length,
                                          const Vector& origin,
                                          float radius) {
  const Vector eo = start - origin;
  const float v = (dir | (origin - start));
  const float disc = radius * radius - ((eo | eo) - v * v);

  if (disc >= 0.f) {
    const float time = (v - Sqrt(disc)) / length;

    if (time >= 0.f && time <= 1.f) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

FUN_ALWAYS_INLINE bool Math::IntersectPlanes3(Vector& i, const Plane& p1, const Plane& p2, const Plane& p3) {
  // Compute determinant, the triple product p1|(p2^p3)==(p1^p2)|p3.
  const float det = (p1 ^ p2) | p3;
  if (Square(det) < Square(0.001f)) {
    // Degenerate.
    i = Vector::ZeroVector;
    return false;
  } else {
    // Compute the intersection point, guaranteed valid if determinant is nonzero.
    i = (p1.w * (p2 ^ p3) + p2.w * (p3 ^ p1) + p3.w * (p1 ^ p2)) / det;
  }
  return true;
}

FUN_ALWAYS_INLINE bool Math::IntersectPlanes2(Vector& i, Vector& d, const Plane& p1, const Plane& p2) {
  // Compute line direction, perpendicular to both plane normals.
  d = p1 ^ p2;
  const float dd = d.SizeSquared();
  if (dd < Square(0.001f)) {
    // Parallel or nearly parallel planes.
    d = i = Vector::ZeroVector;
    return false;
  } else {
    // Compute intersection.
    i = (p1.w * (p2 ^ d) + p2.w * (d ^ p1)) / dd;
    d.Normalize();
    return true;
  }
}

FUN_ALWAYS_INLINE float Math::GetRangePct(Vector2 const& range, float value) {
  return (range.x != range.y) ? (value - range.x) / (range.y - range.x) : range.x;
}

FUN_ALWAYS_INLINE float Math::GetRangeValue(Vector2 const& range, float pct) {
  return Lerp<float>(range.x, range.y, pct);
}

FUN_ALWAYS_INLINE Vector Math::VRand() {
  Vector result;

  float l;
  do {
    // Check random vectors in the unit sphere so result is statistically uniform.
    result.x = FRand() * 2.f - 1.f;
    result.y = FRand() * 2.f - 1.f;
    result.z = FRand() * 2.f - 1.f;
    l = result.SizeSquared();
  } while (l > 1.f || l < KINDA_SMALL_NUMBER);

  return result * (1.f / Sqrt(l));
}

FUN_ALWAYS_INLINE float Math::RoundFromZero(float f) {
  return (f < 0.f) ? FloorToFloat(f) : CeilToFloat(f);
}

FUN_ALWAYS_INLINE double Math::RoundFromZero(double f) {
  return (f < 0.0) ? FloorToDouble(f) : CeilToDouble(f);
}

FUN_ALWAYS_INLINE float Math::RoundToZero(float f) {
  return (f < 0.f) ? CeilToFloat(f) : FloorToFloat(f);
}

FUN_ALWAYS_INLINE double Math::RoundToZero(double f) {
  return (f < 0.0) ? CeilToDouble(f) : FloorToDouble(f);
}

FUN_ALWAYS_INLINE float Math::RoundToNegativeInfinity(float f) {
  return FloorToFloat(f);
}

FUN_ALWAYS_INLINE double Math::RoundToNegativeInfinity(double f) {
  return FloorToDouble(f);
}

FUN_ALWAYS_INLINE float Math::RoundToPositiveInfinity(float f) {
  return CeilToFloat(f);
}

FUN_ALWAYS_INLINE double Math::RoundToPositiveInfinity(double f) {
  return CeilToDouble(f);
}

FUN_ALWAYS_INLINE string Math::FormatIntToHumanReadable(int32 val) {
  String src = String::Format("%i", val);
  String dst;

  if (val > 999) {
    dst = String::Format(", %s", *src.Mid(src.Len() - 3, 3));
    src = src.Left(src.Len() - 3);
  }

  if (val > 999999) {
    dst = String::Format(", %s%s", *src.Mid(src.Len() - 3, 3), *dst);
    src = src.Left(src.Len() - 3);
  }

  //dst = src + dst;
  dst.Prepend(src);

  return dst;
}

template <typename U>
FUN_ALWAYS_INLINE_DEBUGGABLE Rotator Math::Lerp(const Rotator& a, const Rotator& b, const U& alpha) {
  return (a * (1 - alpha) + b * alpha).GetNormalized();
}


template <typename U>
FUN_ALWAYS_INLINE_DEBUGGABLE Quat Math::Lerp(const Quat& a, const Quat& b, const U& alpha) {
  return Quat::Slerp(a, b, alpha);
}


template <typename U>
FUN_ALWAYS_INLINE_DEBUGGABLE Quat Math::BiLerp( const Quat& p00,
                                          const Quat& p10,
                                          const Quat& p01,
                                          const Quat& p11,
                                          const U& frac_x,
                                          const U& frac_y) {
  Quat result;
  result = Lerp(
        Quat::Slerp_NotNormalized(p00, p10, frac_x),
        Quat::Slerp_NotNormalized(p01, p11, frac_x),
        frac_y);
  return result;
}

template <typename U>
FUN_ALWAYS_INLINE_DEBUGGABLE Quat Math::CubicInterp(const Quat& p0,
                                              const Quat& t0,
                                              const Quat& p1,
                                              const Quat& t1,
                                              const U& a) {
  return Quat::Squad(p0, t0, p1, t1, a);
}

template <typename U>
FUN_ALWAYS_INLINE_DEBUGGABLE U Math::CubicCRSplineInterp(const U& p0, const U& p1, const U& p2, const U& p3, const float t0, const float t1, const float t2, const float t3, const float t) {
  //Based on http://www.cemyuksel.com/research/catmullrom_param/catmullrom.pdf
  float inv_t1_minus_t0 = 1.f / (t1 - t0);
  U l01 = (p0 * ((t1 - t) * inv_t1_minus_t0)) + (p1 * ((t - t0) * inv_t1_minus_t0));
  float inv_t2_minus_t1 = 1.f / (t2 - t1);
  U l12 = (p1 * ((t2 - t) * inv_t2_minus_t1)) + (p2 * ((t - t1) * inv_t2_minus_t1));
  float inv_t3_minus_t2 = 1.f / (t3 - t2);
  U l23 = (p2 * ((t3 - t) * inv_t3_minus_t2)) + (p3 * ((t - t2) * inv_t3_minus_t2));

  float inv_t2_minus_t0 = 1.f / (t2 - t0);
  U l012 = (l01 * ((t2 - t) * inv_t2_minus_t0)) + (l12 * ((t - t0) * inv_t2_minus_t0));
  float inv_t3_minus_t1 = 1.f / (t3 - t1);
  U l123 = (l12 * ((t3 - t) * inv_t3_minus_t1)) + (l23 * ((t - t1) * inv_t3_minus_t1));

  return  ((l012 * ((t2 - t) * inv_t2_minus_t1)) + (l123 * ((t - t1) * inv_t2_minus_t1)));
}

template <typename U>
FUN_ALWAYS_INLINE_DEBUGGABLE U Math::CubicCRSplineInterpSafe(const U& p0, const U& p1, const U& p2, const U& p3, const float t0, const float t1, const float t2, const float t3, const float t) {
  //Based on http://www.cemyuksel.com/research/catmullrom_param/catmullrom.pdf
  float t1_minus_t0 = (t1 - t0);
  float t2_minus_t1 = (t2 - t1);
  float t3_minus_t2 = (t3 - t2);
  float t2_minus_t0 = (t2 - t0);
  float t3_minus_t1 = (t3 - t1);
  if (Math::IsNearlyZero(t1_minus_t0) ||
      Math::IsNearlyZero(t2_minus_t1) ||
      Math::IsNearlyZero(t3_minus_t2) ||
      Math::IsNearlyZero(t2_minus_t0) ||
      Math::IsNearlyZero(t3_minus_t1)) {
    //There's going to be a divide by zero here so just bail out and return p1
    return p1;
  }

  float inv_t1_minus_t0 = 1.f / t1_minus_t0;
  U l01 = (p0 * ((t1 - t) * inv_t1_minus_t0)) + (p1 * ((t - t0) * inv_t1_minus_t0));
  float inv_t2_minus_t1 = 1.f / t2_minus_t1;
  U l12 = (p1 * ((t2 - t) * inv_t2_minus_t1)) + (p2 * ((t - t1) * inv_t2_minus_t1));
  float inv_t3_minus_t2 = 1.f / t3_minus_t2;
  U l23 = (p2 * ((t3 - t) * inv_t3_minus_t2)) + (p3 * ((t - t2) * inv_t3_minus_t2));

  float inv_t2_minus_t0 = 1.f / t2_minus_t0;
  U l012 = (l01 * ((t2 - t) * inv_t2_minus_t0)) + (l12 * ((t - t0) * inv_t2_minus_t0));
  float inv_t3_minus_t1 = 1.f / t3_minus_t1;
  U l123 = (l12 * ((t3 - t) * inv_t3_minus_t1)) + (l23 * ((t - t1) * inv_t3_minus_t1));

  return ((l012 * ((t2 - t) * inv_t2_minus_t1)) + (l123 * ((t - t1) * inv_t2_minus_t1)));
}

FUN_ALWAYS_INLINE bool Math::SphereAABBIntersection(const Vector& sphere_center, const float radius_sqr, const Box& aabb) {
  // Accumulates the distance as we iterate axis
  float dist_sqr = 0.f;

  // Check each axis for min/max and add the distance accordingly
  // NOTE: Loop manually unrolled for > 2x speed up
  if (sphere_center.x < aabb.min.x) {
    dist_sqr += Math::Square(sphere_center.x - aabb.min.x);
  } else if (sphere_center.x > aabb.max.x) {
    dist_sqr += Math::Square(sphere_center.x - aabb.max.x);
  } if (sphere_center.y < aabb.min.y) {
    dist_sqr += Math::Square(sphere_center.y - aabb.min.y);
  } else if (sphere_center.y > aabb.max.y) {
    dist_sqr += Math::Square(sphere_center.y - aabb.max.y);
  } if (sphere_center.z < aabb.min.z) {
    dist_sqr += Math::Square(sphere_center.z - aabb.min.z);
  } else if (sphere_center.z > aabb.max.z) {
    dist_sqr += Math::Square(sphere_center.z - aabb.max.z);
  }

  // If the distance is less than or equal to the radius, they intersect
  return dist_sqr <= radius_sqr;
}

FUN_ALWAYS_INLINE bool Math::SphereAABBIntersection(const Sphere& sphere, const Box& aabb) {
  const float radius_sqr = Math::Square(sphere.w);
  // If the distance is less than or equal to the radius, they intersect
  return SphereAABBIntersection(sphere.center, radius_sqr, aabb);
}

} // namespace fun
