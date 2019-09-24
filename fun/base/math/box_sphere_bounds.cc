#include "fun/base/math/box_sphere_bounds.h"

namespace fun {

BoxSphereBounds BoxSphereBounds::TransformBy(const Matrix& m) const {
#if FUN_ENABLE_NAN_DIAGNOSTIC
  if (m.ContainsNaN()) {
    LOG_OR_ENSURE_NAN_ERROR("Input matrix contains NaN/Inf! %s", *m.ToString());
    (const_cast<Matrix*>(&m))->SetIdentity();
  }
#endif

  BoxSphereBounds result;

  const VectorRegister vec_origin = VectorLoadFloat3(&origin);
  const VectorRegister vec_extent = VectorLoadFloat3(&box_extent);

  const VectorRegister m0 = VectorLoadAligned(m.m[0]);
  const VectorRegister m1 = VectorLoadAligned(m.m[1]);
  const VectorRegister m2 = VectorLoadAligned(m.m[2]);
  const VectorRegister m3 = VectorLoadAligned(m.m[3]);

  VectorRegister new_origin = VectorMultiply(VectorReplicate(vec_origin, 0), m0);
  new_origin = VectorMultiplyAdd(VectorReplicate(vec_origin, 1), m1, new_origin);
  new_origin = VectorMultiplyAdd(VectorReplicate(vec_origin, 2), m2, new_origin);
  new_origin = VectorAdd(new_origin, m3);

  VectorRegister new_extent = VectorAbs(VectorMultiply(VectorReplicate(vec_extent, 0), m0));
  new_extent = VectorAdd(new_extent, VectorAbs(VectorMultiply(VectorReplicate(vec_extent, 1), m1)));
  new_extent = VectorAdd(new_extent, VectorAbs(VectorMultiply(VectorReplicate(vec_extent, 2), m2)));

  VectorStoreFloat3(new_extent, &result.box_extent);
  VectorStoreFloat3(new_origin, &result.origin);

  VectorRegister max_radius = VectorMultiply(m0, m0);
  max_radius = VectorMultiplyAdd(m1, m1, max_radius);
  max_radius = VectorMultiplyAdd(m2, m2, max_radius);
  max_radius = VectorMax(VectorMax(max_radius, VectorReplicate(max_radius, 1)), VectorReplicate(max_radius, 2));
  result.sphre_radius = Math::Sqrt(VectorGetComponent(max_radius, 0)) * sphre_radius;

  result.DiagnosticCheckNaN();
  return result;
}

BoxSphereBounds BoxSphereBounds::TransformBy(const Transform& m) const {
#if FUN_ENABLE_NAN_DIAGNOSTIC
  m.DiagnosticCheckNaN_All();
#endif

  const Matrix mat = m.ToMatrixWithScale();
  BoxSphereBounds result = TransformBy(mat);
  return result;
}

} // namespace fun
