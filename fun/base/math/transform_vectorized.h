#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * transform composed of Scale, rotation_ (as a quaternion), and translation.
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
class alignas(16) Transform {
//#if !defined(COREROBJECT_API)
//  #define MAYBE_COREROBJECT_API
//#else
//  #define MAYBE_COREROBJECT_API  COREROBJECT_API
//#endif
//  friend MAYBE_COREROBJECT_API class UScriptStruct* Z_Construct_UScriptStruct_CTransform();

 protected:
  /** rotation_ of this transformation, as a quaternion */
  VectorRegister rotation_;

  /** translation of this transformation, as a vector */
  VectorRegister translation_;

  /** 3D scale (always applied in local space) as a vector */
  VectorRegister scale_;

 public:
  /**
  The identity transformation (rotation_ = Quat::Identity, translation = Vector::ZeroVector, scale_ = (1, 1, 1))
  */
  static FUN_BASE_API const Transform Identity;

#if FUN_ENABLE_NAN_DIAGNOSTIC

  FUN_ALWAYS_INLINE void DiagnosticCheckNaN_Scale3D() const {
    if (VectorContainsNaNOrInfinite(scale_)) {
      LOG_OR_ENSURE_NAN_ERROR("Transform Vectorized scale_ contains NaN");
      const_cast<Transform*>(this)->scale_ = VectorSet_W0(VectorOne());
    }
  }

  FUN_ALWAYS_INLINE void DiagnosticCheckNaN_Translate() const {
    if (VectorContainsNaNOrInfinite(translation_)) {
      LOG_OR_ENSURE_NAN_ERROR("Transform Vectorized translation contains NaN");
      const_cast<Transform*>(this)->translation_ = VectorZero();
    }
  }

  FUN_ALWAYS_INLINE void DiagnosticCheckNaN_Rotate() const {
    if (VectorContainsNaNOrInfinite(rotation_)) {
      LOG_OR_ENSURE_NAN_ERROR("Transform Vectorized rotation_ contains NaN");
      const_cast<Transform*>(this)->rotation_ = VectorSet_W1(VectorZero());
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
      LOG_OR_ENSURE_NAN_ERROR("Transform Vectorized transform is not valid: %s", *ToHumanReadableString());
    }

  }

#else //FUN_ENABLE_NAN_DIAGNOSTIC

  FUN_ALWAYS_INLINE void DiagnosticCheckNaN_Translate() const {}
  FUN_ALWAYS_INLINE void DiagnosticCheckNaN_Rotate() const {}
  FUN_ALWAYS_INLINE void DiagnosticCheckNaN_Scale3D() const {}
  FUN_ALWAYS_INLINE void DiagnosticCheckNaN_All() const {}
  FUN_ALWAYS_INLINE void DiagnosticCheck_IsValid() const {}

#endif //!FUN_ENABLE_NAN_DIAGNOSTIC

  /**
   * Constructor with initialization to the identity transform.
   */
  FUN_ALWAYS_INLINE Transform() {
    // rotation_ = {0, 0, 0, 1)
    rotation_ = VectorSet_W1(VectorZero());
    // translation = {0, 0, 0, 0)
    translation_ = VectorZero();
    // scale_ = {1, 1, 1, 0);
    scale_ = VectorSet_W0(VectorOne());
  }

  /**
   * Constructor with an initial translation
   *
   * \param translation - The value to use for the translation component
   */
  FUN_ALWAYS_INLINE explicit Transform(const Vector& translation) {
    // rotation_ = {0, 0, 0, 1) quaternion identity
    rotation_ =  VectorSet_W1(VectorZero());
    //translation = translation;
    translation_ = MakeVectorRegister(translation.x, translation.y, translation.z, 0.f);
    // scale_ = {1, 1, 1, 0);
    scale_ = VectorSet_W0(VectorOne());

    DiagnosticCheckNaN_All();
  }

  /**
   * Constructor with an initial rotation
   *
   * \param rotation - The value to use for rotation component
   */
  FUN_ALWAYS_INLINE explicit Transform(const Quat& rotation) {
    // rotation_ = rotation
    rotation_ =  VectorLoadAligned(&rotation.x);
    // translation = {0, 0, 0, 0)
    translation_ = VectorZero();
    // scale_ = {1, 1, 1, 0);
    scale_ = VectorSet_W0(VectorOne());

    DiagnosticCheckNaN_All();
  }

  /**
   * Constructor with an initial rotation
   *
   * \param rotation The value to use for rotation component  (after being converted to a quaternion)
   */
  FUN_ALWAYS_INLINE explicit Transform(const Rotator& rotation) {
    Quat quat_rotation = rotation.Quaternion();
    // rotation_ = rotation
    rotation_ =  VectorLoadAligned(&quat_rotation.x);
    // translation = {0, 0, 0, 0)
    translation_ = VectorZero();
    // scale_ = {1, 1, 1, 0);
    scale_ = VectorSet_W0(VectorOne());

    DiagnosticCheckNaN_All();
  }

  /**
   * Constructor with all components initialized
   *
   * \param rotation - The value to use for rotation component
   * \param translation - The value to use for the translation component
   * \param scale - The value to use for the scale component
   */
  FUN_ALWAYS_INLINE Transform(const Quat& rotation, const Vector& translation, const Vector& scale = Vector(1.f)) {
    // rotation_ = rotation
    rotation_ =  VectorLoadAligned(&rotation.x);
    // translation = translation
    translation_ = MakeVectorRegister(translation.x, translation.y, translation.z, 0.f);
    // scale_ = scale
    scale_ = MakeVectorRegister(scale.x, scale.y, scale.z, 0.f);

    DiagnosticCheckNaN_All();
  }

  /**
   * Constructor with all components initialized as VectorRegisters
   *
   * \param rotation - The value to use for rotation component
   * \param translation - The value to use for the translation component
   * \param scale - The value to use for the scale component
   */
  FUN_ALWAYS_INLINE Transform(const VectorRegister& rotation, const VectorRegister& translation, const VectorRegister& scale)
    : rotation_(rotation),
      translation_(translation),
      scale_(scale) {
    DiagnosticCheckNaN_All();
  }

  /**
   * Constructor with all components initialized, taking a Rotator as the rotation component
   *
   * \param rotation - The value to use for rotation component (after being converted to a quaternion)
   * \param translation - The value to use for the translation component
   * \param scale - The value to use for the scale component
   */
  FUN_ALWAYS_INLINE Transform(const Rotator& rotation, const Vector& translation, const Vector& scale = Vector(1.f)) {
    Quat quat_rotation = rotation.Quaternion();
    // rotation_ = rotation
    rotation_ = VectorLoadAligned(&quat_rotation.x);
    // translation = translation
    translation_ = MakeVectorRegister(translation.x, translation.y, translation.z, 0.f);
    // scale_ = scale
    scale_ = MakeVectorRegister(scale.x, scale.y, scale.z, 0.f);

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
    translation_ = MakeVectorRegister(qnan, qnan, qnan, qnan);
    rotation_ = MakeVectorRegister(qnan, qnan, qnan, qnan);
    scale_ = MakeVectorRegister(qnan, qnan, qnan, qnan);
#endif
  }

  /**
   * Copy-constructor
   *
   * \param transform - The source transform from which all components will be copied
   */
  FUN_ALWAYS_INLINE Transform(const Transform& transform)
    : rotation_(transform.rotation_),
      translation_(transform.translation_),
      scale_(transform.scale_) {
    DiagnosticCheckNaN_All();
  }

  /**
   * Constructor for converting a matrix (including scale) into a Transform.
   */
  FUN_ALWAYS_INLINE explicit Transform(const Matrix& matrix) {
    SetFromMatrix(matrix);
    DiagnosticCheckNaN_All();
  }

  /** Constructor that takes basis axes and translation */
  FUN_ALWAYS_INLINE Transform(const Vector& x, const Vector& y, const Vector& z, const Vector& translation) {
    SetFromMatrix(Matrix(x, y, z, translation));
    DiagnosticCheckNaN_All();
  }

  /** Does a debugf of the contents of this transform. */
  FUN_BASE_API void DebugPrint() const;

  /** Debug purpose only **/
  bool DebugEqualMatrix(const Matrix& matrix) const;

  /** Convert Transform contents to a string */
  FUN_BASE_API String ToHumanReadableString() const;

  FUN_BASE_API String ToString() const;

  /** Acceptable form: "%f, %f, %f|%f, %f, %f|%f, %f, %f" */
  FUN_BASE_API bool InitFromString(const String& string);

  /** Copy another transform into this one */
  FUN_ALWAYS_INLINE Transform& operator=(const Transform& other) {
    rotation_ = other.rotation_;
    translation_ = other.translation_;
    scale_ = other.scale_;
    return *this;
  }

