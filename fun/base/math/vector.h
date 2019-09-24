#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * A vector in 3-D space composed of components (x, y, z)
 * with floating point precision.
 */
class Vector {
 public:
  float x;
  float y;
  float z;

  /** a zero vector (0, 0, 0) */
  static FUN_BASE_API const Vector ZeroVector;

  /** Env up vector (0, 0, 1) */
  static FUN_BASE_API const Vector UpVector;

  /** FUN forward vector (1, 0, 0) */
  static FUN_BASE_API const Vector ForwardVector;

  /** FUN right vector (0, 1, 0) */
  static FUN_BASE_API const Vector RightVector;

#if FUN_ENABLE_NAN_DIAGNOSTIC
  FUN_ALWAYS_INLINE void DiagnosticCheckNaN() const {
    if (ContainsNaN()) {
      LOG_OR_ENSURE_NAN_ERROR("Vector contains NaN: %s", *ToString());
      *const_cast<Vector*>(this) = ZeroVector;
    }
  }

  FUN_ALWAYS_INLINE void DiagnosticCheckNaN(const char* message) const {
    if (ContainsNaN()) {
      LOG_OR_ENSURE_NAN_ERROR("%s: Vector contains NaN: %s", message, *ToString());
      *const_cast<Vector*>(this) = ZeroVector;
    }
  }
#else
  FUN_ALWAYS_INLINE void DiagnosticCheckNaN() const {}
  FUN_ALWAYS_INLINE void DiagnosticCheckNaN(const char* message) const {}
#endif

  Vector();
  explicit Vector(float scalar);
  Vector(float x, float y, float z);
  explicit Vector(const Vector2& v2, float z);
  Vector(const Vector4& v4);
  explicit Vector(const LinearColor& color);
  explicit Vector(const IntVector& iv);
  explicit Vector(const IntPoint& ipt);
  explicit Vector(ForceInit_TAG);

#ifdef IMPLEMENT_ASSIGNMENT_OPERATOR_MANUALLY
  Vector& operator = (const Vector& v);
#endif

  Vector operator ^ (const Vector& v) const;
  static Vector CrossProduct(const Vector& a, const Vector& b);
  float operator | (const Vector& v) const;
  static float DotProduct(const Vector& a, const Vector& b);

  Vector operator + (const Vector& v) const;
  Vector operator - (const Vector& v) const;
  Vector operator - (float bias) const;
  Vector operator + (float bias) const;
  Vector operator * (float scale) const;
  Vector operator / (float scale) const;
  Vector operator * (const Vector& v) const;
  Vector operator / (const Vector& v) const;

  bool operator == (const Vector& v) const;
  bool operator != (const Vector& v) const;
  bool Equals(const Vector& v, float tolerance = KINDA_SMALL_NUMBER) const;
  bool AllComponentsEqual(float tolerance = KINDA_SMALL_NUMBER) const;

  Vector operator -() const;
  Vector operator += (const Vector& v);
  Vector operator -= (const Vector& v);
  Vector operator *= (float scale);
  Vector operator /= (float scale);
  Vector operator *= (const Vector& v);
  Vector operator /= (const Vector& v);

  float& operator [] (int32 index);
  float operator [] (int32 index)const;
  float& Component(int32 index);
  float Component(int32 index) const;

  void Set(float x, float y, float z);

  float GetMax() const;
  float GetAbsMax() const;
  float GetMin() const;
  float GetAbsMin() const;

  Vector ComponentMin(const Vector& v) const;
  Vector ComponentMax(const Vector& v) const;

  Vector GetAbs() const;

  float Size() const;
  float SizeSquared() const;
  float Size2D() const ;
  float SizeSquared2D() const ;

  bool IsNearlyZero(float tolerance = KINDA_SMALL_NUMBER) const;
  bool IsZero() const;

  bool Normalize(float tolerance = SMALL_NUMBER);
  bool IsNormalized() const;

  void ToDirectionAndLength(Vector& out_dir, float& out_length) const;

  Vector GetSignVector() const;

  Vector Projection() const;

  Vector GetUnsafeNormal() const;

  Vector GridSnap(const float& grid_size) const;

  Vector BoundToCube(float radius) const;

  Vector GetClampedToSize(float min, float max) const;
  Vector GetClampedToSize2D(float min, float max) const;

