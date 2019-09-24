#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * transform composed of Scale, rotation_ (as a quaternion), and translation_.
 *
 * Transforms can be used to convert from one space to another, for example by transforming
 * positions and directions from local space to world space.
 *
 * Transformation of position vectors is applied in the order:  Scale -> Rotate -> Translate.
 * Transformation of direction vectors is applied in the order: Scale -> Rotate.
 *
 * Order matters when composing transforms: C = a * b will yield a transform C that logically
 * first applies a then b to any subsequent transformation. Note that this is the opposite order of quaternion (Quat) multiplication.
 *
 * Example: local_to_world = (delta_rotation * local_to_world) will change rotation in local space by delta_rotation.
 * Example: local_to_world = (local_to_world * delta_rotation) will change rotation in world space by delta_rotation.
 */
class Transform {
//#if !defined(COREROBJECT_API)
//  #define MAYBE_COREROBJECT_API
//#else
//  #define MAYBE_COREROBJECT_API  COREROBJECT_API
//#endif
//  friend MAYBE_COREROBJECT_API class UScriptStruct* Z_Construct_UScriptStruct_CTransform();

 protected:
  /** rotation_ of this transformation, as a quaternion. */
  Quat rotation_;

  /** translation_ of this transformation, as a vector. */
  Vector translation_;

  /** 3D scale (always applied in local space) as a vector. */
  Vector scale_;

 public:
  /**
  The identity transformation (rotation = Quat::Identity, translation = Vector::ZeroVector, scale = (1, 1, 1)).
  */
  static FUN_BASE_API const Transform Identity;

#if FUN_ENABLE_NAN_DIAGNOSTIC
  FUN_ALWAYS_INLINE void DiagnosticCheckNaN_Scale3D() const {
    if (scale_.ContainsNaN()) {
      LOG_OR_ENSURE_NAN_ERROR("Transform scale_ contains NaN: %s", *scale_.ToString());
      const_cast<Transform*>(this)->scale_ = Vector(1.f);
    }
  }

  FUN_ALWAYS_INLINE void DiagnosticCheckNaN_Translate() const {
    if (translation_.ContainsNaN()) {
      LOG_OR_ENSURE_NAN_ERROR("Transform translation_ contains NaN: %s", *translation_.ToString());
      const_cast<Transform*>(this)->translation_ = Vector::ZeroVector;
    }
  }

  FUN_ALWAYS_INLINE void DiagnosticCheckNaN_Rotate() const {
    if (rotation_.ContainsNaN()) {
      LOG_OR_ENSURE_NAN_ERROR("Transform rotation_ contains NaN: %s", *rotation_.ToString());
      const_cast<Transform*>(this)->rotation_ = Quat::Identity;
    }
  }

  FUN_ALWAYS_INLINE void DiagnosticCheckNaN_All() const {
    DiagnosticCheckNaN_Scale3D();
    DiagnosticCheckNaN_Rotate();
    DiagnosticCheckNaN_Translate();
  }

  FUN_ALWAYS_INLINE void DiagnosticCheck_IsValid() const {
    DiagnosticCheckNaN_All();
    if (!IsValid()) {
      LOG_OR_ENSURE_NAN_ERROR("Transform transform is not valid: %s", *ToHumanReadableString());
    }

  }
#else
  FUN_ALWAYS_INLINE void DiagnosticCheckNaN_Translate() const {}
  FUN_ALWAYS_INLINE void DiagnosticCheckNaN_Rotate() const {}
  FUN_ALWAYS_INLINE void DiagnosticCheckNaN_Scale3D() const {}
  FUN_ALWAYS_INLINE void DiagnosticCheckNaN_All() const {}
  FUN_ALWAYS_INLINE void DiagnosticCheck_IsValid() const {}
#endif

  /**
   * Constructor with initialization to the identity transform.
   */
  FUN_ALWAYS_INLINE Transform()
    : rotation_(0.f, 0.f, 0.f, 1.f),
      translation_(0.f),
      scale_(1.f) {}

  /**
   * Constructor with an initial translation_
   *
   * \param translation - The value to use for the translation component
   */
  FUN_ALWAYS_INLINE explicit Transform(const Vector& translation)
    : rotation_(Quat::Identity),
      translation_(translation),
      scale_(Vector(1.f)) {
    DiagnosticCheckNaN_All();
  }

  /**
   * Constructor with leaving uninitialized memory
   */
  FUN_ALWAYS_INLINE explicit Transform(NoInit_TAG) {
    // Note: This can be used to track down initialization issues with bone transform arrays; but it will
    // cause issues with transient fields such as RootMotionDelta that get initialized to 0 by default
#if FUN_ENABLE_NAN_DIAGNOSTIC
    float qnan = Math::Log2(-5.3f);
    fun_check(Math::IsNaN(qnan));
    translation_ = Vector(qnan, qnan, qnan);
    rotation_ = Quat(qnan, qnan, qnan, qnan);
    scale_ = Vector(qnan, qnan, qnan);
#endif
  }

  /**
   * Constructor with an initial rotation
   *
   * \param rotation - The value to use for rotation component
   */
  FUN_ALWAYS_INLINE explicit Transform(const Quat& rotation)
    : rotation_(rotation),
      translation_(Vector::ZeroVector),
      scale_(Vector(1.f)) {
    DiagnosticCheckNaN_All();
  }

  /**
   * Constructor with an initial rotation
   *
   * \param rotation - The value to use for rotation component  (after being converted to a quaternion)
   */
  FUN_ALWAYS_INLINE explicit Transform(const Rotator& rotation)
    : rotation_(rotation),
      translation_(Vector::ZeroVector),
      scale_(Vector(1.f)) {
    DiagnosticCheckNaN_All();
  }

  /**
   * Constructor with all components initialized
   *
   * \param rotation - The value to use for rotation component
   * \param translation - The value to use for the translation_ component
   * \param scale - The value to use for the scale component
   */
  FUN_ALWAYS_INLINE Transform(const Quat& rotation, const Vector& translation, const Vector& scale = Vector(1.f))
    : rotation_(rotation),
      translation_(translation),
      scale_(scale) {
    DiagnosticCheckNaN_All();
  }

