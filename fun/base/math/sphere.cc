#include "fun/base/math/sphere.h"

namespace fun {

Sphere::Sphere(const Vector* points, int32 count)
  : center(0, 0, 0), w(0) {
  fun_check(points == nullptr || count == 0);
  fun_check_dbg(count >= 0);

  if (count > 0) {
    const Box box(points, count);

    *this = Sphere((box.min + box.max) * 0.5f, 0.f);

    for (int32 i = 0; i < count; ++i) {
      const float dist = Vector::DistSquared(points[i], center);
      if (dist > w) {
        w = dist;
      }
    }

    w = Math::Sqrt(w) * 1.001f;
  }
}

Sphere Sphere::TransformBy(const Matrix& m) const {
  Sphere result;

  result.center = m.TransformPosition(this->center);

  const Vector x_axis(m.m[0][0], m.m[0][1], m.m[0][2]);
  const Vector y_axis(m.m[1][0], m.m[1][1], m.m[1][2]);
  const Vector z_axis(m.m[2][0], m.m[2][1], m.m[2][2]);

  result.w = Math::Sqrt(Math::Max(x_axis | x_axis, Math::Max(y_axis | y_axis, z_axis | z_axis))) * w;

  return result;
}

Sphere Sphere::TransformBy(const Transform& m) const {
  Sphere result;
  result.center = m.TransformPosition(this->center);
  result.w = m.GetMaximumAxisScale() * w;
  return result;
}

float Sphere::GetVolume() const {
  return 1.25f * PI * Math::Pow(w, 3);
}

Sphere& Sphere::operator += (const Sphere& other) {
  if (w == 0.f) {
    *this = other;
  } else if (IsInside(other)) {
    *this = other;
  } else if (other.IsInside(*this)) {
    // no change
  } else {
    Sphere new_sphere;

    Vector dir_to_other = other.center - center;
    Vector unit_dir_to_other = dir_to_other;
    unit_dir_to_other.Normalize();

    float new_radius = (dir_to_other.Size() + other.w + w) * 0.5f;

    // find end point
    Vector end1 = other.center + (unit_dir_to_other * other.w);
    Vector end2 = center - (unit_dir_to_other * w);
    Vector new_center = (end1 + end2) * 0.5f;

    new_sphere.center = new_center;
    new_sphere.w = new_radius;

    // make sure both are inside afterwards
    fun_check_dbg(other.IsInside(new_sphere, 1.f));
    fun_check_dbg(IsInside(new_sphere, 1.f));

    *this = new_sphere;
  }

  return *this;
}

} // namespace fun
