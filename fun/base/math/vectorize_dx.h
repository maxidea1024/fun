#pragma once

#if __cplusplus_cli

// there are compile issues with this file in managed mode, so use the FPU version
#include "fun/base/math/math_vectorize_fpu.h"

#else

#include <DirectXMath.h>
#include <DirectXPackedVector.h>

namespace fun {

//
// Helpers
//

/**
 * float4 vector register type, where the first float (x)
 * is stored in the lowest 32 bits, and so on.
 */
typedef DirectX::XMVECTOR VectorRegister;

// for an DirectX::XMVECTOR, we need a single set of braces (for clang)
#define DECLARE_VECTOR_REGISTER(x, y, z, w)  { x, y, z, w }

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
  using namespace DirectX;
  return DirectX::XMVectorSetInt(x, y, z, w);
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
  return DirectX::XMVectorSet(x, y, z, w);
}

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
#define VectorZero()  DirectX::XMVectorZero()

/**
 * Returns a vector with all ones.
 *
 * \return VectorRegister(1.f, 1.f, 1.f, 1.f)
 */
#define VectorOne()  DirectX::g_XMOne.v

/**
 * Loads 4 FLOATs from unaligned memory.
 *
 * \param ptr - Unaligned memory pointer to the 4 FLOATs
 *
 * \return VectorRegister(ptr[0], ptr[1], ptr[2], ptr[3])
 */
#define VectorLoad(ptr) DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*)(ptr))

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
#define VectorLoadAligned(ptr)  DirectX::XMLoadFloat4A((const DirectX::XMFLOAT4A*)(ptr))

/**
 * Loads 1 float from unaligned memory and replicates it to all 4 elements.
 *
 * \param ptr - Unaligned memory pointer to the float
 *
 * \return VectorRegister(ptr[0], ptr[0], ptr[0], ptr[0])
 */
#define VectorLoadFloat1(ptr)  DirectX::XMVectorReplicatePtr((const float*)(ptr))

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
#define VectorStoreAligned(vec, ptr)  DirectX::XMStoreFloat4A((DirectX::XMFLOAT4A*)(ptr), vec)

/**
 * Performs non-temporal store of a vector to aligned memory without polluting the caches
 *
 * \param vec - vector to store
 * \param ptr - Aligned memory pointer
 */
#define VectorStoreAlignedStreamed(vec, ptr)  XM_STREAM_PS((float*)(ptr), vec)

/**
 * Stores a vector to memory (aligned or unaligned).
 *
 * \param vec - vector to store
 * \param ptr - Memory pointer
 */
#define VectorStore(vec, ptr)  DirectX::XMStoreFloat4((DirectX::XMFLOAT4*)(ptr), vec)

/**
 * Stores the XYZ components of a vector to unaligned memory.
 *
 * \param vec - vector to store XYZ
 * \param ptr - Unaligned memory pointer
*/
#define VectorStoreFloat3(vec, ptr)  DirectX::XMStoreFloat3((DirectX::XMFLOAT3*)(ptr), vec)

/**
 * Stores the x component of a vector to unaligned memory.
 *
 * \param vec - vector to store x
 * \param ptr - Unaligned memory pointer
 */
#define VectorStoreFloat1(vec, ptr)  DirectX::XMStoreFloat((float*)(ptr), vec)

/**
 * Returns an component from a vector.
 *
 * \param vec - vector register
 * \param component_index - Which component to get, x=0, y=1, z=2, w=3
 *
 * \return The component as a float
 */
FUN_ALWAYS_INLINE float VectorGetComponent(VectorRegister vec, uint32 component_index) {
  switch (component_index) {
    case 0:
      return DirectX::XMVectorGetX(vec);
    case 1:
      return DirectX::XMVectorGetY(vec);
    case 2:
      return DirectX::XMVectorGetZ(vec);
    case 3:
      return DirectX::XMVectorGetW(vec);
  }

  return 0.f;
}

/**
 * Replicates one element into all four elements and returns the new vector.
 *
 * \param vec - Source vector
 * \param element_index - Index (0-3) of the element to replicate
 *
 * \return VectorRegister(vec[element_index], vec[element_index], vec[element_index], vec[element_index])
 */
#define VectorReplicate(vec, element_index)  DirectX::XMVectorSwizzle<element_index, element_index, element_index, element_index>(vec)