  Vector GetClampedToMaxSize(float max_size) const;

  Vector GetClampedToMaxSize2D(float max_size) const;

  void AddBounded(const Vector& v, float radius = int16_MAX);

  Vector Reciprocal() const;

  bool IsUniform(float tolerance = KINDA_SMALL_NUMBER) const;

  Vector MirrorByVector(const Vector& mirror_normal) const;

  Vector MirrorByPlane(const Plane& plane) const;

  Vector RotateAngleAxis(const float degrees, const Vector& axis) const;

  Vector GetSafeNormal(float tolerance = SMALL_NUMBER) const;

  Vector GetSafeNormal2D(float tolerance = SMALL_NUMBER) const;

  float CosineAngle2D(Vector b) const;

  Vector ProjectOnTo(const Vector& a) const ;

  Vector ProjectOnToNormal(const Vector& normal) const;

  FUN_BASE_API Rotator ToOrientationRotator() const;

  FUN_BASE_API Quat ToOrientationQuat() const;

  Rotator Rotation() const;

  FUN_BASE_API void FindBestAxisVectors(Vector& axis1, Vector& axis2) const;

  FUN_BASE_API void UnwindEuler();

  bool ContainsNaN() const;

  bool IsUnit(float size_sqr_tolerance = KINDA_SMALL_NUMBER) const;

  String ToString() const;
  String ToCompactString() const;
  bool InitFromString(const String& string);

  Vector2 UnitCartesianToSpherical() const;

  float HeadingAngle() const;

  static FUN_BASE_API void CreateOrthonormalBasis(Vector& x_axis, Vector& y_axis, Vector& z_axis);

  static bool PointsAreSame(const Vector& p, const Vector& q);

  static bool PointsAreNear(const Vector& point1, const Vector& point2, float dist);

  static float PointPlaneDist(const Vector& point, const Vector& plane_base, const Vector& plane_normal);

  static Vector PointPlaneProject(const Vector& point, const Plane& plane);

  static Vector PointPlaneProject(const Vector& point, const Vector& a, const Vector& b, const Vector& c);

  static Vector PointPlaneProject(const Vector& point, const Vector& plane_base, const Vector& plane_normal);

  static Vector VectorPlaneProject(const Vector& v, const Vector& plane_normal);

  static float Distance(const Vector& v1, const Vector& v2);
  static float DistanceSqr(const Vector& v1, const Vector& v2);
  static float DistanceSqrXY(const Vector& v1, const Vector& v2);

  static float BoxPushOut(const Vector& normal, const Vector& Size);

  static bool Parallel(const Vector& normal1, const Vector& normal2, float parallel_cosine_threshold = THRESH_NORMALS_ARE_PARALLEL);

  static bool Coincident(const Vector& normal1, const Vector& normal2, float parallel_cosine_threshold = THRESH_NORMALS_ARE_PARALLEL);

  static bool Orthogonal(const Vector& normal1, const Vector& normal2, float orthogonal_cosine_threshold = THRESH_NORMALS_ARE_ORTHOGONAL);

  static bool Coplanar(const Vector& base1, const Vector& normal1, const Vector& base2, const Vector& normal2, float parallel_cosine_threshold = THRESH_NORMALS_ARE_PARALLEL);

  static float Triple(const Vector& x, const Vector& y, const Vector& z);

  static FUN_BASE_API float EvaluateBezier(const Vector* control_point_list, int32 control_point_count, Array<Vector>& out_points);

  static Vector RadiansToDegrees(const Vector& RadVector);

  static Vector DegreesToRadians(const Vector& DegVector);

  static FUN_BASE_API void GenerateClusterCenters(Array<Vector>& clusters, const Array<Vector>& points, int32 iteration_count, int32 connection_to_bea_valid_count);

  friend Archive& operator & (Archive& ar, Vector& v) {
    // @warning BulkSerialize: Vector is serialized as memory dump
    // See Array::BulkSerialize for detailed description of implied limitations.
    return ar & v.x & v.y & v.z;
  }

  bool Serialize(Archive& ar) {
    ar & *this;
    return true;
  }

  FUN_BASE_API bool NetSerialize(Archive& ar, class RPackageMap* map, bool& out_success);
};

template <> struct IsPOD<Vector> { enum { Value = true }; };


//
// inlines
//

