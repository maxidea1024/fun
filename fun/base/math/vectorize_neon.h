#pragma once

// Include the intrinsic functions header
#include <arm_neon.h>

namespace fun {

//
// Helpers:
//

/** 16-byte vector register type */
typedef float32x4_t __attribute((aligned(16))) VectorRegister;
typedef uint32x4_t  __attribute((aligned(16))) IntVectorRegister;

#define DECLARE_VECTOR_REGISTER(x, y, z, w)  { x, y, z, w }

/**
 * Returns a bitwise equivalent vector based on 4 uint32s.
 *
 * \param x - 1st uint32 component
 * \param y - 2nd uint32 component
 * \param z - 3rd uint32 component
 * \param w - 4th uint32 component
 *
 * \return Bitwise equivalent vector with 4 floats
 */
FUN_ALWAYS_INLINE VectorRegister MakeVectorRegister(uint32 x, uint32 y, uint32 z, uint32 w) {
  union { VectorRegister v; uint32 f[4]; } tmp;
  tmp.f[0] = x;
  tmp.f[1] = y;
  tmp.f[2] = z;
  tmp.f[3] = w;
  return tmp.v;
}

/**
 * Returns a vector based on 4 floats.
 *
 * \param x - 1st float component
 * \param y - 2nd float component
 * \param z - 3rd float component
 * \param w - 4th float component
 *
 * \return vector of the 4 floats
 */
FUN_ALWAYS_INLINE VectorRegister MakeVectorRegister(float x, float y, float z, float w) {
  union { VectorRegister v; float f[4]; } tmp;
  tmp.f[0] = x;
  tmp.f[1] = y;
  tmp.f[2] = z;
  tmp.f[3] = w;
  return tmp.v;
}

//#define VectorPermute(vec1, vec2, Mask)  my_perm(vec1, vec2, Mask)
//
/// ** Reads NumBytesMinusOne+1 bytes from the address pointed to by ptr, always reading the aligned 16 bytes containing the start of ptr, but only reading the next 16 bytes if the data straddles the boundary * /
//FUN_ALWAYS_INLINE VectorRegister VectorLoadNPlusOneUnalignedBytes(const void* ptr, int NumBytesMinusOne)
//{
//  return VectorPermute(my_ld (0, (float*)ptr), my_ld(NumBytesMinusOne, (float*)ptr), my_lvsl(0, (float*)ptr));
//}


//
// Constants:
//

#include "fun/base/math/vectorize_constants.h"

//
// Intrinsics:
//

/**
 * Returns a vector with all zeros.
 *
 * \return VectorRegister(0.f, 0.f, 0.f, 0.f)
 */
FUN_ALWAYS_INLINE VectorRegister VectorZero() {
  return vdupq_n_f32(0.f);
}

/**
 * Returns a vector with all ones.
 *
 * \return VectorRegister(1.f, 1.f, 1.f, 1.f)
 */
FUN_ALWAYS_INLINE VectorRegister VectorOne() {
  return vdupq_n_f32(1.f);
}

/**
 * Loads 4 floats from unaligned memory.
 *
 * \param ptr - Unaligned memory pointer to the 4 floats
 *
 * \return VectorRegister(ptr[0], ptr[1], ptr[2], ptr[3])
 */
FUN_ALWAYS_INLINE VectorRegister VectorLoad(const void* ptr) {
  return vld1q_f32((float32_t*)ptr);
}

/**
 * Loads 3 floats from unaligned memory and leaves w undefined.
 *
 * \param ptr - Unaligned memory pointer to the 3 floats
 *
 * \return VectorRegister(ptr[0], ptr[1], ptr[2], undefined)
 */
#define VectorLoadFloat3(ptr)  MakeVectorRegister(((const float*)(ptr))[0], ((const float*)(ptr))[1], ((const float*)(ptr))[2], 0.f)

/**
 * Loads 3 floats from unaligned memory and sets w=0.
 *
 * \param ptr - Unaligned memory pointer to the 3 floats
 *
 * \return VectorRegister(ptr[0], ptr[1], ptr[2], 0.f)
 */
#define VectorLoadFloat3_W0(ptr)  MakeVectorRegister(((const float*)(ptr))[0], ((const float*)(ptr))[1], ((const float*)(ptr))[2], 0.f)

/**
 * Loads 3 floats from unaligned memory and sets w=1.
 *
 * \param ptr - Unaligned memory pointer to the 3 floats
 *
 * \return VectorRegister(ptr[0], ptr[1], ptr[2], 1.f)
 */
#define VectorLoadFloat3_W1(ptr)  MakeVectorRegister(((const float*)(ptr))[0], ((const float*)(ptr))[1], ((const float*)(ptr))[2], 1.f)

/**
 * Sets a single component of a vector. Must be a define since ElementIndex needs to be a constant integer
 */
#define VectorSetComponent(vec, ElementIndex, scalar)  vsetq_lane_f32(scalar, vec, ElementIndex)

/**
 * Loads 4 floats from aligned memory.
 *
 * \param ptr - Aligned memory pointer to the 4 floats
 *
 * \return VectorRegister(ptr[0], ptr[1], ptr[2], ptr[3])
 */
FUN_ALWAYS_INLINE VectorRegister VectorLoadAligned(const void* ptr) {
  return vld1q_f32((float32_t*)ptr);
}

/**
 * Loads 1 float from unaligned memory and replicates it to all 4 elements.
 *
 * \param ptr - Unaligned memory pointer to the float
 *
 * \return VectorRegister(ptr[0], ptr[0], ptr[0], ptr[0])
 */
FUN_ALWAYS_INLINE VectorRegister VectorLoadFloat1(const void *ptr) {
  return vdupq_n_f32(((float32_t *)ptr)[0]);
}
/**
 * Creates a vector out of three floats and leaves w undefined.
 *
 * \param x - 1st float component
 * \param y - 2nd float component
 * \param z - 3rd float component
 *
 * \return VectorRegister(x, y, z, undefined)
 */
FUN_ALWAYS_INLINE VectorRegister VectorSetFloat3(float x, float y, float z) {
  union { VectorRegister v; float f[4]; } tmp;
  tmp.f[0] = x;
  tmp.f[1] = y;
  tmp.f[2] = z;
  return tmp.v;
}

/**
 * Creates a vector out of four floats.
 *
 * \param x - 1st float component
 * \param y - 2nd float component
 * \param z - 3rd float component
 * \param w - 4th float component
 *
 * \return VectorRegister(x, y, z, w)
 */
FUN_ALWAYS_INLINE VectorRegister VectorSet(float x, float y, float z, float w) {
  return MakeVectorRegister(x, y, z, w);
}

/**
 * Stores a vector to aligned memory.
 *
 * \param vec - vector to store
 * \param ptr - Aligned memory pointer
 */
FUN_ALWAYS_INLINE void VectorStoreAligned(VectorRegister vec, void* ptr) {
  vst1q_f32((float32_t *)ptr, vec);
}

/**
* Same as VectorStoreAligned for Neon.
*
* \param vec - vector to store
* \param ptr - Aligned memory pointer
*/
#define VectorStoreAlignedStreamed(vec, ptr)  VectorStoreAligned(vec, ptr)

/**
 * Stores a vector to memory (aligned or unaligned).
 *
 * \param vec - vector to store
 * \param ptr - Memory pointer
 */
FUN_ALWAYS_INLINE void VectorStore(VectorRegister vec, void* ptr) {
  vst1q_f32((float32_t*)ptr, vec);
}

/**
 * Stores the XYZ components of a vector to unaligned memory.
 *
 * \param vec - vector to store XYZ
 * \param ptr - Unaligned memory pointer
 */
FUN_ALWAYS_INLINE void VectorStoreFloat3(const VectorRegister& vec, void* ptr) {
  vst1q_lane_f32(((float32_t*)ptr) + 0, vec, 0);
  vst1q_lane_f32(((float32_t*)ptr) + 1, vec, 1);
  vst1q_lane_f32(((float32_t*)ptr) + 2, vec, 2);
}

/**
 * Stores the x component of a vector to unaligned memory.
 *
 * \param vec - vector to store x
 * \param ptr - Unaligned memory pointer
 */
FUN_ALWAYS_INLINE void VectorStoreFloat1(VectorRegister vec, void* ptr) {
  vst1q_lane_f32((float32_t*)ptr, vec, 0);
}

/**
 * Replicates one element into all four elements and returns the new vector. Must be a #define for ELementIndex
 * to be a constant integer
 *
 * \param vec - Source vector
 * \param element_index - Index (0-3) of the element to replicate
 *
 * \return VectorRegister(vec[element_index], vec[element_index], vec[element_index], vec[element_index])
 */
#define VectorReplicate(vec, element_index)  vdupq_n_f32(vgetq_lane_f32(vec, element_index))

/**
 * Returns the absolute value (component-wise).
 *
 * \param vec - Source vector
 *
 * \return VectorRegister(abs(vec.x), abs(vec.y), abs(vec.z), abs(vec.w))
 */
FUN_ALWAYS_INLINE VectorRegister VectorAbs(VectorRegister vec) {
  return vabsq_f32(vec);
}

/**
 * Returns the negated value (component-wise).
 *
 * \param vec - Source vector
 *
 * \return VectorRegister(-vec.x, -vec.y, -vec.z, -vec.w)
 */
FUN_ALWAYS_INLINE VectorRegister VectorNegate(VectorRegister vec) {
  return vnegq_f32(vec);
}

/**
 * adds two vectors (component-wise) and returns the result.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x+vec2.x, vec1.y+vec2.y, vec1.z+vec2.z, vec1.w+vec2.w)
 */
FUN_ALWAYS_INLINE VectorRegister VectorAdd(VectorRegister vec1, VectorRegister vec2) {
  return vaddq_f32(vec1, vec2);
}

/**
 * subtracts a vector from another (component-wise) and returns the result.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x-vec2.x, vec1.y-vec2.y, vec1.z-vec2.z, vec1.w-vec2.w)
 */
FUN_ALWAYS_INLINE VectorRegister VectorSubtract(VectorRegister vec1, VectorRegister vec2) {
  return vsubq_f32(vec1, vec2);
}

/**
 * Multiplies two vectors (component-wise) and returns the result.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x*vec2.x, vec1.y*vec2.y, vec1.z*vec2.z, vec1.w*vec2.w)
 */
FUN_ALWAYS_INLINE VectorRegister VectorMultiply(VectorRegister vec1, VectorRegister vec2) {
  return vmulq_f32(vec1, vec2);
}

/**
 * Multiplies two vectors (component-wise), adds in the third vector and returns the result.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 * \param vec3 - 3rd vector
 *
 * \return VectorRegister(vec1.x*vec2.x + vec3.x, vec1.y*vec2.y + vec3.y, vec1.z*vec2.z + vec3.z, vec1.w*vec2.w + vec3.w)
 */
FUN_ALWAYS_INLINE VectorRegister VectorMultiplyAdd(VectorRegister vec1, VectorRegister vec2, VectorRegister vec3) {
  return vmlaq_f32(vec3, vec1, vec2);
}

/**
 * Calculates the dot3 product of two vectors and returns a vector with the result in all 4 components.
 * Only really efficient on Xbox 360.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return d = dot3(vec1.xyz, vec2.xyz), VectorRegister(d, d, d, d)
 */
FUN_ALWAYS_INLINE VectorRegister VectorDot3(const VectorRegister& vec1, const VectorRegister& vec2) {
  VectorRegister tmp = VectorMultiply(vec1, vec2);
  tmp = vsetq_lane_f32(0.f, tmp, 3);
  float32x2_t sum = vpadd_f32(vget_low_f32(tmp), vget_high_f32(tmp));
  sum = vpadd_f32(sum, sum);
  return vdupq_lane_f32(sum, 0);
}

/**
 * Calculates the dot4 product of two vectors and returns a vector with the result in all 4 components.
 * Only really efficient on Xbox 360.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return d = dot4(vec1.xyzw, vec2.xyzw), VectorRegister(d, d, d, d)
 */
FUN_ALWAYS_INLINE VectorRegister VectorDot4(VectorRegister vec1, VectorRegister vec2) {
  VectorRegister tmp = VectorMultiply(vec1, vec2);
  float32x2_t sum = vpadd_f32(vget_low_f32(tmp), vget_high_f32(tmp));
  sum = vpadd_f32(sum, sum);
  return vdupq_lane_f32(sum, 0);
}

/**
 * Creates a four-part mask based on component-wise == compares of the input vectors
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x == vec2.x ? 0xFFFFFFFF : 0, same for yzw)
 */
FUN_ALWAYS_INLINE VectorRegister VectorCompareEQ(const VectorRegister& vec1, const VectorRegister& vec2) {
  return (VectorRegister)vceqq_f32(vec1, vec2);
}

/**
 * Creates a four-part mask based on component-wise != compares of the input vectors
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x != vec2.x ? 0xFFFFFFFF : 0, same for yzw)
 */
FUN_ALWAYS_INLINE VectorRegister VectorCompareNE(const VectorRegister& vec1, const VectorRegister& vec2) {
  return (VectorRegister)vmvnq_u32(vceqq_f32(vec1, vec2));
}

/**
 * Creates a four-part mask based on component-wise > compares of the input vectors
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x > vec2.x ? 0xFFFFFFFF : 0, same for yzw)
 */
FUN_ALWAYS_INLINE VectorRegister VectorCompareGT(const VectorRegister& vec1, const VectorRegister& vec2) {
  return (VectorRegister)vcgtq_f32(vec1, vec2);
}

/**
 * Creates a four-part mask based on component-wise >= compares of the input vectors
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x >= vec2.x ? 0xFFFFFFFF : 0, same for yzw)
 */
FUN_ALWAYS_INLINE VectorRegister VectorCompareGE(const VectorRegister& vec1, const VectorRegister& vec2) {
  return (VectorRegister)vcgeq_f32(vec1, vec2);
}

/**
 * Does a bitwise vector selection based on a mask (e.g., created from VectorCompareXX)
 *
 * \param mask - Mask (when 1: use the corresponding bit from vec1 otherwise from vec2)
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(for each bit i: mask[i] ? vec1[i] : vec2[i])
 */
FUN_ALWAYS_INLINE VectorRegister VectorSelect(const VectorRegister& mask, const VectorRegister& vec1, const VectorRegister& vec2) {
  return vbslq_f32((IntVectorRegister)mask, vec1, vec2);
}

/**
 * Combines two vectors using bitwise OR (treating each vector as a 128 bit field)
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(for each bit i: vec1[i] | vec2[i])
 */
FUN_ALWAYS_INLINE VectorRegister VectorBitwiseOr(const VectorRegister& vec1, const VectorRegister& vec2) {
  return (VectorRegister)vorrq_u32((IntVectorRegister)vec1, (IntVectorRegister)vec2);
}

/**
 * Combines two vectors using bitwise AND (treating each vector as a 128 bit field)
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(for each bit i: vec1[i] & vec2[i])
 */
FUN_ALWAYS_INLINE VectorRegister VectorBitwiseAnd(const VectorRegister& vec1, const VectorRegister& vec2) {
  return (VectorRegister)vandq_u32((IntVectorRegister)vec1, (IntVectorRegister)vec2);
}

/**
 * Combines two vectors using bitwise XOR (treating each vector as a 128 bit field)
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(for each bit i: vec1[i] ^ vec2[i])
 */
FUN_ALWAYS_INLINE VectorRegister VectorBitwiseXor(const VectorRegister& vec1, const VectorRegister& vec2) {
  return (VectorRegister)veorq_u32((IntVectorRegister)vec1, (IntVectorRegister)vec2);
}

/**
 * Swizzles the 4 components of a vector and returns the result.
 *
 * \param vec - Source vector
 * \param x - Index for which component to use for x (literal 0-3)
 * \param y - Index for which component to use for y (literal 0-3)
 * \param z - Index for which component to use for z (literal 0-3)
 * \param w - Index for which component to use for w (literal 0-3)
 *
 * \return The swizzled vector
 */
#define VectorSwizzle(vec, x, y, z, w)  __builtin_shufflevector(vec, vec, x, y, z, w)

/**
 * Creates a vector through selecting two components from each vector via a shuffle mask.
 *
 * \param vec1 - Source vector1
 * \param vec2 - Source vector2
 * \param x - Index for which component of Vector1 to use for x (literal 0-3)
 * \param y - Index for which component to Vector1 to use for y (literal 0-3)
 * \param z - Index for which component to Vector2 to use for z (literal 0-3)
 * \param w - Index for which component to Vector2 to use for w (literal 0-3)
 *
 * \return The swizzled vector
 */
#define VectorShuffle(vec1, vec2, x, y, z, w)  __builtin_shufflevector(vec1, vec2, x, y, z, w)

/**
 * Calculates the cross product of two vectors (XYZ components). w is set to 0.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return cross(vec1.xyz, vec2.xyz). w is set to 0.
 */
FUN_ALWAYS_INLINE VectorRegister VectorCross(const VectorRegister& vec1, const VectorRegister& vec2) {
  VectorRegister cross = VectorSubtract(VectorMultiply(VectorSwizzle(vec1, 1, 2, 0, 1), VectorSwizzle(vec2, 2, 0, 1, 3)), VectorMultiply(VectorSwizzle(vec1, 2, 0, 1, 3), VectorSwizzle(vec2, 1, 2, 0, 1)));
  cross = VectorSetComponent(cross, 3, 0.f);
  return cross;
}

/**
 * Calculates x raised to the power of y (component-wise).
 *
 * \param base - Base vector
 * \param exponent - Exponent vector
 *
 * \return VectorRegister(base.x^exponent.x, base.y^exponent.y, base.z^exponent.z, base.w^exponent.w)
 */
FUN_ALWAYS_INLINE VectorRegister VectorPow(const VectorRegister& base, const VectorRegister& exponent) {
  //@TODO: Optimize this
  union { VectorRegister v; float f[4]; } b, e;
  b.v = base;
  e.v = exponent;
  return MakeVectorRegister(powf(b.f[0], e.f[0]), powf(b.f[1], e.f[1]), powf(b.f[2], e.f[2]), powf(b.f[3], e.f[3]));
}

/**
* Returns an estimate of 1/sqrt(c) for each component of the vector
*
* \param vec - vector
*
* \return VectorRegister(1/sqrt(t), 1/sqrt(t), 1/sqrt(t), 1/sqrt(t))
*/
#define VectorReciprocalSqrt(vec)  vrsqrteq_f32(vec)

/**
 * Computes an estimate of the reciprocal of a vector (component-wise) and returns the result.
 *
 * \param vec - 1st vector
 *
 * \return VectorRegister((Estimate) 1.f / vec.x, (Estimate) 1.f / vec.y, (Estimate) 1.f / vec.z, (Estimate) 1.f / vec.w)
 */
#define VectorReciprocal(vec)  vrecpeq_f32(vec)


/**
 * Return reciprocal Length of the vector
 *
 * \param vec - vector
 *
 * \return VectorRegister(rlen, rlen, rlen, rlen) when rlen = 1/sqrt(dot4(v))
 */
FUN_ALWAYS_INLINE VectorRegister VectorReciprocalLen(const VectorRegister& vector) {
  VectorRegister length_squared = VectorDot4(vector, vector);
  return VectorReciprocalSqrt(length_squared);
}

/**
 * Return the reciprocal of the square root of each component
 *
 * \param vec - vector
 *
 * \return VectorRegister(1/sqrt(vec.x), 1/sqrt(vec.y), 1/sqrt(vec.z), 1/sqrt(vec.w))
 */
FUN_ALWAYS_INLINE VectorRegister VectorReciprocalSqrtAccurate(const VectorRegister& vec) {
  // Perform a single pass of Newton-Raphson iteration on the hardware estimate
  // This is a builtin instruction (VRSQRTS)

  // Initial estimate
  VectorRegister recip_sqrt = VectorReciprocalSqrt(vec);

  // Two refinement
  recip_sqrt = vmulq_f32(vrsqrtsq_f32(vec, vmulq_f32(recip_sqrt, recip_sqrt)), recip_sqrt);
  return vmulq_f32(vrsqrtsq_f32(vec, vmulq_f32(recip_sqrt, recip_sqrt)), recip_sqrt);
}

/**
 * Computes the reciprocal of a vector (component-wise) and returns the result.
 *
 * \param vec - 1st vector
 *
 * \return VectorRegister(1.f / vec.x, 1.f / vec.y, 1.f / vec.z, 1.f / vec.w)
 */
FUN_ALWAYS_INLINE VectorRegister VectorReciprocalAccurate(const VectorRegister& vec) {
  // Perform two passes of Newton-Raphson iteration on the hardware estimate
  // This is a built-in instruction (VRECPS)

  // Initial estimate
  VectorRegister reciprocal = VectorReciprocal(vec);

  // 2 refinement iterations
  reciprocal = vmulq_f32(vrecpsq_f32(vec, reciprocal), reciprocal);
  return vmulq_f32(vrecpsq_f32(vec, reciprocal), reciprocal);
}

/**
 * Divides two vectors (component-wise) and returns the result.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x/vec2.x, vec1.y/vec2.y, vec1.z/vec2.z, vec1.w/vec2.w)
 */
FUN_ALWAYS_INLINE VectorRegister VectorDivide(VectorRegister vec1, VectorRegister vec2) {
  VectorRegister x = VectorReciprocalAccurate(vec2);
  return VectorMultiply(vec1, x);
}

/**
 * Normalize vector
 *
 * \param vec - vector to normalize
 *
 * \return Normalized VectorRegister
 */
FUN_ALWAYS_INLINE VectorRegister VectorNormalize(const VectorRegister& vec) {
  return VectorMultiply(vector, VectorReciprocalLen(vec));
}

/**
 * Loads XYZ and sets w=0
 *
 * \param vec - VectorRegister
 *
 * \return VectorRegister(x, y, z, 0.f)
 */
#define VectorSet_W0(vec)  VectorSetComponent(vec, 3, 0.f)

/**
 * Loads XYZ and sets w=1.
 *
 * \param vec - VectorRegister
 *
 * \return VectorRegister(x, y, z, 1.f)
 */
#define VectorSet_W1(vec)  VectorSetComponent(vec, 3, 1.f)

/**
 * Returns a component from a vector.
 *
 * \param vec - vector register
 * \param component_index - Which component to get, x=0, y=1, z=2, w=3
 *
 * \return The component as a float
 */
FUN_ALWAYS_INLINE float VectorGetComponent(VectorRegister vec, uint32 component_index) {
  float tmp[4];
  VectorStore(vec, tmp);
  return tmp[component_index];
}

/**
 * Multiplies two 4x4 matrices.
 *
 * \param result - Pointer to where the result should be stored
 * \param matrix1 - Pointer to the first matrix
 * \param matrix2 - Pointer to the second matrix
 */
FUN_ALWAYS_INLINE void VectorMatrixMultiply(void* result, const void* matrix1, const void* matrix2) {
  const VectorRegister* a = (const VectorRegister*) matrix1;
  const VectorRegister* b = (const VectorRegister*) matrix2;
  VectorRegister* r = (VectorRegister*) result;
  VectorRegister tmp, r0, r1, r2, r3;

  // First row of result (matrix1[0] * matrix2).
  tmp = vmulq_lane_f32(b[0], vget_low_f32(a[0]), 0);
  tmp = vmlaq_lane_f32(tmp, b[1], vget_low_f32(a[0]), 1);
  tmp = vmlaq_lane_f32(tmp, b[2], vget_high_f32(a[0]), 0);
  r0 = vmlaq_lane_f32(tmp, b[3], vget_high_f32(a[0]), 1);

  // Second row of result (matrix1[1] * matrix2).
  tmp = vmulq_lane_f32(b[0], vget_low_f32(a[1]), 0);
  tmp = vmlaq_lane_f32(tmp, b[1], vget_low_f32(a[1]), 1);
  tmp = vmlaq_lane_f32(tmp, b[2], vget_high_f32(a[1]), 0);
  r1 = vmlaq_lane_f32(tmp, b[3], vget_high_f32(a[1]), 1);

  // Third row of result (matrix1[2] * matrix2).
  tmp = vmulq_lane_f32(b[0], vget_low_f32(a[2]), 0);
  tmp = vmlaq_lane_f32(tmp, b[1], vget_low_f32(a[2]), 1);
  tmp = vmlaq_lane_f32(tmp, b[2], vget_high_f32(a[2]), 0);
  r2 = vmlaq_lane_f32(tmp, b[3], vget_high_f32(a[2]), 1);

  // Fourth row of result (matrix1[3] * matrix2).
  tmp = vmulq_lane_f32(b[0], vget_low_f32(a[3]), 0);
  tmp = vmlaq_lane_f32(tmp, b[1], vget_low_f32(a[3]), 1);
  tmp = vmlaq_lane_f32(tmp, b[2], vget_high_f32(a[3]), 0);
  r3 = vmlaq_lane_f32(tmp, b[3], vget_high_f32(a[3]), 1);

  // Store result
  r[0] = r0;
  r[1] = r1;
  r[2] = r2;
  r[3] = r3;
}

/**
 * Calculate the inverse of an Matrix.
 *
 * \param dst_matrix - Matrix pointer to where the result should be stored
 * \param src_matrix - Matrix pointer to the matrix to be inversed
 */
FUN_ALWAYS_INLINE void VectorMatrixInverse(void *dst_matrix, const void *src_matrix) {
  typedef float Float4x4[4][4];
  const Float4x4& m = *((const Float4x4*) src_matrix);
  Float4x4 result;
  float det[4];
  Float4x4 tmp;

  tmp[0][0] = m[2][2] * m[3][3] - m[2][3] * m[3][2];
  tmp[0][1] = m[1][2] * m[3][3] - m[1][3] * m[3][2];
  tmp[0][2] = m[1][2] * m[2][3] - m[1][3] * m[2][2];

  tmp[1][0] = m[2][2] * m[3][3] - m[2][3] * m[3][2];
  tmp[1][1] = m[0][2] * m[3][3] - m[0][3] * m[3][2];
  tmp[1][2] = m[0][2] * m[2][3] - m[0][3] * m[2][2];

  tmp[2][0] = m[1][2] * m[3][3] - m[1][3] * m[3][2];
  tmp[2][1] = m[0][2] * m[3][3] - m[0][3] * m[3][2];
  tmp[2][2] = m[0][2] * m[1][3] - m[0][3] * m[1][2];

  tmp[3][0] = m[1][2] * m[2][3] - m[1][3] * m[2][2];
  tmp[3][1] = m[0][2] * m[2][3] - m[0][3] * m[2][2];
  tmp[3][2] = m[0][2] * m[1][3] - m[0][3] * m[1][2];

  det[0] = m[1][1]*tmp[0][0] - m[2][1]*tmp[0][1] + m[3][1]*tmp[0][2];
  det[1] = m[0][1]*tmp[1][0] - m[2][1]*tmp[1][1] + m[3][1]*tmp[1][2];
  det[2] = m[0][1]*tmp[2][0] - m[1][1]*tmp[2][1] + m[3][1]*tmp[2][2];
  det[3] = m[0][1]*tmp[3][0] - m[1][1]*tmp[3][1] + m[2][1]*tmp[3][2];

  float determinant = m[0][0]*det[0] - m[1][0]*det[1] + m[2][0]*det[2] - m[3][0]*det[3];
  const float inv_det = 1.f / determinant;

  result[0][0] =  inv_det * det[0];
  result[0][1] = -inv_det * det[1];
  result[0][2] =  inv_det * det[2];
  result[0][3] = -inv_det * det[3];
  result[1][0] = -inv_det * (m[1][0]*tmp[0][0] - m[2][0]*tmp[0][1] + m[3][0]*tmp[0][2]);
  result[1][1] =  inv_det * (m[0][0]*tmp[1][0] - m[2][0]*tmp[1][1] + m[3][0]*tmp[1][2]);
  result[1][2] = -inv_det * (m[0][0]*tmp[2][0] - m[1][0]*tmp[2][1] + m[3][0]*tmp[2][2]);
  result[1][3] =  inv_det * (m[0][0]*tmp[3][0] - m[1][0]*tmp[3][1] + m[2][0]*tmp[3][2]);
  result[2][0] =  inv_det * (
              m[1][0] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) -
              m[2][0] * (m[1][1] * m[3][3] - m[1][3] * m[3][1]) +
              m[3][0] * (m[1][1] * m[2][3] - m[1][3] * m[2][1])
              );
  result[2][1] = -inv_det * (
              m[0][0] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) -
              m[2][0] * (m[0][1] * m[3][3] - m[0][3] * m[3][1]) +
              m[3][0] * (m[0][1] * m[2][3] - m[0][3] * m[2][1])
              );
  result[2][2] =  inv_det * (
              m[0][0] * (m[1][1] * m[3][3] - m[1][3] * m[3][1]) -
              m[1][0] * (m[0][1] * m[3][3] - m[0][3] * m[3][1]) +
              m[3][0] * (m[0][1] * m[1][3] - m[0][3] * m[1][1])
              );
  result[2][3] = -inv_det * (
              m[0][0] * (m[1][1] * m[2][3] - m[1][3] * m[2][1]) -
              m[1][0] * (m[0][1] * m[2][3] - m[0][3] * m[2][1]) +
              m[2][0] * (m[0][1] * m[1][3] - m[0][3] * m[1][1])
              );
  result[3][0] = -inv_det * (
              m[1][0] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]) -
              m[2][0] * (m[1][1] * m[3][2] - m[1][2] * m[3][1]) +
              m[3][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
              );
  result[3][1] =  inv_det * (
              m[0][0] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]) -
              m[2][0] * (m[0][1] * m[3][2] - m[0][2] * m[3][1]) +
              m[3][0] * (m[0][1] * m[2][2] - m[0][2] * m[2][1])
              );
  result[3][2] = -inv_det * (
              m[0][0] * (m[1][1] * m[3][2] - m[1][2] * m[3][1]) -
              m[1][0] * (m[0][1] * m[3][2] - m[0][2] * m[3][1]) +
              m[3][0] * (m[0][1] * m[1][2] - m[0][2] * m[1][1])
              );
  result[3][3] =  inv_det * (
              m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
              m[1][0] * (m[0][1] * m[2][2] - m[0][2] * m[2][1]) +
              m[2][0] * (m[0][1] * m[1][2] - m[0][2] * m[1][1])
              );

  UnsafeMemory::Memcpy(dst_matrix, &result, 16*sizeof(float));
}