  /**
   * Constructor with all components initialized, taking a Rotator as the rotation component
   *
   * \param rotation - The value to use for rotation component (after being converted to a quaternion)
   * \param translation - The value to use for the translation_ component
   * \param scale - The value to use for the scale component
   */
  FUN_ALWAYS_INLINE Transform(const Rotator& rotation, const Vector& translation, const Vector& scale = Vector(1.f))
    : rotation_(rotation),
      translation_(translation),
      scale_(scale) {
    DiagnosticCheckNaN_All();
  }

  /**
  Copy-constructor

  \param InTransform - The source transform from which all components will be copied
  */
  FUN_ALWAYS_INLINE Transform(const Transform& InTransform)
    : rotation_(InTransform.rotation_),
      translation_(InTransform.translation_),
      scale_(InTransform.scale_) {
    DiagnosticCheckNaN_All();
  }

  /**
  Constructor for converting a matrix (including scale) into a Transform.
  */
  FUN_ALWAYS_INLINE explicit Transform(const Matrix& matrix) {
    SetFromMatrix(matrix);
    DiagnosticCheckNaN_All();
  }

  /** Constructor that takes basis axes and translation_ */
  FUN_ALWAYS_INLINE Transform(const Vector& x, const Vector& y, const Vector& z, const Vector& translation) {
    SetFromMatrix(Matrix(x, y, z, translation));
    DiagnosticCheckNaN_All();
  }

  /**
  Does a debugf of the contents of this transform.
  */
  FUN_BASE_API void DebugPrint() const;

  /** Debug purpose only **/
  bool DebugEqualMatrix(const Matrix& matrix) const;

  /** Convert Transform contents to a string */
  FUN_BASE_API String ToHumanReadableString() const;

  FUN_BASE_API String ToString() const;

  /** Acceptable form: "%f, %f, %f|%f, %f, %f|%f, %f, %f" */
  FUN_BASE_API bool InitFromString(const String& string);

#ifdef IMPLEMENT_ASSIGNMENT_OPERATOR_MANUALLY
  /**
   * Copy another transform into this one
   */
  FUN_ALWAYS_INLINE Transform& operator = (const Transform& other) {
    this->rotation_ = other.rotation_;
    this->translation_ = other.translation_;
    this->scale_ = other.scale_;
    return *this;
  }
#endif

  /**
   * Convert this transform to a transformation matrix with scaling.
   */
  FUN_ALWAYS_INLINE Matrix ToMatrixWithScale() const {
    Matrix matrix;

#if !(FUN_BUILD_SHIPPING || FUN_BUILD_TEST) && WITH_EDITORONLY_DATA
    // Make sure rotation_ is normalized when we turn it into a matrix.
    fun_check(IsRotationNormalized());
#endif
    matrix.m[3][0] = translation_.x;
    matrix.m[3][1] = translation_.y;
    matrix.m[3][2] = translation_.z;

    const float x2 = rotation_.x + rotation_.x;
    const float y2 = rotation_.y + rotation_.y;
    const float z2 = rotation_.z + rotation_.z; {
      const float xx2 = rotation_.x * x2;
      const float yy2 = rotation_.y * y2;
      const float zz2 = rotation_.z * z2;

      matrix.m[0][0] = (1.f - (yy2 + zz2)) * scale_.x;
      matrix.m[1][1] = (1.f - (xx2 + zz2)) * scale_.y;
      matrix.m[2][2] = (1.f - (xx2 + yy2)) * scale_.z;
    } {
      const float yz2 = rotation_.y * z2;
      const float wx2 = rotation_.w * x2;

      matrix.m[2][1] = (yz2 - wx2) * scale_.z;
      matrix.m[1][2] = (yz2 + wx2) * scale_.y;
    } {
      const float xy2 = rotation_.x * y2;
      const float wz2 = rotation_.w * z2;

      matrix.m[1][0] = (xy2 - wz2) * scale_.y;
      matrix.m[0][1] = (xy2 + wz2) * scale_.x;
    } {
      const float xz2 = rotation_.x * z2;
      const float wy2 = rotation_.w * y2;

      matrix.m[2][0] = (xz2 + wy2) * scale_.z;
      matrix.m[0][2] = (xz2 - wy2) * scale_.x;
    }

    matrix.m[0][3] = 0.f;
    matrix.m[1][3] = 0.f;
    matrix.m[2][3] = 0.f;
    matrix.m[3][3] = 1.f;

    return matrix;
  }

  /**
   * Convert this transform to matrix with scaling and compute the inverse of that.
   */
  FUN_ALWAYS_INLINE Matrix ToInverseMatrixWithScale() const {
    // todo: optimize
    return ToMatrixWithScale().Inverse();
  }

  /**
   * Convert this transform to inverse.
   */
  FUN_ALWAYS_INLINE Transform Inverse() const {
    Quat inv_rotation = rotation_.Inverse();
    Vector inv_scale = scale_.Reciprocal();
    Vector inv_translation = inv_rotation * (inv_scale * -translation_);
    return Transform(inv_rotation, inv_translation, inv_scale);
  }

  /**
   * Convert this transform to a transformation matrix, ignoring its scaling
   */
  FUN_ALWAYS_INLINE Matrix ToMatrixNoScale() const {
    Matrix matrix;

#if !(FUN_BUILD_SHIPPING || FUN_BUILD_TEST) && WITH_EDITORONLY_DATA
    // Make sure rotation_ is normalized when we turn it into a matrix.
    fun_check(IsRotationNormalized());
#endif
    matrix.m[3][0] = translation_.x;
    matrix.m[3][1] = translation_.y;
    matrix.m[3][2] = translation_.z;

    const float x2 = rotation_.x + rotation_.x;
    const float y2 = rotation_.y + rotation_.y;
    const float z2 = rotation_.z + rotation_.z; {
      const float xx2 = rotation_.x * x2;
      const float yy2 = rotation_.y * y2;
      const float zz2 = rotation_.z * z2;

      matrix.m[0][0] = (1.f - (yy2 + zz2));
      matrix.m[1][1] = (1.f - (xx2 + zz2));
      matrix.m[2][2] = (1.f - (xx2 + yy2));
    } {
      const float yz2 = rotation_.y * z2;
      const float wx2 = rotation_.w * x2;

      matrix.m[2][1] = (yz2 - wx2);
      matrix.m[1][2] = (yz2 + wx2);
    } {
      const float xy2 = rotation_.x * y2;
      const float wz2 = rotation_.w * z2;

      matrix.m[1][0] = (xy2 - wz2);
      matrix.m[0][1] = (xy2 + wz2);
    } {
      const float xz2 = rotation_.x * z2;
      const float wy2 = rotation_.w * y2;

      matrix.m[2][0] = (xz2 + wy2);
      matrix.m[0][2] = (xz2 - wy2);
    }

    matrix.m[0][3] = 0.f;
    matrix.m[1][3] = 0.f;
    matrix.m[2][3] = 0.f;
    matrix.m[3][3] = 1.f;

    return matrix;
  }

