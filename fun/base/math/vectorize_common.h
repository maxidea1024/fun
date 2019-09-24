#pragma once

//namespace fun {

FUN_ALWAYS_INLINE bool VectorIsAligned(const void* ptr) {
  return !(intptr_t(ptr) & (SIMD_ALIGNMENT - 1));
}

// Returns a normalized 4 vector = vector / |vector|.
// There is no handling of zero length vectors, use VectorNormalizeSafe if this is a possible input.
FUN_ALWAYS_INLINE VectorRegister VectorNormalizeAccurate(const VectorRegister& vector) {
  const VectorRegister length_sqr = VectorDot4(vector, vector);
  const VectorRegister inv_length = VectorReciprocalSqrtAccurate(length_sqr);
  const VectorRegister normalized_vector = VectorMultiply(inv_length, vector);
  return normalized_vector;
}

// Returns ((vector dot vector) >= 1e-8) ? (vector / |vector|) : default_value
// Uses accurate 1/sqrt, not the estimate
FUN_ALWAYS_INLINE VectorRegister VectorNormalizeSafe(const VectorRegister& vector, const VectorRegister& default_value) {
  const VectorRegister length_sqr = VectorDot4(vector, vector);
  const VectorRegister non_zero_mask = VectorCompareGE(length_sqr, VectorizeConstants::SmallLengthThreshold);
  const VectorRegister inv_length = VectorReciprocalSqrtAccurate(length_sqr);
  const VectorRegister normalized_vector = VectorMultiply(inv_length, vector);
  return VectorSelect(non_zero_mask, normalized_vector, default_value);
}

/**
 * Returns non-zero if any element in vec1 is lesser than the corresponding element in vec2, otherwise 0.
 *
 * \param vec1 - 1st source vector
 * \param vec2 - 2nd source vector
 *
 * \return Non-zero integer if (vec1.x < vec2.x) || (vec1.y < vec2.y) || (vec1.z < vec2.z) || (vec1.w < vec2.w)
 */
FUN_ALWAYS_INLINE uint32 VectorAnyLesserThan(VectorRegister vec1, VectorRegister vec2) {
  return VectorAnyGreaterThan(vec2, vec1);
}

/**
 * Returns non-zero if all elements in vec1 are greater than the corresponding elements in vec2, otherwise 0.
 *
 * \param vec1 - 1st source vector
 * \param vec2 - 2nd source vector
 *
 * \return Non-zero integer if (vec1.x > vec2.x) && (vec1.y > vec2.y) && (vec1.z > vec2.z) && (vec1.w > vec2.w)
 */
FUN_ALWAYS_INLINE uint32 VectorAllGreaterThan(VectorRegister vec1, VectorRegister vec2) {
  return !VectorAnyGreaterThan(vec2, vec1);
}

/**
 * Returns non-zero if all elements in vec1 are lesser than the corresponding elements in vec2, otherwise 0.
 *
 * \param vec1 - 1st source vector
 * \param vec2 - 2nd source vector
 *
 * \return Non-zero integer if (vec1.x < vec2.x) && (vec1.y < vec2.y) && (vec1.z < vec2.z) && (vec1.w < vec2.w)
 */
FUN_ALWAYS_INLINE uint32 VectorAllLesserThan(VectorRegister vec1, VectorRegister vec2) {
  return !VectorAnyGreaterThan(vec1, vec2);
}


//
// VectorRegister specialization of templates.
//

/** Returns the smaller of the two values (operates on each component individually) */
template <> FUN_ALWAYS_INLINE VectorRegister CGenericPlatformMath::Min(const VectorRegister a, const VectorRegister b) {
  return VectorMin(a, b);
}

/** Returns the larger of the two values (operates on each component individually) */
template <> FUN_ALWAYS_INLINE VectorRegister CGenericPlatformMath::Max(const VectorRegister a, const VectorRegister b) {
  return VectorMax(a, b);
}

