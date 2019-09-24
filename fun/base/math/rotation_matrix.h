#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Rotation matrix no translation
 */
class RotationMatrix : public RotationTranslationMatrix {
 public:
  /**
   * Constructor.
   *
   * \param rot - Rotation.
   */
  RotationMatrix(const Rotator& rot)
    : RotationTranslationMatrix(rot, Vector::ZeroVector) {}

  /**
   * matrix factory. Return an Matrix so we don't have type conversion issues in expressions.
   */
  static Matrix Make(Rotator const& rot) {
    return RotationMatrix(rot);
  }

  /**
   * matrix factory. Return an Matrix so we don't have type conversion issues in expressions.
   */
  static FUN_BASE_API Matrix Make(Quat const& rot);

  /**
   * Builds a rotation matrix given only a x_axis. y and z are unspecified but will be orthonormal. x_axis need not be normalized.
   */
  static FUN_BASE_API Matrix MakeFromX(Vector const& x_axis);

  /**
   * Builds a rotation matrix given only a y_axis. x and z are unspecified but will be orthonormal. y_axis need not be normalized.
   */
  static FUN_BASE_API Matrix MakeFromY(Vector const& y_axis);

  /**
   * Builds a rotation matrix given only a z_axis. x and y are unspecified but will be orthonormal. z_axis need not be normalized.
   */
  static FUN_BASE_API Matrix MakeFromZ(Vector const& z_axis);

  /**
   * Builds a matrix with given x and y axes. x will remain fixed, y may be changed minimally to enforce orthogonality. z will be computed. Inputs need not be normalized.
   */
  static FUN_BASE_API Matrix MakeFromXY(Vector const& x_axis, Vector const& y_axis);

  /**
   * Builds a matrix with given x and z axes. x will remain fixed, z may be changed minimally to enforce orthogonality. y will be computed. Inputs need not be normalized.
   */
  static FUN_BASE_API Matrix MakeFromXZ(Vector const& x_axis, Vector const& z_axis);

  /**
   * Builds a matrix with given y and x axes. y will remain fixed, x may be changed minimally to enforce orthogonality. z will be computed. Inputs need not be normalized.
   */
  static FUN_BASE_API Matrix MakeFromYX(Vector const& y_axis, Vector const& x_axis);

  /**
   * Builds a matrix with given y and z axes. y will remain fixed, z may be changed minimally to enforce orthogonality. x will be computed. Inputs need not be normalized.
   */
  static FUN_BASE_API Matrix MakeFromYZ(Vector const& y_axis, Vector const& z_axis);

  /**
   * Builds a matrix with given z and x axes. z will remain fixed, x may be changed minimally to enforce orthogonality. y will be computed. Inputs need not be normalized.
   */
  static FUN_BASE_API Matrix MakeFromZX(Vector const& z_axis, Vector const& x_axis);

  /**
   * Builds a matrix with given z and y axes. z will remain fixed, y may be changed minimally to enforce orthogonality. x will be computed. Inputs need not be normalized.
   */
  static FUN_BASE_API Matrix MakeFromZY(Vector const& z_axis, Vector const& y_axis);
};

} // namespace fun