  /** Set this transform to the weighted blend of the supplied two transforms. */
  FUN_ALWAYS_INLINE void Blend(const Transform& atom1, const Transform& atom2, float alpha) {
#if !(FUN_BUILD_SHIPPING || FUN_BUILD_TEST) && WITH_EDITORONLY_DATA
    // Check that all bone atoms coming from animation are normalized
    fun_check(atom1.IsRotationNormalized());
    fun_check(atom2.IsRotationNormalized());
#endif
    if (alpha <= ZERO_ANIMWEIGHT_THRESH) {
      // if blend is all the way for child1, then just copy its bone atoms
      (*this) = atom1;
    } else if (alpha >= 1.f - ZERO_ANIMWEIGHT_THRESH) {
      // if blend is all the way for child2, then just copy its bone atoms
      (*this) = atom2;
    } else {
      // Simple linear interpolation for translation_ and scale.
      translation_ = Math::Lerp(atom1.translation_, atom2.translation_, alpha);
      scale_ = Math::Lerp(atom1.scale_, atom2.scale_, alpha);
      rotation_ = Quat::FastLerp(atom1.rotation_, atom2.rotation_, alpha);

      // ..and renormalize
      rotation_.Normalize();
    }
  }

  /** Set this transform to the weighted blend of it and the supplied transform. */
  FUN_ALWAYS_INLINE void BlendWith(const Transform& other_atom, float alpha) {
#if !(FUN_BUILD_SHIPPING || FUN_BUILD_TEST) && WITH_EDITORONLY_DATA
    // Check that all bone atoms coming from animation are normalized
    fun_check(IsRotationNormalized());
    fun_check(other_atom.IsRotationNormalized());
#endif
    if (alpha > ZERO_ANIMWEIGHT_THRESH) {
      if (alpha >= 1.f - ZERO_ANIMWEIGHT_THRESH) {
        // if blend is all the way for child2, then just copy its bone atoms
        (*this) = other_atom;
      } else {
        // Simple linear interpolation for translation_ and scale.
        translation_ = Math::Lerp(translation_, other_atom.translation_, alpha);
        scale_ = Math::Lerp(scale_, other_atom.scale_, alpha);
        rotation_ = Quat::FastLerp(rotation_, other_atom.rotation_, alpha);

        // ..and renormalize
        rotation_.Normalize();
      }
    }
  }

  /**
   * Quaternion addition is wrong here. This is just a special case for linear interpolation.
   * Use only within blends!!
   * rotation_ part is NOT normalized!!
   */
  FUN_ALWAYS_INLINE Transform operator + (const Transform& atom) const {
    return Transform(rotation_ + atom.rotation_, translation_ + atom.translation_, scale_ + atom.scale_);
  }

  FUN_ALWAYS_INLINE Transform& operator += (const Transform& atom) {
    translation_ += atom.translation_;

    rotation_.x += atom.rotation_.x;
    rotation_.y += atom.rotation_.y;
    rotation_.z += atom.rotation_.z;
    rotation_.w += atom.rotation_.w;

    scale_ += atom.scale_;

    DiagnosticCheckNaN_All();
    return *this;
  }

  FUN_ALWAYS_INLINE Transform operator * (ScalarRegister scale) const {
    return Transform(rotation_ * scale, translation_ * scale, scale_ * scale);
  }

  FUN_ALWAYS_INLINE Transform& operator *= (ScalarRegister scale) {
    translation_ *= scale;
    rotation_.x  *= scale;
    rotation_.y  *= scale;
    rotation_.z  *= scale;
    rotation_.w  *= scale;
    scale_ *= scale;
    DiagnosticCheckNaN_All();

    return *this;
  }

  /**
   * Return a transform that is the result of this multiplied by another transform.
   * Order matters when composing transforms : C = a * b will yield a transform C that logically first applies a then b to any subsequent transformation.
   *
   * \param - other - other transform by which to multiply.
   *
   * \return new transform: this * other
   */
  FUN_ALWAYS_INLINE Transform operator*(const Transform& other) const;

  /**
   * Sets this transform to the result of this multiplied by another transform.
   * Order matters when composing transforms : C = a * b will yield a transform C that logically first applies a then b to any subsequent transformation.
   *
   * \param other - other transform by which to multiply.
   */
  FUN_ALWAYS_INLINE void operator*=(const Transform& other);

  /**
   * Return a transform that is the result of this multiplied by another transform (made only from a rotation).
   * Order matters when composing transforms : C = a * b will yield a transform C that logically first applies a then b to any subsequent transformation.
   *
   * \param other - other quaternion rotation by which to multiply.
   *
   * \return new transform: this * Transform(other)
   */
  FUN_ALWAYS_INLINE Transform operator*(const Quat& other) const;

  /**
   * Sets this transform to the result of this multiplied by another transform (made only from a rotation).
   * Order matters when composing transforms : C = a * b will yield a transform C that logically first applies a then b to any subsequent transformation.
   *
   * \param other - other quaternion rotation by which to multiply.
   */
  FUN_ALWAYS_INLINE void operator*=(const Quat& other);

  FUN_ALWAYS_INLINE void ScaleTranslation(const Vector& scale);
  FUN_ALWAYS_INLINE void ScaleTranslation(const float& Scale);
  FUN_ALWAYS_INLINE void RemoveScaling(float tolerance = SMALL_NUMBER);
  FUN_ALWAYS_INLINE float GetMaximumAxisScale() const;
  FUN_ALWAYS_INLINE float GetMinimumAxisScale() const;
  // Inverse does not work well with VQS format(in particular non-uniform), so removing it, but made two below functions to be used instead.

