#pragma once

#include "HAL/FunMemory.h"

#if __cplusplus_cli

// there are compile issues with this file in managed mode,
// so use the FPU version
#include "fun/base/math/vectorize_fpu.h"

#else

// We require SSE2
#include <emmintrin.h>

namespace fun {

//
// Helpers:
//

/**
 * float4 vector register type, where the first float (x)
 * is stored in the lowest 32 bits, and so on.
 */
typedef __m128  VectorRegister;
typedef __m128i VectorRegisterInt;

// for an __m128, we need a single set of braces (for clang)
#define DECLARE_VECTOR_REGISTER(x, y, z, w)  { x, y, z, w }

/**
 * \param a0 - Selects which element (0-3) from 'a' into 1st slot in the result
 * \param a1 - Selects which element (0-3) from 'a' into 2nd slot in the result
 * \param b2 - Selects which element (0-3) from 'b' into 3rd slot in the result
 * \param b3 - Selects which element (0-3) from 'b' into 4th slot in the result
 */
#define SHUFFLEMASK(a0, a1, b2, b3)  ((a0) | ((a1)<<2) | ((b2)<<4) | ((b3)<<6))

/**
 * Returns a bitwise equivalent vector based on 4 DWORDs.
 *
 * \param x - 1st uint32 component
 * \param y - 2nd uint32 component
 * \param z - 3rd uint32 component
 * \param w - 4th uint32 component
 *
 * \return Bitwise equivalent vector with 4 floats
 */
FUN_ALWAYS_INLINE VectorRegister MakeVectorRegister(uint32 x, uint32 y, uint32 z, uint32 w) {
  union { VectorRegister v; VectorRegisterInt i; } tmp;
  tmp.i = _mm_setr_epi32(x, y, z, w);
  return tmp.v;
}

/**
 * Returns a vector based on 4 FLOATs.
 *
 * \param x - 1st float component
 * \param y - 2nd float component
 * \param z - 3rd float component
 * \param w - 4th float component
 *
 * \return vector of the 4 FLOATs
 */
FUN_ALWAYS_INLINE VectorRegister MakeVectorRegister(float x, float y, float z, float w) {
  return _mm_setr_ps(x, y, z, w);
}

//
// Constants:
//

#include "vectorize_constants.h"

//
// Intrinsics:
//

/**
 * Returns a vector with all zeros.
 *
 * \return VectorRegister(0.f, 0.f, 0.f, 0.f)
 */
#define VectorZero()  _mm_setzero_ps()

/**
 * Returns a vector with all ones.
 *
 * \return VectorRegister(1.f, 1.f, 1.f, 1.f)
 */
#define VectorOne()  (VectorizeConstants::FloatOne)

/**
 * Returns an component from a vector.
 *
 * \param vec - vector register
 * \param component_index - Which component to get, x=0, y=1, z=2, w=3
 *
 * \return - The component as a float
 */
FUN_ALWAYS_INLINE float VectorGetComponent(VectorRegister vec, uint32 component_index) {
  return (((float*)&(vec))[component_index]);
}

/**
 * Loads 4 FLOATs from unaligned memory.
 *
 * \param ptr - Unaligned memory pointer to the 4 FLOATs
 *
 * \return VectorRegister(ptr[0], ptr[1], ptr[2], ptr[3])
 */
#define VectorLoad(ptr)  _mm_loadu_ps((float*)(ptr))

/**
 * Loads 3 FLOATs from unaligned memory and leaves w undefined.
 *
 * \param ptr - Unaligned memory pointer to the 3 FLOATs
 *
 * \return VectorRegister(ptr[0], ptr[1], ptr[2], undefined)
 */
#define VectorLoadFloat3(ptr)  MakeVectorRegister(((const float*)(ptr))[0], ((const float*)(ptr))[1], ((const float*)(ptr))[2], 0.f)

/**
 * Loads 3 FLOATs from unaligned memory and sets w=0.
 *
 * \param ptr - Unaligned memory pointer to the 3 FLOATs
 *
 * \return VectorRegister(ptr[0], ptr[1], ptr[2], 0.f)
 */
#define VectorLoadFloat3_W0(ptr)  MakeVectorRegister(((const float*)(ptr))[0], ((const float*)(ptr))[1], ((const float*)(ptr))[2], 0.f)

/**
 * Loads 3 FLOATs from unaligned memory and sets w=1.
 *
 * \param ptr - Unaligned memory pointer to the 3 FLOATs
 *
 * \return VectorRegister(ptr[0], ptr[1], ptr[2], 1.f)
 */
#define VectorLoadFloat3_W1(ptr)  MakeVectorRegister(((const float*)(ptr))[0], ((const float*)(ptr))[1], ((const float*)(ptr))[2], 1.f)

/**
 * Loads 4 FLOATs from aligned memory.
 *
 * \param ptr - Aligned memory pointer to the 4 FLOATs
 *
 * \return VectorRegister(ptr[0], ptr[1], ptr[2], ptr[3])
 */
#define VectorLoadAligned(ptr)  _mm_load_ps((float*)(ptr))

/**
 * Loads 1 float from unaligned memory and replicates it to all 4 elements.
 *
 * \param ptr - Unaligned memory pointer to the float
 *
 * \return VectorRegister(ptr[0], ptr[0], ptr[0], ptr[0])
 */
#define VectorLoadFloat1(ptr)  _mm_load1_ps((float*)(ptr))

/**
 * Creates a vector out of three FLOATs and leaves w undefined.
 *
 * \param x - 1st float component
 * \param y - 2nd float component
 * \param z - 3rd float component
 *
 * \return VectorRegister(x, y, z, undefined)
 */
#define VectorSetFloat3(x, y, z)  MakeVectorRegister(x, y, z, 0.f)

/**
 * Propagates passed in float to all registers
 *
 * \param f - Float to set
 *
 * \return VectorRegister(f, f, f, f)
 */
#define VectorSetFloat1(f)  _mm_set1_ps(f)

/**
 * Creates a vector out of four FLOATs.
 *
 * \param x - 1st float component
 * \param y - 2nd float component
 * \param z - 3rd float component
 * \param w - 4th float component
 *
 * \return VectorRegister(x, y, z, w)
 */
#define VectorSet(x, y, z, w)  MakeVectorRegister(x, y, z, w)

/**
 * Stores a vector to aligned memory.
 *
 * \param vec - vector to store
 * \param ptr - Aligned memory pointer
 */
#define VectorStoreAligned(vec, ptr)  _mm_store_ps((float*)(ptr), vec)

/**
 * Performs non-temporal store of a vector to aligned memory without polluting the caches
 *
 * \param vec - vector to store
 * \param ptr - Aligned memory pointer
 */
#define VectorStoreAlignedStreamed(vec, ptr)  _mm_stream_ps((float*)(ptr), vec)

/**
 * Stores a vector to memory (aligned or unaligned).
 *
 * \param vec - vector to store
 * \param ptr - Memory pointer
 */
#define VectorStore(vec, ptr)  _mm_storeu_ps((float*)(ptr), vec)

/**
 * Stores the XYZ components of a vector to unaligned memory.
 *
 * \param vec - vector to store XYZ
 * \param ptr - Unaligned memory pointer
 */
FUN_ALWAYS_INLINE void VectorStoreFloat3(const VectorRegister& vec, void* ptr) {
  union { VectorRegister v; float f[4]; } tmp;
  tmp.v = vec;
  float* float_ptr = (float*)(ptr);
  float_ptr[0] = tmp.f[0];
  float_ptr[1] = tmp.f[1];
  float_ptr[2] = tmp.f[2];
}

/**
 * Stores the x component of a vector to unaligned memory.
 *
 * \param vec - vector to store x
 * \param ptr - Unaligned memory pointer
 */
#define VectorStoreFloat1(vec, ptr)  _mm_store_ss((float*)(ptr), vec)

/**
 * Replicates one element into all four elements and returns the new vector.
 *
 * \param vec - Source vector
 * \param element_index - Index (0-3) of the element to replicate
 *
 * \return VectorRegister(vec[element_index], vec[element_index], vec[element_index], vec[element_index])
 */
#define VectorReplicate(vec, element_index)  _mm_shuffle_ps(vec, vec, SHUFFLEMASK(element_index, element_index, element_index, element_index))

/**
 * Returns the absolute value (component-wise).
 *
 * \param vec - Source vector
 *
 * \return VectorRegister(abs(vec.x), abs(vec.y), abs(vec.z), abs(vec.w))
 */
#define VectorAbs(vec)  _mm_and_ps(vec, VectorizeConstants::SignMask)

/**
 * Returns the negated value (component-wise).
 *
 * \param vec - Source vector
 *
 * \return VectorRegister(-vec.x, -vec.y, -vec.z, -vec.w)
 */
#define VectorNegate(vec)  _mm_sub_ps(_mm_setzero_ps(), vec)

/**
 * adds two vectors (component-wise) and returns the result.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x+vec2.x, vec1.y+vec2.y, vec1.z+vec2.z, vec1.w+vec2.w)
 */
#define VectorAdd(vec1, vec2)  _mm_add_ps(vec1, vec2)

/**
 * subtracts a vector from another (component-wise) and returns the result.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x-vec2.x, vec1.y-vec2.y, vec1.z-vec2.z, vec1.w-vec2.w)
 */
#define VectorSubtract(vec1, vec2)  _mm_sub_ps(vec1, vec2)

/**
 * Multiplies two vectors (component-wise) and returns the result.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x*vec2.x, vec1.y*vec2.y, vec1.z*vec2.z, vec1.w*vec2.w)
 */
#define VectorMultiply(vec1, vec2)  _mm_mul_ps(vec1, vec2)

/**
 * Multiplies two vectors (component-wise), adds in the third vector and returns the result.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 * \param vec3 - 3rd vector
 *
 * \return VectorRegister(vec1.x*vec2.x + vec3.x, vec1.y*vec2.y + vec3.y, vec1.z*vec2.z + vec3.z, vec1.w*vec2.w + vec3.w)
 */
#define VectorMultiplyAdd(vec1, vec2, vec3)  _mm_add_ps(_mm_mul_ps(vec1, vec2), vec3)

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
  return VectorAdd(VectorReplicate(tmp, 0), VectorAdd(VectorReplicate(tmp, 1), VectorReplicate(tmp, 2)));
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
FUN_ALWAYS_INLINE VectorRegister VectorDot4(const VectorRegister& vec1, const VectorRegister& vec2) {
  VectorRegister tmp1, tmp2;
  tmp1 = VectorMultiply(vec1, vec2);
  tmp2 = _mm_shuffle_ps(tmp1, tmp1, SHUFFLEMASK(2, 3, 0, 1));  // (z, w, x, y).
  tmp1 = VectorAdd(tmp1, tmp2);                // (x*x + z*z, y*y + w*w, z*z + x*x, w*w + y*y)
  tmp2 = _mm_shuffle_ps(tmp1, tmp1, SHUFFLEMASK(1, 2, 3, 0));  // Rotate left 4 bytes (y, z, w, x).
  return VectorAdd(tmp1, tmp2);                 // (x*x + z*z + y*y + w*w, y*y + w*w + z*z + x*x, z*z + x*x + w*w + y*y, w*w + y*y + x*x + z*z)
}


/**
 * Creates a four-part mask based on component-wise == compares of the input vectors
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x == vec2.x ? 0xFFFFFFFF : 0, same for yzw)
 */
#define VectorCompareEQ(vec1, vec2)  _mm_cmpeq_ps(vec1, vec2)

/**
 * Creates a four-part mask based on component-wise != compares of the input vectors
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x != vec2.x ? 0xFFFFFFFF : 0, same for yzw)
 */
#define VectorCompareNE(vec1, vec2)  _mm_cmpneq_ps(vec1, vec2)

/**
 * Creates a four-part mask based on component-wise > compares of the input vectors
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x > vec2.x ? 0xFFFFFFFF : 0, same for yzw)
 */
#define VectorCompareGT(vec1, vec2)  _mm_cmpgt_ps(vec1, vec2)

/**
 * Creates a four-part mask based on component-wise >= compares of the input vectors
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x >= vec2.x ? 0xFFFFFFFF : 0, same for yzw)
 */
#define VectorCompareGE(vec1, vec2)  _mm_cmpge_ps(vec1, vec2)

/**
 * Does a bitwise vector selection based on a mask (e.g., created from VectorCompareXX)
 *
 * \param mask  - Mask (when 1: use the corresponding bit from vec1 otherwise from vec2)
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 * \return VectorRegister(for each bit i: mask[i] ? vec1[i] : vec2[i])
 */
FUN_ALWAYS_INLINE VectorRegister VectorSelect(const VectorRegister& mask, const VectorRegister& vec1, const VectorRegister& vec2) {
  return _mm_xor_ps(vec2, _mm_and_ps(mask, _mm_xor_ps(vec1, vec2)));
}

/**
 * Combines two vectors using bitwise OR (treating each vector as a 128 bit field)
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(for each bit i: vec1[i] | vec2[i])
 */
#define VectorBitwiseOr(vec1, vec2)  _mm_or_ps(vec1, vec2)

/**
 * Combines two vectors using bitwise AND (treating each vector as a 128 bit field)
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(for each bit i: vec1[i] & vec2[i])
 */
#define VectorBitwiseAnd(vec1, vec2)  _mm_and_ps(vec1, vec2)

/**
 * Combines two vectors using bitwise XOR (treating each vector as a 128 bit field)
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(for each bit i: vec1[i] ^ vec2[i])
 */
#define VectorBitwiseXor(vec1, vec2)  _mm_xor_ps(vec1, vec2)

/**
 * Calculates the cross product of two vectors (XYZ components). w is set to 0.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return cross(vec1.xyz, vec2.xyz). w is set to 0.
 */
FUN_ALWAYS_INLINE VectorRegister VectorCross(const VectorRegister& vec1, const VectorRegister& vec2) {
  VectorRegister a_yzxw = _mm_shuffle_ps(vec1, vec1, SHUFFLEMASK(1, 2, 0, 3));
  VectorRegister b_zxyw = _mm_shuffle_ps(vec2, vec2, SHUFFLEMASK(2, 0, 1, 3));
  VectorRegister a_zxyw = _mm_shuffle_ps(vec1, vec1, SHUFFLEMASK(2, 0, 1, 3));
  VectorRegister b_yzxw = _mm_shuffle_ps(vec2, vec2, SHUFFLEMASK(1, 2, 0, 3));
  return VectorSubtract(VectorMultiply(a_yzxw, b_zxyw), VectorMultiply(a_zxyw, b_yzxw));
}

/**
 * Calculates x raised to the power of y (component-wise).
 *
 * \param base - base vector
 * \param exponent - exponent vector
 *
 * \return VectorRegister(base.x^exponent.x, base.y^exponent.y, base.z^exponent.z, base.w^exponent.w)
 */
FUN_ALWAYS_INLINE VectorRegister VectorPow(const VectorRegister& base, const VectorRegister& exponent) {
  //@TODO: Optimize
  union { VectorRegister v; float f[4]; } b, e;
  b.v = base;
  e.v = exponent;
  return _mm_setr_ps(powf(b.f[0], e.f[0]), powf(b.f[1], e.f[1]), powf(b.f[2], e.f[2]), powf(b.f[3], e.f[3]));
}

/**
 * Returns an estimate of 1/sqrt(c) for each component of the vector
 *
 * \param vec - vector
 *
 * \return VectorRegister(1/sqrt(t), 1/sqrt(t), 1/sqrt(t), 1/sqrt(t))
 */
#define VectorReciprocalSqrt(vec)  _mm_rsqrt_ps(vec)

/**
 * Computes an estimate of the reciprocal of a vector (component-wise) and returns the result.
 *
 * \param vec - 1st vector
 *
 * \return VectorRegister((Estimate) 1.f / vec.x, (Estimate) 1.f / vec.y, (Estimate) 1.f / vec.z, (Estimate) 1.f / vec.w)
 */
#define VectorReciprocal(vec)  _mm_rcp_ps(vec)

/**
 * Return Reciprocal Length of the vector (estimate)
 *
 * \param vec - vector
 *
 * \return VectorRegister(rlen, rlen, rlen, rlen) when rlen = 1/sqrt(dot4(v))
 */
FUN_ALWAYS_INLINE VectorRegister VectorReciprocalLen(const VectorRegister& vec) {
  VectorRegister recip_len = VectorDot4(vec, vec);
  return VectorReciprocalSqrt(recip_len);
}

/**
 * Return the reciprocal of the square root of each component
 *
 * \param vec - vector
 *
 * \return VectorRegister(1/sqrt(vec.x), 1/sqrt(vec.y), 1/sqrt(vec.z), 1/sqrt(vec.w))
 */
FUN_ALWAYS_INLINE VectorRegister VectorReciprocalSqrtAccurate(const VectorRegister& vec) {
  // Perform two passes of Newton-Raphson iteration on the hardware estimate
  //    v^-0.5 = x
  // => x^2 = v^-1
  // => 1/(x^2) = v
  // => F(x) = x^-2 - v
  //    F'(x) = -2x^-3

  //    x1 = x0 - F(x0)/F'(x0)
  // => x1 = x0 + 0.5 * (x0^-2 - vec) * x0^3
  // => x1 = x0 + 0.5 * (x0 - vec * x0^3)
  // => x1 = x0 + x0 * (0.5 - 0.5 * vec * x0^2)

  const VectorRegister ONE_HALF = VectorizeConstants::FloatOneHalf;
  const VectorRegister VEC_DIV_BY_2 = VectorMultiply(vec, ONE_HALF);

  // Initial estimate
  const VectorRegister x0 = VectorReciprocalSqrt(vec);

  // First iteration
  VectorRegister x1 = VectorMultiply(x0, x0);
  x1 = VectorSubtract(ONE_HALF, VectorMultiply(VEC_DIV_BY_2, x1));
  x1 = VectorMultiplyAdd(x0, x1, x0);

  // Second iteration
  VectorRegister x2 = VectorMultiply(x1, x1);
  x2 = VectorSubtract(ONE_HALF, VectorMultiply(VEC_DIV_BY_2, x2));
  x2 = VectorMultiplyAdd(x1, x2, x1);

  return x2;
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
  //   x1 = x0 - f(x0) / f'(x0)
  //
  //    1 / vec = x
  // => x * vec = 1
  // => F(x) = x * vec - 1
  //    F'(x) = vec
  // => x1 = x0 - (x0 * vec - 1) / vec
  //
  // Since 1/vec is what we're trying to solve, use an estimate for it, x0
  // => x1 = x0 - (x0 * vec - 1) * x0 = 2 * x0 - vec * x0^2

  // Initial estimate
  const VectorRegister x0 = VectorReciprocal(vec);

  // First iteration
  const VectorRegister x0_squared = VectorMultiply(x0, x0);
  const VectorRegister x0_times2 = VectorAdd(x0, x0);
  const VectorRegister x1 = VectorSubtract(x0_times2, VectorMultiply(vec, x0_squared));

  // Second iteration
  const VectorRegister x1_squared = VectorMultiply(x1, x1);
  const VectorRegister x1_times2 = VectorAdd(x1, x1);
  const VectorRegister x2 = VectorSubtract(x1_times2, VectorMultiply(vec, x1_squared));

  return x2;
}

/**
 * Normalize vector
 *
 * \param vec - vector to normalize
 *
 * \return Normalized VectorRegister
 */
FUN_ALWAYS_INLINE VectorRegister VectorNormalize(const VectorRegister& vec) {
  return VectorMultiply(vec, VectorReciprocalLen(vec));
}

/**
 * Loads XYZ and sets w=0
 *
 * \param vec - VectorRegister
 *
 * \return VectorRegister(x, y, z, 0.f)
 */
#define VectorSet_W0(vec)  _mm_and_ps(vec, VectorizeConstants::XYZMask)

/**
 * Loads XYZ and sets w=1
 *
 * \param vec - VectorRegister
 *
 * \return VectorRegister(x, y, z, 1.f)
 */
FUN_ALWAYS_INLINE VectorRegister VectorSet_W1(const VectorRegister& vec) {
  // tmp = (vec[2]. vectorvec3], 1.f, 1.f)
  VectorRegister tmp = _mm_movehl_ps(VectorOne(), vec);

  // Return (vec[0], vec[1], vec[2], 1.f)
  return _mm_shuffle_ps(vec, tmp, SHUFFLEMASK(0, 1, 0, 3));
}

/**
 * Multiplies two 4x4 matrices.
 *
 * \param result - Pointer to where the result should be stored
 * \param matrix1 - Pointer to the first matrix
 * \param matrix2 - Pointer to the second matrix
 */
FUN_ALWAYS_INLINE void VectorMatrixMultiply(void* result, const void* matrix1, const void* matrix2) {
  const VectorRegister* a = (const VectorRegister*)matrix1;
  const VectorRegister* b = (const VectorRegister*)matrix2;
  VectorRegister* r = (VectorRegister*) result;
  VectorRegister tmp, r0, r1, r2, r3;

  // First row of result (matrix1[0] * matrix2).
  tmp = VectorMultiply(VectorReplicate(a[0], 0), b[0]);
  tmp = VectorMultiplyAdd(VectorReplicate(a[0], 1), b[1], tmp);
  tmp = VectorMultiplyAdd(VectorReplicate(a[0], 2), b[2], tmp);
  r0  = VectorMultiplyAdd(VectorReplicate(a[0], 3), b[3], tmp);

  // Second row of result (matrix1[1] * matrix2).
  tmp = VectorMultiply(VectorReplicate(a[1], 0), b[0]);
  tmp = VectorMultiplyAdd(VectorReplicate(a[1], 1), b[1], tmp);
  tmp = VectorMultiplyAdd(VectorReplicate(a[1], 2), b[2], tmp);
  r1  = VectorMultiplyAdd(VectorReplicate(a[1], 3), b[3], tmp);

  // Third row of result (matrix1[2] * matrix2).
  tmp = VectorMultiply(VectorReplicate(a[2], 0), b[0]);
  tmp = VectorMultiplyAdd(VectorReplicate(a[2], 1), b[1], tmp);
  tmp = VectorMultiplyAdd(VectorReplicate(a[2], 2), b[2], tmp);
  r2  = VectorMultiplyAdd(VectorReplicate(a[2], 3), b[3], tmp);

  // Fourth row of result (matrix1[3] * matrix2).
  tmp = VectorMultiply(VectorReplicate(a[3], 0), b[0]);
  tmp = VectorMultiplyAdd(VectorReplicate(a[3], 1), b[1], tmp);
  tmp = VectorMultiplyAdd(VectorReplicate(a[3], 2), b[2], tmp);
  r3  = VectorMultiplyAdd(VectorReplicate(a[3], 3), b[3], tmp);

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
 *
 * TODO: vector version of this function that doesn't use D3DX
 */
FUN_ALWAYS_INLINE void VectorMatrixInverse(void* dst_matrix, const void* src_matrix) {
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

  const float determinant = m[0][0]*det[0] - m[1][0]*det[1] + m[2][0]*det[2] - m[3][0]*det[3];
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
 * \Calculate Homogeneous transform.
 * \
 * \\param vec_p - VectorRegister
 * \\param matrix_m - Matrix pointer to the matrix to apply transform
 * \
 * \\return VectorRegister = vec_p*matrix_m
 */
FUN_ALWAYS_INLINE VectorRegister VectorTransformVector(const VectorRegister& vec_p, const void* matrix_m) {
  const VectorRegister* m = (const VectorRegister*)matrix_m;
  VectorRegister vec_x, vec_y, vec_z, vec_w;

  // Splat x, y, z and w
  vec_x = VectorReplicate(vec_p, 0);
  vec_y = VectorReplicate(vec_p, 1);
  vec_z = VectorReplicate(vec_p, 2);
  vec_w = VectorReplicate(vec_p, 3);
  // Mul by the matrix
  vec_x = VectorMultiply(vec_x, m[0]);
  vec_y = VectorMultiply(vec_y, m[1]);
  vec_z = VectorMultiply(vec_z, m[2]);
  vec_w = VectorMultiply(vec_w, m[3]);
  // Add them all together
  vec_x = VectorAdd(vec_x, vec_y);
  vec_z = VectorAdd(vec_z, vec_w);
  vec_x = VectorAdd(vec_x, vec_z);

  return vec_x;
}

/**
 * Returns the minimum values of two vectors (component-wise).
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(min(vec1.x, vec2.x), min(vec1.y, vec2.y), min(vec1.z, vec2.z), min(vec1.w, vec2.w))
 */
#define VectorMin(vec1, vec2)  _mm_min_ps(vec1, vec2)

/**
 * Returns the maximum values of two vectors (component-wise).
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(max(vec1.x, vec2.x), max(vec1.y, vec2.y), max(vec1.z, vec2.z), max(vec1.w, vec2.w))
 */
#define VectorMax(vec1, vec2)  _mm_max_ps(vec1, vec2)

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
#define VectorSwizzle(vec, x, y, z, w)  _mm_shuffle_ps(vec, vec, SHUFFLEMASK(x, y, z, w))

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
#define VectorShuffle(vec1, vec2, x, y, z, w)  _mm_shuffle_ps(vec1, vec2, SHUFFLEMASK(x, y, z, w))

/**
 * These functions return a vector mask to indicate which components pass the comparison.
 * Each component is 0xffffffff if it passes, 0x00000000 if it fails.
 *
 * \param vec1 - 1st source vector
 * \param vec2 - 2nd source vector
 *
 * \return vector with a mask for each component.
 */
#define VectorMask_LT(vec1, vec2)  _mm_cmplt_ps(vec1, vec2)
#define VectorMask_LE(vec1, vec2)  _mm_cmple_ps(vec1, vec2)
#define VectorMask_GT(vec1, vec2)  _mm_cmpgt_ps(vec1, vec2)
#define VectorMask_GE(vec1, vec2)  _mm_cmpge_ps(vec1, vec2)
#define VectorMask_EQ(vec1, vec2)  _mm_cmpeq_ps(vec1, vec2)
#define VectorMask_NE(vec1, vec2)  _mm_cmpneq_ps(vec1, vec2)

/**
 * Returns an integer bit-mask (0x00 - 0x0f) based on the sign-bit for each component in a vector.
 *
 * \param vec_mask - vector
 *
 * \return Bit0 = sign(vec_mask.x), Bit1 = sign(vec_mask.y), Bit2 = sign(vec_mask.z), Bit3 = sign(vec_mask.w)
 */
#define VectorMaskBits(vec_mask)  _mm_movemask_ps(vec_mask)

/**
 * Divides two vectors (component-wise) and returns the result.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x/vec2.x, vec1.y/vec2.y, vec1.z/vec2.z, vec1.w/vec2.w)
 */
#define VectorDivide(vec1, vec2)  _mm_div_ps(vec1, vec2)

/**
 * Merges the XYZ components of one vector with the w component of another vector and returns the result.
 *
 * \param vec_xyz - Source vector for XYZ_
 * \param vec_w - Source register for ___W (note: the fourth component is used, not the first)
 *
 * \return VectorRegister(vec_xyz.x, vec_xyz.y, vec_xyz.z, vec_w.w)
 */
#define VectorMergeVecXYZ_VecW(vec_xyz, vec_w)  VectorSelect(VectorizeConstants::XYZMask, vec_xyz, vec_w)

/**
 * Loads 4 BYTEs from unaligned memory and converts them into 4 FLOATs.
 * IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar FLOATs after you've used this intrinsic!
 *
 * \param ptr - Unaligned memory pointer to the 4 BYTEs.
 *
 * \return VectorRegister(float(ptr[0]), float(ptr[1]), float(ptr[2]), float(ptr[3]))
 */
// Looks complex but is really quite straightforward:
// Load as 32-bit value, unpack 4x unsigned bytes to 4x 16-bit ints, then unpack again into 4x 32-bit ints, then convert to 4x floats
#define VectorLoadByte4(ptr)  _mm_cvtepi32_ps(_mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(*(int32*)ptr), _mm_setzero_si128()), _mm_setzero_si128()))

/**
 * Loads 4 BYTEs from unaligned memory and converts them into 4 FLOATs in reversed order.
 * IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar FLOATs after you've used this intrinsic!
 *
 * \param ptr - Unaligned memory pointer to the 4 BYTEs.
 *
 * \return VectorRegister(float(ptr[3]), float(ptr[2]), float(ptr[1]), float(ptr[0]))
 */
FUN_ALWAYS_INLINE VectorRegister VectorLoadByte4Reverse(void* ptr) {
  VectorRegister tmp = VectorLoadByte4(ptr);
  return _mm_shuffle_ps(tmp, tmp, SHUFFLEMASK(3, 2, 1, 0));
}

/**
 * Converts the 4 FLOATs in the vector to 4 BYTEs, clamped to [0, 255], and stores to unaligned memory.
 * IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar FLOATs after you've used this intrinsic!
 *
 * \param vec - vector containing 4 FLOATs
 * \param ptr - Unaligned memory pointer to store the 4 BYTEs.
 */
FUN_ALWAYS_INLINE void VectorStoreByte4(const VectorRegister& vec, void* ptr) {
  // Looks complex but is really quite straightforward:
  // Convert 4x floats to 4x 32-bit ints, then pack into 4x 16-bit ints, then into 4x 8-bit unsigned ints, then store as a 32-bit value
  *(int32*)ptr = _mm_cvtsi128_si32(_mm_packus_epi16(_mm_packs_epi32(_mm_cvttps_epi32(vec), _mm_setzero_si128()), _mm_setzero_si128()));
}

/**
 * Returns non-zero if any element in vec1 is greater than the corresponding element in vec2, otherwise 0.
 *
 * \param vec1 - 1st source vector
 * \param vec2 - 2nd source vector
 *
 * \return Non-zero integer if (vec1.x > vec2.x) || (vec1.y > vec2.y) || (vec1.z > vec2.z) || (vec1.w > vec2.w)
 */
#define VectorAnyGreaterThan(vec1, vec2)  _mm_movemask_ps(_mm_cmpgt_ps(vec1, vec2))

/**
 * Resets the floating point registers so that they can be used again.
 * Some intrinsics use these for MMX purposes (e.g. VectorLoadByte4 and VectorStoreByte4).
 *
 * This is no longer necessary now that we don't use MMX instructions
 */
#define VectorResetFloatRegisters()

/**
 * Returns the control register.
 *
 * \return The uint32 control register
 */
#define VectorGetControlRegister()  _mm_getcsr()

/**
 * Sets the control register.
 *
 * \param ControlStatus - The uint32 control status value to set
 */
#define VectorSetControlRegister(ControlStatus)  _mm_setcsr(ControlStatus)

/**
 * Control status bit to round all floating point math results towards zero.
 */
#define VECTOR_ROUND_TOWARD_ZERO  _MM_ROUND_TOWARD_ZERO

/**
 * Multiplies two quaternions; the order matters.
 *
 * Order matters when composing quaternions: C = VectorQuaternionMultiply2(a, b) will yield a quaternion C = a * b
 * that logically first applies b then a to any subsequent transformation (right first, then left).
 *
 * \param quat1 - Pointer to the first quaternion
 * \param quat2 - Pointer to the second quaternion
 *
 * \return quat1 * quat2
 */
FUN_ALWAYS_INLINE VectorRegister VectorQuaternionMultiply2(const VectorRegister& quat1, const VectorRegister& quat2) {
  VectorRegister result = VectorMultiply(VectorReplicate(quat1, 3), quat2);
  result = VectorMultiplyAdd(VectorMultiply(VectorReplicate(quat1, 0), VectorSwizzle(quat2, 3, 2, 1, 0)), VectorizeConstants::QMULTI_SIGN_MASK0, result);
  result = VectorMultiplyAdd(VectorMultiply(VectorReplicate(quat1, 1), VectorSwizzle(quat2, 2, 3, 0, 1)), VectorizeConstants::QMULTI_SIGN_MASK1, result);
  result = VectorMultiplyAdd(VectorMultiply(VectorReplicate(quat1, 2), VectorSwizzle(quat2, 1, 0, 3, 2)), VectorizeConstants::QMULTI_SIGN_MASK2, result);
  return result;
}

/**
 * Multiplies two quaternions; the order matters.
 *
 * When composing quaternions: VectorQuaternionMultiply(C, a, b) will yield a quaternion C = a * b
 * that logically first applies b then a to any subsequent transformation (right first, then left).
 *
 * \param result - Pointer to where the result quat1 * quat2 should be stored
 * \param quat1 - Pointer to the first quaternion (must not be the destination)
 * \param quat2 - Pointer to the second quaternion (must not be the destination)
 */
FUN_ALWAYS_INLINE void VectorQuaternionMultiply(void* __restrict result, const void* __restrict quat1, const void* __restrict quat2) {
  *((VectorRegister*)result) = VectorQuaternionMultiply2(*((const VectorRegister*)quat1), *((const VectorRegister*)quat2));
}

/**
 * Returns true if the vector contains a component that is either NAN or +/-infinite.
 */
FUN_ALWAYS_INLINE bool VectorContainsNaNOrInfinite(const VectorRegister& vec) {
  bool is_not_nan = _mm_movemask_ps(_mm_cmpneq_ps(vec, vec)) == 0; // Test for the fact that NAN != NAN

  // Test for infinity, technique "stolen" from DirectXMathVector.inl

  // Mask off signs
  VectorRegister inf_test = _mm_and_ps(vec, VectorizeConstants::SignMask);
  // Compare to infinity. If any are infinity, the signs are true.
  bool IsNotInf = _mm_movemask_ps(_mm_cmpeq_ps(inf_test, VectorizeConstants::FloatInfinity)) == 0;

  return !(is_not_nan & IsNotInf);
}

FUN_ALWAYS_INLINE VectorRegister VectorTruncate(const VectorRegister& x) {
  return _mm_cvtepi32_ps(_mm_cvttps_epi32(x));
}

FUN_ALWAYS_INLINE VectorRegister VectorFractional(const VectorRegister& x) {
  return VectorSubtract(x, VectorTruncate(x));
}

FUN_ALWAYS_INLINE VectorRegister VectorCeil(const VectorRegister& x) {
  VectorRegister trunc = VectorTruncate(x);
  VectorRegister pos_mask = VectorCompareGE(x, VectorizeConstants::FloatZero);
  VectorRegister add = VectorSelect(pos_mask, VectorizeConstants::FloatOne, (VectorizeConstants::FloatZero));
  return VectorAdd(trunc, add);
}

FUN_ALWAYS_INLINE VectorRegister VectorFloor(const VectorRegister& x) {
  VectorRegister trunc = VectorTruncate(x);
  VectorRegister pos_mask = VectorCompareGE(x, (VectorizeConstants::FloatZero));
  VectorRegister sub = VectorSelect(pos_mask, (VectorizeConstants::FloatZero), (VectorizeConstants::FloatOne));
  return VectorSubtract(trunc, sub);
}

FUN_ALWAYS_INLINE VectorRegister VectorMod(const VectorRegister& x, const VectorRegister& y) {
  VectorRegister tmp = VectorTruncate(VectorDivide(x, y));
  return VectorSubtract(x, VectorMultiply(y, tmp));
}

FUN_ALWAYS_INLINE VectorRegister VectorSign(const VectorRegister& x) {
  VectorRegister mask = VectorCompareGE(x, (VectorizeConstants::FloatZero));
  return VectorSelect(mask, (VectorizeConstants::FloatOne), (VectorizeConstants::FloatMinusOne));
}

FUN_ALWAYS_INLINE VectorRegister VectorStep(const VectorRegister& x) {
  VectorRegister mask = VectorCompareGE(x, (VectorizeConstants::FloatZero));
  return VectorSelect(mask, (VectorizeConstants::FloatOne), (VectorizeConstants::FloatZero));
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

/**
 * Using "static const float ..." or "static const VectorRegister ..." in functions creates the branch and code to construct those constants.
 * Doing this in FUN_ALWAYS_INLINE not only means you introduce a branch per static, but you bloat the inlined code immensely.
 * Defining these constants at the global scope causes them to be created at startup, and avoids the cost at the function level.
 * Doing it at the function level is okay for anything that is a simple "const float", but usage of "sqrt()" here forces actual function calls.
 */
namespace VectorSinConstantsSSE {
  static const float p = 0.225f;
  static const float a = (16 * sqrt(p));
  static const float b = ((1 - p) / sqrt(p));

  static const VectorRegister A = MakeVectorRegister(a, a, a, a);
  static const VectorRegister B = MakeVectorRegister(b, b, b, b);
}

FUN_ALWAYS_INLINE VectorRegister VectorSin(const VectorRegister& x) {
  //Sine approximation using a squared parabola restrained to f(0) = 0, f(PI) = 0, f(PI/2) = 1.
  //based on a good discussion here http://forum.devmaster.net/t/fast-and-accurate-sine-cosine/9648
  //After approx 2.5 million tests comparing to sin():
  //Average error of 0.000128
  //Max error of 0.001091

  VectorRegister y = VectorMultiply(x, VectorizeConstants::OneOverTwoPi);
  y = VectorSubtract(y, VectorFloor(VectorAdd(y, VectorizeConstants::FloatOneHalf)));
  y = VectorMultiply(VectorSinConstantsSSE::A, VectorMultiply(y, VectorSubtract(VectorizeConstants::FloatOneHalf, VectorAbs(y))));
  return VectorMultiply(y, VectorAdd(VectorSinConstantsSSE::B, VectorAbs(y)));
}

FUN_ALWAYS_INLINE VectorRegister VectorCos(const VectorRegister& x) {
  return VectorSin(VectorAdd(x, VectorizeConstants::PiByTwo));
}

/**
 * Computes the sine and cosine of each component of a vector.
 *
 * \param vec_sin_angles - VectorRegister Pointer to where the Sin result should be stored
 * \param vec_cos_angles - VectorRegister Pointer to where the Cos result should be stored
 * \param vec_angles - VectorRegister Pointer to the input angles
 */
FUN_ALWAYS_INLINE void VectorSinCos(VectorRegister* __restrict vec_sin_angles, VectorRegister* __restrict vec_cos_angles, const VectorRegister* __restrict vec_angles) {
  // Map to [-pi, pi]
  // x = a - 2pi * round(a/2pi)
  // Note the round(), not truncate(). In this case round() can round halfway cases using round-to-nearest-even OR round-to-nearest.

  // quotient = round(a/2pi)
  VectorRegister quotient = VectorMultiply(*vec_angles, VectorizeConstants::OneOverTwoPi);
  quotient = _mm_cvtepi32_ps(_mm_cvtps_epi32(quotient)); // round to nearest even is the default rounding mode but that's fine here.
  // x = a - 2pi * quotient
  VectorRegister x = VectorSubtract(*vec_angles, VectorMultiply(VectorizeConstants::TwoPi, quotient));

  // Map in [-pi/2, pi/2]
  VectorRegister sign = VectorBitwiseAnd(x, VectorizeConstants::SignBit);
  VectorRegister c = VectorBitwiseOr(VectorizeConstants::Pi, sign);  // pi when x >= 0, -pi when x < 0
  VectorRegister absx = VectorAbs(x);
  VectorRegister rflx = VectorSubtract(c, x);
  VectorRegister comp = VectorCompareGT(absx, VectorizeConstants::PiByTwo);
  x = VectorSelect(comp, rflx, x);
  sign = VectorSelect(comp, VectorizeConstants::FloatMinusOne, VectorizeConstants::FloatOne);

  const VectorRegister x_squared = VectorMultiply(x, x);

  // 11-degree minimax approximation
  //*ScalarSin = (((((-2.3889859e-08f * y2 + 2.7525562e-06f) * y2 - 0.00019840874f) * y2 + 0.0083333310f) * y2 - 0.16666667f) * y2 + 1.f) * y;
  const VectorRegister sin_coeff0 = MakeVectorRegister(1.f, -0.16666667f, 0.0083333310f, -0.00019840874f);
  const VectorRegister sin_coeff1 = MakeVectorRegister(2.7525562e-06f, -2.3889859e-08f, /*unused*/ 0.f, /*unused*/ 0.f);

  VectorRegister s2;
  s2 = VectorReplicate(sin_coeff1, 1);
  s2 = VectorMultiplyAdd(x_squared, s2, VectorReplicate(sin_coeff1, 0));
  s2 = VectorMultiplyAdd(x_squared, s2, VectorReplicate(sin_coeff0, 3));
  s2 = VectorMultiplyAdd(x_squared, s2, VectorReplicate(sin_coeff0, 2));
  s2 = VectorMultiplyAdd(x_squared, s2, VectorReplicate(sin_coeff0, 1));
  s2 = VectorMultiplyAdd(x_squared, s2, VectorReplicate(sin_coeff0, 0));
  *vec_sin_angles = VectorMultiply(s2, x);

  // 10-degree minimax approximation
  //*ScalarCos = sign * (((((-2.6051615e-07f * y2 + 2.4760495e-05f) * y2 - 0.0013888378f) * y2 + 0.041666638f) * y2 - 0.5f) * y2 + 1.f);
  const VectorRegister cos_coeff0 = MakeVectorRegister(1.f, -0.5f, 0.041666638f, -0.0013888378f);
  const VectorRegister cos_coeff1 = MakeVectorRegister(2.4760495e-05f, -2.6051615e-07f, /*unused*/ 0.f, /*unused*/ 0.f);

  VectorRegister c2;
  c2 = VectorReplicate(cos_coeff1, 1);
  c2 = VectorMultiplyAdd(x_squared, c2, VectorReplicate(cos_coeff1, 0));
  c2 = VectorMultiplyAdd(x_squared, c2, VectorReplicate(cos_coeff0, 3));
  c2 = VectorMultiplyAdd(x_squared, c2, VectorReplicate(cos_coeff0, 2));
  c2 = VectorMultiplyAdd(x_squared, c2, VectorReplicate(cos_coeff0, 1));
  c2 = VectorMultiplyAdd(x_squared, c2, VectorReplicate(cos_coeff0, 0));
  *vec_cos_angles = VectorMultiply(c2, sign);
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

// to be continued...

} // namespace fun

#endif