/**
 * Returns the absolute value (component-wise).
 *
 * \param vec - Source vector
 *
 * \return VectorRegister(abs(vec.x), abs(vec.y), abs(vec.z), abs(vec.w))
 */
#define VectorAbs(vec)  DirectX::XMVectorAbs(vec)

/**
 * Returns the negated value (component-wise).
 *
 * \param vec - Source vector
 *
 * \return VectorRegister(-vec.x, -vec.y, -vec.z, -vec.w)
 */
#define VectorNegate(vec)  DirectX::XMVectorNegate(vec)

/**
 * adds two vectors (component-wise) and returns the result.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x+vec2.x, vec1.y+vec2.y, vec1.z+vec2.z, vec1.w+vec2.w)
 */
#define VectorAdd(vec1, vec2)  DirectX::XMVectorAdd(vec1, vec2)

/**
 * subtracts a vector from another (component-wise) and returns the result.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x-vec2.x, vec1.y-vec2.y, vec1.z-vec2.z, vec1.w-vec2.w)
 */
 #define VectorSubtract(vec1, vec2)  DirectX::XMVectorSubtract(vec1, vec2)

/**
 * Multiplies two vectors (component-wise) and returns the result.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x*vec2.x, vec1.y*vec2.y, vec1.z*vec2.z, vec1.w*vec2.w)
 */
#define VectorMultiply(vec1, vec2)  DirectX::XMVectorMultiply(vec1, vec2)

/**
 * Divides two vectors (component-wise) and returns the result.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x/vec2.x, vec1.y/vec2.y, vec1.z/vec2.z, vec1.w/vec2.w)
 */
#define VectorDivide(vec1, vec2)  DirectX::XMVectorDivide(vec1, vec2)

/**
 * Multiplies two vectors (component-wise), adds in the third vector and returns the result.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 * \param vec3 - 3rd vector
 *
 * \return VectorRegister(vec1.x*vec2.x + vec3.x, vec1.y*vec2.y + vec3.y, vec1.z*vec2.z + vec3.z, vec1.w*vec2.w + vec3.w)
 */
#define VectorMultiplyAdd(vec1, vec2, vec3)  DirectX::XMVectorMultiplyAdd(vec1, vec2, vec3)

/**
 * Calculates the dot3 product of two vectors and returns a vector with the result in all 4 components.
 * Only really efficient on Xbox 360.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return d = dot3(vec1.xyz, vec2.xyz), VectorRegister(d, d, d, d)
 */
#define VectorDot3(vec1, vec2)  DirectX::XMVector3Dot(vec1, vec2)

/**
 * Calculates the dot4 product of two vectors and returns a vector with the result in all 4 components.
 * Only really efficient on Xbox 360.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return d = dot4(vec1.xyzw, vec2.xyzw), VectorRegister(d, d, d, d)
 */
#define VectorDot4(vec1, vec2)  DirectX::XMVector4Dot(vec1, vec2)

/**
 * Creates a four-part mask based on component-wise == compares of the input vectors
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x == vec2.x ? 0xFFFFFFFF : 0, same for yzw)
 */
#define VectorCompareEQ(vec1, vec2)  DirectX::XMVectorEqual(vec1, vec2)

/**
 * Creates a four-part mask based on component-wise != compares of the input vectors
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x != vec2.x ? 0xFFFFFFFF : 0, same for yzw)
 */
#define VectorCompareNE(vec1, vec2)  DirectX::XMVectorNotEqual(vec1, vec2)

/**
 * Creates a four-part mask based on component-wise > compares of the input vectors
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x > vec2.x ? 0xFFFFFFFF : 0, same for yzw)
 */
#define VectorCompareGT(vec1, vec2)  DirectX::XMVectorGreater(vec1, vec2)

/**
 * Creates a four-part mask based on component-wise >= compares of the input vectors
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x >= vec2.x ? 0xFFFFFFFF : 0, same for yzw)
 */
#define VectorCompareGE(vec1, vec2)  DirectX::XMVectorGreaterOrEqual(vec1, vec2)

/**
 * Does a bitwise vector selection based on a mask (e.g., created from VectorCompareXX)
 *
 * \param Mask - Mask (when 1: use the corresponding bit from vec1 otherwise from vec2)
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(for each bit i: Mask[i] ? vec1[i] : vec2[i])
 */