/**
 * Calculate Homogeneous transform.
 *
 * \param vec_p - VectorRegister
 * \param matrix_m - Matrix pointer to the matrix to apply transform
 *
 * \return VectorRegister = vec_p*matrix_m
 */
FUN_ALWAYS_INLINE VectorRegister VectorTransformVector(const VectorRegister& vec_p, const void* matrix_m) {
  const VectorRegister* m = (const VectorRegister*)matrix_m;
  VectorRegister v_tmp_x, v_tmp_y, v_tmp_z, v_tmp_w;

  // Splat x, y, z and w
  v_tmp_x = VectorReplicate(vec_p, 0);
  v_tmp_y = VectorReplicate(vec_p, 1);
  v_tmp_z = VectorReplicate(vec_p, 2);
  v_tmp_w = VectorReplicate(vec_p, 3);
  // Mul by the matrix
  v_tmp_x = VectorMultiply(v_tmp_x, m[0]);
  v_tmp_y = VectorMultiply(v_tmp_y, m[1]);
  v_tmp_z = VectorMultiply(v_tmp_z, m[2]);
  v_tmp_w = VectorMultiply(v_tmp_w, m[3]);
  // Add them all together
  v_tmp_x = VectorAdd(v_tmp_x, v_tmp_y);
  v_tmp_z = VectorAdd(v_tmp_z, v_tmp_w);
  v_tmp_x = VectorAdd(v_tmp_x, v_tmp_z);

  return v_tmp_x;
}