  FUN_ALWAYS_INLINE Matrix ToMatrixWithScale() const {
    Matrix out_matrix;
    VectorRegister diagonals_xyz;
    VectorRegister adds;
    VectorRegister subtracts;

    ToMatrixInternal(diagonals_xyz, adds, subtracts);
    const VectorRegister DiagonalsXYZ_W0 = VectorSet_W0(diagonals_xyz);

    // out_matrix.m[0][0] = (1.f - (yy2 + zz2)) * Scale.x;    // Diagonal.x
    // out_matrix.m[0][1] = (xy2 + wz2) * Scale.x;             // adds.x
    // out_matrix.m[0][2] = (xz2 - wy2) * Scale.x;             // subtracts.z
    // out_matrix.m[0][3] = 0.f;                              // DiagonalsXYZ_W0.w
    const VectorRegister AddX_DC_DiagX_DC = VectorShuffle(adds, DiagonalsXYZ_W0, 0, 0, 0, 0);
    const VectorRegister SubZ_DC_DiagW_DC = VectorShuffle(subtracts, DiagonalsXYZ_W0, 2, 0, 3, 0);
    const VectorRegister row0 = VectorShuffle(AddX_DC_DiagX_DC, SubZ_DC_DiagW_DC, 2, 0, 0, 2);

    // out_matrix.m[1][0] = (xy2 - wz2) * Scale.y;             // subtracts.x
    // out_matrix.m[1][1] = (1.f - (xx2 + zz2)) * Scale.y;    // Diagonal.y
    // out_matrix.m[1][2] = (yz2 + wx2) * Scale.y;             // adds.y
    // out_matrix.m[1][3] = 0.f;                            // DiagonalsXYZ_W0.w
    const VectorRegister SubX_DC_DiagY_DC = VectorShuffle(subtracts, DiagonalsXYZ_W0, 0, 0, 1, 0);
    const VectorRegister AddY_DC_DiagW_DC = VectorShuffle(adds, DiagonalsXYZ_W0, 1, 0, 3, 0);
    const VectorRegister row1 = VectorShuffle(SubX_DC_DiagY_DC, AddY_DC_DiagW_DC, 0, 2, 0, 2);

    // out_matrix.m[2][0] = (xz2 + wy2) * Scale.z;             // adds.z
    // out_matrix.m[2][1] = (yz2 - wx2) * Scale.z;             // subtracts.y
    // out_matrix.m[2][2] = (1.f - (xx2 + yy2)) * Scale.z;    // Diagonals.z
    // out_matrix.m[2][3] = 0.f;                              // DiagonalsXYZ_W0.w
    const VectorRegister AddZ_DC_SubY_DC = VectorShuffle(adds, subtracts, 2, 0, 1, 0);
    const VectorRegister row2 = VectorShuffle(AddZ_DC_SubY_DC, DiagonalsXYZ_W0, 0, 2, 2, 3);

    VectorStoreAligned(row0, &(out_matrix.m[0][0]));
    VectorStoreAligned(row1, &(out_matrix.m[1][0]));
    VectorStoreAligned(row2, &(out_matrix.m[2][0]));

    // out_matrix.m[3][0] = translation_.x;
    // out_matrix.m[3][1] = translation_.y;
    // out_matrix.m[3][2] = translation_.z;
    // out_matrix.m[3][3] = 1.f;
    const VectorRegister row3 = VectorSet_W1(translation_);
    VectorStoreAligned(row3, &(out_matrix.m[3][0]));

    return out_matrix;
  }

  /** Convert this transform to matrix with scaling and compute the inverse of that. */
  FUN_ALWAYS_INLINE Matrix ToInverseMatrixWithScale() const {
    // todo: optimize
    return ToMatrixWithScale().Inverse();
  }

  /** Convert this transform to inverse. */
  FUN_ALWAYS_INLINE Transform Inverse() const {
    // Replacement of Inverse of Matrix
    if (VectorAnyGreaterThan(VectorAbs(scale_), VectorizeConstants::SmallNumber)) {
      return InverseFast();
    } else {
      return Transform::Identity;
    }
  }

  /** Convert this transform to a transformation matrix, ignoring its scaling */
  FUN_ALWAYS_INLINE Matrix ToMatrixNoScale() const {
    Matrix out_matrix;
    VectorRegister diagonals_xyz;
    VectorRegister adds;
    VectorRegister subtracts;

    ToMatrixInternalNoScale(diagonals_xyz, adds, subtracts);
    const VectorRegister DiagonalsXYZ_W0 = VectorSet_W0(diagonals_xyz);

    // out_matrix.m[0][0] = (1.f - (yy2 + zz2));            // Diagonal.x
    // out_matrix.m[0][1] = (xy2 + wz2);                     // adds.x
    // out_matrix.m[0][2] = (xz2 - wy2);                     // subtracts.z
    // out_matrix.m[0][3] = 0.f;                            // DiagonalsXYZ_W0.w
    const VectorRegister AddX_DC_DiagX_DC = VectorShuffle(adds, DiagonalsXYZ_W0, 0, 0, 0, 0);
    const VectorRegister SubZ_DC_DiagW_DC = VectorShuffle(subtracts, DiagonalsXYZ_W0, 2, 0, 3, 0);
    const VectorRegister row0 = VectorShuffle(AddX_DC_DiagX_DC, SubZ_DC_DiagW_DC, 2, 0, 0, 2);

    // out_matrix.m[1][0] = (xy2 - wz2);                     // subtracts.x
    // out_matrix.m[1][1] = (1.f - (xx2 + zz2));            // Diagonal.y
    // out_matrix.m[1][2] = (yz2 + wx2);                     // adds.y
    // out_matrix.m[1][3] = 0.f;                            // DiagonalsXYZ_W0.w
    const VectorRegister SubX_DC_DiagY_DC = VectorShuffle(subtracts, DiagonalsXYZ_W0, 0, 0, 1, 0);
    const VectorRegister AddY_DC_DiagW_DC = VectorShuffle(adds, DiagonalsXYZ_W0, 1, 0, 3, 0);
    const VectorRegister row1 = VectorShuffle(SubX_DC_DiagY_DC, AddY_DC_DiagW_DC, 0, 2, 0, 2);

    // out_matrix.m[2][0] = (xz2 + wy2);                     // adds.z
    // out_matrix.m[2][1] = (yz2 - wx2);                     // subtracts.y
    // out_matrix.m[2][2] = (1.f - (xx2 + yy2));            // Diagonals.z
    // out_matrix.m[2][3] = 0.f;                            // DiagonalsXYZ_W0.w
    const VectorRegister AddZ_DC_SubY_DC = VectorShuffle(adds, subtracts, 2, 0, 1, 0);
    const VectorRegister row2 = VectorShuffle(AddZ_DC_SubY_DC, DiagonalsXYZ_W0, 0, 2, 2, 3);

    VectorStoreAligned(row0, &(out_matrix.m[0][0]));
    VectorStoreAligned(row1, &(out_matrix.m[1][0]));
    VectorStoreAligned(row2, &(out_matrix.m[2][0]));

    // out_matrix.m[3][0] = translation_.x;
    // out_matrix.m[3][1] = translation_.y;
    // out_matrix.m[3][2] = translation_.z;
    // out_matrix.m[3][3] = 1.f;
    const VectorRegister row3 = VectorSet_W1(translation_);
    VectorStoreAligned(row3, &(out_matrix.m[3][0]));

    return out_matrix;
  }