  /*******************************************************************************************
  The below 2 functions are the ones to get delta transform and return Transform format that can be concatenated
  Inverse itself can't concatenate with VQS format(since VQS always transform from S->q->T, where inverse happens from T(-1)->q(-1)->S(-1))
  So these 2 provides ways to fix this
  GetRelativeTransform returns this*other(-1) and parameter is other(not other(-1))
  GetRelativeTransformReverse returns this(-1)*other, and parameter is other.
******************************************************************************************/
  FUN_BASE_API Transform GetRelativeTransform(const Transform& other) const;
  FUN_BASE_API Transform GetRelativeTransformReverse(const Transform& other) const;
  /**
   * Set current transform and the relative to parent_transform.
   * Equates to This = This->GetRelativeTransform(Parent), but saves the intermediate Transform storage and copy.
   */
  FUN_BASE_API void SetToRelativeTransform(const Transform& parent_transform);

  FUN_ALWAYS_INLINE Vector4 TransformVector4(const Vector4& v) const;
  FUN_ALWAYS_INLINE Vector4 TransformVector4NoScale(const Vector4& v) const;
  FUN_ALWAYS_INLINE Vector TransformPosition(const Vector& v) const;
  FUN_ALWAYS_INLINE Vector TransformPositionNoScale(const Vector& v) const;


  /** Inverts the matrix and then transforms v - correctly handles scaling in this matrix. */
  FUN_ALWAYS_INLINE Vector InverseTransformPosition(const Vector& v) const;

  FUN_ALWAYS_INLINE Vector InverseTransformPositionNoScale(const Vector& v) const;

  FUN_ALWAYS_INLINE Vector TransformVector(const Vector& v) const;

  FUN_ALWAYS_INLINE Vector TransformVectorNoScale(const Vector& v) const;

  /**
   * transform a direction vector by the inverse of this matrix - will not take into account translation_ part.
   * If you want to transform a surface normal (or plane) and correctly account for non-uniform scaling you should use TransformByUsingAdjointT with adjoint of matrix inverse.
   */
  FUN_ALWAYS_INLINE Vector InverseTransformVector(const Vector& v) const;

  FUN_ALWAYS_INLINE Vector InverseTransformVectorNoScale(const Vector& v) const;

  FUN_ALWAYS_INLINE Transform GetScaled(float scale) const;
  FUN_ALWAYS_INLINE Transform GetScaled(Vector scale) const;
  FUN_ALWAYS_INLINE Vector GetScaledAxis(Axis axis) const;
  FUN_ALWAYS_INLINE Vector GetUnitAxis(Axis axis) const;
  FUN_ALWAYS_INLINE void Mirror(Axis mirror_axis, Axis flip_axis);
  static FUN_ALWAYS_INLINE Vector GetSafeScaleReciprocal(const Vector& scale);

  // tmp function for easy conversion
  FUN_ALWAYS_INLINE Vector GetLocation() const {
    return translation_;
  }

  FUN_ALWAYS_INLINE Rotator GetRotation() const {
    return rotation_.GetRotation();
  }

  /** Calculate the  */
  FUN_ALWAYS_INLINE float GetDeterminant() const {
    return scale_.x * scale_.y * scale_.z;
  }

  /** Set the translation_ of this transformation */
  FUN_ALWAYS_INLINE void SetLocation(const Vector& origin) {
    translation_ = origin;
    DiagnosticCheckNaN_Translate();
  }

  /**
   * Checks the components for NaN's
   *
   * \return Returns true if any component (rotation, translation_, or scale) is a NAN
   */
  bool ContainsNaN() const {
    return (translation_.ContainsNaN() || rotation_.ContainsNaN() || scale_.ContainsNaN());
  }

  FUN_ALWAYS_INLINE bool IsValid() const {
    if (ContainsNaN()) {
      return false;
    }

    if (!rotation_.IsNormalized()) {
      return false;
    }

    return true;
  }