/**
 * Returns the minimum values of two vectors (component-wise).
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(min(vec1.x, vec2.x), min(vec1.y, vec2.y), min(vec1.z, vec2.z), min(vec1.w, vec2.w))
 */
FUN_ALWAYS_INLINE VectorRegister VectorMin(VectorRegister vec1, VectorRegister vec2) {
  return vminq_f32(vec1, vec2);
}

/**
 * Returns the maximum values of two vectors (component-wise).
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(max(vec1.x, vec2.x), max(vec1.y, vec2.y), max(vec1.z, vec2.z), max(vec1.w, vec2.w))
 */
FUN_ALWAYS_INLINE VectorRegister VectorMax(VectorRegister vec1, VectorRegister vec2) {
  return vmaxq_f32(vec1, vec2);
}

/**
 * Merges the XYZ components of one vector with the w component of another vector and returns the result.
 *
 * \param vec_xyz - Source vector for XYZ_
 * \param vec_w - Source register for ___W (note: the fourth component is used, not the first)
 *
 * \return VectorRegister(vec_xyz.x, vec_xyz.y, vec_xyz.z, vec_w.w)
 */
FUN_ALWAYS_INLINE VectorRegister VectorMergeVecXYZ_VecW(const VectorRegister& vec_xyz, const VectorRegister& vec_w) {
  return vsetq_lane_f32(vgetq_lane_f32(vec_w, 3), vec_xyz, 3);
}