  /** Set this transform to the weighted blend of the supplied two transforms. */
  FUN_ALWAYS_INLINE void Blend(const Transform& atom1, const Transform& atom2, float alpha) {
#if !(FUN_BUILD_SHIPPING || FUN_BUILD_TEST) && WITH_EDITORONLY_DATA
    // Check that all bone atoms coming from animation are normalized
    fun_check(atom1.IsRotationNormalized());
    fun_check(atom2.IsRotationNormalized());
#endif

    if (Math::Abs(alpha) <= ZERO_ANIMWEIGHT_THRESH) {
      // if blend is all the way for child1, then just copy its bone atoms
      (*this) = atom1;
    } else if (Math::Abs(alpha - 1.f) <= ZERO_ANIMWEIGHT_THRESH) {
      // if blend is all the way for child2, then just copy its bone atoms
      (*this) = atom2;
    } else {
      // Simple linear interpolation for translation and scale.
      ScalarRegister blend_weight = ScalarRegister(alpha);

      translation_ = Math::Lerp(atom1.translation_, atom2.translation_, blend_weight.Value);
      scale_ = Math::Lerp(atom1.scale_, atom2.scale_, blend_weight.Value);

      VectorRegister rotation_vec = VectorLerpQuat(atom1.rotation_, atom2.rotation_, blend_weight.Value);

      // ..and renormalize
      rotation_ = VectorNormalizeQuaternion(rotation_vec);

      DiagnosticCheckNaN_All(); // MR
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
        // Simple linear interpolation for translation and scale.
        ScalarRegister blend_weight = ScalarRegister(alpha);
        translation_ = Math::Lerp(translation_, other_atom.translation_, blend_weight.Value);

        scale_ = Math::Lerp(scale_, other_atom.scale_, blend_weight.Value);

        VectorRegister rotation_vec = VectorLerpQuat(rotation_, other_atom.rotation_, blend_weight.Value);

        // ..and renormalize
        rotation_ = VectorNormalizeQuaternion(rotation_vec);

        DiagnosticCheckNaN_All();
      }
    }
  }

  /**
   * Quaternion addition is wrong here. This is just a special case for linear interpolation.
   * Use only within blends!!
   * rotation_ part is NOT normalized!!
   */
  FUN_ALWAYS_INLINE Transform operator+(const Transform& atom) const {
    return Transform(VectorAdd(rotation_, atom.rotation_), VectorAdd(translation_, atom.translation_), VectorAdd(scale_, atom.scale_));
  }

  FUN_ALWAYS_INLINE Transform& operator+=(const Transform& atom) {
    translation_ = VectorAdd(translation_, atom.translation_);
    rotation_ = VectorAdd(rotation_, atom.rotation_);
    scale_ = VectorAdd(scale_, atom.scale_);

    return *this;
  }

  FUN_ALWAYS_INLINE Transform operator*(const ScalarRegister& Mult) const {
    return Transform(VectorMultiply(rotation_, Mult), VectorMultiply(translation_, Mult), VectorMultiply(scale_, Mult));
  }

  FUN_ALWAYS_INLINE Transform& operator*=(const ScalarRegister& Mult) {
    translation_= VectorMultiply(translation_, Mult);
    rotation_ = VectorMultiply(rotation_, Mult);
    scale_ = VectorMultiply(scale_, Mult);

    return *this;
  }

  FUN_ALWAYS_INLINE Transform operator*(const Transform& other) const;
  FUN_ALWAYS_INLINE void operator*=(const Transform& other);
  FUN_ALWAYS_INLINE Transform operator*(const Quat& other) const;
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
  Set current transform and the relative to parent_transform.
  Equates to This = This->GetRelativeTransform(Parent), but saves the intermediate Transform storage and copy.
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
   * transform a direction vector by the inverse of this matrix - will not take into account translation part.
   * If you want to transform a surface normal (or plane) and correctly account for non-uniform scaling you should use TransformByUsingAdjointT with adjoint of matrix inverse.
   */
  FUN_ALWAYS_INLINE Vector InverseTransformVector(const Vector& v) const;

  FUN_ALWAYS_INLINE Vector InverseTransformVectorNoScale(const Vector& v) const;

  FUN_ALWAYS_INLINE Transform GetScaled(float scale) const;
  FUN_ALWAYS_INLINE Transform GetScaled(Vector scale) const;
  FUN_ALWAYS_INLINE Vector GetScaledAxis(Axis axis) const;
  FUN_ALWAYS_INLINE Vector GetUnitAxis(Axis axis) const;
  FUN_ALWAYS_INLINE void Mirror(Axis mirror_axis, Axis flip_axis);
  FUN_ALWAYS_INLINE Vector GetSafeScaleReciprocal(const Vector& scale, float tolerance=0.f) const;

  FUN_ALWAYS_INLINE Vector GetLocation() const {
    Vector location;
    VectorStoreFloat3(translation_, &location);
    return location;
  }

  FUN_ALWAYS_INLINE Rotator GetRotator() const {
    Quat rotation;
    VectorStoreAligned(rotation_, &rotation);
    return rotation.ToRotator();
  }

  /** Calculate the */
  FUN_ALWAYS_INLINE float GetDeterminant() const {
    //#todo - vectorized version of this
    Vector4 scale;
    VectorStoreAligned(scale_, &scale);
    return scale.x * scale.y * scale.z;
  }

  /** Set the translation of this transformation */
  FUN_ALWAYS_INLINE void SetLocation(const Vector& origin) {
    translation_ = VectorLoadFloat3_W0(&origin);
    DiagnosticCheckNaN_Translate();
  }

  /**
   * Checks the components for NaN's
   *
   * \return Returns true if any component (rotation, translation, or scale) is a NAN
   */
  bool ContainsNaN() const {
    if (VectorContainsNaNOrInfinite(rotation_)) {
      return true;
    }

    if (VectorContainsNaNOrInfinite(translation_)) {
      return true;
    }

    if (VectorContainsNaNOrInfinite(scale_)) {
      return true;
    }

    return false;
  }

  FUN_ALWAYS_INLINE bool IsValid() const {
    if (ContainsNaN()) {
      return false;
    }

    if (!IsRotationNormalized()) {
      return false;
    }

    return true;
  }

  // Serializer.
  FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar, Transform& m) {
    //@TODO: This is an unpleasant cast
    ar & *reinterpret_cast<Vector4*>(&(m.rotation_));
    ar & *reinterpret_cast<Vector*>(&(m.translation_));
    ar & *reinterpret_cast<Vector*>(&(m.scale_));

    if (ar.IsLoading()) {
      m.translation_ = VectorSet_W0(m.translation_);
      m.scale_ = VectorSet_W0(m.scale_);
    }
    return ar;
  }

  // Binary comparison operators.
  /*
  bool operator==(const Transform& other) const {
    return rotation_==other.rotation_ && translation_==other.translation_ && scale_==other.scale_;
  }
  bool operator!=(const Transform& other) const {
    return rotation_!=other.rotation_ || translation_!=other.translation_ || scale_!=other.scale_;
  }
  */

 private:
  FUN_ALWAYS_INLINE bool Private_RotationEquals(const VectorRegister& rotation, const ScalarRegister& tolerance = ScalarRegister(VectorizeConstants::KindaSmallNumber)) const {
    // !((Math::Abs(x-q.x) > tolerance) || (Math::Abs(y-q.y) > tolerance) || (Math::Abs(z-q.z) > tolerance) || (Math::Abs(w-q.w) > tolerance))
    const VectorRegister RotationSub = VectorAbs(VectorSubtract(rotation_, rotation));
    // !((Math::Abs(x+q.x) > tolerance) || (Math::Abs(y+q.y) > tolerance) || (Math::Abs(z+q.z) > tolerance) || (Math::Abs(w+q.w) > tolerance))
    const VectorRegister RotationAdd = VectorAbs(VectorAdd(rotation_, rotation));
    return !VectorAnyGreaterThan(RotationSub, tolerance.Value) || !VectorAnyGreaterThan(RotationAdd, tolerance.Value);
  }

  FUN_ALWAYS_INLINE bool Private_TranslationEquals(const VectorRegister& translation, const ScalarRegister& tolerance = ScalarRegister(VectorizeConstants::KindaSmallNumber)) const {
    // !((Math::Abs(x-v.x) > tolerance) || (Math::Abs(y-v.y) > tolerance) || (Math::Abs(z-v.z) > tolerance))
    const VectorRegister TranslationDiff = VectorAbs(VectorSubtract(translation_, translation));
    return !VectorAnyGreaterThan(TranslationDiff, tolerance.Value);
  }

  FUN_ALWAYS_INLINE bool Private_Scale3DEquals(const VectorRegister& scale, const ScalarRegister& tolerance = ScalarRegister(VectorizeConstants::KindaSmallNumber)) const {
    // !((Math::Abs(x-v.x) > tolerance) || (Math::Abs(y-v.y) > tolerance) || (Math::Abs(z-v.z) > tolerance))
    const VectorRegister ScaleDiff = VectorAbs(VectorSubtract(scale_, scale));
    return !VectorAnyGreaterThan(ScaleDiff, tolerance.Value);
  }

 public:
  // Test if a's rotation equals b's rotation, within a tolerance. Preferred over "a.GetRotation().Equals(b.GetRotation())" because it is faster on some platforms.
  FUN_ALWAYS_INLINE static bool AreRotationsEqual(const Transform& a, const Transform& b, float tolerance = KINDA_SMALL_NUMBER) {
    return a.Private_RotationEquals(b.rotation_, ScalarRegister(tolerance));
  }

  // Test if a's translation equals b's translation, within a tolerance. Preferred over "a.GetTranslation().Equals(b.GetTranslation())" because it avoids VectorRegister->Vector conversion.
  FUN_ALWAYS_INLINE static bool AreTranslationsEqual(const Transform& a, const Transform& b, float tolerance = KINDA_SMALL_NUMBER) {
    return a.Private_TranslationEquals(b.translation_, ScalarRegister(tolerance));
  }

  // Test if a's scale equals b's scale, within a tolerance. Preferred over "a.GetScale3D().Equals(b.GetScale3D())" because it avoids VectorRegister->Vector conversion.
  FUN_ALWAYS_INLINE static bool AreScale3DsEqual(const Transform& a, const Transform& b, float tolerance = KINDA_SMALL_NUMBER) {
    return a.Private_Scale3DEquals(b.scale_, ScalarRegister(tolerance));
  }


  // Test if this transform's rotation equals another's rotation, within a tolerance. Preferred over "GetRotation().Equals(other.GetRotation())" because it is faster on some platforms.
  FUN_ALWAYS_INLINE bool RotationEquals(const Transform& other, float tolerance = KINDA_SMALL_NUMBER) const {
    return AreRotationsEqual(*this, other, tolerance);
  }

  // Test if this transform's translation equals another's translation, within a tolerance. Preferred over "GetTranslation().Equals(other.GetTranslation())" because it avoids VectorRegister->Vector conversion.
  FUN_ALWAYS_INLINE bool TranslationEquals(const Transform& other, float tolerance = KINDA_SMALL_NUMBER) const {
    return AreTranslationsEqual(*this, other, tolerance);
  }

  // Test if this transform's scale equals another's scale, within a tolerance. Preferred over "GetScale3D().Equals(other.GetScale3D())" because it avoids VectorRegister->Vector conversion.
  FUN_ALWAYS_INLINE bool Scale3DEquals(const Transform& other, float tolerance = KINDA_SMALL_NUMBER) const {
    return AreScale3DsEqual(*this, other, tolerance);
  }


  // Test if all components of the transforms are equal, within a tolerance.
  FUN_ALWAYS_INLINE bool Equals(const Transform& other, float tolerance = KINDA_SMALL_NUMBER) const {
    const ScalarRegister ToleranceRegister(tolerance);
    return Private_TranslationEquals(other.translation_, ToleranceRegister) && Private_RotationEquals(other.rotation_, ToleranceRegister) && Private_Scale3DEquals(other.scale_, ToleranceRegister);
  }

  // Test if rotation and translation components of the transforms are equal, within a tolerance.
  FUN_ALWAYS_INLINE bool EqualsNoScale(const Transform& other, float tolerance = KINDA_SMALL_NUMBER) const {
    const ScalarRegister ToleranceRegister(tolerance);
    return Private_TranslationEquals(other.translation_, ToleranceRegister) && Private_RotationEquals(other.rotation_, ToleranceRegister);
  }

  FUN_ALWAYS_INLINE static void Multiply(Transform* out_transform, const Transform* a, const Transform* b);

  /**
   * Sets the components
   *
   * \param rotation - The new value for the rotation_ component
   * \param translation - The new value for the translation component
   * \param scale - The new value for the scale_ component
   */
  FUN_ALWAYS_INLINE void SetComponents(const Quat& rotation, const Vector& translation, const Vector& scale) {
    rotation_ = VectorLoadAligned(&rotation);
    translation_ = VectorLoadFloat3_W0(&translation);
    scale_ = VectorLoadFloat3_W0(&scale);

    DiagnosticCheckNaN_All();
  }

  /**
   * Sets the components to the identity transform:
   *   rotation = (0, 0, 0, 1)
   *   translation = (0, 0, 0)
   *   scale = (1, 1, 1)
   */
  FUN_ALWAYS_INLINE void SetIdentity() {
    // rotation_ = {0, 0, 0, 1)
    rotation_ = VectorSet_W1(VectorZero());
    // translation_ = {0, 0, 0, 0)
    translation_ = VectorZero();
    // scale_ = {1, 1, 1, 0);
    scale_ = VectorSet_W0 (VectorOne());
  }

  /**
   * Scales the scale_ component by a new factor
   *
   * \param scale - The value to multiply scale_ with
   */
  FUN_ALWAYS_INLINE void MultiplyScale(const Vector& scale) {
    scale_ = VectorMultiply(scale_, VectorLoadFloat3_W0(&scale));
    DiagnosticCheckNaN_Scale3D();
  }

  /**
   * Sets the translation component
   *
   * \param NewTranslation - The new value for the translation component
   */
  FUN_ALWAYS_INLINE void SetTranslation(const Vector& translation) {
    translation_ = VectorLoadFloat3_W0(&translation);
    DiagnosticCheckNaN_Translate();
  }

  /** Copy translation from another Transform. */
  FUN_ALWAYS_INLINE void CopyTranslation(const Transform& other) {
    translation_ = other.translation_;
  }

  /**
   * Concatenates another rotation to this transformation
   *
   * \param delta_rotation - The rotation to concatenate in the following fashion: rotation_ = rotation_ * delta_rotation
   */
  FUN_ALWAYS_INLINE void ConcatenateRotation(const Quat& delta_rotation) {
    rotation_ = VectorQuaternionMultiply2(rotation_, VectorLoadAligned(&delta_rotation));
    DiagnosticCheckNaN_Rotate();
  }

  /**
   * Adjusts the translation component of this transformation
   *
   * \param delta_translation - The translation to add in the following fashion: translation += delta_translation
   */
  FUN_ALWAYS_INLINE void AddToTranslation(const Vector& delta_translation) {
    translation_ = VectorAdd(translation_, VectorLoadFloat3_W0(&delta_translation));
    DiagnosticCheckNaN_Translate();
  }

  /**
   * Add the translations from two Transforms and return the result.
   *
   * \return a.translation + b.translation
   */
  FUN_ALWAYS_INLINE static Vector AddTranslations(const Transform& a, const Transform& b) {
    Vector result;
    VectorStoreFloat3(VectorAdd(a.translation_, b.translation_), &result);
    return result;
  }

  /**
   * Subtract translations from two Transforms and return the difference.
   *
   * \return a.translation - b.translation.
   */
  FUN_ALWAYS_INLINE static Vector SubtractTranslations(const Transform& a, const Transform& b) {
    Vector result;
    VectorStoreFloat3(VectorSubtract(a.translation_, b.translation_), &result);
    return result;
  }

  /**
   * Sets the rotation component
   *
   * \param rotation - The new value for the rotation component
   */
  FUN_ALWAYS_INLINE void SetRotation(const Quat& rotation) {
    rotation_ = VectorLoadAligned(&rotation);
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
    scale_ = VectorLoadFloat3_W0(&scale);
    DiagnosticCheckNaN_Scale3D();
  }

  /** Copy scale from another Transform. */
  FUN_ALWAYS_INLINE void CopyScale3D(const Transform& other) {
    scale_ = other.scale_;
  }

  /**
   * Sets both the translation and scale_ components at the same time
   *
   * \param translation - The new value for the translation component
   * \param scale - The new value for the scale_ component
   */
  FUN_ALWAYS_INLINE void SetTranslationAndScale3D(const Vector& translation, const Vector& scale) {
    translation_ = VectorLoadFloat3_W0(&NewTranslation);
    scale_ = VectorLoadFloat3_W0(&scale);

    DiagnosticCheckNaN_Translate();
    DiagnosticCheckNaN_Scale3D();
  }

  /**
   * Accumulates another transform with this one, with an optional blending weight
   *
   * rotation_ is accumulated additively, in the shortest direction (rotation_ = rotation_ +/- delta_atom.rotation_ * Weight)
   * translation is accumulated additively (translation += delta_atom.translation * Weight)
   * scale_ is accumulated additively (scale_ += delta_atom.Scale * Weight)
   *
   * \param delta_atom - The other transform to accumulate into this one
   * \param Weight - The weight to multiply delta_atom by before it is accumulated.
   */
  FUN_ALWAYS_INLINE void AccumulateWithShortestRotation(const Transform& delta_atom, const ScalarRegister& blend_weight) {
    const VectorRegister blended_rotation = VectorMultiply(delta_atom.rotation_, blend_weight.Value);

    rotation_ = VectorAccumulateQuaternionShortestPath(rotation_, blended_rotation);

    translation_ = VectorMultiplyAdd(delta_atom.translation_, blend_weight, translation_);
    scale_ = VectorMultiplyAdd(delta_atom.scale_, blend_weight, scale_);

    DiagnosticCheckNaN_All();
  }

  /**
  Accumulates another transform with this one

  rotation_ is accumulated multiplicatively (rotation_ = source_atom.rotation_ * rotation_)
  translation is accumulated additively (translation += source_atom.translation)
  scale_ is accumulated multiplicatively (scale_ *= source_atom.scale_)

  \param source_atom - The other transform to accumulate into this one
  */
  FUN_ALWAYS_INLINE void Accumulate(const Transform& source_atom) {
    const VectorRegister blended_rotation = source_atom.rotation_;
    const VectorRegister rotation_w = VectorReplicate(blended_rotation, 3);

    // if (Square(source_atom.rotation_.w) < 1.f - DELTA * DELTA)
    if (VectorAnyGreaterThan(VectorizeConstants::RotationSignificantThreshold, VectorMultiply(rotation_w, rotation_w))) {
      // rotation_ = source_atom.rotation_ * rotation_;
      rotation_ = VectorQuaternionMultiply2(blended_rotation, rotation_);
    }

    // translation_ += source_atom.translation_;
    // Scale *= source_atom.Scale;
    translation_ = VectorAdd(translation_, source_atom.translation_);
    scale_ = VectorMultiply(scale_, source_atom.scale_);

    DiagnosticCheckNaN_All();

    fun_check_dbg(IsRotationNormalized());
  }

  /**
  Accumulates another transform with this one, with a blending weight

  Let source_atom = atom * blend_weight
  rotation_ is accumulated multiplicatively (rotation_ = atom.rotation_ * rotation_).
  translation is accumulated additively (translation += atom.translation)
  scale_ is accumulated multiplicatively (scale_ *= atom.scale_)

  Note: rotation_ will not be normalized! Will have to be done manually.

  \param atom - The other transform to accumulate into this one
  \param blend_weight - The weight to multiply atom by before it is accumulated.
  */
  FUN_ALWAYS_INLINE void Accumulate(const Transform& atom, const ScalarRegister& blend_weight) {
    // source_atom = atom * blend_weight;
    const VectorRegister blended_rotation = VectorMultiply(atom.rotation_, blend_weight.Value);
    const VectorRegister rotation_w = VectorReplicate(blended_rotation, 3);

    // Add ref pose relative animation to base animation, only if rotation is significant.
    // if (Square(source_atom.rotation_.w) < 1.f - DELTA * DELTA)
    if (VectorAnyGreaterThan(VectorizeConstants::RotationSignificantThreshold, VectorMultiply(rotation_w, rotation_w))) {
      // rotation_ = source_atom.rotation_ * rotation_;
      rotation_ = VectorQuaternionMultiply2(blended_rotation, rotation_);
    }

    // translation += source_atom.translation;
    // Scale *= source_atom.Scale;
    translation_ = VectorAdd(translation_, atom.translation_);
    scale_ = VectorMultiply(scale_, atom.scale_);

    DiagnosticCheckNaN_All();
  }

  /**
  Set the translation and scale_ components of this transform to a linearly interpolated combination of two other transforms

  translation = Math::Lerp(source_atom1.translation, source_atom2.translation, alpha)
  scale_ = Math::Lerp(source_atom1.scale_, source_atom2.scale_, alpha)

  \param source_atom1 - The starting point source atom (used 100% if alpha is 0)
  \param source_atom2 - The ending point source atom (used 100% if alpha is 1)
  \param alpha - The blending weight between source_atom1 and source_atom2
  */
  FUN_ALWAYS_INLINE void LerpTranslationScale3D(const Transform& source_atom1, const Transform& source_atom2, const ScalarRegister& alpha) {
    translation_ = Math::Lerp(source_atom1.translation_, source_atom2.translation_, alpha.Value);
    scale_ = Math::Lerp(source_atom1.scale_, source_atom2.scale_, alpha.Value);

    DiagnosticCheckNaN_Translate();
    DiagnosticCheckNaN_Scale3D();
  }

  /**
  Accumulates another transform with this one

  rotation_ is accumulated multiplicatively (rotation_ = source_atom.rotation_ * rotation_)
  translation is accumulated additively (translation += source_atom.translation)
  Scale is accumulated additively (scale_ += source_atom.scale_)

  \param source_atom - The other transform to accumulate into this one
  */
  FUN_ALWAYS_INLINE void AccumulateWithAdditiveScale3D(const Transform& source_atom) {
    // Add ref pose relative animation to base animation, only if rotation is significant.
    const VectorRegister rotation_w = VectorReplicate(source_atom.rotation_, 3);

    // if (Square(source_atom.rotation_.w) < 1.f - DELTA * DELTA)
    if (VectorAnyGreaterThan(VectorizeConstants::RotationSignificantThreshold, VectorMultiply(rotation_w, rotation_w))) {
      // rotation_ = source_atom.rotation_ * rotation_;
      rotation_ = VectorQuaternionMultiply2(source_atom.rotation_, rotation_);
    }

    translation_ = VectorAdd(translation_, source_atom.translation_);
    scale_ = VectorAdd(scale_, source_atom.scale_);

    DiagnosticCheckNaN_All();
  }

  /**
  Normalize the rotation component of this transformation
  */
  FUN_ALWAYS_INLINE void NormalizeRotation() {
    rotation_ = VectorNormalizeQuaternion(rotation_);
    DiagnosticCheckNaN_Rotate();
  }

  /**
  Checks whether the rotation component is normalized or not

  \return true if the rotation component is normalized, and false otherwise.
  */
  FUN_ALWAYS_INLINE bool IsRotationNormalized() const {
    const VectorRegister test_value = VectorAbs(VectorSubtract(VectorOne(), VectorDot4(rotation_, rotation_)));
    return !VectorAnyGreaterThan(test_value, VectorizeConstants::ThreshQuatNormalized);
  }

  /**
  Blends the Identity transform with a weighted source transform and accumulates that into a destination transform

  source_atom = Blend(Identity, source_atom, blend_weight)
  final_atom.rotation_ = source_atom.rotation_ * final_atom.rotation_
  final_atom.translation += source_atom.translation
  final_atom.scale_ *= source_atom.scale_

  \param final_atom - [in/out] The atom to accumulate the blended source atom into
  \param source_atom - The target transformation (used when blend_weight = 1)
  \param alpha - The blend weight between Identity and source_atom
  */
  FUN_ALWAYS_INLINE static void BlendFromIdentityAndAccumulate(Transform& final_atom, Transform& source_atom, const ScalarRegister& blend_weight) {
    const VectorRegister Const0001 = VectorizeConstants::Float0001;
    const VectorRegister ConstNegative0001 = VectorSubtract(VectorZero(), Const0001);
    const VectorRegister VOneMinusAlpha = VectorSubtract(VectorOne(), blend_weight.Value);

    // Blend rotation
    //     to ensure the 'shortest route', we make sure the dot product between the both rotations is positive.
    //     const float Bias = (|a.b| >= 0 ? 1 : -1)
    //     BlendedAtom.rotation_ = (b * alpha) + (a * (Bias * (1.f - alpha)));
    //     BlendedAtom.rotation_.QuaternionNormalize();
    //  Note: a = (0, 0, 0, 1), which simplifies things a lot; only care about sign of b.w now, instead of doing a dot product
    const VectorRegister RotationB = source_atom.rotation_;

    const VectorRegister QuatRotationDirMask = VectorCompareGE(RotationB, VectorZero());
    const VectorRegister BiasTimesA = VectorSelect(QuatRotationDirMask, Const0001, ConstNegative0001);
    const VectorRegister RotateBTimesWeight = VectorMultiply(RotationB, blend_weight.Value);
    const VectorRegister UnnormalizedRotation = VectorMultiplyAdd(BiasTimesA, VOneMinusAlpha, RotateBTimesWeight);

    // Normalize blended rotation (result = (q.q >= 1e-8) ? (q / |q|) : (0, 0, 0, 1))
    const VectorRegister blended_rotation = VectorNormalizeSafe(UnnormalizedRotation, Const0001);

    // final_atom.rotation_ = BlendedAtom.rotation_ * final_atom.rotation_;
    final_atom.rotation_ = VectorQuaternionMultiply2(blended_rotation, final_atom.rotation_);

    // Blend translation and scale
    //    BlendedAtom.translation = Lerp(Zero, source_atom.translation, alpha);
    //    BlendedAtom.Scale = Lerp(1, source_atom.Scale, alpha);
    const VectorRegister BlendedTranslation = Math::Lerp(VectorZero(), source_atom.translation_, blend_weight.Value);
    const VectorRegister BlendedScale3D = Math::Lerp(VectorOne(), source_atom.scale_, blend_weight.Value);

    // Apply translation and scale to final atom
    //     final_atom.translation += BlendedAtom.translation
    //     final_atom.Scale *= BlendedAtom.Scale
    final_atom.translation_ = VectorAdd(final_atom.translation_, BlendedTranslation);
    final_atom.scale_ = VectorMultiply(final_atom.scale_, BlendedScale3D);

    fun_check_dbg(final_atom.IsRotationNormalized());
  }

  /**
  Returns the rotation component

  \return The rotation component
  */
  FUN_ALWAYS_INLINE Quat GetRotation() const {
    DiagnosticCheckNaN_Rotate();
    Quat rotation;
    VectorStoreAligned(rotation_, &rotation);
    return rotation;
  }

  /**
  Returns the translation component

  \return The translation component
  */
  FUN_ALWAYS_INLINE Vector GetTranslation() const {
    DiagnosticCheckNaN_Translate();
    Vector OutTranslation;
    VectorStoreFloat3(translation_, &OutTranslation);
    return OutTranslation;
  }

  /**
  Returns the scale_ component

  \return The scale_ component
  */
  FUN_ALWAYS_INLINE Vector GetScale3D() const {
    DiagnosticCheckNaN_Scale3D();
    Vector scale;
    VectorStoreFloat3(scale_, &scale);
    return scale;
  }

  //@maxidea
  /**
  Returns an opaque copy of the rotation component
  This method should be used when passing rotation from one Transform to another

  \return The rotation component
  */
  //DEPRECATED(4.5, "Transform::GetRotationV() is deprecated, use Transform::GetRotation() instead.")
  //FUN_ALWAYS_INLINE Quat GetRotationV() const
  //{
  //  DiagnosticCheckNaN_Rotate();
  //  Quat rotation;
  //  VectorStoreAligned(rotation_, &rotation);
  //  return rotation;
  //}
  //
  /**
  // * Returns an opaque copy of the translation component
  // * This method should be used when passing translation from one Transform to another
  // *
  // * \return The translation component
  // */
  //DEPRECATED(4.5, "Transform::GetTranslationV() is deprecated, use Transform::GetTranslation() instead.")
  //FUN_ALWAYS_INLINE Vector GetTranslationV() const
  //{
  //  DiagnosticCheckNaN_Translate();
  //  Vector OutTranslation;
  //  VectorStoreFloat3(translation_, &OutTranslation);
  //  return OutTranslation;
  //}
  //
  ///**
  // * Returns an opaque copy of the scale_ component
  // * This method should be used when passing scale_ from one Transform to another
  // *
  // * \return The scale_ component
  // */
  //DEPRECATED(4.5, "Transform::GetScale3DV() is deprecated, use Transform::GetScale3D() instead.")
  //FUN_ALWAYS_INLINE Vector GetScale3DV() const
  //{
  //  DiagnosticCheckNaN_Scale3D();
  //  Vector OutScale3D;
  //  VectorStoreFloat3(scale_, &OutScale3D);
  //  return OutScale3D;
  //}

  /**
  Sets the rotation_ and scale_ of this transformation from another transform

  \param src_ba - The transform to copy rotation and scale_ from
  */
  FUN_ALWAYS_INLINE void CopyRotationPart(const Transform& src_ba) {
    rotation_ = src_ba.rotation_;
    scale_ = src_ba.scale_;

    DiagnosticCheckNaN_Rotate();
    DiagnosticCheckNaN_Scale3D();
  }

  /**
  Sets the translation and scale_ of this transformation from another transform

  \param src_ba - The transform to copy translation and scale_ from
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
    Vector scale = m.ExtractScaling();
    scale_ = VectorLoadFloat3_W0(&scale);

    // If there is negative scaling going on, we handle that here
    if (matrix.Determinant() < 0.f) {
      // Assume it is along x and modify transform accordingly.
      // It doesn't actually matter which axis we choose, the 'appearance' will be the same
      scale_ = VectorMultiply(scale_, VectorizeConstants::FloatMinus1_111);
      m.SetAxis(0, -m.GetScaledAxis(Axis::X));
    }

    Quat rotation = Quat(m);
    rotation_ = VectorLoadAligned(&rotation);
    Vector translation = matrix.GetOrigin();
    translation_ = VectorLoadFloat3_W0(&translation);

    // Normalize rotation
    rotation_ = VectorNormalizeQuaternion(rotation_);
  }

 private:
  FUN_ALWAYS_INLINE void ToMatrixInternal(VectorRegister& OutDiagonals, VectorRegister& OutAdds, VectorRegister& OutSubtracts) const {
#if !(FUN_BUILD_SHIPPING || FUN_BUILD_TEST) && WITH_EDITORONLY_DATA
    // Make sure rotation_ is normalized when we turn it into a matrix.
    fun_check(IsRotationNormalized());
#endif

    const VectorRegister RotationX2Y2Z2 = VectorAdd(rotation_, rotation_);    // x2, y2, z2
    const VectorRegister RotationXX2YY2ZZ2 = VectorMultiply(RotationX2Y2Z2, rotation_);  // xx2, yy2, zz2

    // The diagonal terms of the rotation matrix are:
    //   (1 - (yy2 + zz2)) * scale
    //   (1 - (xx2 + zz2)) * scale
    //   (1 - (xx2 + yy2)) * scale
    const VectorRegister yy2_xx2_xx2 = VectorSwizzle(RotationXX2YY2ZZ2, 1, 0, 0, 0);
    const VectorRegister zz2_zz2_yy2 = VectorSwizzle(RotationXX2YY2ZZ2, 2, 2, 1, 0);
    const VectorRegister DiagonalSum = VectorAdd(yy2_xx2_xx2, zz2_zz2_yy2);
    const VectorRegister Diagonals = VectorSubtract(VectorOne(), DiagonalSum);
    OutDiagonals = VectorMultiply(Diagonals, scale_);

    // Grouping the non-diagonal elements in the rotation block by operations:
    //    ((x*y2, y*z2, x*z2) + (w*z2, w*x2, w*y2)) * scale.xyz and
    //    ((x*y2, y*z2, x*z2) - (w*z2, w*x2, w*y2)) * scale.yxz
    // Rearranging so the lhs and rhs are in the same order as for +
    //    ((x*y2, y*z2, x*z2) - (w*z2, w*x2, w*y2)) * scale.yxz

    // RotBase = x*y2, y*z2, x*z2
    // RotOffset = w*z2, w*x2, w*y2
    const VectorRegister x_y_x = VectorSwizzle(rotation_, 0, 1, 0, 0);
    const VectorRegister y2_z2_z2 = VectorSwizzle(RotationX2Y2Z2, 1, 2, 2, 0);
    const VectorRegister RotBase = VectorMultiply(x_y_x, y2_z2_z2);

    const VectorRegister w_w_w = VectorReplicate(rotation_, 3);
    const VectorRegister z2_x2_y2 = VectorSwizzle(RotationX2Y2Z2, 2, 0, 1, 0);
    const VectorRegister RotOffset = VectorMultiply(w_w_w, z2_x2_y2);

    // adds = (RotBase + RotOffset)*scale_ :  (x*y2 + w*z2) * scale_.x , (y*z2 + w*x2) * scale_.y, (x*z2 + w*y2) * scale_.z
    // subtracts = (RotBase - RotOffset)*Scale3DYZX :  (x*y2 - w*z2) * scale_.y , (y*z2 - w*x2) * scale_.z, (x*z2 - w*y2) * scale_.x
    const VectorRegister adds = VectorAdd(RotBase, RotOffset);
    OutAdds = VectorMultiply(adds, scale_);
    const VectorRegister Scale3DYZXW = VectorSwizzle(scale_, 1, 2, 0, 3);
    const VectorRegister subtracts = VectorSubtract(RotBase, RotOffset);
    OutSubtracts = VectorMultiply(subtracts , Scale3DYZXW);
  }

  FUN_ALWAYS_INLINE void ToMatrixInternalNoScale(VectorRegister& OutDiagonals, VectorRegister& OutAdds, VectorRegister& OutSubtracts) const {
#if !(FUN_BUILD_SHIPPING || FUN_BUILD_TEST) && WITH_EDITORONLY_DATA
    // Make sure rotation_ is normalized when we turn it into a matrix.
    fun_check(IsRotationNormalized());
#endif
    const VectorRegister RotationX2Y2Z2 = VectorAdd(rotation_, rotation_);    // x2, y2, z2
    const VectorRegister RotationXX2YY2ZZ2 = VectorMultiply(RotationX2Y2Z2, rotation_);  // xx2, yy2, zz2

    // The diagonal terms of the rotation matrix are:
    //   (1 - (yy2 + zz2))
    //   (1 - (xx2 + zz2))
    //   (1 - (xx2 + yy2))
    const VectorRegister yy2_xx2_xx2 = VectorSwizzle(RotationXX2YY2ZZ2, 1, 0, 0, 0);
    const VectorRegister zz2_zz2_yy2 = VectorSwizzle(RotationXX2YY2ZZ2, 2, 2, 1, 0);
    const VectorRegister DiagonalSum = VectorAdd(yy2_xx2_xx2, zz2_zz2_yy2);
    OutDiagonals = VectorSubtract(VectorOne(), DiagonalSum);

    // Grouping the non-diagonal elements in the rotation block by operations:
    //    ((x*y2, y*z2, x*z2) + (w*z2, w*x2, w*y2)) and
    //    ((x*y2, y*z2, x*z2) - (w*z2, w*x2, w*y2))
    // Rearranging so the lhs and rhs are in the same order as for +
    //    ((x*y2, y*z2, x*z2) - (w*z2, w*x2, w*y2))

    // RotBase = x*y2, y*z2, x*z2
    // RotOffset = w*z2, w*x2, w*y2
    const VectorRegister x_y_x = VectorSwizzle(rotation_, 0, 1, 0, 0);
    const VectorRegister y2_z2_z2 = VectorSwizzle(RotationX2Y2Z2, 1, 2, 2, 0);
    const VectorRegister RotBase = VectorMultiply(x_y_x, y2_z2_z2);

    const VectorRegister w_w_w = VectorReplicate(rotation_, 3);
    const VectorRegister z2_x2_y2 = VectorSwizzle(RotationX2Y2Z2, 2, 0, 1, 0);
    const VectorRegister RotOffset = VectorMultiply(w_w_w, z2_x2_y2);

    // adds = (RotBase + RotOffset):  (x*y2 + w*z2) , (y*z2 + w*x2), (x*z2 + w*y2)
    // subtracts = (RotBase - RotOffset) :  (x*y2 - w*z2) , (y*z2 - w*x2), (x*z2 - w*y2)
    OutAdds = VectorAdd(RotBase, RotOffset);
    OutSubtracts = VectorSubtract(RotBase, RotOffset);
  }

  /**
  mathematically if you have 0 scale, it should be infinite,
  however, in practice if you have 0 scale, and relative transform doesn't make much sense
  anymore because you should be instead of showing gigantic infinite mesh
  also returning BIG_NUMBER causes sequential NaN issues by multiplying
  so we hardcode as 0
  */
  static FUN_ALWAYS_INLINE VectorRegister GetSafeScaleReciprocal(const VectorRegister& InScale, const ScalarRegister& tolerance = ScalarRegister(VectorZero())) {
    // safe_reciprocal_scale.x = (InScale.x == 0) ? 0.f : 1/InScale.x; // same for YZW
    VectorRegister safe_reciprocal_scale;

    /// VectorRegister(1.f / InScale.x, 1.f / InScale.y, 1.f / InScale.z, 1.f / InScale.w)
    const VectorRegister ReciprocalScale = VectorReciprocalAccurate(InScale);

    //VectorRegister(vec1.x == vec2.x ? 0xFFFFFFFF : 0, same for yzw)
    const VectorRegister ScaleZeroMask = VectorCompareGE(tolerance.Value, VectorAbs(InScale));

    //const VectorRegister ScaleZeroMask = VectorCompareEQ(InScale, VectorZero());

    // VectorRegister(for each bit i: Mask[i] ? vec1[i] : vec2[i])
    safe_reciprocal_scale = VectorSelect(ScaleZeroMask, VectorZero(), ReciprocalScale);

    return safe_reciprocal_scale;
  }

  /** Returns Inverse transform of this Transform **/
  FUN_ALWAYS_INLINE Transform InverseFast() const {
    // Inverse QST (a) = QST (~a)
    // Since a*~a = Identity,
    // a(P) = q(a)*S(a)*P*-q(a) + T(a)
    // ~a(a(P)) = q(~a)*S(~a)*(q(a)*S(a)*P*-q(a) + T(a))*-q(~a) + T(~a) = Identity
    // q(~a)*q(a)*S(~a)*S(a)*P*-q(a)*-q(~a) + q(~a)*S(~a)*T(a)*-q(~a) + T(~a) = Identity
    // [q(~a)*q(a)]*[S(~a)*S(a)]*P*-[q(~a)*q(a)] + [q(~a)*S(~a)*T(a)*-q(~a) + T(~a)] = I

    // Identity q = (0, 0, 0, 1) = q(~a)*q(a)
    // Identity Scale = 1 = S(~a)*S(a)
    // Identity translation = (0, 0, 0) = [q(~a)*S(~a)*T(a)*-q(~a) + T(~a)]

    //  q(~a) = q(~a)
    //  S(~a) = 1.f/S(a)
    //  T(~a) = - (q(~a)*S(~a)*T(a)*q(a))
    fun_check_dbg(IsRotationNormalized());
    fun_check_dbg(VectorAnyGreaterThan(VectorAbs(scale_), VectorizeConstants::SmallNumber));

    // Invert the scale
    const VectorRegister InvScale = VectorSet_W0(GetSafeScaleReciprocal(scale_ , ScalarRegister(VectorizeConstants::SmallNumber)));

    // Invert the rotation
    const VectorRegister InvRotation = VectorQuaternionInverse(rotation_);

    // Invert the translation
    const VectorRegister ScaledTranslation = VectorMultiply(InvScale, translation_);
    const VectorRegister t2 = VectorQuaternionRotateVector(InvRotation, ScaledTranslation);
    const VectorRegister InvTranslation = VectorSet_W0(VectorNegate(t2));

    return Transform(InvRotation, InvTranslation, InvScale);
  }
};