FUN_ALWAYS_INLINE Vector operator * (float scale, const Vector& v) {
  return v.operator*(scale);
}

FUN_ALWAYS_INLINE uint32 HashOf(const Vector& vector) {
  return Crc::Crc32(&vector, sizeof vector);
}

FUN_ALWAYS_INLINE uint32 HashOf(const Vector2& vector) {
  return Crc::Crc32(&vector, sizeof vector);
}

#if FUN_ARCH_LITTLE_ENDIAN
  #define INTEL_ORDER_VECTOR(x)  (x)
#else
  static FUN_ALWAYS_INLINE Vector INTEL_ORDER_VECTOR(const Vector& v) {
    return Vector(INTEL_ORDERF(v.x), INTEL_ORDERF(v.y), INTEL_ORDERF(v.z));
  }
#endif

FUN_ALWAYS_INLINE float ComputeSquaredDistanceFromBoxToPoint(const Vector& mins, const Vector& maxs, const Vector& point) {
  // Accumulates the distance as we iterate axis
  float dist_sqr = 0.f;

  // Check each axis for min/max and add the distance accordingly
  // NOTE: Loop manually unrolled for > 2x speed up
  if (point.x < mins.x) {
    dist_sqr += Math::Square(point.x - mins.x);
  } else if (point.x > maxs.x) {
    dist_sqr += Math::Square(point.x - maxs.x);
  }

  if (point.y < mins.y) {
    dist_sqr += Math::Square(point.y - mins.y);
  } else if (point.y > maxs.y) {
    dist_sqr += Math::Square(point.y - maxs.y);
  }

  if (point.z < mins.z) {
    dist_sqr += Math::Square(point.z - mins.z);
  } else if (point.z > maxs.z) {
    dist_sqr += Math::Square(point.z - maxs.z);
  }

  return dist_sqr;
}

FUN_ALWAYS_INLINE Vector::Vector(const Vector2& v2, float z)
  : x(v2.x), y(v2.y), z(z) {
  DiagnosticCheckNaN();
}