  // Serializer.
  FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar, Transform& m) {
    ar & m.rotation_;
    ar & m.translation_;
    ar & m.scale_;
    return ar;
  }

  //@maxidea: why disabled??
  // Binary comparison operators.
  //bool operator==(const Transform& other) const
  //{
  //  return rotation_==other.rotation_ && translation_==other.translation_ && scale_==other.scale_;
  //}
  //bool operator!=(const Transform& other) const
  //{
  //  return rotation_!=other.rotation_ || translation_!=other.translation_ || scale_!=other.scale_;
  //}

 private:
  FUN_ALWAYS_INLINE bool Private_RotationEquals(const Quat& rotation, const float tolerance = KINDA_SMALL_NUMBER) const {
    return rotation_.Equals(rotation, tolerance);
  }

  FUN_ALWAYS_INLINE bool Private_TranslationEquals(const Vector& translation, const float tolerance = KINDA_SMALL_NUMBER) const {
    return translation_.Equals(translation, tolerance);
  }

  FUN_ALWAYS_INLINE bool Private_Scale3DEquals(const Vector& scale, const float tolerance = KINDA_SMALL_NUMBER) const {
    return scale_.Equals(scale, tolerance);
  }

 public:
  // Test if a's rotation equals b's rotation, within a tolerance. Preferred over "a.GetRotation().Equals(b.GetRotation())" because it is faster on some platforms.
  FUN_ALWAYS_INLINE static bool AreRotationsEqual(const Transform& a, const Transform& b, float tolerance = KINDA_SMALL_NUMBER) {
    return a.Private_RotationEquals(b.rotation_, tolerance);
  }

  // Test if a's translation_ equals b's translation_, within a tolerance. Preferred over "a.GetTranslation().Equals(b.GetTranslation())" because it is faster on some platforms.
  FUN_ALWAYS_INLINE static bool AreTranslationsEqual(const Transform& a, const Transform& b, float tolerance = KINDA_SMALL_NUMBER) {
    return a.Private_TranslationEquals(b.translation_, tolerance);
  }

  // Test if a's scale equals b's scale, within a tolerance. Preferred over "a.GetScale3D().Equals(b.GetScale3D())" because it is faster on some platforms.
  FUN_ALWAYS_INLINE static bool AreScale3DsEqual(const Transform& a, const Transform& b, float tolerance = KINDA_SMALL_NUMBER) {
    return a.Private_Scale3DEquals(b.scale_, tolerance);
  }

  // Test if this transform's rotation equals another's rotation, within a tolerance. Preferred over "GetRotation().Equals(other.GetRotation())" because it is faster on some platforms.
  FUN_ALWAYS_INLINE bool RotationEquals(const Transform& other, float tolerance = KINDA_SMALL_NUMBER) const {
    return AreRotationsEqual(*this, other, tolerance);
  }

  // Test if this transform's translation_ equals another's translation_, within a tolerance. Preferred over "GetTranslation().Equals(other.GetTranslation())" because it is faster on some platforms.
  FUN_ALWAYS_INLINE bool TranslationEquals(const Transform& other, float tolerance = KINDA_SMALL_NUMBER) const {
    return AreTranslationsEqual(*this, other, tolerance);
  }

  // Test if this transform's scale equals another's scale, within a tolerance. Preferred over "GetScale3D().Equals(other.GetScale3D())" because it is faster on some platforms.
  FUN_ALWAYS_INLINE bool Scale3DEquals(const Transform& other, float tolerance = KINDA_SMALL_NUMBER) const {
    return AreScale3DsEqual(*this, other, tolerance);
  }

  // Test if all components of the transforms are equal, within a tolerance.
  FUN_ALWAYS_INLINE bool Equals(const Transform& other, float tolerance = KINDA_SMALL_NUMBER) const {
    return Private_TranslationEquals(other.translation_, tolerance) && Private_RotationEquals(other.rotation_, tolerance) && Private_Scale3DEquals(other.scale_, tolerance);
  }

  // Test if rotation and translation_ components of the transforms are equal, within a tolerance.
  FUN_ALWAYS_INLINE bool EqualsNoScale(const Transform& other, float tolerance = KINDA_SMALL_NUMBER) const {
    return Private_TranslationEquals(other.translation_, tolerance) && Private_RotationEquals(other.rotation_, tolerance);
  }

  /**
   * Create a new transform: out_transform = a * b.
   *
   * Order matters when composing transforms : a * b will yield a transform that logically first applies a then b to any subsequent transformation.
   *
   * \param result - pointer to transform that will store the result of a * b.
   * \param a - transform a.
   * \param b - transform b.
   */
  FUN_ALWAYS_INLINE static void Multiply(Transform* result, const Transform* a, const Transform* b);

  /**
   * Sets the components
   *
   * \param rotation - The new value for the rotation_ component
   * \param translation - The new value for the translation_ component
   * \param scale - The new value for the scale_ component
   */
  FUN_ALWAYS_INLINE void SetComponents(const Quat& rotation, const Vector& translation, const Vector& scale) {
    rotation_ = rotation;
    translation_ = translation;
    scale_ = scale;

    DiagnosticCheckNaN_All();
  }

  /**
   * Sets the components to the identity transform:
   *   rotation = (0, 0, 0, 1)
   *   translation = (0, 0, 0)
   *   scale = (1, 1, 1)
   */
  FUN_ALWAYS_INLINE void SetIdentity() {
    rotation_ = Quat::Identity;
    translation_ = Vector::ZeroVector;
    scale_ = Vector(1, 1, 1);
  }

  /**
   * Scales the scale_ component by a new factor
   *
   * \param scale - The value to multiply scale_ with
   */
  FUN_ALWAYS_INLINE void MultiplyScale(const Vector& scale) {
    scale_ *= scale;
    DiagnosticCheckNaN_Scale3D();
  }

  /**
   * Sets the translation_ component
   *
   * \param translation - The new value for the translation_ component
   */
  FUN_ALWAYS_INLINE void SetTranslation(const Vector& translation) {
    translation_ = translation;
    DiagnosticCheckNaN_Translate();
  }

  /** Copy translation_ from another Transform. */
  FUN_ALWAYS_INLINE void CopyTranslation(const Transform& other) {
    translation_ = other.translation_;
  }

  /**
   * Concatenates another rotation to this transformation
   *
   * \param delta_rotation - The rotation to concatenate in the following fashion: rotation_ = rotation_ * delta_rotation
   */
  FUN_ALWAYS_INLINE void ConcatenateRotation(const Quat& delta_rotation) {
    rotation_ = rotation_ * delta_rotation;
    DiagnosticCheckNaN_Rotate();
  }

  /**
   * Adjusts the translation_ component of this transformation
   *
   * \param delta_translation - The translation_ to add in the following fashion: translation_ += delta_translation
   */
  FUN_ALWAYS_INLINE void AddToTranslation(const Vector& delta_translation) {
    translation_ += delta_translation;
    DiagnosticCheckNaN_Translate();
  }

  /**
   * Add the translations from two Transforms and return the result.
   *
   * \return a.translation_ + b.translation_
   */
  FUN_ALWAYS_INLINE static Vector AddTranslations(const Transform& a, const Transform& b) {
    return a.translation_ + b.translation_;
  }

  /**
   * Subtract translations from two Transforms and return the difference.
   *
   * \return a.translation_ - b.translation_.
   */
  FUN_ALWAYS_INLINE static Vector SubtractTranslations(const Transform& a, const Transform& b) {
    return a.translation_ - b.translation_;
  }

  /**
   * Sets the rotation component
   *
   * \param rotation - The new value for the rotation component
   */
  FUN_ALWAYS_INLINE void SetRotation(const Quat& rotation) {
    rotation_ = rotation;
    DiagnosticCheckNaN_Rotate();
  }

  /** Copy rotation from another Transform. */
  FUN_ALWAYS_INLINE void CopyRotation(const Transform& other) {
    rotation_ = other.rotation_;
  }

  /**
   * Sets the scale_ component
   *
   * \param NewScale3D - The new value for the scale_ component
   */
  FUN_ALWAYS_INLINE void SetScale3D(const Vector& scale) {
    scale_ = scale;
    DiagnosticCheckNaN_Scale3D();
  }

  /** Copy scale from another Transform. */
  FUN_ALWAYS_INLINE void CopyScale3D(const Transform& other) {
    scale_ = other.scale_;
  }

  /**
   * Sets both the translation_ and scale_ components at the same time
   *
   * \param translation - The new value for the translation_ component
   * \param scale - The new value for the scale_ component
   */
  FUN_ALWAYS_INLINE void SetTranslationAndScale3D(const Vector& translation, const Vector& scale) {
    translation_ = translation;
    scale_ = scale;

    DiagnosticCheckNaN_Translate();
    DiagnosticCheckNaN_Scale3D();
  }

  /**
   * Accumulates another transform with this one, with an optional blending weight
   *
   * rotation_ is accumulated additively, in the shortest direction (rotation_ = rotation_ +/- delta_atom.rotation_ * weight)
   * translation_ is accumulated additively (translation_ += delta_atom.translation_ * weight)
   * scale_ is accumulated additively (scale_ += delta_atom.scale_ * weight)
   *
   * \param delta_atom - The other transform to accumulate into this one
   * \param weight - The weight to multiply delta_atom by before it is accumulated.
   */
  FUN_ALWAYS_INLINE void AccumulateWithShortestRotation(const Transform& delta_atom, float weight = 1.f) {
    Transform atom(delta_atom * weight);

    // to ensure the 'shortest route', we make sure the dot product between the accumulator and the incoming child atom is positive.
    if ((atom.rotation_ | rotation_) < 0.f) {
      rotation_.x -= atom.rotation_.x;
      rotation_.y -= atom.rotation_.y;
      rotation_.z -= atom.rotation_.z;
      rotation_.w -= atom.rotation_.w;
    } else {
      rotation_.x += atom.rotation_.x;
      rotation_.y += atom.rotation_.y;
      rotation_.z += atom.rotation_.z;
      rotation_.w += atom.rotation_.w;
    }

    translation_ += atom.translation_;
    scale_ += atom.scale_;

    DiagnosticCheckNaN_All();
  }

  /**
   * Accumulates another transform with this one
   *
   * rotation_ is accumulated multiplicatively (rotation_ = source_atom.rotation_ * rotation_)
   * translation_ is accumulated additively (translation_ += source_atom.translation_)
   * scale_ is accumulated multiplicatively (scale_ *= source_atom.scale_)
   *
   * \param source_atom - The other transform to accumulate into this one
   */
  FUN_ALWAYS_INLINE void Accumulate(const Transform& source_atom) {
    // Add ref pose relative animation to base animation, only if rotation is significant.
    if (Math::Square(source_atom.rotation_.w) < 1.f - DELTA * DELTA) {
      rotation_ = source_atom.rotation_ * rotation_;
    }

    translation_ += source_atom.translation_;
    scale_ *= source_atom.scale_;

    DiagnosticCheckNaN_All();

    fun_check_dbg(IsRotationNormalized());
  }

  /**
   * Accumulates another transform with this one, with a blending weight
   *
   * Let source_atom = atom * blend_weight
   * rotation_ is accumulated multiplicatively (rotation_ = source_atom.rotation_ * rotation_).
   * translation_ is accumulated additively (translation_ += source_atom.translation_)
   * scale_ is accumulated multiplicatively (scale_ *= source_atom.scale_)
   *
   * Note: rotation_ will not be normalized! Will have to be done manually.
   *
   * \param atom - The other transform to accumulate into this one
   * \param blend_weight - The weight to multiply atom by before it is accumulated.
   */
  FUN_ALWAYS_INLINE void Accumulate(const Transform& atom, float blend_weight) {
    Transform source_atom(atom * blend_weight);

    // Add ref pose relative animation to base animation, only if rotation is significant.
    if (Math::Square(source_atom.rotation_.w) < 1.f - DELTA * DELTA) {
      rotation_ = source_atom.rotation_ * rotation_;
    }

    translation_ += source_atom.translation_;
    scale_ *= source_atom.scale_;

    DiagnosticCheckNaN_All();
  }

  /**
   * Set the translation_ and scale_ components of this transform to a linearly interpolated combination of two other transforms
   *
   * translation_ = Math::Lerp(source_atom1.translation_, source_atom2.translation_, alpha)
   * scale_ = Math::Lerp(source_atom1.scale_, source_atom2.scale_, alpha)
   *
   * \param source_atom1 - The starting point source atom (used 100% if alpha is 0)
   * \param source_atom2 - The ending point source atom (used 100% if alpha is 1)
   * \param alpha - The blending weight between source_atom1 and source_atom2
   */
  FUN_ALWAYS_INLINE void LerpTranslationScale3D(const Transform& source_atom1, const Transform& source_atom2, ScalarRegister alpha) {
    translation_ = Math::Lerp(source_atom1.translation_, source_atom2.translation_, alpha);
    scale_ = Math::Lerp(source_atom1.scale_, source_atom2.scale_, alpha);

    DiagnosticCheckNaN_Translate();
    DiagnosticCheckNaN_Scale3D();
  }

  /**
   * Accumulates another transform with this one
   *
   * rotation_ is accumulated multiplicatively (rotation_ = source_atom.rotation_ * rotation_)
   * translation_ is accumulated additively (translation_ += source_atom.translation_)
   * scale_ is accumulated additively (scale_ += source_atom.scale_)
   *
   * \param source_atom - The other transform to accumulate into this one
   */
  FUN_ALWAYS_INLINE void AccumulateWithAdditiveScale3D(const Transform& source_atom) {
    if (Math::Square(source_atom.rotation_.w) < 1.f - DELTA * DELTA) {
      rotation_ = source_atom.rotation_ * rotation_;
    }

    translation_ += source_atom.translation_;
    scale_ += source_atom.scale_;

    DiagnosticCheckNaN_All();
  }

  /**
   * Normalize the rotation component of this transformation
   */
  FUN_ALWAYS_INLINE void NormalizeRotation() {
    rotation_.Normalize();
    DiagnosticCheckNaN_Rotate();
  }

  /**
   * Checks whether the rotation component is normalized or not
   *
   * \return true if the rotation component is normalized, and false otherwise.
   */
  FUN_ALWAYS_INLINE bool IsRotationNormalized() const {
    return rotation_.IsNormalized();
  }

  /**
   * Blends the Identity transform with a weighted source transform and accumulates that into a destination transform
   *
   * source_atom = Blend(Identity, source_atom, blend_weight)
   * final_atom.rotation_ = source_atom.rotation_ * final_atom.rotation_
   * final_atom.translation_ += source_atom.translation_
   * final_atom.scale_ *= source_atom.scale_
   *
   * \param final_atom - [in/out] The atom to accumulate the blended source atom into
   * \param source_atom - The target transformation (used when blend_weight = 1); this is modified during the process
   * \param blend_weight - The blend weight between Identity and source_atom
   */
  FUN_ALWAYS_INLINE static void BlendFromIdentityAndAccumulate(Transform& final_atom, Transform& source_atom, float blend_weight) {
    const Transform IdentityAtom = Transform::Identity;

    // Scale delta by weight
    if (blend_weight < (1.f - ZERO_ANIMWEIGHT_THRESH)) {
      source_atom.Blend(IdentityAtom, source_atom, blend_weight);
    }

    // Add ref pose relative animation to base animation, only if rotation is significant.
    if (Math::Square(source_atom.rotation_.w) < 1.f - DELTA * DELTA) {
      final_atom.rotation_ = source_atom.rotation_ * final_atom.rotation_;
    }

    final_atom.translation_ += source_atom.translation_;
    final_atom.scale_ *= source_atom.scale_;

    final_atom.DiagnosticCheckNaN_All();

    fun_check_dbg(final_atom.IsRotationNormalized());
  }

  /**
   * Returns the rotation component
   *
   * \return The rotation component
   */
  FUN_ALWAYS_INLINE Quat GetRotation() const {
    DiagnosticCheckNaN_Rotate();
    return rotation_;
  }

  /**
   * Returns the translation_ component
   *
   * \return The translation_ component
   */
  FUN_ALWAYS_INLINE Vector GetTranslation() const {
    DiagnosticCheckNaN_Translate();
    return translation_;
  }

  /**
   * Returns the scale_ component
   *
   * \return The scale_ component
   */
  FUN_ALWAYS_INLINE Vector GetScale() const {
    DiagnosticCheckNaN_Scale3D();
    return scale_;
  }

  /**
   * Sets the rotation_ and scale_ of this transformation from another transform
   *
   * \param src_ba - The transform to copy rotation and scale_ from
   */
  FUN_ALWAYS_INLINE void CopyRotationPart(const Transform& src_ba) {
    rotation_ = src_ba.rotation_;
    scale_ = src_ba.scale_;

    DiagnosticCheckNaN_Rotate();
    DiagnosticCheckNaN_Scale3D();
  }

  /**
   * Sets the translation_ and scale_ of this transformation from another transform
   *
   * \param src_ba - The transform to copy translation_ and scale_ from
   */
  FUN_ALWAYS_INLINE void CopyTranslationAndScale3D(const Transform& src_ba) {
    translation_ = src_ba.translation_;
    scale_ = src_ba.scale_;

    DiagnosticCheckNaN_Translate();
    DiagnosticCheckNaN_Scale3D();
  }

  void SetFromMatrix(const Matrix& matrix) {
    Matrix m = matrix;

    // Get the 3D scale from the matrix
    scale_ = m.ExtractScaling();

    // If there is negative scaling going on, we handle that here
    if (matrix.Determinant() < 0.f) {
      // Assume it is along x and modify transform accordingly.
      // It doesn't actually matter which axis we choose, the 'appearance' will be the same
      scale_.x *= -1.f;
      m.SetAxis(0, -m.GetScaledAxis(Axis::X));
    }

    rotation_ = Quat(m);
    translation_ = matrix.GetOrigin();

    // Normalize rotation
    rotation_.Normalize();
  }
};

