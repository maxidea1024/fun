#include "fun/base/digest/md4_digester.h"

namespace fun {

MD4Disgester::MD4Disgester() {
  // NOOP
}

MD4Disgester::~MD4Disgester() {
  // NOOP
}

int32 MD4Disgester::GetDigestLength() {
  return DIGEST_SIZE;
}

void MD4Disgester::Reset() {
  // NOOP
}

const Digester::Digest& MD4Disgester::GetDigest() {
  static const uint8 PADDING[64] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };
  uint8 bits[8];
  uint32 index, pad_len;

  /* Save number of bits */
  Encode(bits, context_.count, 8);

  /* Pad out to 56 mod 64. */
  index = (uint32)((context_.count[0] >> 3) & 0x3f);
  pad_len = (index < 56) ? (56 - index) : (120 - index);
  Update(PADDING, pad_len);

  /* Append length (before padding) */
  Update(bits, 8);

  /* Store state in digest_array */
  uint8 digest_array[16];
  Encode(digest_array, context_.state, 16);

  digest_.Clear();
  digest_.Append((const char*)digest_array, sizeof(digest_array));

  /* Zeroize sensitive information. */
  UnsafeMemory::Memzero(&context_, sizeof(context_));
  Reset();
  return digest_;
}

void MD4Disgester::UpdateImpl(const void* _input, size_t input_len) {
  const uint8* input = (const uint8*)_input;
  uint32 i, index, part_len;

  // Compute number of bytes mod 64
  index = (uint32)((context_.count[0] >> 3) & 0x3F);

  // Update number of bits
  if ((context_.count[0] += ((uint32)input_len << 3)) < ((uint32)input_len << 3)) {
    context_.count[1]++;
  }
  context_.count[1] += ((uint32)input_len >> 29);

  part_len = 64 - index;

  // Transform as many times as possible.
  if (input_len >= part_len) {
    UnsafeMemory::Memcpy(&context_.buffer[index], input, part_len);
    Transform(context_.state, context_.buffer);

    for (i = part_len; i + 63 < input_len; i += 64) {
      Transform(context_.state, &input[i]);
    }

    index = 0;
  } else {
    i = 0;
  }

  /* Buffer remaining input */
  UnsafeMemory::Memcpy(&context_.buffer[index], &input[i], input_len-i);
}

/* Constants for MD4Transform routine. */
#define S11 3
#define S12 7
#define S13 11
#define S14 19
#define S21 3
#define S22 5
#define S23 9
#define S24 13
#define S31 3
#define S32 9
#define S33 11
#define S34 15

/* F, G and H are basic MD4 functions. */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))

/* ROTATE_LEFT rotates x left n bits. */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG and HH are transformations for rounds 1, 2 and 3 */
/* Rotation is separate from addition to prevent recomputation */
#define FF(a, b, c, d, x, s) { \
    (a) += F ((b), (c), (d)) + (x); \
    (a) = ROTATE_LEFT ((a), (s)); \
  }
#define GG(a, b, c, d, x, s) { \
    (a) += G ((b), (c), (d)) + (x) + (uint32)0x5a827999; \
    (a) = ROTATE_LEFT ((a), (s)); \
  }
#define HH(a, b, c, d, x, s) { \
    (a) += H ((b), (c), (d)) + (x) + (uint32)0x6ed9eba1; \
    (a) = ROTATE_LEFT ((a), (s)); \
  }