FUN_ALWAYS_INLINE Vector Vector::RotateAngleAxis(const float degrees, const Vector& axis) const {
  float s, c;
  Math::SinCos(&s, &c, Math::DegreesToRadians(degrees));

  const float xx  = axis.x * axis.x;
  const float yy  = axis.y * axis.y;
  const float zz  = axis.z * axis.z;

  const float xy  = axis.x * axis.y;
  const float yz  = axis.y * axis.z;
  const float zx  = axis.z * axis.x;

  const float xs  = axis.x * s;
  const float ys  = axis.y * s;
  const float zs  = axis.z * s;

  const float omc = 1.f - c;

  return Vector(
          (omc * xx + c ) * x + (omc * xy - zs) * y + (omc * zx + ys) * z,
          (omc * xy + zs) * x + (omc * yy + c ) * y + (omc * yz - xs) * z,
          (omc * zx - ys) * x + (omc * yz + xs) * y + (omc * zz + c ) * z;
}

FUN_ALWAYS_INLINE bool Vector::PointsAreSame(const Vector& p, const Vector& q) {
  float tmp;
  tmp = p.x - q.x;
  if ((tmp > -THRESH_POINTS_ARE_SAME) && (tmp < THRESH_POINTS_ARE_SAME)) {
    tmp = p.y - q.y;
    if ((tmp > -THRESH_POINTS_ARE_SAME) && (tmp < THRESH_POINTS_ARE_SAME)) {
      tmp = p.z - q.z;
      if ((tmp > -THRESH_POINTS_ARE_SAME) && (tmp < THRESH_POINTS_ARE_SAME)) {
        return true;
      }
    }
  }
  return false;
}

FUN_ALWAYS_INLINE bool Vector::PointsAreNear(const Vector& point1, const Vector& point2, float dist) {
  float tmp;
  tmp = (point1.x - point2.x); if (Math::Abs(tmp) >= dist) return false;
  tmp = (point1.y - point2.y); if (Math::Abs(tmp) >= dist) return false;
  tmp = (point1.z - point2.z); if (Math::Abs(tmp) >= dist) return false;
  return true;
}

FUN_ALWAYS_INLINE float Vector::PointPlaneDist(const Vector& point, const Vector& plane_base, const Vector& plane_normal) {
  return (point - plane_base) | plane_normal;
}

FUN_ALWAYS_INLINE Vector Vector::PointPlaneProject(const Vector& point, const Vector& plane_base, const Vector& plane_normal) {
  //Find the distance of x from the plane
  //Add the distance back along the normal from the point
  return point - Vector::PointPlaneDist(point, plane_base, plane_normal) * plane_normal;
}

FUN_ALWAYS_INLINE Vector Vector::VectorPlaneProject(const Vector& v, const Vector& plane_normal) {
  return v - v.ProjectOnToNormal(plane_normal);
}

FUN_ALWAYS_INLINE bool Vector::Parallel(const Vector& normal1, const Vector& normal2, float parallel_cosine_threshold) {
  const float NormalDot = normal1 | normal2;
  return Math::Abs(NormalDot) >= parallel_cosine_threshold;
}

FUN_ALWAYS_INLINE bool Vector::Coincident(const Vector& normal1, const Vector& normal2, float parallel_cosine_threshold) {
  const float NormalDot = normal1 | normal2;
  return NormalDot >= parallel_cosine_threshold;
}

FUN_ALWAYS_INLINE bool Vector::Orthogonal(const Vector& normal1, const Vector& normal2, float orthogonal_cosine_threshold) {
  const float NormalDot = normal1 | normal2;
  return Math::Abs(NormalDot) <= orthogonal_cosine_threshold;
}

FUN_ALWAYS_INLINE bool Vector::Coplanar(const Vector& base1, const Vector& normal1, const Vector& base2, const Vector& normal2, float parallel_cosine_threshold) {
  if (!Vector::Parallel(normal1, normal2, parallel_cosine_threshold)) {
    return false;
  } else if (Vector::PointPlaneDist(base2, base1, normal1) > THRESH_POINT_ON_PLANE) {
    return false;
  } else {
    return true;
  }
}

FUN_ALWAYS_INLINE float Vector::Triple(const Vector& x, const Vector& y, const Vector& z) {
  return
          ( (x.x * (y.y * z.z - y.z * z.y))
          + (x.y * (y.z * z.x - y.x * z.z))
          + (x.z * (y.x * z.y - y.y * z.x)));
}

FUN_ALWAYS_INLINE Vector Vector::RadiansToDegrees(const Vector& RadVector) {
  return RadVector * RADIANS_TO_DEGREES;//(180.f / PI);
}

FUN_ALWAYS_INLINE Vector Vector::DegreesToRadians(const Vector& DegVector) {
  return DegVector * DEGREES_TO_RADIANS;//(PI / 180.f);
}

FUN_ALWAYS_INLINE Vector::Vector() {
  // NOOP
}

FUN_ALWAYS_INLINE Vector::Vector(float scalar)
  : x(scalar), y(scalar), z(scalar) {
  DiagnosticCheckNaN();
}

FUN_ALWAYS_INLINE Vector::Vector(float x, float y, float z)
  : x(x), y(y), z(z) {
  DiagnosticCheckNaN();
}

FUN_ALWAYS_INLINE Vector::Vector(const LinearColor& color)
  : x(color.r), y(color.g), z(color.b) {
  DiagnosticCheckNaN();
}

FUN_ALWAYS_INLINE Vector::Vector(const IntVector& iv)
  : x(iv.x), y(iv.y), z(iv.z) {
  DiagnosticCheckNaN();
}

FUN_ALWAYS_INLINE Vector::Vector(const IntPoint& ipt)
  : x(ipt.x), y(ipt.y), z(0.f) {
  DiagnosticCheckNaN();
}

FUN_ALWAYS_INLINE Vector::Vector(ForceInit_TAG)
  : x(0.f), y(0.f), z(0.f) {
}

#ifdef IMPLEMENT_ASSIGNMENT_OPERATOR_MANUALLY
FUN_ALWAYS_INLINE Vector& Vector::operator = (const Vector& v) {
  x = v.x;
  y = v.y;
  z = v.z;
  DiagnosticCheckNaN();
  return *this;
}
#endif

FUN_ALWAYS_INLINE Vector Vector::operator ^ (const Vector& v) const {
  return Vector(y * v.z - z * v.y,
                z * v.x - x * v.z,
                x * v.y - y * v.x);
}

FUN_ALWAYS_INLINE Vector Vector::CrossProduct(const Vector& a, const Vector& b) {
  return a ^ b;
}

FUN_ALWAYS_INLINE float Vector::operator | (const Vector& v) const {
  return x*v.x + y*v.y + z*v.z;
}

FUN_ALWAYS_INLINE float Vector::DotProduct(const Vector& a, const Vector& b) {
  return a | b;
}

FUN_ALWAYS_INLINE Vector Vector::operator + (const Vector& v) const {
  return Vector(x + v.x, y + v.y, z + v.z);
}

FUN_ALWAYS_INLINE Vector Vector::operator - (const Vector& v) const {
  return Vector(x - v.x, y - v.y, z - v.z);
}

FUN_ALWAYS_INLINE Vector Vector::operator - (float bias) const {
  return Vector(x - bias, y - bias, z - bias);
}

FUN_ALWAYS_INLINE Vector Vector::operator + (float bias) const {
  return Vector(x + bias, y + bias, z + bias);
}

FUN_ALWAYS_INLINE Vector Vector::operator * (float scale) const {
  return Vector(x * scale, y * scale, z * scale);
}

FUN_ALWAYS_INLINE Vector Vector::operator / (float scale) const {
  const float one_over_scale = 1.f / scale;
  return Vector(x * one_over_scale, y * one_over_scale, z * one_over_scale);
}

FUN_ALWAYS_INLINE Vector Vector::operator * (const Vector& v) const {
  return Vector(x * v.x, y * v.y, z * v.z);
}

FUN_ALWAYS_INLINE Vector Vector::operator / (const Vector& v) const {
  return Vector(x / v.x, y / v.y, z / v.z);
}

FUN_ALWAYS_INLINE bool Vector::operator == (const Vector& v) const {
  return x == v.x && y == v.y && z == v.z;
}

FUN_ALWAYS_INLINE bool Vector::operator != (const Vector& v) const {
  return x != v.x || y != v.y || z != v.z;
}

FUN_ALWAYS_INLINE bool Vector::Equals(const Vector& v, float tolerance) const {
  return  Math::Abs(x - v.x) <= tolerance &&
          Math::Abs(y - v.y) <= tolerance &&
          Math::Abs(z - v.z) <= tolerance;
}

FUN_ALWAYS_INLINE bool Vector::AllComponentsEqual(float tolerance) const {
  return  Math::Abs(x - y) <= tolerance &&
          Math::Abs(x - z) <= tolerance &&
          Math::Abs(y - z) <= tolerance;
}

FUN_ALWAYS_INLINE Vector Vector::operator -() const {
  return Vector(-x, -y, -z);
}

FUN_ALWAYS_INLINE Vector Vector::operator += (const Vector& v) {
  x += v.x;
  y += v.y;
  z += v.z;
  DiagnosticCheckNaN();
  return *this;
}

FUN_ALWAYS_INLINE Vector Vector::operator -= (const Vector& v) {
  x -= v.x;
  y -= v.y;
  z -= v.z;
  DiagnosticCheckNaN();
  return *this;
}

FUN_ALWAYS_INLINE Vector Vector::operator *= (float scale) {
  x *= scale;
  y *= scale;
  z *= scale;
  DiagnosticCheckNaN();
  return *this;
}

FUN_ALWAYS_INLINE Vector Vector::operator /= (float scale) {
  const float one_over_scale = 1.f / scale;
  x *= one_over_scale;
  y *= one_over_scale;
  z *= one_over_scale;
  DiagnosticCheckNaN();
  return *this;
}

FUN_ALWAYS_INLINE Vector Vector::operator *= (const Vector& v) {
  x *= v.x;
  y *= v.y;
  z *= v.z;
  DiagnosticCheckNaN();
  return *this;
}

FUN_ALWAYS_INLINE Vector Vector::operator /= (const Vector& v) {
  x /= v.x;
  y /= v.y;
  z /= v.z;
  DiagnosticCheckNaN();
  return *this;
}

FUN_ALWAYS_INLINE float& Vector::operator [] (int32 index) {
  fun_check(index >= 0 && index < 3);
  return index == 0 ? x : (index == 1 ? y : z);
}

FUN_ALWAYS_INLINE float Vector::operator [] (int32 index)const {
  fun_check(index >= 0 && index < 3);
  return index == 0 ? x : (index == 1 ? y : z);
}

FUN_ALWAYS_INLINE void Vector::Set(float x, float y, float z) {
  this->x = x;
  this->y = y;
  this->z = z;
  DiagnosticCheckNaN();
}

FUN_ALWAYS_INLINE float Vector::GetMax() const {
  return Math::Max(Math::Max(x, y), z);
}

FUN_ALWAYS_INLINE float Vector::GetAbsMax() const {
  return Math::Max(Math::Max(Math::Abs(x), Math::Abs(y)), Math::Abs(z));
}

FUN_ALWAYS_INLINE float Vector::GetMin() const {
  return Math::Min(Math::Min(x, y), z);
}

FUN_ALWAYS_INLINE float Vector::GetAbsMin() const {
  return Math::Min(Math::Min(Math::Abs(x), Math::Abs(y)), Math::Abs(z));
}

FUN_ALWAYS_INLINE Vector Vector::ComponentMin(const Vector& v) const {
  return Vector(Math::Min(x, v.x), Math::Min(y, v.y), Math::Min(z, v.z));
}

FUN_ALWAYS_INLINE Vector Vector::ComponentMax(const Vector& v) const {
  return Vector(Math::Max(x, v.x), Math::Max(y, v.y), Math::Max(z, v.z));
}

FUN_ALWAYS_INLINE Vector Vector::GetAbs() const {
  return Vector(Math::Abs(x), Math::Abs(y), Math::Abs(z));
}

FUN_ALWAYS_INLINE float Vector::Size() const {
  return Math::Sqrt(x*x + y*y + z*z);
}

FUN_ALWAYS_INLINE float Vector::SizeSquared() const {
  return x*x + y*y + z*z;
}

FUN_ALWAYS_INLINE float Vector::Size2D() const {
  return Math::Sqrt(x*x + y*y);
}

FUN_ALWAYS_INLINE float Vector::SizeSquared2D() const {
  return x*x + y*y;
}

FUN_ALWAYS_INLINE bool Vector::IsNearlyZero(float tolerance) const {
  return  Math::Abs(x) <= tolerance &&
          Math::Abs(y) <= tolerance &&
          Math::Abs(z) <= tolerance;
}

FUN_ALWAYS_INLINE bool Vector::IsZero() const {
  return x == 0.f && y == 0.f && z == 0.f;
}

FUN_ALWAYS_INLINE bool Vector::Normalize(float tolerance) {
  const float length_sqr = x*x + y*y + z*z;
  if (length_sqr > tolerance) {
    const float scale = Math::InvSqrt(length_sqr);
    x *= scale;
    y *= scale;
    z *= scale;
    return true;
  }

  return false;
}

FUN_ALWAYS_INLINE bool Vector::IsNormalized() const {
  return (Math::Abs(1.f - SizeSquared()) < THRESH_VECTOR_NORMALIZED);
}

FUN_ALWAYS_INLINE void Vector::ToDirectionAndLength(Vector& out_dir, float& out_length) const {
  out_length = Size();
  if (out_length > SMALL_NUMBER) {
    float one_over_length = 1.f / out_length;
    out_dir = Vector(x*one_over_length, y*one_over_length, z*one_over_length);
  }
  else {
    out_dir = Vector::ZeroVector;
  }
}

FUN_ALWAYS_INLINE Vector Vector::GetSignVector() const {
  return Vector(Math::FloatSelect(x, 1.f, -1.f),
                Math::FloatSelect(y, 1.f, -1.f),
                Math::FloatSelect(z, 1.f, -1.f));
}

FUN_ALWAYS_INLINE Vector Vector::Projection() const {
  const float one_over_z = 1.f / z;
  return Vector(x*one_over_z, y*one_over_z, 1);
}

FUN_ALWAYS_INLINE Vector Vector::GetUnsafeNormal() const {
  const float scale = Math::InvSqrt(x*x+y*y+z*z);
  return Vector(x*scale, y*scale, z*scale);
}

FUN_ALWAYS_INLINE Vector Vector::GridSnap(const float& grid_size) const {
  return Vector(Math::GridSnap(x, grid_size),
                Math::GridSnap(y, grid_size),
                Math::GridSnap(z, grid_size));
}

FUN_ALWAYS_INLINE Vector Vector::BoundToCube(float radius) const {
  return Vector(Math::Clamp(x, -radius, radius),
                Math::Clamp(y, -radius, radius),
                Math::Clamp(z, -radius, radius));
}

FUN_ALWAYS_INLINE Vector Vector::GetClampedToSize(float min, float max) const {
  float vec_size = Size();
  const Vector vec_dir = (vec_size > SMALL_NUMBER) ? (*this/vec_size) : Vector::ZeroVector;
  vec_size = Math::Clamp(vec_size, min, max);
  return vec_size * vec_dir;
}

FUN_ALWAYS_INLINE Vector Vector::GetClampedToSize2D(float min, float max) const {
  float vec_size_2d = Size2D();
  const Vector vec_dir = (vec_size_2d > SMALL_NUMBER) ? (*this/vec_size_2d) : Vector::ZeroVector;
  vec_size_2d = Math::Clamp(vec_size_2d, min, max);
  return Vector(vec_size_2d * vec_dir.x, vec_size_2d * vec_dir.y, z);
}

FUN_ALWAYS_INLINE Vector Vector::GetClampedToMaxSize(float max_size) const {
  if (max_size < KINDA_SMALL_NUMBER) {
    return Vector::ZeroVector;
  }

  const float size_sqr = SizeSquared();
  if (size_sqr > Math::Square(max_size)) {
    const float scale = max_size * Math::InvSqrt(size_sqr);
    return Vector(x*scale, y*scale, z*scale);
  } else {
    return *this;
  }
}

FUN_ALWAYS_INLINE Vector Vector::GetClampedToMaxSize2D(float max_size) const {
  if (max_size < KINDA_SMALL_NUMBER) {
    return Vector(0.f, 0.f, z);
  }

  const float size_sqr_2d = SizeSquared2D();
  if (size_sqr_2d > Math::Square(max_size)) {
    const float scale = max_size * Math::InvSqrt(size_sqr_2d);
    return Vector(x*scale, y*scale, z);
  } else {
    return *this;
  }
}

FUN_ALWAYS_INLINE void Vector::AddBounded(const Vector& v, float radius) {
  *this = (*this + v).BoundToCube(radius);
}

FUN_ALWAYS_INLINE float& Vector::Component(int32 index) {
  return (&x)[index];
}

FUN_ALWAYS_INLINE float Vector::Component(int32 index) const {
  return (&x)[index];
}

FUN_ALWAYS_INLINE Vector Vector::Reciprocal() const {
  Vector rec_vector;
  if (x != 0.f) {
    rec_vector.x = 1.f/x;
  } else {
    rec_vector.x = BIG_NUMBER;
  }

  if (y != 0.f) {
    rec_vector.y = 1.f/y;
  } else {
    rec_vector.y = BIG_NUMBER;
  }

  if (z != 0.f) {
    rec_vector.z = 1.f/z;
  } else {
    rec_vector.z = BIG_NUMBER;
  }

  return rec_vector;
}

FUN_ALWAYS_INLINE bool Vector::IsUniform(float tolerance) const {
  return AllComponentsEqual(tolerance);
}

FUN_ALWAYS_INLINE Vector Vector::MirrorByVector(const Vector& mirror_normal) const {
  return *this - mirror_normal * (2.f * (*this | mirror_normal));
}

FUN_ALWAYS_INLINE Vector Vector::GetSafeNormal(float tolerance) const {
  const float length_sqr = x*x + y*y + z*z;

  // Not sure if it's safe to add tolerance in there. Might introduce too many errors
  if (length_sqr == 1.f) {
    return *this;
  } else if (length_sqr < tolerance) {
    return Vector::ZeroVector;
  }

  const float scale = Math::InvSqrt(length_sqr);
  return Vector(x*scale, y*scale, z*scale);
}

FUN_ALWAYS_INLINE Vector Vector::GetSafeNormal2D(float tolerance) const {
  const float length_sqr = x*x + y*y;

  // Not sure if it's safe to add tolerance in there. Might introduce too many errors
  if (length_sqr == 1.f) {
    if (z == 0.f) {
      return *this;
    } else {
      return Vector(x, y, 0.f);
    }
  } else if (length_sqr < tolerance) {
    return Vector::ZeroVector;
  }

  const float scale = Math::InvSqrt(length_sqr);
  return Vector(x*scale, y*scale, 0.f);
}

FUN_ALWAYS_INLINE float Vector::CosineAngle2D(Vector b) const {
  Vector a(*this);
  a.z = 0.f;
  b.z = 0.f;
  a.Normalize();
  b.Normalize();
  return a | b;
}

FUN_ALWAYS_INLINE Vector Vector::ProjectOnTo(const Vector& a) const {
  return (a * ((*this | a) / (a | a)));
}

FUN_ALWAYS_INLINE Vector Vector::ProjectOnToNormal(const Vector& normal) const {
  return (normal * (*this | normal));
}

FUN_ALWAYS_INLINE bool Vector::ContainsNaN() const {
  return (Math::IsNaN(x) || !Math::IsFinite(x) ||
          Math::IsNaN(y) || !Math::IsFinite(y) ||
          Math::IsNaN(z) || !Math::IsFinite(z));
}

FUN_ALWAYS_INLINE bool Vector::IsUnit(float size_sqr_tolerance) const {
  return Math::Abs(1.f - SizeSquared()) < size_sqr_tolerance;
}

FUN_ALWAYS_INLINE String Vector::ToString() const {
  return String::Format("x=%3.3f y=%3.3f z=%3.3f", x, y, z);
}

FUN_ALWAYS_INLINE String Vector::ToCompactString() const {
  if (IsNearlyZero()) {
    return String::Format("v(0)");
  }

  String ret("v(");
  bool is_empty_string = true;
  if (!Math::IsNearlyZero(x)) {
    ret += String::Format("x=%.2f", x);
    is_empty_string = false;
  }

  if (!Math::IsNearlyZero(y)) {
    if (!is_empty_string) {
      ret += String(",");
    }
    ret += String::Format("y=%.2f", y);
    is_empty_string = false;
  }

  if (!Math::IsNearlyZero(z)) {
    if (!is_empty_string) {
      ret += String(",");
    }
    ret += String::Format("z=%.2f", z);
    is_empty_string = false;
  }

  ret += String(")");
  return ret;
}

FUN_ALWAYS_INLINE bool Vector::InitFromString(const String& string) {
  x = y = z = 0.f;

  // The initialization is only successful if the x, y, and z
  // values can all be parsed from the string
  const bool ok =
    Parse::Value(*string, "x=", x) &&
    Parse::Value(*string, "y=", y) &&
    Parse::Value(*string, "z=", z);

  return ok;
}

FUN_ALWAYS_INLINE Vector2 Vector::UnitCartesianToSpherical() const {
  fun_check_dbg(IsUnit());
  const float theta = Math::Acos(z / Size());
  const float phi = Math::Atan2(y, x);
  return Vector2(theta, phi);
}

FUN_ALWAYS_INLINE float Vector::HeadingAngle() const {
  // Project Dir into z plane.
  Vector plane_dir = *this;
  plane_dir.z = 0.f;
  plane_dir = plane_dir.GetSafeNormal();

  float angle = Math::Acos(plane_dir.x);

  if (plane_dir.y < 0.f) {
    angle *= -1.f;
  }

  return angle;
}

FUN_ALWAYS_INLINE float Vector::Distance(const Vector& v1, const Vector& v2) {
  return Math::Sqrt(Math::Square(v2.x - v1.x) + Math::Square(v2.y - v1.y) + Math::Square(v2.z - v1.z));
}

FUN_ALWAYS_INLINE float Vector::DistanceSqr(const Vector& v1, const Vector& v2) {
  return Math::Square(v2.x - v1.x) + Math::Square(v2.y - v1.y) + Math::Square(v2.z - v1.z);
}

FUN_ALWAYS_INLINE float Vector::DistanceSqrXY(const Vector& v1, const Vector& v2) {
  return Math::Square(v2.x - v1.x) + Math::Square(v2.y - v1.y);
}

FUN_ALWAYS_INLINE float Vector::BoxPushOut(const Vector& normal, const Vector& size) {
  return Math::Abs(normal.x * size.x) + Math::Abs(normal.y * size.y) + Math::Abs(normal.z * size.z);
}

FUN_ALWAYS_INLINE Vector ClampVector(const Vector& v, const Vector& min, const Vector& max) {
  return Vector(Math::Clamp(v.x, min.x, max.x),
                Math::Clamp(v.y, min.y, max.y),
                Math::Clamp(v.z, min.z, max.z));
}

} // namespace fun