/**
 * Loads 4 uint8s from unaligned memory and converts them into 4 floats.
 * IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar floats after you've used this intrinsic!
 *
 * \param ptr - Unaligned memory pointer to the 4 uint8s.
 *
 * \return VectorRegister(float(ptr[0]), float(ptr[1]), float(ptr[2]), float(ptr[3]))
 */
FUN_ALWAYS_INLINE VectorRegister VectorLoadByte4(const void* ptr) {
  // OPTIMIZE ME!
  const uint8* p = (const uint8*)ptr;
  return MakeVectorRegister((float)p[0], (float)p[1], (float)p[2], (float)p[3]);
}

/**
 * Loads 4 uint8s from unaligned memory and converts them into 4 floats in reversed order.
 * IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar floats after you've used this intrinsic!
 *
 * \param ptr - Unaligned memory pointer to the 4 uint8s.
 *
 * \return VectorRegister(float(ptr[3]), float(ptr[2]), float(ptr[1]), float(ptr[0]))
 */
FUN_ALWAYS_INLINE VectorRegister VectorLoadByte4Reverse(const void* ptr) {
  // OPTIMIZE ME!
  const uint8* p = (const uint8*)ptr;
  return MakeVectorRegister((float)p[3], (float)p[2], (float)p[1], (float)p[0]);
}

