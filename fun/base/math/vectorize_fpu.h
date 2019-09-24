#pragma once

#include "fun/base/base.h"
#include "FunMemory.h"

namespace fun {

//
// Helpers:
//

/**
 * float4 vector register type, where the first float (x)
 * is stored in the lowest 32 bits, and so on.
 */
struct VectorRegister {
  float v[4];
};

// For a struct of 4 floats, we need the double braces
#define DECLARE_VECTOR_REGISTER(x, y, z, w)  { { x, y, z, w } }

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
  VectorRegister vec;
  ((uint32&)vec.v[0]) = x;
  ((uint32&)vec.v[1]) = y;
  ((uint32&)vec.v[2]) = z;
  ((uint32&)vec.v[3]) = w;
  return vec;
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
  VectorRegister vec = { { x, y, z, w } };
  return vec;
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
#define VectorZero()  (VectorizeConstants::FloatZero)

/**
 * Returns a vector with all ones.
 *
 * \return VectorRegister(1.f, 1.f, 1.f, 1.f)
 */
#define VectorOne()  (VectorizeConstants::FloatOne)

/**
 * Loads 4 FLOATs from unaligned memory.
 *
 * \param ptr - Unaligned memory pointer to the 4 FLOATs
 *
 * \return VectorRegister(ptr[0], ptr[1], ptr[2], ptr[3])
 */
#define VectorLoad(ptr)  MakeVectorRegister(((const float*)(ptr))[0], ((const float*)(ptr))[1], ((const float*)(ptr))[2], ((const float*)(ptr))[3])

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
#define VectorLoadAligned(ptr)  MakeVectorRegister(((const float*)(ptr))[0], ((const float*)(ptr))[1], ((const float*)(ptr))[2], ((const float*)(ptr))[3])

/**
 * Loads 1 float from unaligned memory and replicates it to all 4 elements.
 *
 * \param ptr - Unaligned memory pointer to the float
 *
 * \return VectorRegister(ptr[0], ptr[0], ptr[0], ptr[0])
 */
#define VectorLoadFloat1(ptr)  MakeVectorRegister(((const float*)(ptr))[0], ((const float*)(ptr))[0], ((const float*)(ptr))[0], ((const float*)(ptr))[0])

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
Stores a vector to aligned memory.

\param vec - vector to store
\param ptr - Aligned memory pointer
*/
#define VectorStoreAligned(vec, ptr)  UnsafeMemory::Memcpy(ptr, &(vec), 16)

/**
 * Performs non-temporal store of a vector to aligned memory without polluting the caches
 *
 * \param vec - vector to store
 * \param ptr - Aligned memory pointer
*/
#define VectorStoreAlignedStreamed(vec, ptr)  VectorStoreAligned(vec , ptr)

/**
 * Stores a vector to memory (aligned or unaligned).
 *
 * \param vec - vector to store
 * \param ptr - Memory pointer
 */
#define VectorStore(vec, ptr)  UnsafeMemory::Memcpy(ptr, &(vec), 16)

/**
 * Stores the XYZ components of a vector to unaligned memory.
 *
 * \param vec - vector to store XYZ
 * \param ptr - Unaligned memory pointer
 */
#define VectorStoreFloat3(vec, ptr)  UnsafeMemory::Memcpy(ptr, &(vec), 12)

/**
 * Stores the x component of a vector to unaligned memory.
 *
 * \param vec - vector to store x
 * \param ptr - Unaligned memory pointer
 */
#define VectorStoreFloat1(vec, ptr)  UnsafeMemory::Memcpy(ptr, &(vec), 4)

/**
 * Replicates one element into all four elements and returns the new vector.
 *
 * \param vec - Source vector
 * \param element_index - Index (0-3) of the element to replicate
 *
 * \return VectorRegister(vec[element_index], vec[element_index], vec[element_index], vec[element_index])
 */
#define VectorReplicate(vec, element_index)  MakeVectorRegister((vec).v[element_index], (vec).v[element_index], (vec).v[element_index], (vec).v[element_index])

/**
 * Returns the absolute value (component-wise).
 *
 * \param vec - Source vector
 *
 * \return VectorRegister(abs(vec.x), abs(vec.y), abs(vec.z), abs(vec.w))
 */
FUN_ALWAYS_INLINE VectorRegister VectorAbs(const VectorRegister& vec) {
  VectorRegister ret;
  ret.v[0] = Math::Abs(vec.v[0]);
  ret.v[1] = Math::Abs(vec.v[1]);
  ret.v[2] = Math::Abs(vec.v[2]);
  ret.v[3] = Math::Abs(vec.v[3]);
  return ret;
}

/**
 * Returns the negated value (component-wise).
 *
 * \param vec - Source vector
 *
 * \return VectorRegister(-vec.x, -vec.y, -vec.z, -vec.w)
 */
#define VectorNegate(vec)  MakeVectorRegister(-(vec).v[0], -(vec).v[1], -(vec).v[2], -(vec).v[3])

/**
 * adds two vectors (component-wise) and returns the result.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x+vec2.x, vec1.y+vec2.y, vec1.z+vec2.z, vec1.w+vec2.w)
 */
FUN_ALWAYS_INLINE VectorRegister VectorAdd(const VectorRegister& vec1, const VectorRegister& vec2) {
  VectorRegister ret;
  ret.v[0] = vec1.v[0] + vec2.v[0];
  ret.v[1] = vec1.v[1] + vec2.v[1];
  ret.v[2] = vec1.v[2] + vec2.v[2];
  ret.v[3] = vec1.v[3] + vec2.v[3];
  return ret;
}

/**
 * subtracts a vector from another (component-wise) and returns the result.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x-vec2.x, vec1.y-vec2.y, vec1.z-vec2.z, vec1.w-vec2.w)
 */
FUN_ALWAYS_INLINE VectorRegister VectorSubtract(const VectorRegister& vec1, const VectorRegister& vec2) {
  VectorRegister ret;
  ret.v[0] = vec1.v[0] - vec2.v[0];
  ret.v[1] = vec1.v[1] - vec2.v[1];
  ret.v[2] = vec1.v[2] - vec2.v[2];
  ret.v[3] = vec1.v[3] - vec2.v[3];
  return ret;
}

/**
 * Multiplies two vectors (component-wise) and returns the result.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x*vec2.x, vec1.y*vec2.y, vec1.z*vec2.z, vec1.w*vec2.w)
 */
FUN_ALWAYS_INLINE VectorRegister VectorMultiply(const VectorRegister& vec1, const VectorRegister& vec2) {
  VectorRegister ret;
  ret.v[0] = vec1.v[0] * vec2.v[0];
  ret.v[1] = vec1.v[1] * vec2.v[1];
  ret.v[2] = vec1.v[2] * vec2.v[2];
  ret.v[3] = vec1.v[3] * vec2.v[3];
  return ret;
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
FUN_ALWAYS_INLINE VectorRegister VectorMultiplyAdd(const VectorRegister& vec1, const VectorRegister& vec2, const VectorRegister& vec3) {
  VectorRegister ret;
  ret.v[0] = vec1.v[0] * vec2.v[0] + vec3.v[0];
  ret.v[1] = vec1.v[1] * vec2.v[1] + vec3.v[1];
  ret.v[2] = vec1.v[2] * vec2.v[2] + vec3.v[2];
  ret.v[3] = vec1.v[3] * vec2.v[3] + vec3.v[3];
  return ret;
}

/**
 * Divides two vectors (component-wise) and returns the result.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(vec1.x/vec2.x, vec1.y/vec2.y, vec1.z/vec2.z, vec1.w/vec2.w)
 */
FUN_ALWAYS_INLINE VectorRegister VectorDivide(const VectorRegister& vec1, const VectorRegister& vec2) {
  VectorRegister ret;
  ret.v[0] = vec1.v[0] / vec2.v[0];
  ret.v[1] = vec1.v[1] / vec2.v[1];
  ret.v[2] = vec1.v[2] / vec2.v[2];
  ret.v[3] = vec1.v[3] / vec2.v[3];
  return ret;
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
  const float dot = vec1.v[0] * vec2.v[0] + vec1.v[1] * vec2.v[1] + vec1.v[2] * vec2.v[2];
  VectorRegister vec = { { dot, dot, dot, dot } };
  return vec;
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
  const float dot = vec1.v[0] * vec2.v[0] + vec1.v[1] * vec2.v[1] + vec1.v[2] * vec2.v[2] + vec1.v[3] * vec2.v[3];
  VectorRegister vec = { { dot, dot, dot, dot } };
  return vec;
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
  return MakeVectorRegister(
    (uint32)( vec1.v[0] == vec2.v[0] ? 0xFFFFFFFF : 0),
              vec1.v[1] == vec2.v[1] ? 0xFFFFFFFF : 0,
              vec1.v[2] == vec2.v[2] ? 0xFFFFFFFF : 0,
              vec1.v[3] == vec2.v[3] ? 0xFFFFFFFF : 0);
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
  return MakeVectorRegister(
    (uint32)( vec1.v[0] != vec2.v[0] ? 0xFFFFFFFF : 0),
              vec1.v[1] != vec2.v[1] ? 0xFFFFFFFF : 0,
              vec1.v[2] != vec2.v[2] ? 0xFFFFFFFF : 0,
              vec1.v[3] != vec2.v[3] ? 0xFFFFFFFF : 0);
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
  return MakeVectorRegister(
    (uint32)( vec1.v[0] > vec2.v[0] ? 0xFFFFFFFF : 0),
              vec1.v[1] > vec2.v[1] ? 0xFFFFFFFF : 0,
              vec1.v[2] > vec2.v[2] ? 0xFFFFFFFF : 0,
              vec1.v[3] > vec2.v[3] ? 0xFFFFFFFF : 0);
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
  return MakeVectorRegister(
    (uint32)( vec1.v[0] >= vec2.v[0] ? 0xFFFFFFFF : 0),
              vec1.v[1] >= vec2.v[1] ? 0xFFFFFFFF : 0,
              vec1.v[2] >= vec2.v[2] ? 0xFFFFFFFF : 0,
              vec1.v[3] >= vec2.v[3] ? 0xFFFFFFFF : 0);
}

/**
 * Does a bitwise vector selection based on a mask (e.g., created from VectorCompareXX)
 *
 * \param mask  mask (when 1: use the corresponding bit from vec1 otherwise from vec2)
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(for each bit i: mask[i] ? vec1[i] : vec2[i])
 */
FUN_ALWAYS_INLINE VectorRegister VectorSelect(const VectorRegister& mask, const VectorRegister& vec1, const VectorRegister& vec2) {
  const uint32* v1 = (uint32*)(&(vec1.v[0]));
  const uint32* v2 = (uint32*)(&(vec2.v[0]));
  const uint32* m  = (uint32*)(&(mask.v[0]));

  return MakeVectorRegister(
          v2[0] ^ (m[0] & (v2[0] ^ v1[0])),
          v2[1] ^ (m[1] & (v2[1] ^ v1[1])),
          v2[2] ^ (m[2] & (v2[2] ^ v1[2])),
          v2[3] ^ (m[3] & (v2[3] ^ v1[3])));
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
  return MakeVectorRegister(
    (uint32)( ((uint32*)(vec1.v))[0] | ((uint32*)(vec2.v))[0]),
              ((uint32*)(vec1.v))[1] | ((uint32*)(vec2.v))[1],
              ((uint32*)(vec1.v))[2] | ((uint32*)(vec2.v))[2],
              ((uint32*)(vec1.v))[3] | ((uint32*)(vec2.v))[3]);
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
  return MakeVectorRegister(
    (uint32)( ((uint32*)(vec1.v))[0] & ((uint32*)(vec2.v))[0]),
              ((uint32*)(vec1.v))[1] & ((uint32*)(vec2.v))[1],
              ((uint32*)(vec1.v))[2] & ((uint32*)(vec2.v))[2],
              ((uint32*)(vec1.v))[3] & ((uint32*)(vec2.v))[3]);
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
  return MakeVectorRegister(
    (uint32)( ((uint32*)(vec1.v))[0] ^ ((uint32*)(vec2.v))[0]),
              ((uint32*)(vec1.v))[1] ^ ((uint32*)(vec2.v))[1],
              ((uint32*)(vec1.v))[2] ^ ((uint32*)(vec2.v))[2],
              ((uint32*)(vec1.v))[3] ^ ((uint32*)(vec2.v))[3]);
}

/**
 * Calculates the cross product of two vectors (XYZ components). w is set to 0.
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return cross(vec1.xyz, vec2.xyz). w is set to 0.
 */
FUN_ALWAYS_INLINE VectorRegister VectorCross(const VectorRegister& vec1, const VectorRegister& vec2) {
  VectorRegister vec;
  vec.v[0] = vec1.v[1] * vec2.v[2] - vec1.v[2] * vec2.v[1];
  vec.v[1] = vec1.v[2] * vec2.v[0] - vec1.v[0] * vec2.v[2];
  vec.v[2] = vec1.v[0] * vec2.v[1] - vec1.v[1] * vec2.v[0];
  vec.v[3] = 0.f;
  return vec;
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
  VectorRegister vec;
  vec.v[0] = Math::Pow(base.v[0], exponent.v[0]);
  vec.v[1] = Math::Pow(base.v[1], exponent.v[1]);
  vec.v[2] = Math::Pow(base.v[2], exponent.v[2]);
  vec.v[3] = Math::Pow(base.v[3], exponent.v[3]);
  return vec;
}

/**
 * Returns an estimate of 1/sqrt(c) for each component of the vector
 *
 * \param vec - vector
 *
 * \return VectorRegister(1/sqrt(t), 1/sqrt(t), 1/sqrt(t), 1/sqrt(t))
 */
FUN_ALWAYS_INLINE VectorRegister VectorReciprocalSqrt(const VectorRegister& vec) {
  return MakeVectorRegister(1.f / Math::Sqrt(vec.v[0]), 1.f / Math::Sqrt(vec.v[1]), 1.f / Math::Sqrt(vec.v[2]), 1.f / Math::Sqrt(vec.v[3]));
}

/**
 * Computes an estimate of the reciprocal of a vector (component-wise) and returns the result.
 *
 * \param vec - 1st vector
 *
 * \return VectorRegister((Estimate) 1.f / vec.x, (Estimate) 1.f / vec.y, (Estimate) 1.f / vec.z, (Estimate) 1.f / vec.w)
 */
FUN_ALWAYS_INLINE VectorRegister VectorReciprocal(const VectorRegister& vec) {
  return MakeVectorRegister(1.f / vec.v[0], 1.f / vec.v[1], 1.f / vec.v[2], 1.f / vec.v[3]);
}

/**
 * Return Reciprocal Length of the vector
 *
 * \param vec - vector
 *
 * \return VectorRegister(ool, ool, ool, ool) when ool = 1/sqrt(dot4(v))
 */
FUN_ALWAYS_INLINE VectorRegister VectorReciprocalLen(const VectorRegister& vector) {
  VectorRegister l = VectorDot4(vector, vector);
  float ool = 1.f / Math::Sqrt(l.v[0]);

  VectorRegister result;
  result.v[0] = ool;
  result.v[1] = ool;
  result.v[2] = ool;
  result.v[3] = ool;
  return result;
}

/**
 * Return the reciprocal of the square root of each component
 *
 * \param vec - vector
 *
 * \return VectorRegister(1/sqrt(vec.x), 1/sqrt(vec.y), 1/sqrt(vec.z), 1/sqrt(vec.w))
 */
#define VectorReciprocalSqrtAccurate(vec)  VectorReciprocalSqrt(vec)

/**
 * Computes the reciprocal of a vector (component-wise) and returns the result.
 *
 * \param vec - 1st vector
 *
 * \return VectorRegister(1.f / vec.x, 1.f / vec.y, 1.f / vec.z, 1.f / vec.w)
 */
#define VectorReciprocalAccurate(vec)  VectorReciprocal(vec)

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
#define VectorSet_W0(vec)  MakeVectorRegister((vec).v[0], (vec).v[1], (vec).v[2], 0.f)

/**
 * Loads XYZ and sets w=1
 *
 * \param vec - VectorRegister
 *
 * \return VectorRegister(x, y, z, 1.f)
 */
#define VectorSet_W1(vec)  MakeVectorRegister((vec).v[0], (vec).v[1], (vec).v[2], 1.f)


// 40% faster version of the Quaternion multiplication.
#define USE_FAST_QUAT_MUL  1

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
FUN_ALWAYS_INLINE void VectorQuaternionMultiply(void* result, const void* quat1, const void* quat2) {
  typedef float Float4[4];
  const Float4& a = *((const Float4*)quat1);
  const Float4& b = *((const Float4*)quat2);
  Float4& r = *((Float4*)result);

#if USE_FAST_QUAT_MUL
  const float t0 = (a[2] - a[1]) * (b[1] - b[2]);
  const float t1 = (a[3] + a[0]) * (b[3] + b[0]);
  const float t2 = (a[3] - a[0]) * (b[1] + b[2]);
  const float t3 = (a[1] + a[2]) * (b[3] - b[0]);
  const float t4 = (a[2] - a[0]) * (b[0] - b[1]);
  const float t5 = (a[2] + a[0]) * (b[0] + b[1]);
  const float t6 = (a[3] + a[1]) * (b[3] - b[2]);
  const float t7 = (a[3] - a[1]) * (b[3] + b[2]);
  const float t8 = t5 + t6 + t7;
  const float t9 = 0.5f * (t4 + t8);

  r[0] = t1 + t9 - t8;
  r[1] = t2 + t9 - t7;
  r[2] = t3 + t9 - t6;
  r[3] = t0 + t9 - t5;
#else
  // store intermediate results in temporaries
  const float tx = a[3]*b[0] + a[0]*b[3] + a[1]*b[2] - a[2]*b[1];
  const float ty = a[3]*b[1] - a[0]*b[2] + a[1]*b[3] + a[2]*b[0];
  const float tz = a[3]*b[2] + a[0]*b[1] - a[1]*b[0] + a[2]*b[3];
  const float tw = a[3]*b[3] - a[0]*b[0] - a[1]*b[1] - a[2]*b[2];

  // copy intermediate result to *this
  r[0] = tx;
  r[1] = ty;
  r[2] = tz;
  r[3] = tw;
#endif
}

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
  VectorRegister result;
  VectorQuaternionMultiply(&result, &quat1, &quat2);
  return result;
}

/**
 * Multiplies two 4x4 matrices.
 *
 * \param result - Pointer to where the result should be stored
 * \param matrix1 - Pointer to the first matrix
 * \param matrix2 - Pointer to the second matrix
 */
FUN_ALWAYS_INLINE void VectorMatrixMultiply(void* result, const void* matrix1, const void* matrix2) {
  typedef float Float4x4[4][4];
  const Float4x4& a = *((const Float4x4*)matrix1);
  const Float4x4& b = *((const Float4x4*)matrix2);
  Float4x4 tmp;
  tmp[0][0] = a[0][0] * b[0][0] + a[0][1] * b[1][0] + a[0][2] * b[2][0] + a[0][3] * b[3][0];
  tmp[0][1] = a[0][0] * b[0][1] + a[0][1] * b[1][1] + a[0][2] * b[2][1] + a[0][3] * b[3][1];
  tmp[0][2] = a[0][0] * b[0][2] + a[0][1] * b[1][2] + a[0][2] * b[2][2] + a[0][3] * b[3][2];
  tmp[0][3] = a[0][0] * b[0][3] + a[0][1] * b[1][3] + a[0][2] * b[2][3] + a[0][3] * b[3][3];

  tmp[1][0] = a[1][0] * b[0][0] + a[1][1] * b[1][0] + a[1][2] * b[2][0] + a[1][3] * b[3][0];
  tmp[1][1] = a[1][0] * b[0][1] + a[1][1] * b[1][1] + a[1][2] * b[2][1] + a[1][3] * b[3][1];
  tmp[1][2] = a[1][0] * b[0][2] + a[1][1] * b[1][2] + a[1][2] * b[2][2] + a[1][3] * b[3][2];
  tmp[1][3] = a[1][0] * b[0][3] + a[1][1] * b[1][3] + a[1][2] * b[2][3] + a[1][3] * b[3][3];

  tmp[2][0] = a[2][0] * b[0][0] + a[2][1] * b[1][0] + a[2][2] * b[2][0] + a[2][3] * b[3][0];
  tmp[2][1] = a[2][0] * b[0][1] + a[2][1] * b[1][1] + a[2][2] * b[2][1] + a[2][3] * b[3][1];
  tmp[2][2] = a[2][0] * b[0][2] + a[2][1] * b[1][2] + a[2][2] * b[2][2] + a[2][3] * b[3][2];
  tmp[2][3] = a[2][0] * b[0][3] + a[2][1] * b[1][3] + a[2][2] * b[2][3] + a[2][3] * b[3][3];

  tmp[3][0] = a[3][0] * b[0][0] + a[3][1] * b[1][0] + a[3][2] * b[2][0] + a[3][3] * b[3][0];
  tmp[3][1] = a[3][0] * b[0][1] + a[3][1] * b[1][1] + a[3][2] * b[2][1] + a[3][3] * b[3][1];
  tmp[3][2] = a[3][0] * b[0][2] + a[3][1] * b[1][2] + a[3][2] * b[2][2] + a[3][3] * b[3][2];
  tmp[3][3] = a[3][0] * b[0][3] + a[3][1] * b[1][3] + a[3][2] * b[2][3] + a[3][3] * b[3][3];
  UnsafeMemory::Memcpy(result, &tmp, 16*sizeof(float));
}

/**
 * Calculate the inverse of an Matrix.
 *
 * \param dst_matrix - Matrix pointer to where the result should be stored
 * \param src_matrix - Matrix pointer to the matrix to be inversed
 */
FUN_ALWAYS_INLINE void VectorMatrixInverse(void* dst_matrix, const void* src_matrix) {
  typedef float Float4x4[4][4];
  const Float4x4& m = *((const Float4x4*)src_matrix);
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
  typedef float Float4x4[4][4];
  union { VectorRegister v; float f[4]; } tmp, result;
  tmp.v = vec_p;
  const Float4x4& m = *((const Float4x4*)matrix_m);

  result.f[0] = tmp.f[0] * m[0][0] + tmp.f[1] * m[1][0] + tmp.f[2] * m[2][0] + tmp.f[3] * m[3][0];
  result.f[1] = tmp.f[0] * m[0][1] + tmp.f[1] * m[1][1] + tmp.f[2] * m[2][1] + tmp.f[3] * m[3][1];
  result.f[2] = tmp.f[0] * m[0][2] + tmp.f[1] * m[1][2] + tmp.f[2] * m[2][2] + tmp.f[3] * m[3][2];
  result.f[3] = tmp.f[0] * m[0][3] + tmp.f[1] * m[1][3] + tmp.f[2] * m[2][3] + tmp.f[3] * m[3][3];

  return result.v;
}

/**
 * Returns the minimum values of two vectors (component-wise).
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(min(vec1.x, vec2.x), min(vec1.y, vec2.y), min(vec1.z, vec2.z), min(vec1.w, vec2.w))
 */
FUN_ALWAYS_INLINE VectorRegister VectorMin(const VectorRegister& vec1, const VectorRegister& vec2) {
  VectorRegister vec;
  vec.v[0] = Math::Min(vec1.v[0], vec2.v[0]);
  vec.v[1] = Math::Min(vec1.v[1], vec2.v[1]);
  vec.v[2] = Math::Min(vec1.v[2], vec2.v[2]);
  vec.v[3] = Math::Min(vec1.v[3], vec2.v[3]);
  return vec;
}

/**
 * Returns the maximum values of two vectors (component-wise).
 *
 * \param vec1 - 1st vector
 * \param vec2 - 2nd vector
 *
 * \return VectorRegister(max(vec1.x, vec2.x), max(vec1.y, vec2.y), max(vec1.z, vec2.z), max(vec1.w, vec2.w))
 */
FUN_ALWAYS_INLINE VectorRegister VectorMax(const VectorRegister& vec1, const VectorRegister& vec2) {
  VectorRegister vec;
  vec.v[0] = Math::Max(vec1.v[0], vec2.v[0]);
  vec.v[1] = Math::Max(vec1.v[1], vec2.v[1]);
  vec.v[2] = Math::Max(vec1.v[2], vec2.v[2]);
  vec.v[3] = Math::Max(vec1.v[3], vec2.v[3]);
  return vec;
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
#define VectorSwizzle(vec, x, y, z, w)  MakeVectorRegister((vec).v[x], (vec).v[y], (vec).v[z], (vec).v[w])

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
#define VectorShuffle(vec1, vec2, x, y, z, w)  MakeVectorRegister((vec1).v[x], (vec1).v[y], (vec2).v[z], (vec2).v[w])

/**
 * Merges the XYZ components of one vector with the w component of another vector and returns the result.
 *
 * \param vec_xyz - Source vector for XYZ_
 * \param vec_w - Source register for ___W (note: the fourth component is used, not the first)
 *
 * \return VectorRegister(vec_xyz.x, vec_xyz.y, vec_xyz.z, vec_w.w)
 */
FUN_ALWAYS_INLINE VectorRegister VectorMergeVecXYZ_VecW(const VectorRegister& vec_xyz, const VectorRegister& vec_w) {
  return MakeVectorRegister(vec_xyz.v[0], vec_xyz.v[1], vec_xyz.v[2], vec_w.v[3]);
}

/**
 * Loads 4 BYTEs from unaligned memory and converts them into 4 FLOATs.
 * IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar FLOATs after you've used this intrinsic!
 *
 * \param ptr - Unaligned memory pointer to the 4 BYTEs.
 *
 * \return VectorRegister(float(ptr[0]), float(ptr[1]), float(ptr[2]), float(ptr[3]))
 */
#define VectorLoadByte4(ptr)  MakeVectorRegister(float(((const uint8*)(ptr))[0]), float(((const uint8*)(ptr))[1]), float(((const uint8*)(ptr))[2]), float(((const uint8*)(ptr))[3]))

/**
 * Loads 4 BYTEs from unaligned memory and converts them into 4 FLOATs in reversed order.
 * IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar FLOATs after you've used this intrinsic!
 *
 * \param ptr - Unaligned memory pointer to the 4 BYTEs.
 *
 * \return VectorRegister(float(ptr[3]), float(ptr[2]), float(ptr[1]), float(ptr[0]))
 */
#define VectorLoadByte4Reverse(ptr)  MakeVectorRegister(float(((const uint8*)(ptr))[3]), float(((const uint8*)(ptr))[2]), float(((const uint8*)(ptr))[1]), float(((const uint8*)(ptr))[0]))

/**
 * Converts the 4 FLOATs in the vector to 4 BYTEs, clamped to [0, 255], and stores to unaligned memory.
 * IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar FLOATs after you've used this intrinsic!
 *
 * \param vec - vector containing 4 FLOATs
 * \param ptr - Unaligned memory pointer to store the 4 BYTEs.
 */
FUN_ALWAYS_INLINE void VectorStoreByte4(const VectorRegister& vec, void* ptr) {
  uint8* b = (uint8*)ptr;
  b[0] = uint8(vec.v[0]);
  b[1] = uint8(vec.v[1]);
  b[2] = uint8(vec.v[2]);
  b[3] = uint8(vec.v[3]);
}

/**
 * Returns non-zero if any element in vec1 is greater than the corresponding element in vec2, otherwise 0.
 *
 * \param vec1 - 1st source vector
 * \param vec2 - 2nd source vector
 *
 * \return Non-zero integer if (vec1.x > vec2.x) || (vec1.y > vec2.y) || (vec1.z > vec2.z) || (vec1.w > vec2.w)
 */
FUN_ALWAYS_INLINE uint32 VectorAnyGreaterThan(const VectorRegister& vec1, const VectorRegister& vec2) {
  // Note: Bitwise OR:ing all results together to avoid branching.
  return (vec1.v[0] > vec2.v[0]) | (vec1.v[1] > vec2.v[1]) | (vec1.v[2] > vec2.v[2]) | (vec1.v[3] > vec2.v[3]);
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
 * Returns an component from a vector.
 *
 * \param vec - vector register
 * \param component_index - Which component to get, x=0, y=1, z=2, w=3
 *
 * \return The component as a float
 */
FUN_ALWAYS_INLINE float VectorGetComponent(VectorRegister vec, uint32 component_index) {
  return (((float*)&(vec))[component_index]);
}


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


/**
 * Computes the sine and cosine of each component of a vector.
 *
 * \param vec_sin_angles - VectorRegister Pointer to where the Sin result should be stored
 * \param vec_cos_angles - VectorRegister Pointer to where the Cos result should be stored
 * \param vec_angles - VectorRegister Pointer to the input angles
 */
FUN_ALWAYS_INLINE void VectorSinCos(VectorRegister* vec_sin_angles, VectorRegister* vec_cos_angles, const VectorRegister* vec_angles) {
  union { VectorRegister v; float F[4]; } vec_sin, vec_cos, vec_angles;
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
  return Math::IsNaN(vec.v[0]) || Math::IsNaN(vec.v[1]) || Math::IsNaN(vec.v[2]) || Math::IsNaN(vec.v[3]) ||
        !Math::IsFinite(vec.v[0]) || !Math::IsFinite(vec.v[1]) || !Math::IsFinite(vec.v[2]) || !Math::IsFinite(vec.v[3]);
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