#define VectorSelect(Mask, vec1, vec2)  DirectX::XMVectorSelect(vec2, vec1, Mask)

/**
 * Combines two vectors using bitwise OR (treating each vector as a 128 bit field)
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(for each bit i: vec1[i] | vec2[i])
 */
#define VectorBitwiseOr(vec1, vec2)  DirectX::XMVectorOrInt(vec1, vec2)

/**
 * Combines two vectors using bitwise AND (treating each vector as a 128 bit field)
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(for each bit i: vec1[i] & vec2[i])
 */
#define VectorBitwiseAnd(vec1, vec2)  DirectX::XMVectorAndInt(vec1, vec2)

/**
 * Combines two vectors using bitwise XOR (treating each vector as a 128 bit field)
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(for each bit i: vec1[i] ^ vec2[i])
 */
#define VectorBitwiseXor(vec1, vec2)  DirectX::XMVectorXorInt(vec1, vec2)

/**
 * Returns an integer bit-mask (0x00 - 0x0f) based on the sign-bit for each component in a vector.
 *
 * \param vec_mask - vector
 *
 * \return Bit0 = sign(vec_mask.x), Bit1 = sign(vec_mask.y), Bit2 = sign(vec_mask.z), Bit3 = sign(vec_mask.w)
 */
#define VectorMaskBits(vec_mask)  _mm_movemask_ps(vec_mask)

/**
 * Calculates the cross product of two vectors (XYZ components). w is set to 0.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return cross(vec1.xyz, vec2.xyz). w is set to 0.
 */
#define VectorCross(vec1, vec2)  DirectX::XMVector3Cross(vec1, vec2)

/**
 * Calculates x raised to the power of y (component-wise).
 *
 * \param base - Base vector
 * \param exponent - Exponent vector
 *
 * \return VectorRegister(base.x^exponent.x, base.y^exponent.y, base.z^exponent.z, base.w^exponent.w)
 */
#define VectorPow(base, exponent)  DirectX::XMVectorPow(base, exponent)

/**
 * Returns an estimate of 1/sqrt(c) for each component of the vector
 *
 * \param vec - vector
 *
 * \return VectorRegister(1/sqrt(t), 1/sqrt(t), 1/sqrt(t), 1/sqrt(t))
 */
#define VectorReciprocalSqrt(vec)  DirectX::XMVectorReciprocalSqrtEst(vec)

/**
 * Computes an estimate of the reciprocal of a vector (component-wise) and returns the result.
 *
 * \param vec - 1st vector
 *
 * \return VectorRegister((Estimate) 1.f / vec.x, (Estimate) 1.f / vec.y, (Estimate) 1.f / vec.z, (Estimate) 1.f / vec.w)
 */
#define VectorReciprocal(vec)  DirectX::XMVectorReciprocalEst(vec)

/**
 * Return Reciprocal Length of the vector
 *
 * \param vec - vector
 *
 * \return VectorRegister(rlen, rlen, rlen, rlen) when rlen = 1/sqrt(dot4(v))
 */
#define VectorReciprocalLen(vec)  DirectX::XMVector4ReciprocalLengthEst(vec)

/**
 * Return the reciprocal of the square root of each component
 *
 * \param vec - vector
 *
 * \return VectorRegister(1/sqrt(vec.x), 1/sqrt(vec.y), 1/sqrt(vec.z), 1/sqrt(vec.w))
 */
#define VectorReciprocalSqrtAccurate(vec)  DirectX::XMVectorReciprocalSqrt(vec)

/**
 * Computes the reciprocal of a vector (component-wise) and returns the result.
 *
 * \param vec - 1st vector
 *
 * \return VectorRegister(1.f / vec.x, 1.f / vec.y, 1.f / vec.z, 1.f / vec.w)
 */
#define VectorReciprocalAccurate(vec)  DirectX::XMVectorReciprocal(vec)

/**
 * Normalize vector
 *
 * \param vec - vector to normalize
 *
 * \return Normalized VectorRegister
 */
#define VectorNormalize(vec)  DirectX::XMVector4NormalizeEst(vec)