template <> struct IsPOD<Transform> { enum { Value = true }; };


//
// inlines
//

FUN_ALWAYS_INLINE void Transform::ScaleTranslation(const Vector& scale) {
  translation_ *= scale;

  DiagnosticCheckNaN_Translate();
}

FUN_ALWAYS_INLINE void Transform::ScaleTranslation(const float& scale) {
  translation_ *= scale;

  DiagnosticCheckNaN_Translate();
}

FUN_ALWAYS_INLINE void Transform::RemoveScaling(float tolerance/* = SMALL_NUMBER*/) {
  scale_ = Vector(1, 1, 1);
  rotation_.Normalize();

  DiagnosticCheckNaN_Rotate();
  DiagnosticCheckNaN_Scale3D();
}

FUN_ALWAYS_INLINE void Transform::Multiply(Transform* out_transform, const Transform* a, const Transform* b) {
  a->DiagnosticCheckNaN_All();
  b->DiagnosticCheckNaN_All();

  fun_check_dbg(a->IsRotationNormalized());
  fun_check_dbg(b->IsRotationNormalized());

  //  When q = quaternion, S = single scalar scale, and T = translation_
  //  QST(a) = q(a), S(a), T(a), and QST(b) = q(b), S(b), T(b)

  //  QST (AxB)

  // QST(a) = q(a)*S(a)*P*-q(a) + T(a)
  // QST(AxB) = q(b)*S(b)*QST(a)*-q(b) + T(b)
  // QST(AxB) = q(b)*S(b)*[q(a)*S(a)*P*-q(a) + T(a)]*-q(b) + T(b)
  // QST(AxB) = q(b)*S(b)*q(a)*S(a)*P*-q(a)*-q(b) + q(b)*S(b)*T(a)*-q(b) + T(b)
  // QST(AxB) = [q(b)*q(a)]*[S(b)*S(a)]*P*-[q(b)*q(a)] + q(b)*S(b)*T(a)*-q(b) + T(b)

  //  q(AxB) = q(b)*q(a)
  //  S(AxB) = S(a)*S(b)
  //  T(AxB) = q(b)*S(b)*T(a)*-q(b) + T(b)

  Transform ret;
  ret.rotation_ = b->rotation_*a->rotation_;

  ret.scale_ = a->scale_*b->scale_;
  ret.translation_ = b->rotation_*(b->scale_*a->translation_) + b->translation_;
  // we do not support matrix transform when non-uniform
  // that was removed at rev 21 with FUN

  *out_transform = ret;
  ret.DiagnosticCheckNaN_All();
}