// Specialization of Lerp template that works with vector registers
template <> FUN_ALWAYS_INLINE VectorRegister Math::Lerp(const VectorRegister& a, const VectorRegister& b, const VectorRegister& alpha) {
  const VectorRegister delta = VectorSubtract(b, a);
  return VectorMultiplyAdd(alpha, delta, a);
}

// a and b are quaternions.  The result is a + (|a.b| >= 0 ? 1 : -1) * b
FUN_ALWAYS_INLINE VectorRegister VectorAccumulateQuaternionShortestPath(const VectorRegister& a, const VectorRegister& b) {
  // Blend rotation
  //     to ensure the 'shortest route', we make sure the dot product between the both rotations is positive.
  //     const float bias = (|a.b| >= 0 ? 1 : -1)
  //     return a + b * bias;
  const VectorRegister zero = VectorZero();
  const VectorRegister rotation_dot = VectorDot4(a, b);
  const VectorRegister quat_rotation_dir_mask = VectorCompareGE(rotation_dot, zero);
  const VectorRegister negative_b = VectorSubtract(zero, b);
  const VectorRegister bias_times_b = VectorSelect(quat_rotation_dir_mask, b, negative_b);
  return VectorAdd(a, biastimesb);
}

// Normalize quaternion (result = (q.q >= 1e-8) ? (q / |q|) : (0, 0, 0, 1))
FUN_ALWAYS_INLINE VectorRegister VectorNormalizeQuaternion(const VectorRegister& unnormalized_quat) {
  return VectorNormalizeSafe(unnormalized_quat, VectorizeConstants::Float0001);
}

// Normalize rotator
FUN_ALWAYS_INLINE VectorRegister VectorNormalizeRotator(const VectorRegister& unnormalized_rotator) {
  // shift in the range [-360, 360]
  VectorRegister v0 = VectorMod(unnormalized_rotator, VectorizeConstants::Float360);
  VectorRegister v1 = VectorAdd(V0, VectorizeConstants::Float360);
  VectorRegister v2 = VectorSelect(VectorCompareGE(v0, VectorZero()), v0, v1);

  // shift to [-180, 180]
  VectorRegister v3 = VectorSubtract(v2, VectorizeConstants::Float360);
  VectorRegister v4 = VectorSelect(VectorCompareGT(v2, VectorizeConstants::Float180), v3, v2);

  return  V4;
}

/**
 * Fast Linear Quaternion Interpolation for quaternions stored in VectorRegisters.
 * result is NOT normalized.
 */
FUN_ALWAYS_INLINE VectorRegister VectorLerpQuat(const VectorRegister& a, const VectorRegister& b, const VectorRegister& alpha) {
  // Blend rotation
  //     to ensure the 'shortest route', we make sure the dot product between the both rotations is positive.
  //     const float bias = (|a.b| >= 0 ? 1 : -1)
  //     Rotation = (b * alpha) + (a * (bias * (1.f - alpha)));
  const VectorRegister zero = VectorZero();

  const VectorRegister one_minus_alpha = VectorSubtract(VectorOne(), alpha);

  const VectorRegister rotation_dot = VectorDot4(a, b);
  const VectorRegister quat_rotation_dir_mask = VectorCompareGE(rotation_dot, zero);
  const VectorRegister negative_a = VectorSubtract(zero, a);
  const VectorRegister bias_times_a = VectorSelect(quat_rotation_dir_mask, a, negative_a);
  const VectorRegister b_times_weight = VectorMultiply(b, alpha);
  const VectorRegister unnormalized_result = VectorMultiplyAdd(bias_times_a, one_minus_alpha, b_times_weight);

  return unnormalized_result;
}

/**
 * Bi-Linear Quaternion interpolation for quaternions stored in VectorRegisters.
 * result is NOT normalized.
 */
FUN_ALWAYS_INLINE VectorRegister VectorBiLerpQuat(const VectorRegister& p00, const VectorRegister& p10, const VectorRegister& p01, const VectorRegister& p11, const VectorRegister& frac_x, const VectorRegister& frac_y) {
  return VectorLerpQuat(
    VectorLerpQuat(p00, p10, frac_x),
    VectorLerpQuat(p01, p11, frac_x),
    frac_y);
}