/**
 * Converts the 4 floats in the vector to 4 uint8s, clamped to [0, 255], and stores to unaligned memory.
 * IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar floats after you've used this intrinsic!
 *
 * \param vec - vector containing 4 floats
 * \param ptr - Unaligned memory pointer to store the 4 uint8s.
 */
FUN_ALWAYS_INLINE void VectorStoreByte4(VectorRegister vec, void* ptr) {
  uint16x8_t u16x8 = (uint16x8_t)vcvtq_u32_f32(VectorMin(vec, VectorizeConstants::Float255));
  uint8x8_t u8x8 = (uint8x8_t)vget_low_u16(vuzpq_u16(u16x8, u16x8).val[0]);
  u8x8 = vuzp_u8(u8x8, u8x8).val[0];
  uint32_t buf[2];
  vst1_u8((uint8_t *)buf, u8x8);
  *(uint32_t *)ptr = buf[0];
}

/**
 * Converts the 4 floats in the vector to 4 fp16 and stores based off bool to [un]aligned memory.
 *
 * \param vec - vector containing 4 floats
 * \param ptr - Memory pointer to store the 4 fp16's.
 */
template <bool IsAligned>
FUN_ALWAYS_INLINE void VectorStoreHalf4(VectorRegister vec, void* __restrict ptr) {
  float16x4_t f16x4 = (float16x4_t)vcvt_f16_f32(vec);
  if (IsAligned) {
    vst1_u8((uint8_t *)ptr, f16x4);
  }
  else {
    uint32_t buf[2];
    vst1_u8((uint8_t *)buf, f16x4);
    *(uint32_t *)ptr = buf[0];
  }
}

