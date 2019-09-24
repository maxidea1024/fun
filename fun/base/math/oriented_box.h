#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Structure for arbitrarily oriented boxes (not necessarily axis-aligned).
 */
class OrientedBox {
 public:
  /** Holds the center of the box. */
  Vector center;

  /** Holds the x-axis vector of the box. Must be a unit vector. */
  Vector axis_x;

  /** Holds the y-axis vector of the box. Must be a unit vector. */
  Vector axis_y;

  /** Holds the z-axis vector of the box. Must be a unit vector. */
  Vector axis_z;

  /** Holds the extent of the box along its x-axis. */
  float extent_x;

  /** Holds the extent of the box along its y-axis. */
  float extent_y;

  /** Holds the extent of the box along its z-axis. */
  float extent_z;

  /**
   * Default constructor.
   *
   * Constructs a unit-sized, origin-centered box with axes aligned to the coordinate system.
   */
  OrientedBox()
    : center(0.f),
      axis_x(1.f, 0.f, 0.f),
      axis_y(0.f, 1.f, 0.f),
      axis_z(0.f, 0.f, 1.f),
      extent_x(1.f),
      extent_y(1.f),
      extent_z(1.f) {}

  /**
   * Fills in the verts array with the eight vertices of the box.
   *
   * \param verts - The array to fill in with the vertices.
   */
  FUN_ALWAYS_INLINE void CalcVertices(Vector* verts) const;

  /**
   * Finds the projection interval of the box when projected onto axis.
   *
   * \param axis - The unit vector defining the axis to project the box onto.
   */
  FUN_ALWAYS_INLINE FloatInterval Project(const Vector& axis) const;
};


//
// inlines
//

FUN_ALWAYS_INLINE void OrientedBox::CalcVertices(Vector* verts) const {
  static const float signs[] = { -1.f, 1.f };

  for (int32 i = 0; i < 2; i++) {
    for (int32 j = 0; j < 2; j++) {
      for (int32 k = 0; k < 2; k++) {
        *verts++ = center + signs[i] * axis_x * extent_x + signs[j] * axis_y * extent_y + signs[k] * axis_z * extent_z;
      }
    }
  }
}

FUN_ALWAYS_INLINE FloatInterval OrientedBox::Project(const Vector& axis) const {
  static const float signs[] = {-1.f, 1.f};

  // calculate the projections of the box center and the extent-scaled axes.
  float projected_center = axis | center;
  float projected_axis_x = axis | (extent_x * axis_x);
  float projected_axis_y = axis | (extent_y * axis_y);
  float projected_axis_z = axis | (extent_z * axis_z);

  FloatInterval projection_interval;

  for (int32 i = 0; i < 2; i++) {
    for (int32 j = 0; j < 2; j++) {
      for (int32 k = 0; k < 2; k++) {
        // project the box vertex onto the axis.
        float projected_vertex = projected_center + signs[i] * projected_axis_x + signs[j] * projected_axis_y + signs[k] * projected_axis_z;

        // if necessary, expand the projection interval to include the box vertex projection.
        projection_interval.Include(projected_vertex);
      }
    }
  }

  return projection_interval;
}

template <> struct IsPOD<OrientedBox> { enum { Value = true }; };

} // namespace fun