/**
 * Inverse quaternion (-x, -y, -z, w)
 */
FUN_ALWAYS_INLINE VectorRegister VectorQuaternionInverse(const VectorRegister& normalized_quat) {
  return VectorMultiply(VectorizeConstants::QINV_SIGN_MASK, normalized_quat);
}

/**
 * Rotate a vector using a unit Quaternion.
 *
 * \param quat - Unit Quaternion to use for rotation.
 * \param vector_w0 - vector to rotate. w component must be zero.
 *
 * \return vector after rotation by quat.
 */
FUN_ALWAYS_INLINE VectorRegister VectorQuaternionRotateVector(const VectorRegister& quat, const VectorRegister& vector_w0) {
  // q * v * q.Inverse
  //const VectorRegister InverseRotation = VectorQuaternionInverse(quat);
  //const VectorRegister tmp = VectorQuaternionMultiply2(quat, vector_w0);
  //const VectorRegister Rotated = VectorQuaternionMultiply2(tmp, InverseRotation);

  // Equivalence of above can be shown to be:
  // http://people.csail.mit.edu/bkph/articles/Quaternions.pdf
  // v' = v + 2w(q x v) + (2Q x (q x v))
  // refactor:
  // v' = v + w(2(q x v)) + (q x (2(q x v)))
  // t = 2(q x v);
  // v' = v + w*(t) + (q x t)

  const VectorRegister QW = VectorReplicate(quat, 3);
  VectorRegister t = VectorCross(quat, vector_w0);
  t = VectorAdd(t, t);
  const VectorRegister v_tmp0 = VectorMultiplyAdd(QW, t, vector_w0);
  const VectorRegister v_tmp1 = VectorCross(quat, t);
  const VectorRegister rotated = VectorAdd(v_tmp0, v_tmp1);
  return rotated;
}

/**
 * Rotate a vector using the inverse of a unit Quaternion (rotation in the opposite direction).
 *
 * \param quat - Unit Quaternion to use for rotation.
 * \param vector_w0 - vector to rotate. w component must be zero.
 *
 * \return vector after rotation by the inverse of quat.
 */
FUN_ALWAYS_INLINE VectorRegister VectorQuaternionInverseRotateVector(const VectorRegister& quat, const VectorRegister& vector_w0) {
  // q.Inverse * v * q
  //const VectorRegister inverse_rotation = VectorQuaternionInverse(quat);
  //const VectorRegister tmp = VectorQuaternionMultiply2(inverse_rotation, vector_w0);
  //const VectorRegister rotated = VectorQuaternionMultiply2(tmp, quat);

  const VectorRegister q_inv = VectorQuaternionInverse(quat);
  return VectorQuaternionRotateVector(q_inv, vector_w0);
}

/**
 * Rotate a vector using a pointer to a unit Quaternion.
 *
 * \param result - Pointer to where the result should be stored
 * \param quat - Pointer to the unit quaternion (must not be the destination)
 * \param vector_w0 - Pointer to the vector (must not be the destination). w component must be zero.
 */
FUN_ALWAYS_INLINE void VectorQuaternionRotateVectorPtr(void* __restrict result, const void* __restrict quat, const void* __restrict vector_w0) {
  *((VectorRegister*)result) = VectorQuaternionRotateVector(*((const VectorRegister*)quat), *((const VectorRegister*)vector_w0));
}

/**
 * Rotate a vector using the inverse of a unit Quaternion (rotation in the opposite direction).
 *
 * \param result - Pointer to where the result should be stored
 * \param quat - Pointer to the unit quaternion (must not be the destination)
 * \param vector_w0 - Pointer to the vector (must not be the destination). w component must be zero.
 */
FUN_ALWAYS_INLINE void VectorQuaternionInverseRotateVectorPtr(void* __restrict result, const void* __restrict quat, const void* __restrict vector_w0) {
  *((VectorRegister*)result) = VectorQuaternionInverseRotateVector(*((const VectorRegister*)quat), *((const VectorRegister*)vector_w0));
}

//} // namespace fun