/**
 * Returns non-zero if any element in vec1 is greater than the corresponding element in vec2, otherwise 0.
 *
 * \param vec1 - 1st source vector
 * \param vec2 - 2nd source vector
 *
 * \return Non-zero integer if (vec1.x > vec2.x) || (vec1.y > vec2.y) || (vec1.z > vec2.z) || (vec1.w > vec2.w)
 */
FUN_ALWAYS_INLINE int32 VectorAnyGreaterThan(VectorRegister vec1, VectorRegister vec2) {
  uint16x8_t u16x8 = (uint16x8_t)vcgtq_f32(vec1, vec2);
  uint8x8_t u8x8 = (uint8x8_t)vget_low_u16(vuzpq_u16(u16x8, u16x8).val[0]);
  u8x8 = vuzp_u8(u8x8, u8x8).val[0];
  uint32_t buf[2];
  vst1_u8((uint8_t *)buf, u8x8);
  return (int32)buf[0]; // each byte of output corresponds to a component comparison
}

/**
 * Resets the floating point registers so that they can be used again.
 * Some intrinsics use these for MMX purposes (e.g. VectorLoadByte4 and VectorStoreByte4).
 */
#define VectorResetFloatRegisters()

/**
 * Returns the control register.
 *
 * \return The uint32 control register
 */