/**
 * Loads XYZ and sets w=0
 *
 * \param vec - VectorRegister
 *
 * \return VectorRegister(x, y, z, 0.f)
 */
#define VectorSet_W0(vec)  DirectX::XMVectorAndInt(vec , DirectX::g_XMMask3)

/**
 * Loads XYZ and sets w=1
 *
 * \param vec - VectorRegister
 *
 * \return VectorRegister(x, y, z, 1.f)
 */
#define VectorSet_W1(vec)  DirectX::XMVectorPermute<0, 1, 2, 7>(vec, VectorOne())

/**
 * Multiplies two 4x4 matrices.
 *
 * \param result - Pointer to where the result should be stored
 * \param matrix1 - Pointer to the first matrix
 * \param matrix2 - Pointer to the second matrix
 */
FUN_ALWAYS_INLINE void VectorMatrixMultiply(Matrix* result, const Matrix* matrix1, const Matrix* matrix2) {
  using namespace DirectX;
  XMMATRIX xm_matrix1 = XMLoadFloat4x4A((const XMFLOAT4X4A*)(matrix1));
  XMMATRIX xm_matrix2 = XMLoadFloat4x4A((const XMFLOAT4X4A*)(matrix2));
  XMMATRIX xm_matrix_r = XMMatrixMultiply(xm_matrix1, xm_matrix2);
  XMStoreFloat4x4A((XMFLOAT4X4A*)(result), xm_matrix_r);
}

/**
 * Calculate the inverse of an Matrix.
 *
 * \param dst_matrix - Matrix pointer to where the result should be stored
 * \param src_matrix - Matrix pointer to the matrix to be inversed
 */
FUN_ALWAYS_INLINE void VectorMatrixInverse(Matrix* dst_matrix, const Matrix* src_matrix) {
  using namespace DirectX;
  XMMATRIX xm_src_matrix = XMLoadFloat4x4A((const XMFLOAT4X4A*)(src_matrix));
  XMMATRIX xm_dst_matrix = XMMatrixInverse(nullptr, xm_src_matrix);
  XMStoreFloat4x4A((XMFLOAT4X4A*)(dst_matrix), xm_dst_matrix);
}

/**
 * Calculate Homogeneous transform.
 *
 * \param vec_p - VectorRegister
 * \param matrix_m - Matrix pointer to the matrix to apply transform
 *
 * \return VectorRegister = vec_p*matrix_m
 */
FUN_ALWAYS_INLINE VectorRegister VectorTransformVector(const VectorRegister& vec_p, const Matrix* matrix_m) {
  using namespace DirectX;
  XMMATRIX m1 = XMLoadFloat4x4A((const XMFLOAT4X4A*)(matrix_m));
  return XMVector4Transform(vec_p, m1);
}

/**
 * Returns the minimum values of two vectors (component-wise).
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(min(vec1.x, vec2.x), min(vec1.y, vec2.y), min(vec1.z, vec2.z), min(vec1.w, vec2.w))
 */
#define VectorMin(vec1, vec2)  DirectX::XMVectorMin(vec1, vec2)

/**
 * Returns the maximum values of two vectors (component-wise).
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(max(vec1.x, vec2.x), max(vec1.y, vec2.y), max(vec1.z, vec2.z), max(vec1.w, vec2.w))
 */
#define VectorMax(vec1, vec2)  DirectX::XMVectorMax(vec1, vec2)

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
#define VectorSwizzle(vec, x, y, z, w)  DirectX::XMVectorSwizzle<x, y, z, w>(vec)

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
#define VectorShuffle(vec1, vec2, x, y, z, w)  DirectX::XMVectorPermute<x, y, z+4, w+4>(vec1, vec2)

/**
 * Merges the XYZ components of one vector with the w component of another vector and returns the result.
 *
 * \param vec_xyz - Source vector for XYZ_
 * \param vec_w - Source register for ___W (note: the fourth component is used, not the first)
 *
 * \return VectorRegister(vec_xyz.x, vec_xyz.y, vec_xyz.z, vec_w.w)
 */
FUN_ALWAYS_INLINE VectorRegister VectorMergeVecXYZ_VecW(const VectorRegister& vec_xyz, const VectorRegister& vec_w) {
  using namespace DirectX;
  return DirectX::XMVectorSelect(vec_xyz, vec_w, g_XMMaskW);
}