template <> struct IsPOD<Transform> { enum { Value = true }; };


//
// inlines
//

/** Scale the translation part of the transform by the supplied vector. */
FUN_ALWAYS_INLINE void Transform::ScaleTranslation(const Vector& scale)
{
  VectorRegister VectorInScale3D = VectorLoadFloat3_W0(&scale);
  translation_ = VectorMultiply(translation_, VectorInScale3D);
  DiagnosticCheckNaN_Translate();
}

/** Scale the translation part of the transform by the supplied float. */
FUN_ALWAYS_INLINE void Transform::ScaleTranslation(const float& InScale)
{
  ScaleTranslation(Vector(InScale));
}

// this function is from matrix, and all it does is to normalize rotation portion
FUN_ALWAYS_INLINE void Transform::RemoveScaling(float tolerance/* = SMALL_NUMBER*/)
{
  scale_ = VectorSet_W0(VectorOne());
  NormalizeRotation();

  DiagnosticCheckNaN_Rotate();
  DiagnosticCheckNaN_Scale3D();
}

/** Returns Multiplied transform of 2 Transforms **/
FUN_ALWAYS_INLINE void Transform::Multiply(Transform* out_transform, const Transform* a, const Transform* b)
{
  a->DiagnosticCheckNaN_All();
  b->DiagnosticCheckNaN_All();

  fun_check_dbg(a->IsRotationNormalized());
  fun_check_dbg(b->IsRotationNormalized());

  //  When q = quaternion, S = single scalar scale, and T = translation
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

  const VectorRegister QuatA = a->rotation_;
  const VectorRegister QuatB = b->rotation_;
  const VectorRegister TranslateA = a->translation_;
  const VectorRegister TranslateB = b->translation_;
  const VectorRegister ScaleA = a->scale_;
  const VectorRegister ScaleB = b->scale_;

  // RotationResult = b.rotation_ * a.rotation_
  out_transform->rotation_ = VectorQuaternionMultiply2(QuatB, QuatA);

  // TranslateResult = b.Rotate(b.Scale * a.translation) + b.Translate
  const VectorRegister ScaledTransA = VectorMultiply(TranslateA, ScaleB);
  const VectorRegister RotatedTranslate = VectorQuaternionRotateVector(QuatB, ScaledTransA);
  out_transform->translation_ = VectorAdd(RotatedTranslate, TranslateB);

  // ScaleResult = Scale.b * Scale.a
  out_transform->scale_ = VectorMultiply(ScaleA, ScaleB);;
}