FUN_ALWAYS_INLINE Transform Transform::GetScaled(float scale) const {
  Transform ret(*this);
  ret.scale_ *= scale;
  ret.DiagnosticCheckNaN_Scale3D();
  return ret;
}

FUN_ALWAYS_INLINE Transform Transform::GetScaled(Vector scale) const {
  Transform ret(*this);
  ret.scale_ *= InScale;
  ret.DiagnosticCheckNaN_Scale3D();
  return a;
}

FUN_ALWAYS_INLINE Vector4 Transform::TransformVector4NoScale(const Vector4& v) const {
  DiagnosticCheckNaN_All();

  // if not, this won't work
  fun_check_dbg(v.w == 0.f || v.w == 1.f);

  //transform using QST is following
  //QST(P) = q*S*P*-q + T where q = quaternion, S = scale, T = translation_
  Vector4 transform = Vector4(rotation_.RotateVector(Vector(v)), 0.f);
  if (v.w == 1.f) {
    transform += Vector4(translation_, 1.f);
  }

  return transform;
}

FUN_ALWAYS_INLINE Vector4 Transform::TransformVector4(const Vector4& v) const {
  DiagnosticCheckNaN_All();

  // if not, this won't work
  fun_check_dbg(v.w == 0.f || v.w == 1.f);

  //transform using QST is following
  //QST(P) = q*S*P*-q + T where q = quaternion, S = scale, T = translation_

  Vector4 transform = Vector4(rotation_.RotateVector(scale_*Vector(v)), 0.f);
  if (v.w == 1.f) {
    transform += Vector4(translation_, 1.f);
  }

  return transform;
}