/**
 * Loads 4 BYTEs from unaligned memory and converts them into 4 FLOATs.
 * IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar FLOATs after you've used this intrinsic!
 *
 * \param ptr - Unaligned memory pointer to the 4 BYTEs.
 *
 * \return VectorRegister(float(ptr[0]), float(ptr[1]), float(ptr[2]), float(ptr[3]))
 */
#define VectorLoadByte4(ptr)  DirectX::PackedVector::XMLoadUByte4((const DirectX::PackedVector::XMUBYTE4*)(ptr))

/**
 * Loads 4 BYTEs from unaligned memory and converts them into 4 FLOATs in reversed order.
 * IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar FLOATs after you've used this intrinsic!
 *
 * \param ptr - Unaligned memory pointer to the 4 BYTEs.
 *
 * \return VectorRegister(float(ptr[3]), float(ptr[2]), float(ptr[1]), float(ptr[0]))
 */
FUN_ALWAYS_INLINE VectorRegister VectorLoadByte4Reverse(const uint8* ptr) {
  VectorRegister tmp = VectorLoadByte4(ptr);
  return VectorSwizzle(tmp, 3, 2, 1, 0);
}

/**
 * Converts the 4 FLOATs in the vector to 4 BYTEs, clamped to [0, 255], and stores to unaligned memory.
 * IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar FLOATs after you've used this intrinsic!
 *
 * \param vec - vector containing 4 FLOATs
 * \param ptr - Unaligned memory pointer to store the 4 BYTEs.
 */
#define VectorStoreByte4(vec, ptr)  DirectX::PackedVector::XMStoreUByte4((DirectX::PackedVector::XMUBYTE4*)(ptr), vec)

/**
 * Returns non-zero if any element in vec1 is greater than the corresponding element in vec2, otherwise 0.
 *
 * \param vec1 - 1st source vector
 * \param vec2 - 2nd source vector
 *
 * \return Non-zero integer if (vec1.x > vec2.x) || (vec1.y > vec2.y) || (vec1.z > vec2.z) || (vec1.w > vec2.w)
 */
FUN_ALWAYS_INLINE uint32 VectorAnyGreaterThan(const VectorRegister& vec1, const VectorRegister& vec2) {
  using namespace DirectX;
  // Returns a comparison value that can be examined using functions such as XMComparisonAllTrue
  uint32_t comparison_value = XMVector4GreaterR(vec1, vec2);

  //Returns true if any of the compared components are true
  return (uint32)XMComparisonAnyTrue(comparison_value);
}

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
 * Order matters when composing quaternions: C = VectorQuaternionMultiply2(A, B) will yield a quaternion C = AB
 * that logically first applies B then A to any subsequent transformation (right first, then left).
 *
 * \param quat1 - Pointer to the first quaternion
 * \param quat2 - Pointer to the second quaternion
 *
 * \return Quat1Quat2
 */
FUN_ALWAYS_INLINE VectorRegister VectorQuaternionMultiply2(const VectorRegister& quat1, const VectorRegister& quat2) {
  // DirectXMath uses reverse parameter order to FunMath
  // XMQuaternionMultiply(FXMVECTOR q1, FXMVECTOR q2)
  // Returns the product q2*q1 (which is the concatenation of a rotation q1 followed by the rotation q2)

  // [ (q2.wQ1.x) + (q2.xQ1.w) + (q2.yQ1.z) - (q2.zQ1.y),
  //   (q2.wQ1.y) - (q2.xQ1.z) + (q2.yQ1.w) + (q2.zQ1.x),
  //   (q2.wQ1.z) + (q2.xQ1.y) - (q2.yQ1.x) + (q2.zQ1.w),
  //   (q2.wQ1.w) - (q2.xQ1.x) - (q2.yQ1.y) - (q2.zQ1.z) ]
  return DirectX::XMQuaternionMultiply(quat2, quat1);
}

/**
 * Multiplies two quaternions; the order matters.
 *
 * When composing quaternions: VectorQuaternionMultiply(C, A, B) will yield a quaternion C = AB
 * that logically first applies B then A to any subsequent transformation (right first, then left).
 *
 * \param result - Pointer to where the result Quat1Quat2 should be stored
 * \param quat1 - Pointer to the first quaternion (must not be the destination)
 * \param quat2 - Pointer to the second quaternion (must not be the destination)
 */