#define VectorGetControlRegister()  0

/**
 * Sets the control register.
 *
 * \param ControlStatus - The uint32 control status value to set
 */
#define VectorSetControlRegister(ControlStatus)

/**
 * Control status bit to round all floating point math results towards zero.
 */
#define VECTOR_ROUND_TOWARD_ZERO  0

static const VectorRegister QMULTI_SIGN_MASK0 = MakeVectorRegister(1.f, -1.f, 1.f, -1.f);
static const VectorRegister QMULTI_SIGN_MASK1 = MakeVectorRegister(1.f, 1.f, -1.f, -1.f);
static const VectorRegister QMULTI_SIGN_MASK2 = MakeVectorRegister(-1.f, 1.f, 1.f, -1.f);

/**
* Multiplies two quaternions; the order matters.
*
* Order matters when composing quaternions: C = VectorQuaternionMultiply2(A, B) will yield a quaternion C = A * B
* that logically first applies B then A to any subsequent transformation (right first, then left).
*
* \param quat1 - Pointer to the first quaternion
* \param quat2 - Pointer to the second quaternion
*
* \return quat1 * quat2
*/
FUN_ALWAYS_INLINE VectorRegister VectorQuaternionMultiply2(const VectorRegister& quat1, const VectorRegister& quat2) {
  VectorRegister result = VectorMultiply(VectorReplicate(quat1, 3), quat2);
  result = VectorMultiplyAdd(VectorMultiply(VectorReplicate(quat1, 0), VectorSwizzle(quat2, 3, 2, 1, 0)), QMULTI_SIGN_MASK0, result);
  result = VectorMultiplyAdd(VectorMultiply(VectorReplicate(quat1, 1), VectorSwizzle(quat2, 2, 3, 0, 1)), QMULTI_SIGN_MASK1, result);
  result = VectorMultiplyAdd(VectorMultiply(VectorReplicate(quat1, 2), VectorSwizzle(quat2, 1, 0, 3, 2)), QMULTI_SIGN_MASK2, result);
  return result;
}

/**
 * Multiplies two quaternions; the order matters.
 *
 * When composing quaternions: VectorQuaternionMultiply(C, A, B) will yield a quaternion C = A * B
 * that logically first applies B then A to any subsequent transformation (right first, then left).
 *
 * \param result - Pointer to where the result quat1 * quat2 should be stored
 * \param quat1 - Pointer to the first quaternion (must not be the destination)
 * \param quat2 - Pointer to the second quaternion (must not be the destination)
 */
FUN_ALWAYS_INLINE void VectorQuaternionMultiply(void* __restrict result, const void* __restrict quat1, const void* __restrict quat2) {
  *((VectorRegister*)result) = VectorQuaternionMultiply2(*((const VectorRegister*)quat1), *((const VectorRegister*)quat2));
}

/**
 * Computes the sine and cosine of each component of a vector.
 *
 * \param vec_sin_angles - VectorRegister Pointer to where the Sin result should be stored
 * \param vec_cos_angles - VectorRegister Pointer to where the Cos result should be stored
 * \param vec_angles - VectorRegister Pointer to the input angles
 */
FUN_ALWAYS_INLINE void VectorSinCos(VectorRegister* vec_sin_angles, VectorRegister* vec_cos_angles, const VectorRegister* vec_angles) {
  union { VectorRegister v; float f[4]; } vec_sin, vec_cos, vec_angles;
  vec_angles.v = *vec_angles;

  Math::SinCos(&vec_sin.f[0], &vec_cos.f[0], vec_angles.f[0]);
  Math::SinCos(&vec_sin.f[1], &vec_cos.f[1], vec_angles.f[1]);
  Math::SinCos(&vec_sin.f[2], &vec_cos.f[2], vec_angles.f[2]);
  Math::SinCos(&vec_sin.f[3], &vec_cos.f[3], vec_angles.f[3]);

  *vec_sin_angles = vec_sin.v;
  *vec_cos_angles = vec_cos.v;
}