/**
 * Apply Scale to this transform
 */
FUN_ALWAYS_INLINE Transform Transform::GetScaled(float InScale) const
{
  Transform a(*this);

  VectorRegister VScale = VectorLoadFloat1(&InScale);
  a.scale_ = VectorMultiply(a.scale_, VScale);

  a.DiagnosticCheckNaN_Scale3D();

  return a;
}

/**
 * Apply Scale to this transform
 */
FUN_ALWAYS_INLINE Transform Transform::GetScaled(Vector InScale) const
{
  Transform a(*this);

  VectorRegister VScale = VectorLoadFloat3_W0(&InScale);
  a.scale_ = VectorMultiply(a.scale_, VScale);

  a.DiagnosticCheckNaN_Scale3D();

  return a;
}

FUN_ALWAYS_INLINE Vector4 Transform::TransformVector4NoScale(const Vector4& v) const
{
  DiagnosticCheckNaN_All();

  // if not, this won't work
  fun_check_dbg(v.w == 0.f || v.w == 1.f);

  const VectorRegister InputVector = VectorLoadAligned(&v);

  //transform using QST is following
  //QST(P) = q.Rotate(S*P) + T where q = quaternion, S = 1.f, T = translation

  //RotatedVec = q.Rotate(v.x, v.y, v.z, 0.f)
  const VectorRegister InputVectorW0 = VectorSet_W0(InputVector);
  const VectorRegister RotatedVec = VectorQuaternionRotateVector(rotation_, InputVectorW0);

  // NewVect.XYZ += translation * w
  // NewVect.w += 1 * w
  const VectorRegister WWWW = VectorReplicate(InputVector, 3);
  const VectorRegister TranslatedVec = VectorMultiplyAdd(translation_, WWWW, RotatedVec);

  Vector4 NewVectOutput;
  VectorStoreAligned(TranslatedVec, &NewVectOutput);
  return NewVectOutput;
}