void MD4Disgester::Transform(uint32 state[4], const uint8 block[64]) {
  uint32 a = state[0], b = state[1], c = state[2], d = state[3], x[16];

  Decode(x, block, 64);

  /* Round 1 */
  FF(a, b, c, d, x[ 0], S11); /* 1 */
  FF(d, a, b, c, x[ 1], S12); /* 2 */
  FF(c, d, a, b, x[ 2], S13); /* 3 */
  FF(b, c, d, a, x[ 3], S14); /* 4 */
  FF(a, b, c, d, x[ 4], S11); /* 5 */
  FF(d, a, b, c, x[ 5], S12); /* 6 */
  FF(c, d, a, b, x[ 6], S13); /* 7 */
  FF(b, c, d, a, x[ 7], S14); /* 8 */
  FF(a, b, c, d, x[ 8], S11); /* 9 */
  FF(d, a, b, c, x[ 9], S12); /* 10 */
  FF(c, d, a, b, x[10], S13); /* 11 */
  FF(b, c, d, a, x[11], S14); /* 12 */
  FF(a, b, c, d, x[12], S11); /* 13 */
  FF(d, a, b, c, x[13], S12); /* 14 */
  FF(c, d, a, b, x[14], S13); /* 15 */
  FF(b, c, d, a, x[15], S14); /* 16 */

  /* Round 2 */
  GG(a, b, c, d, x[ 0], S21); /* 17 */
  GG(d, a, b, c, x[ 4], S22); /* 18 */
  GG(c, d, a, b, x[ 8], S23); /* 19 */
  GG(b, c, d, a, x[12], S24); /* 20 */
  GG(a, b, c, d, x[ 1], S21); /* 21 */
  GG(d, a, b, c, x[ 5], S22); /* 22 */
  GG(c, d, a, b, x[ 9], S23); /* 23 */
  GG(b, c, d, a, x[13], S24); /* 24 */
  GG(a, b, c, d, x[ 2], S21); /* 25 */
  GG(d, a, b, c, x[ 6], S22); /* 26 */
  GG(c, d, a, b, x[10], S23); /* 27 */
  GG(b, c, d, a, x[14], S24); /* 28 */
  GG(a, b, c, d, x[ 3], S21); /* 29 */
  GG(d, a, b, c, x[ 7], S22); /* 30 */
  GG(c, d, a, b, x[11], S23); /* 31 */
  GG(b, c, d, a, x[15], S24); /* 32 */

  /* Round 3 */
  HH(a, b, c, d, x[ 0], S31); /* 33 */
  HH(d, a, b, c, x[ 8], S32); /* 34 */
  HH(c, d, a, b, x[ 4], S33); /* 35 */
  HH(b, c, d, a, x[12], S34); /* 36 */
  HH(a, b, c, d, x[ 2], S31); /* 37 */
  HH(d, a, b, c, x[10], S32); /* 38 */
  HH(c, d, a, b, x[ 6], S33); /* 39 */
  HH(b, c, d, a, x[14], S34); /* 40 */
  HH(a, b, c, d, x[ 1], S31); /* 41 */
  HH(d, a, b, c, x[ 9], S32); /* 42 */
  HH(c, d, a, b, x[ 5], S33); /* 43 */
  HH(b, c, d, a, x[13], S34); /* 44 */
  HH(a, b, c, d, x[ 3], S31); /* 45 */
  HH(d, a, b, c, x[11], S32); /* 46 */
  HH(c, d, a, b, x[ 7], S33); /* 47 */
  HH(b, c, d, a, x[15], S34); /* 48 */

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;

  /* Zeroize sensitive information. */
  UnsafeMemory::Memset(x, 0, sizeof(x));
}

void MD4Disgester::Encode(uint8* output, const uint32* input, int32 len) {
  uint32 i, j;
  for (i = 0, j = 0; j < len; i++, j += 4) {
    output[j]   = (uint8)(input[i] & 0xff);
    output[j+1] = (uint8)((input[i] >> 8) & 0xff);
    output[j+2] = (uint8)((input[i] >> 16) & 0xff);
    output[j+3] = (uint8)((input[i] >> 24) & 0xff);
  }
}

void MD4Disgester::Decode(uint32* output, const uint8* input, int32 len) {
  uint32 i, j;
  for (i = 0, j = 0; j < len; i++, j += 4) {
    output[i] = ((uint32)input[j]) | (((uint32)input[j+1]) << 8) |
                (((uint32)input[j+2]) << 16) | (((uint32)input[j+3]) << 24);
  }
}

} // namespace fun