// Returns true if the vector contains a component that is either NAN or +/-infinite.
FUN_ALWAYS_INLINE bool VectorContainsNaNOrInfinite(const VectorRegister& vec) {
  fun_check_msg(false, "Not implemented for NEON"); //@TODO: Implement this method for NEON
  return false;
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorExp(const VectorRegister& x) {
  return MakeVectorRegister(Math::Exp(VectorGetComponent(x, 0)), Math::Exp(VectorGetComponent(x, 1)), Math::Exp(VectorGetComponent(x, 2)), Math::Exp(VectorGetComponent(x, 3)));
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorExp2(const VectorRegister& x) {
  return MakeVectorRegister(Math::Exp2(VectorGetComponent(x, 0)), Math::Exp2(VectorGetComponent(x, 1)), Math::Exp2(VectorGetComponent(x, 2)), Math::Exp2(VectorGetComponent(x, 3)));
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorLog(const VectorRegister& x) {
  return MakeVectorRegister(Math::Loge(VectorGetComponent(x, 0)), Math::Loge(VectorGetComponent(x, 1)), Math::Loge(VectorGetComponent(x, 2)), Math::Loge(VectorGetComponent(x, 3)));
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorLog2(const VectorRegister& x) {
  return MakeVectorRegister(Math::Log2(VectorGetComponent(x, 0)), Math::Log2(VectorGetComponent(x, 1)), Math::Log2(VectorGetComponent(x, 2)), Math::Log2(VectorGetComponent(x, 3)));
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorSin(const VectorRegister& x) {
  return MakeVectorRegister(Math::Sin(VectorGetComponent(x, 0)), Math::Sin(VectorGetComponent(x, 1)), Math::Sin(VectorGetComponent(x, 2)), Math::Sin(VectorGetComponent(x, 3)));
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorCos(const VectorRegister& x) {
  return MakeVectorRegister(Math::Cos(VectorGetComponent(x, 0)), Math::Cos(VectorGetComponent(x, 1)), Math::Cos(VectorGetComponent(x, 2)), Math::Cos(VectorGetComponent(x, 3)));
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorTan(const VectorRegister& x) {
  return MakeVectorRegister(Math::Tan(VectorGetComponent(x, 0)), Math::Tan(VectorGetComponent(x, 1)), Math::Tan(VectorGetComponent(x, 2)), Math::Tan(VectorGetComponent(x, 3)));
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorASin(const VectorRegister& x) {
  return MakeVectorRegister(Math::Asin(VectorGetComponent(x, 0)), Math::Asin(VectorGetComponent(x, 1)), Math::Asin(VectorGetComponent(x, 2)), Math::Asin(VectorGetComponent(x, 3)));
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorACos(const VectorRegister& x) {
  return MakeVectorRegister(Math::Acos(VectorGetComponent(x, 0)), Math::Acos(VectorGetComponent(x, 1)), Math::Acos(VectorGetComponent(x, 2)), Math::Acos(VectorGetComponent(x, 3)));
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorATan(const VectorRegister& x) {
  return MakeVectorRegister(Math::Atan(VectorGetComponent(x, 0)), Math::Atan(VectorGetComponent(x, 1)), Math::Atan(VectorGetComponent(x, 2)), Math::Atan(VectorGetComponent(x, 3)));
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorATan2(const VectorRegister& x, const VectorRegister& y) {
  return MakeVectorRegister(Math::Atan2(VectorGetComponent(x, 0), VectorGetComponent(y, 0)),
                            Math::Atan2(VectorGetComponent(x, 1), VectorGetComponent(y, 1)),
                            Math::Atan2(VectorGetComponent(x, 2), VectorGetComponent(y, 2)),
                            Math::Atan2(VectorGetComponent(x, 3), VectorGetComponent(y, 3)));
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorCeil(const VectorRegister& x) {
  return MakeVectorRegister(Math::CeilToFloat(VectorGetComponent(x, 0)), Math::CeilToFloat(VectorGetComponent(x, 1)), Math::CeilToFloat(VectorGetComponent(x, 2)), Math::CeilToFloat(VectorGetComponent(x, 3)));
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorFloor(const VectorRegister& x) {
  return MakeVectorRegister(Math::FloorToFloat(VectorGetComponent(x, 0)), Math::FloorToFloat(VectorGetComponent(x, 1)), Math::FloorToFloat(VectorGetComponent(x, 2)), Math::FloorToFloat(VectorGetComponent(x, 3)));
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorTruncate(const VectorRegister& x) {
  return MakeVectorRegister(Math::TruncToFloat(VectorGetComponent(x, 0)), Math::TruncToFloat(VectorGetComponent(x, 1)), Math::TruncToFloat(VectorGetComponent(x, 2)), Math::TruncToFloat(VectorGetComponent(x, 3)));
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorFractional(const VectorRegister& x) {
  return VectorSubtract(x, VectorTruncate(x));
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorMod(const VectorRegister& x, const VectorRegister& y) {
  return MakeVectorRegister(Math::Fmod(VectorGetComponent(x, 0), VectorGetComponent(y, 0)),
                            Math::Fmod(VectorGetComponent(x, 1), VectorGetComponent(y, 1)),
                            Math::Fmod(VectorGetComponent(x, 2), VectorGetComponent(y, 2)),
                            Math::Fmod(VectorGetComponent(x, 3), VectorGetComponent(y, 3)));
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorSign(const VectorRegister& x) {
  return MakeVectorRegister(
    (float)(VectorGetComponent(x, 0) >= 0.f ? 1.f : 0.f),
    (float)(VectorGetComponent(x, 1) >= 0.f ? 1.f : 0.f),
    (float)(VectorGetComponent(x, 2) >= 0.f ? 1.f : 0.f),
    (float)(VectorGetComponent(x, 3) >= 0.f ? 1.f : 0.f));
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorStep(const VectorRegister& x) {
  return MakeVectorRegister(
    (float)(VectorGetComponent(x, 0) >= 0.f ? 1.f : -1.f),
    (float)(VectorGetComponent(x, 1) >= 0.f ? 1.f : -1.f),
    (float)(VectorGetComponent(x, 2) >= 0.f ? 1.f : -1.f),
    (float)(VectorGetComponent(x, 3) >= 0.f ? 1.f : -1.f));
}

// to be continued...

} // namespace fun