FUN_ALWAYS_INLINE Vector4 Transform::TransformVector4(const Vector4& v) const
{
  DiagnosticCheckNaN_All();

  // if not, this won't work
  fun_check_dbg(v.w == 0.f || v.w == 1.f);

  const VectorRegister InputVector = VectorLoadAligned(&v);

  //transform using QST is following
  //QST(P) = q.Rotate(S*P) + T where q = quaternion, S = scale, T = translation

  //RotatedVec = q.Rotate(Scale*v.x, Scale*v.y, Scale*v.z, 0.f)
  const VectorRegister InputVectorW0 = VectorSet_W0(InputVector);
  const VectorRegister ScaledVec = VectorMultiply(scale_, InputVectorW0);
  const VectorRegister RotatedVec = VectorQuaternionRotateVector(rotation_, ScaledVec);

  // NewVect.XYZ += translation * w
  // NewVect.w += 1 * w
  const VectorRegister WWWW = VectorReplicate(InputVector, 3);
  const VectorRegister TranslatedVec = VectorMultiplyAdd(translation_, WWWW, RotatedVec);

  Vector4 NewVectOutput;
  VectorStoreAligned(TranslatedVec, &NewVectOutput);
  return NewVectOutput;
}

FUN_ALWAYS_INLINE Vector Transform::TransformPosition(const Vector& v) const
{
  DiagnosticCheckNaN_All();

  const VectorRegister InputVectorW0 = VectorLoadFloat3_W0(&v);

  //transform using QST is following
  //QST(P) = q.Rotate(S*P) + T where q = quaternion, S = scale, T = translation

  //RotatedVec = q.Rotate(Scale*v.x, Scale*v.y, Scale*v.z, 0.f)
  const VectorRegister ScaledVec = VectorMultiply(scale_, InputVectorW0);
  const VectorRegister RotatedVec = VectorQuaternionRotateVector(rotation_, ScaledVec);

  const VectorRegister TranslatedVec = VectorAdd(RotatedVec, translation_);

  Vector result;
  VectorStoreFloat3(TranslatedVec, &result);
  return result;
}

