#include "CorePrivatePCH.h"

namespace fun {

Box::Box(const Vector* points, int32 count)
    min(0, 0, 0),
    max(0, 0, 0),
    is_valid(0) {
  for (int32 i = 0; i < count; ++i) {
    *this += points[i];
  }
}

Box::Box(const Array<Vector>& points)
  : min(0, 0, 0),
    max(0, 0, 0),
    is_valid(0) {
  for (int32 i = 0; i < points.Count(); ++i) {
    *this += points[i];
  }
}

Box Box::TransformBy(const Matrix& m) const {
  // if we are not valid, return another invalid box.
  if (!is_valid) {
    return Box(0);
  }

  Box new_box;

  const VectorRegister vec_min = VectorLoadFloat3(&min);
  const VectorRegister vec_max = VectorLoadFloat3(&max);

  const VectorRegister m0 = VectorLoadAligned(m.m[0]);
  const VectorRegister m1 = VectorLoadAligned(m.m[1]);
  const VectorRegister m2 = VectorLoadAligned(m.m[2]);
  const VectorRegister m3 = VectorLoadAligned(m.m[3]);

  const VectorRegister half = VectorSetFloat3(0.5f, 0.5f, 0.5f);
  const VectorRegister origin = VectorMultiply(VectorAdd(vec_max, vec_min), half);
  const VectorRegister extent = VectorMultiply(VectorSubtract(vec_max, vec_min), half);

  VectorRegister new_origin = VectorMultiply(VectorReplicate(origin, 0), m0);
  new_origin = VectorMultiplyAdd(VectorReplicate(origin, 1), m1, new_origin);
  new_origin = VectorMultiplyAdd(VectorReplicate(origin, 2), m2, new_origin);
  new_origin = VectorAdd(new_origin, m3);

  VectorRegister new_extent = VectorAbs(VectorMultiply(VectorReplicate(extent, 0), m0));
  new_extent = VectorAdd(new_extent, VectorAbs(VectorMultiply(VectorReplicate(extent, 1), m1)));
  new_extent = VectorAdd(new_extent, VectorAbs(VectorMultiply(VectorReplicate(extent, 2), m2)));

  const VectorRegister new_vec_min = VectorSubtract(new_origin, new_extent);
  const VectorRegister new_vec_max = VectorAdd(new_origin, new_extent);

  VectorStoreFloat3(new_vec_min, &new_box.min);
  VectorStoreFloat3(new_vec_max, &new_box.max);

  new_box.is_valid = true;

  return new_box;
}

Box Box::TransformBy(const Transform& m) const {
  return TransformBy(m.ToMatrixWithScale());
}

Box Box::InverseTransformBy(const Transform& m) const {
  const Vector vertices[8] = {
    Vector(min),
    Vector(min.x, min.y, max.z),
    Vector(min.x, max.y, min.z),
    Vector(max.x, min.y, min.z),
    Vector(max.x, max.y, min.z),
    Vector(max.x, min.y, max.z),
    Vector(min.x, max.y, max.z),
    Vector(max)
  };

  Box new_box(0);

  for (int32 i = 0; i < countof(vertices); ++i) {
    Vector4 projected_vertex = m.InverseTransformPosition(vertices[i]);
    new_box += projected_vertex;
  }

  return new_box;
}

Box Box::TransformProjectBy(const Matrix& m) const {
  const Vector vertices[8] = {
    Vector(min),
    Vector(min.x, min.y, max.z),
    Vector(min.x, max.y, min.z),
    Vector(max.x, min.y, min.z),
    Vector(max.x, max.y, min.z),
    Vector(max.x, min.y, max.z),
    Vector(min.x, max.y, max.z),
    Vector(max)
  };

  Box new_box(0);

  for (int32 i = 0; i < countof(vertices); ++i) {
    Vector4 projected_vertex = m.TransformPosition(vertices[i]);
    new_box += ((Vector)projected_vertex) / projected_vertex.w;
  }

  return new_box;
}

Box Box::Overlap(const Box& other) const {
  if (Intersect(other) == false) {
    static Box empty_box(ForceInit);
    return empty_box;
  }

  // otherwise they overlap
  // so find overlapping box
  Vector min_v, max_v;

  min_v.x = Math::Max(min.x, other.min.x);
  max_v.x = Math::Min(max.x, other.max.x);

  min_v.y = Math::Max(min.y, other.min.y);
  max_v.y = Math::Min(max.y, other.max.y);

  min_v.z = Math::Max(min.z, other.min.z);
  max_v.z = Math::Min(max.z, other.max.z);

  return Box(min_v, max_v);
}

} // namespace fun