FUN_ALWAYS_INLINE Vector Transform::TransformPosition(const Vector& v) const {
  DiagnosticCheckNaN_All();
  return rotation_.RotateVector(scale_*v) + translation_;
}

FUN_ALWAYS_INLINE Vector Transform::TransformPositionNoScale(const Vector& v) const {
  DiagnosticCheckNaN_All();
  return rotation_.RotateVector(v) + translation_;
}

FUN_ALWAYS_INLINE Vector Transform::TransformVector(const Vector& v) const {
  DiagnosticCheckNaN_All();
  return rotation_.RotateVector(scale_*v);
}

FUN_ALWAYS_INLINE Vector Transform::TransformVectorNoScale(const Vector& v) const {
  DiagnosticCheckNaN_All();
  return rotation_.RotateVector(v);
}

FUN_ALWAYS_INLINE Vector Transform::InverseTransformPosition(const Vector& v) const {
  DiagnosticCheckNaN_All();
  return (rotation_.UnrotateVector(v - translation_)) * GetSafeScaleReciprocal(scale_);
}

FUN_ALWAYS_INLINE Vector Transform::InverseTransformPositionNoScale(const Vector& v) const {
  DiagnosticCheckNaN_All();
  return (rotation_.UnrotateVector(v - translation_));
}

FUN_ALWAYS_INLINE Vector Transform::InverseTransformVector(const Vector& v) const {
  DiagnosticCheckNaN_All();
  return (rotation_.UnrotateVector(v)) * GetSafeScaleReciprocal(scale_);
}

FUN_ALWAYS_INLINE Vector Transform::InverseTransformVectorNoScale(const Vector& v) const {
  DiagnosticCheckNaN_All();
  return rotation_.UnrotateVector(v);
}

FUN_ALWAYS_INLINE Transform Transform::operator * (const Transform& other) const {
  Transform ret;
  Multiply(&ret, this, &other);
  return ret;
}

FUN_ALWAYS_INLINE void Transform::operator *= (const Transform& other) {
  Multiply(this, this, &other);
}

FUN_ALWAYS_INLINE Transform Transform::operator * (const Quat& other) const {
  Transform ret, other_transform(other, Vector::ZeroVector, Vector(1.f));
  Multiply(&ret, this, &other_transform);
  return ret;
}

FUN_ALWAYS_INLINE void Transform::operator *= (const Quat& other) {
  Transform other_transform(other, Vector::ZeroVector, Vector(1.f));
  Multiply(this, this, &other_transform);
}

FUN_ALWAYS_INLINE Vector Transform::GetScaledAxis(Axis axis) const {
  if (axis == Axis::X) {
    return TransformVector(Vector(1.f, 0.f, 0.f));
  } else if (axis == Axis::Y) {
    return TransformVector(Vector(0.f, 1.f, 0.f));
  }

  return TransformVector(Vector(0.f, 0.f, 1.f));
}

FUN_ALWAYS_INLINE Vector Transform::GetUnitAxis(Axis axis) const {
  if (axis == Axis::X) {
    return TransformVectorNoScale(Vector(1.f, 0.f, 0.f));
  } else if (axis == Axis::Y) {
    return TransformVectorNoScale(Vector(0.f, 1.f, 0.f));
  }

  return TransformVectorNoScale(Vector(0.f, 0.f, 1.f));
}

FUN_ALWAYS_INLINE void Transform::Mirror(Axis mirror_axis, Axis flip_axis) {
  // We do convert to matrix for mirroring.
  Matrix m = ToMatrixWithScale();
  m.Mirror(mirror_axis, flip_axis);
  SetFromMatrix(m);
}

FUN_ALWAYS_INLINE float Transform::GetMaximumAxisScale() const {
  DiagnosticCheckNaN_Scale3D();
  return scale_.GetAbsMax();
}

FUN_ALWAYS_INLINE float Transform::GetMinimumAxisScale() const {
  DiagnosticCheckNaN_Scale3D();
  return scale_.GetAbsMin();
}

//TODO tolerance를 적용하는게 좋을듯 싶은데...
FUN_ALWAYS_INLINE Vector Transform::GetSafeScaleReciprocal(const Vector& scale) {
  Vector safe_reciprocal_scale;
  safe_reciprocal_scale.x = (scale.x == 0) ? 0.f : (1.f / scale.x);
  safe_reciprocal_scale.y = (scale.y == 0) ? 0.f : (1.f / scale.y);
  safe_reciprocal_scale.z = (scale.z == 0) ? 0.f : (1.f / scale.z);
  return safe_reciprocal_scale;

  //Vector safe_reciprocal_scale;
  //safe_reciprocal_scale.x = (Math::Abs(scale.x) <= tolerance) ? 0.f : (1.f / scale.x);
  //safe_reciprocal_scale.y = (Math::Abs(scale.y) <= tolerance) ? 0.f : (1.f / scale.y);
  //safe_reciprocal_scale.z = (Math::Abs(scale.z) <= tolerance) ? 0.f : (1.f / scale.z);
  //return safe_reciprocal_scale;
}

} // namespace fun