FUN_ALWAYS_INLINE void VectorQuaternionMultiply(Quat* result, const Quat* quat1, const Quat* quat2) {
  VectorRegister xm_quat1 = VectorLoadAligned(quat1);
  VectorRegister xm_quat2 = VectorLoadAligned(quat2);
  VectorRegister xm_result = VectorQuaternionMultiply2(xm_quat1, xm_quat2);
  VectorStoreAligned(xm_result, result);
}

/**
 * Multiplies two quaternions; the order matters.
 *
 * When composing quaternions: VectorQuaternionMultiply(C, A, B) will yield a quaternion C = AB
 * that logically first applies B then A to any subsequent transformation (right first, then left).
 *
 * \param result - Pointer to where the result Quat1Quat2 should be stored
 * \param quat1 - Pointer to the first quaternion (must not be the destination)
 * \param quat2 - Pointer to the second quaternion (must not be the destination)
 */
FUN_ALWAYS_INLINE void VectorQuaternionMultiply(VectorRegister* result, const VectorRegister* vec_quat1, const VectorRegister* vec_quat2) {
  *result = VectorQuaternionMultiply2(*vec_quat1, *vec_quat2);
}

FUN_ALWAYS_INLINE void VectorQuaternionVector3Rotate(Vector* result, const Vector* vec, const Quat* quat) {
  VectorRegister xm_vec = VectorLoad(vec);
  VectorRegister xm_quat = VectorLoadAligned(quat);
  VectorRegister xm_result = DirectX::XMVector3Rotate(xm_vec, xm_quat);
  VectorStoreFloat3(xm_result, result);
}

FUN_ALWAYS_INLINE void VectorQuaternionVector3InverseRotate(Vector* result, const Vector* vec, const Quat* quat) {
  VectorRegister xm_vec = VectorLoad(vec);
  VectorRegister xm_quat = VectorLoadAligned(quat);
  VectorRegister xm_result = DirectX::XMVector3InverseRotate(xm_vec, xm_quat);
  VectorStoreFloat3(xm_result, result);
}

/**
 * Computes the sine and cosine of each component of a vector.
 *
 * \param vec_sin_angles - VectorRegister Pointer to where the Sin result should be stored
 * \param vec_cos_angles - VectorRegister Pointer to where the Cos result should be stored
 * \param vec_angles - VectorRegister Pointer to the input angles
 */