FUN_ALWAYS_INLINE Vector Transform::TransformPositionNoScale(const Vector& v) const
{
  DiagnosticCheckNaN_All();

  const VectorRegister InputVectorW0 = VectorLoadFloat3_W0(&v);

  //transform using QST is following
  //QST(P) = q.Rotate(S*P) + T where q = quaternion, S = 1.f, T = translation

  //RotatedVec = q.Rotate(v.x, v.y, v.z, 0.f)
  const VectorRegister RotatedVec = VectorQuaternionRotateVector(rotation_, InputVectorW0);

  const VectorRegister TranslatedVec = VectorAdd(RotatedVec, translation_);

  Vector result;
  VectorStoreFloat3(TranslatedVec, &result);
  return result;
}

FUN_ALWAYS_INLINE Vector Transform::TransformVector(const Vector& v) const
{
  DiagnosticCheckNaN_All();

  const VectorRegister InputVectorW0 = VectorLoadFloat3_W0(&v);

  //RotatedVec = q.Rotate(Scale*v.x, Scale*v.y, Scale*v.z, 0.f)
  const VectorRegister ScaledVec = VectorMultiply(scale_, InputVectorW0);
  const VectorRegister RotatedVec = VectorQuaternionRotateVector(rotation_, ScaledVec);

  Vector result;
  VectorStoreFloat3(RotatedVec, &result);
  return result;
}

FUN_ALWAYS_INLINE Vector Transform::TransformVectorNoScale(const Vector& v) const
{
  DiagnosticCheckNaN_All();

  const VectorRegister InputVectorW0 = VectorLoadFloat3_W0(&v);

  //RotatedVec = q.Rotate(v.x, v.y, v.z, 0.f)
  const VectorRegister RotatedVec = VectorQuaternionRotateVector(rotation_, InputVectorW0);

  Vector result;
  VectorStoreFloat3(RotatedVec, &result);
  return result;
}