FUN_ALWAYS_INLINE void VectorSinCos( VectorRegister* __restrict vec_sin_angles,
                          VectorRegister* __restrict vec_cos_angles,
                          const VectorRegister* __restrict vec_angles) {
  using namespace DirectX;
  // Force the value within the bounds of pi
  XMVECTOR x = XMVectorModAngles(*vec_angles);

  // Map in [-pi/2, pi/2] with sin(y) = sin(x), cos(y) = sign*cos(x).
  XMVECTOR sign = _mm_and_ps(x, g_XMNegativeZero);
  __m128 c = _mm_or_ps(g_XMPi, sign);  // pi when x >= 0, -pi when x < 0
  __m128 absx = _mm_andnot_ps(sign, x);  // |x|
  __m128 rflx = _mm_sub_ps(c, x);
  __m128 comp = _mm_cmple_ps(absx, g_XMHalfPi);
  __m128 select0 = _mm_and_ps(comp, x);
  __m128 select1 = _mm_andnot_ps(comp, rflx);
  x = _mm_or_ps(select0, select1);
  select0 = _mm_and_ps(comp, g_XMOne);
  select1 = _mm_andnot_ps(comp, g_XMNegativeOne);
  sign = _mm_or_ps(select0, select1);

  __m128 x2 = _mm_mul_ps(x, x);

  // Compute polynomial approximation of sine
  const XMVECTOR sc1 = g_XMSinCoefficients1;
  XMVECTOR v_constants = XM_PERMUTE_PS(sc1, _MM_SHUFFLE(0, 0, 0, 0));
  __m128 result = _mm_mul_ps(v_constants, x2);

  const XMVECTOR sc0 = g_XMSinCoefficients0;
  v_constants = XM_PERMUTE_PS(sc0, _MM_SHUFFLE(3, 3, 3, 3));
  result = _mm_add_ps(result, v_constants);
  result = _mm_mul_ps(result, x2);

  v_constants = XM_PERMUTE_PS(sc0, _MM_SHUFFLE(2, 2, 2, 2));
  result = _mm_add_ps(result, v_constants);
  result = _mm_mul_ps(result, x2);

  v_constants = XM_PERMUTE_PS(sc0, _MM_SHUFFLE(1, 1, 1, 1));
  result = _mm_add_ps(result, v_constants);
  result = _mm_mul_ps(result, x2);

  v_constants = XM_PERMUTE_PS(sc0, _MM_SHUFFLE(0, 0, 0, 0));
  result = _mm_add_ps(result, v_constants);
  result = _mm_mul_ps(result, x2);
  result = _mm_add_ps(result, g_XMOne);
  result = _mm_mul_ps(result, x);
  *vec_sin_angles = result;

  // Compute polynomial approximation of cosine
  const XMVECTOR cc1 = g_XMCosCoefficients1;
  v_constants = XM_PERMUTE_PS(cc1, _MM_SHUFFLE(0, 0, 0, 0));
  result = _mm_mul_ps(v_constants, x2);

  const XMVECTOR cc0 = g_XMCosCoefficients0;
  v_constants = XM_PERMUTE_PS(cc0, _MM_SHUFFLE(3, 3, 3, 3));
  result = _mm_add_ps(result, v_constants);
  result = _mm_mul_ps(result, x2);

  v_constants = XM_PERMUTE_PS(cc0, _MM_SHUFFLE(2, 2, 2, 2));
  result = _mm_add_ps(result, v_constants);
  result = _mm_mul_ps(result, x2);

  v_constants = XM_PERMUTE_PS(cc0, _MM_SHUFFLE(1, 1, 1, 1));
  result = _mm_add_ps(result, v_constants);
  result = _mm_mul_ps(result, x2);

  v_constants = XM_PERMUTE_PS(cc0, _MM_SHUFFLE(0, 0, 0, 0));
  result = _mm_add_ps(result, v_constants);
  result = _mm_mul_ps(result, x2);
  result = _mm_add_ps(result, g_XMOne);
  result = _mm_mul_ps(result, sign);
  *vec_cos_angles = result;
}

// Returns true if the vector contains a component that is either NAN or +/-infinite.
FUN_ALWAYS_INLINE bool VectorContainsNaNOrInfinite(const VectorRegister& vec) {
  using namespace DirectX;
  return (XMVector4IsNaN(vec) || XMVector4IsInfinite(vec));
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorExp(const VectorRegister& x) {
  return MakeVectorRegister(Math::Exp(VectorGetComponent(x, 0)), Math::Exp(VectorGetComponent(x, 1)), Math::Exp(VectorGetComponent(x, 2)), Math::Exp(VectorGetComponent(x, 3)));
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorExp2(const VectorRegister& x) {
  return DirectX::XMVectorExp2(x);
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorLog(const VectorRegister& x) {
  return MakeVectorRegister(Math::Loge(VectorGetComponent(x, 0)), Math::Loge(VectorGetComponent(x, 1)), Math::Loge(VectorGetComponent(x, 2)), Math::Loge(VectorGetComponent(x, 3)));
}

//TODO: Vectorize
FUN_ALWAYS_INLINE VectorRegister VectorLog2(const VectorRegister& x) {
  return DirectX::XMVectorLog2(x);
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

FUN_ALWAYS_INLINE VectorRegister VectorCeil(const VectorRegister& x) {
  return DirectX::XMVectorCeiling(x);
}

FUN_ALWAYS_INLINE VectorRegister VectorFloor(const VectorRegister& x) {
  return DirectX::XMVectorFloor(x);
}

FUN_ALWAYS_INLINE VectorRegister VectorTruncate(const VectorRegister& x) {
  return DirectX::XMVectorTruncate(x);
}

FUN_ALWAYS_INLINE VectorRegister VectorFractional(const VectorRegister& x) {
  return VectorSubtract(x, VectorTruncate(x));
}

FUN_ALWAYS_INLINE VectorRegister VectorMod(const VectorRegister& x, const VectorRegister& y) {
  return DirectX::XMVectorMod(x, y);
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

#endif //!__cplusplus_cli