// do backward operation when inverse, translation -> rotation -> scale
FUN_ALWAYS_INLINE Vector Transform::InverseTransformPosition(const Vector& v) const
{
  DiagnosticCheckNaN_All();

  const VectorRegister InputVector = VectorLoadFloat3_W0(&v);

  // (v-translation)
  const VectorRegister TranslatedVec = VectorSet_W0(VectorSubtract(InputVector, translation_));

  // (rotation_.Inverse() * (v-translation))
  const VectorRegister VR = VectorQuaternionInverseRotateVector(rotation_, TranslatedVec);

  // GetSafeScaleReciprocal(scale_);
  const VectorRegister SafeReciprocal = GetSafeScaleReciprocal(scale_);

  // (rotation_.Inverse() * (v-translation)) * GetSafeScaleReciprocal(scale_);
  const VectorRegister VResult = VectorMultiply(VR, SafeReciprocal);

  Vector result;
  VectorStoreFloat3(VResult, &result);
  return result;
}

// do backward operation when inverse, translation -> rotation
FUN_ALWAYS_INLINE Vector Transform::InverseTransformPositionNoScale(const Vector& v) const
{
  DiagnosticCheckNaN_All();

  const VectorRegister InputVector = VectorLoadFloat3_W0(&v);

  // (v-translation)
  const VectorRegister TranslatedVec = VectorSet_W0(VectorSubtract(InputVector, translation_));

  // (rotation_.Inverse() * (v-translation))
  const VectorRegister VResult = VectorQuaternionInverseRotateVector(rotation_, TranslatedVec);

  Vector result;
  VectorStoreFloat3(VResult, &result);
  return result;
}

// do backward operation when inverse, translation -> rotation -> scale
FUN_ALWAYS_INLINE Vector Transform::InverseTransformVector(const Vector& v) const
{
  DiagnosticCheckNaN_All();

  const VectorRegister InputVector = VectorLoadFloat3_W0(&v);

  // (rotation_.Inverse() * v) aka. Vector Quat::operator*(const Vector& v) const
  const VectorRegister VR = VectorQuaternionInverseRotateVector(rotation_, InputVector);

  // GetSafeScaleReciprocal(scale_);
  const VectorRegister SafeReciprocal = GetSafeScaleReciprocal(scale_);

  // (rotation_.Inverse() * v) * GetSafeScaleReciprocal(scale_);
  const VectorRegister VResult = VectorMultiply(VR, SafeReciprocal);

  Vector result;
  VectorStoreFloat3(VResult, &result);
  return result;
}

// do backward operation when inverse, translation -> rotation
FUN_ALWAYS_INLINE Vector Transform::InverseTransformVectorNoScale(const Vector& v) const
{
  DiagnosticCheckNaN_All();

  VectorRegister InputVector = VectorLoadFloat3_W0(&v);

  // (rotation_.Inverse() * v)
  VectorRegister VResult = VectorQuaternionInverseRotateVector(rotation_, InputVector);

  Vector result;
  VectorStoreFloat3(VResult, &result);
  return result;
}

FUN_ALWAYS_INLINE Transform Transform::operator*(const Transform& other) const
{
  Transform Output;
  Multiply(&Output, this, &other);
  return Output;
}

FUN_ALWAYS_INLINE void Transform::operator*=(const Transform& other)
{
  Multiply(this, this, &other);
}

FUN_ALWAYS_INLINE Transform Transform::operator*(const Quat& other) const
{
  Transform Output, OtherTransform(other, Vector::ZeroVector, Vector(1.f));
  Multiply(&Output, this, &OtherTransform);
  return Output;
}

FUN_ALWAYS_INLINE void Transform::operator*=(const Quat& other)
{
  Transform OtherTransform(other, Vector::ZeroVector, Vector(1.f));
  Multiply(this, this, &OtherTransform);
}

// x = 0, y = 1, z = 2
FUN_ALWAYS_INLINE Vector Transform::GetScaledAxis(Axis InAxis) const
{
  if (InAxis == Axis::X) {
    return TransformVector(Vector(1.f, 0.f, 0.f));
  }
  else if (InAxis == Axis::Y) {
    return TransformVector(Vector(0.f, 1.f, 0.f));
  }

  return TransformVector(Vector(0.f, 0.f, 1.f));
}

// x = 0, y = 1, z = 2
FUN_ALWAYS_INLINE Vector Transform::GetUnitAxis(Axis InAxis) const
{
  if (InAxis == Axis::X) {
    return TransformVectorNoScale(Vector(1.f, 0.f, 0.f));
  }
  else if (InAxis == Axis::Y) {
    return TransformVectorNoScale(Vector(0.f, 1.f, 0.f));
  }

  return TransformVectorNoScale(Vector(0.f, 0.f, 1.f));
}

FUN_ALWAYS_INLINE void Transform::Mirror(Axis mirror_axis, Axis flip_axis)
{
  // We do convert to matrix for mirroring.
  Matrix m = ToMatrixWithScale();
  m.Mirror(mirror_axis, flip_axis);
  SetFromMatrix(m);
}

/** same version of Matrix::GetMaximumAxisScale function **/
/** \return the maximum magnitude of any row of the matrix. */
FUN_ALWAYS_INLINE float Transform::GetMaximumAxisScale() const
{
  DiagnosticCheckNaN_Scale3D();

  float Scale3DAbsMax;
  // Scale3DAbsXYZ1 = { Abs(x), Abs(y)), Abs(z), 0 }
  const VectorRegister Scale3DAbsXYZ0 =  VectorAbs(scale_);
  // Scale3DAbsYZX1 = { Abs(y), Abs(z)), Abs(x), 0 }
  const VectorRegister Scale3DAbsYZX0 = VectorSwizzle(Scale3DAbsXYZ0, 1, 2, 0, 3);
  // Scale3DAbsZXY1 = { Abs(z), Abs(x)), Abs(y), 0 }
  const VectorRegister Scale3DAbsZXY0 = VectorSwizzle(Scale3DAbsXYZ0, 2, 0, 1, 3);
  // t0 = { Max(Abs(x), Abs(y)), Max(Abs(y), Abs(z)), Max(Abs(z), Abs(x)), 0 }
  const VectorRegister t0 = VectorMax(Scale3DAbsXYZ0, Scale3DAbsYZX0);
  // t1 = { Max(Abs(x), Abs(y), Abs(z)), Max(Abs(y), Abs(z), Abs(x)), Max(Abs(z), Abs(x), Abs(y)), 0 }
  const VectorRegister t2 = VectorMax(t0, Scale3DAbsZXY0);
  // Scale3DAbsMax = Max(Abs(x), Abs(y), Abs(z));
  VectorStoreFloat1(t2, &Scale3DAbsMax);

  return Scale3DAbsMax;
}

/** \return the minimum magnitude of all components of the 3D scale. */
FUN_ALWAYS_INLINE float Transform::GetMinimumAxisScale() const
{
  DiagnosticCheckNaN_Scale3D();

  float Scale3DAbsMin;
  // Scale3DAbsXYZ1 = { Abs(x), Abs(y)), Abs(z), 0 }
  const VectorRegister Scale3DAbsXYZ0 =  VectorAbs(scale_);
  // Scale3DAbsYZX1 = { Abs(y), Abs(z)), Abs(x), 0 }
  const VectorRegister Scale3DAbsYZX0 = VectorSwizzle(Scale3DAbsXYZ0, 1, 2, 0, 3);
  // Scale3DAbsZXY1 = { Abs(z), Abs(x)), Abs(y), 0 }
  const VectorRegister Scale3DAbsZXY0 = VectorSwizzle(Scale3DAbsXYZ0, 2, 0, 1, 3);
  // t0 = { Min(Abs(x), Abs(y)), Min(Abs(y), Abs(z)), Min(Abs(z), Abs(x)), 0 }
  const VectorRegister t0 = VectorMin(Scale3DAbsXYZ0, Scale3DAbsYZX0);
  // t1 = { Min(Abs(x), Abs(y), Abs(z)), Min(Abs(y), Abs(z), Abs(x)), Min(Abs(z), Abs(x), Abs(y)), 0 }
  const VectorRegister t2 = VectorMin(t0, Scale3DAbsZXY0);
  // Scale3DAbsMax = Min(Abs(x), Abs(y), Abs(z));
  VectorStoreFloat1(t2, &Scale3DAbsMin);

  return Scale3DAbsMin;
}

/**
mathematically if you have 0 scale, it should be infinite,
however, in practice if you have 0 scale, and relative transform doesn't make much sense
anymore because you should be instead of showing gigantic infinite mesh
also returning BIG_NUMBER causes sequential NaN issues by multiplying
so we hardcode as 0
*/
FUN_ALWAYS_INLINE Vector Transform::GetSafeScaleReciprocal(const Vector& scale, float tolerance) const
{
  Vector safe_reciprocal_scale;
  safe_reciprocal_scale.x = (Math::Abs(scale.x) <= tolerance) ? 0.f : 1.f / scale.x;
  safe_reciprocal_scale.y = (Math::Abs(scale.y) <= tolerance) ? 0.f : 1.f / scale.y;
  safe_reciprocal_scale.z = (Math::Abs(scale.z) <= tolerance) ? 0.f : 1.f / scale.z;
  return safe_reciprocal_scale;
}


} // namespace fun
