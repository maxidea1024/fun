#include "fun/base/digest/sha3_digester.h"

namespace fun {

#ifdef _MSC_VER
#pragma intrinsic(memcpy)
#endif

#if defined(__GNUC__)
#define ALIGN(x) __attribute__ ((aligned(x)))
#elif defined(_MSC_VER)
#define ALIGN(x) __declspec(align(x))
#elif defined(__ARMCC_VERSION)
#define ALIGN(x) __align(x)
#else
#define ALIGN(x)
#endif

#ifdef FUN_PTR_IS_64_BIT
static const uint64 KeccakRoundConstants[24] = {
  0x0000000000000001,
  0x0000000000008082,
  0x800000000000808a,
  0x8000000080008000,
  0x000000000000808b,
  0x0000000080000001,
  0x8000000080008081,
  0x8000000000008009,
  0x000000000000008a,
  0x0000000000000088,
  0x0000000080008009,
  0x000000008000000a,
  0x000000008000808b,
  0x800000000000008b,
  0x8000000000008089,
  0x8000000000008003,
  0x8000000000008002,
  0x8000000000000080,
  0x000000000000800a,
  0x800000008000000a,
  0x8000000080008081,
  0x8000000000008080,
  0x0000000080000001,
  0x8000000080008008,
};
static const uint32 KeccakRhoOffsets[25] = {
  0, 1, 62, 28, 27, 36, 44, 6, 55, 20, 3, 10, 43, 25, 39, 41, 45, 15, 21, 8, 18, 2, 61, 56, 14
};
#define KeccakP1600_stateAlignment  8
#else
static const uint32 KeccakRoundConstants[24][2] = {
  0x00000001, 0x00000000,
  0x00000000, 0x00000089,
  0x00000000, 0x8000008B,
  0x00000000, 0x80008080,
  0x00000001, 0x0000008B,
  0x00000001, 0x00008000,
  0x00000001, 0x80008088,
  0x00000001, 0x80000082,
  0x00000000, 0x0000000B,
  0x00000000, 0x0000000A,
  0x00000001, 0x00008082,
  0x00000000, 0x00008003,
  0x00000001, 0x0000808B,
  0x00000001, 0x8000000B,
  0x00000001, 0x8000008A,
  0x00000001, 0x80000081,
  0x00000000, 0x80000081,
  0x00000000, 0x80000008,
  0x00000000, 0x00000083,
  0x00000000, 0x80008003,
  0x00000001, 0x80008088,
  0x00000000, 0x80000088,
  0x00000001, 0x00008000,
  0x00000000, 0x80008082
};

static const uint32 KeccakRhoOffsets[25] = {
  0, 1, 62, 28, 27, 36, 44, 6, 55, 20, 3, 10, 43, 25, 39, 41, 45, 15, 21, 8, 18, 2, 61, 56, 14
};
#define KeccakP1600_stateAlignment  4
#endif

ALIGN(KeccakP1600_stateAlignment)
typedef struct KeccakWidth1600_SpongeInstanceStruct {
  uint8 state[200];
  uint32 rate;
  uint32 byteIOIndex;
  int squeezing;
} KeccakWidth1600_SpongeInstance;

typedef struct {
  KeccakWidth1600_SpongeInstance sponge;
  uint32 fixedOutputLength;
  uint8 delimitedSuffix;
} HASHCONTEXT;


SHA3Digester::SHA3Digester(Algorithm algorithm)
  : context_(nullptr), algorithm_(algorithm) {
  digest_.Reserve(GetDigestLength());
  Reset();
}

SHA3Digester::~SHA3Digester() {
  Reset();
  UnsafeMemory::Free(context_);
}

#ifdef FUN_PTR_IS_64_BIT
void KeccakP1600_AddBytes(void *state, const uint8 *data, uint32 offset, uint32 length) {
  fun_check(offset < 200);
  fun_check(offset + length <= 200);
  for (uint32 i = 0; i<length; ++i) ((uint8 *)state)[offset + i] ^= data[i];
}

void KeccakP1600_AddByte(void *state, uint8 byte, uint32 offset) {
  fun_check(offset < 200);
  ((uint8 *)state)[offset] ^= byte;
}

#define ROL64(a, offset)  ((offset != 0) ? ((((uint64)a) << offset) ^ (((uint64)a) >> (64-offset))) : a)
#define index(x, y)  (((x)%5)+5*((y)%5))

static void theta(uint64 *A) {
  uint32 x, y;
  uint64 C[5], D[5];

  for (x = 0; x<5; x++) {
    C[x] = 0;
    for (y = 0; y<5; y++)
      C[x] ^= A[index(x, y)];
  }
  for (x = 0; x<5; x++)
    D[x] = ROL64(C[(x + 1) % 5], 1) ^ C[(x + 4) % 5];
  for (x = 0; x<5; x++)
    for (y = 0; y<5; y++)
      A[index(x, y)] ^= D[x];
}

static void rho(uint64 *A) {
  uint32 x, y;

  for (x = 0; x<5; x++) for (y = 0; y<5; y++)
    A[index(x, y)] = ROL64(A[index(x, y)], KeccakRhoOffsets[index(x, y)]);
}

static void pi(uint64 *A) {
  uint32 x, y;
  uint64 tempA[25];

  for (x = 0; x<5; x++) for (y = 0; y<5; y++)
    tempA[index(x, y)] = A[index(x, y)];
  for (x = 0; x<5; x++) for (y = 0; y<5; y++)
    A[index(0 * x + 1 * y, 2 * x + 3 * y)] = tempA[index(x, y)];
}

static void chi(uint64 *A) {
  uint32 x, y;
  uint64 C[5];

  for (y = 0; y<5; y++) {
    for (x = 0; x<5; x++)
      C[x] = A[index(x, y)] ^ ((~A[index(x + 1, y)]) & A[index(x + 2, y)]);
    for (x = 0; x<5; x++)
      A[index(x, y)] = C[x];
  }
}

static void iota(uint64 *A, uint32 indexRound) {
  A[index(0, 0)] ^= KeccakRoundConstants[indexRound];
}

void KeccakP1600Round(uint64 *state, uint32 indexRound) {
  theta(state);
  rho(state);
  pi(state);
  chi(state);
  iota(state, indexRound);
}

void KeccakP1600OnWords(uint64 *state, uint32 nrRounds) {
  for (uint32 i = (24 - nrRounds); i < 24; i++) KeccakP1600Round(state, i);
}

void KeccakP1600_Permute_24rounds(void *state) {
#if FUN_ARCH_LITTLE_ENDIAN
  KeccakP1600OnWords((uint64*)state, 24);
#else
  uint64 stateAsWords[1600 / 64];
  fromBytesToWords(stateAsWords, (const uint8 *)state);
  KeccakP1600OnWords(stateAsWords, 24);
  fromWordsToBytes((uint8 *)state, stateAsWords);
#endif
}

void KeccakP1600_ExtractBytes(const void *state, uint8 *data, uint32 offset, uint32 length) {
  fun_check(offset < 200);
  fun_check(offset + length <= 200);
  memcpy(data, (uint8*)state + offset, length);
}
#else
void toBitInterleaving(uint32 low, uint32 high, uint32 *even, uint32 *odd) {
  uint32 i;

  *even = 0;
  *odd = 0;
  for (i = 0; i<64; i++) {
    uint32 inBit;
    if (i < 32)
      inBit = (low >> i) & 1;
    else
      inBit = (high >> (i - 32)) & 1;
    if ((i % 2) == 0)
      *even |= inBit << (i / 2);
    else
      *odd |= inBit << ((i - 1) / 2);
  }
}

void fromBitInterleaving(uint32 even, uint32 odd, uint32 *low, uint32 *high) {
  uint32 i;

  *low = 0;
  *high = 0;
  for (i = 0; i<64; i++) {
    uint32 inBit;
    if ((i % 2) == 0)
      inBit = (even >> (i / 2)) & 1;
    else
      inBit = (odd >> ((i - 1) / 2)) & 1;
    if (i < 32)
      *low |= inBit << i;
    else
      *high |= inBit << (i - 32);
  }
}

void KeccakP1600_AddBytesInLane(void *state, uint32 lanePosition, const uint8 *data, uint32 offset, uint32 length) {
  if ((lanePosition < 25) && (offset < 8) && (offset + length <= 8)) {
    UInt8 laneAsBytes[8];
    uint32 low, high;
    uint32 lane[2];
    uint32 *stateAsHalfLanes;

    UnsafeMemory::Memset(laneAsBytes, 0, 8);
    memcpy(laneAsBytes + offset, data, length);
    low = laneAsBytes[0]
      | ((uint32)(laneAsBytes[1]) << 8)
      | ((uint32)(laneAsBytes[2]) << 16)
      | ((uint32)(laneAsBytes[3]) << 24);
    high = laneAsBytes[4]
      | ((uint32)(laneAsBytes[5]) << 8)
      | ((uint32)(laneAsBytes[6]) << 16)
      | ((uint32)(laneAsBytes[7]) << 24);
    toBitInterleaving(low, high, lane, lane + 1);
    stateAsHalfLanes = (uint32*)state;
    stateAsHalfLanes[lanePosition * 2 + 0] ^= lane[0];
    stateAsHalfLanes[lanePosition * 2 + 1] ^= lane[1];
  }
}

void KeccakP1600_AddBytes(void *state, const uint8 *data, uint32 offset, uint32 length) {
  uint32 lanePosition = offset / 8;
  uint32 offsetInLane = offset % 8;

  fun_check(offset < 200);
  fun_check(offset + length <= 200);
  while (length > 0) {
    uint32 bytesInLane = 8 - offsetInLane;
    if (bytesInLane > length)
      bytesInLane = length;
    KeccakP1600_AddBytesInLane(state, lanePosition, data, offsetInLane, bytesInLane);
    length -= bytesInLane;
    lanePosition++;
    offsetInLane = 0;
    data += bytesInLane;
  }
}

void KeccakP1600_AddByte(void *state, uint8 byte, uint32 offset) {
  uint8 data[1];
  fun_check(offset < 200);
  data[0] = byte;
  KeccakP1600_AddBytes(state, data, offset, 1);
}

#define ROL32(a, offset) ((offset != 0) ? ((((uint32)a) << offset) ^ (((uint32)a) >> (32-offset))) : a)
#define index(x, y,z) ((((x)%5)+5*((y)%5))*2 + z)
void ROL64(uint32 inEven, uint32 inOdd, uint32 *outEven, uint32 *outOdd, uint32 offset) {
  if ((offset % 2) == 0) {
    *outEven = ROL32(inEven, offset / 2);
    *outOdd = ROL32(inOdd, offset / 2);
  } else {
    *outEven = ROL32(inOdd, (offset + 1) / 2);
    *outOdd = ROL32(inEven, (offset - 1) / 2);
  }
}

static void theta(uint32 *A) {
  uint32 x, y, z;
  uint32 C[5][2], D[5][2];
  for (x = 0; x<5; x++) {
    for (z = 0; z<2; z++) {
      C[x][z] = 0;
      for (y = 0; y<5; y++)
        C[x][z] ^= A[index(x, y, z)];
    }
  }
  for (x = 0; x<5; x++) {
    ROL64(C[(x + 1) % 5][0], C[(x + 1) % 5][1], &(D[x][0]), &(D[x][1]), 1);
    for (z = 0; z<2; z++)
      D[x][z] ^= C[(x + 4) % 5][z];
  }
  for (x = 0; x<5; x++)
    for (y = 0; y<5; y++)
      for (z = 0; z<2; z++)
        A[index(x, y, z)] ^= D[x][z];
}

static void rho(uint32 *A) {
  uint32 x, y;
  for (x = 0; x<5; x++) for (y = 0; y<5; y++)
    ROL64(A[index(x, y, 0)], A[index(x, y, 1)], &(A[index(x, y, 0)]), &(A[index(x, y, 1)]), KeccakRhoOffsets[5 * y + x]);
}

static void pi(uint32 *A) {
  uint32 x, y, z;
  uint32 tempA[50];
  for (x = 0; x<5; x++) for (y = 0; y<5; y++) for (z = 0; z<2; z++)
    tempA[index(x, y, z)] = A[index(x, y, z)];
  for (x = 0; x<5; x++) for (y = 0; y<5; y++) for (z = 0; z<2; z++)
    A[index(0 * x + 1 * y, 2 * x + 3 * y, z)] = tempA[index(x, y, z)];
}

static void chi(uint32 *A) {
  uint32 x, y, z;
  uint32 C[5][2];
  for (y = 0; y<5; y++) {
    for (x = 0; x<5; x++)
      for (z = 0; z<2; z++)
        C[x][z] = A[index(x, y, z)] ^ ((~A[index(x + 1, y, z)]) & A[index(x + 2, y, z)]);
    for (x = 0; x<5; x++)
      for (z = 0; z<2; z++)
        A[index(x, y, z)] = C[x][z];
  }
}

static void iota(uint32 *A, uint32 indexRound) {
  A[index(0, 0, 0)] ^= KeccakRoundConstants[indexRound][0];
  A[index(0, 0, 1)] ^= KeccakRoundConstants[indexRound][1];
}

void KeccakP1600_PermutationOnWords(uint32 *state, uint32 nrRounds) {
  for (uint32 i = (24 - nrRounds); i < 24; i++) {
    theta(state);
    rho(state);
    pi(state);
    chi(state);
    iota(state, i);
  }
}

void KeccakP1600_ExtractBytesInLane(const void *state, uint32 lanePosition, uint8 *data, uint32 offset, uint32 length) {
  if ((lanePosition < 25) && (offset < 8) && (offset + length <= 8)) {
    uint32 *stateAsHalfLanes = (uint32*)state;
    uint32 lane[2];
    UInt8 laneAsBytes[8];
    fromBitInterleaving(stateAsHalfLanes[lanePosition * 2], stateAsHalfLanes[lanePosition * 2 + 1], lane, lane + 1);
    laneAsBytes[0] = lane[0] & 0xFF;
    laneAsBytes[1] = (lane[0] >> 8) & 0xFF;
    laneAsBytes[2] = (lane[0] >> 16) & 0xFF;
    laneAsBytes[3] = (lane[0] >> 24) & 0xFF;
    laneAsBytes[4] = lane[1] & 0xFF;
    laneAsBytes[5] = (lane[1] >> 8) & 0xFF;
    laneAsBytes[6] = (lane[1] >> 16) & 0xFF;
    laneAsBytes[7] = (lane[1] >> 24) & 0xFF;
    memcpy(data, laneAsBytes + offset, length);
  }
}

void KeccakP1600_ExtractBytes(const void *state, uint8 *data, uint32 offset, uint32 length) {
  uint32 lanePosition = offset / 8;
  uint32 offsetInLane = offset % 8;

  fun_check(offset < 200);
  fun_check(offset + length <= 200);
  while (length > 0) {
    uint32 bytesInLane = 8 - offsetInLane;
    if (bytesInLane > length)
      bytesInLane = length;
    KeccakP1600_ExtractBytesInLane(state, lanePosition, data, offsetInLane, bytesInLane);
    length -= bytesInLane;
    lanePosition++;
    offsetInLane = 0;
    data += bytesInLane;
  }
}

void KeccakP1600_Permute_24rounds(void *state) {
  uint32 *stateAsHalfLanes = (uint32*)state;
  {
    UInt8 stateAsBytes[1600 / 8];
    KeccakP1600_ExtractBytes(state, stateAsBytes, 0, 1600 / 8);
  }
  KeccakP1600_PermutationOnWords(stateAsHalfLanes, 24);
  {
    UInt8 stateAsBytes[1600 / 8];
    KeccakP1600_ExtractBytes(state, stateAsBytes, 0, 1600 / 8);
  }
}
#endif

int32 SHA3Digester::GetDigestLength() {
  return (int32)algorithm_ / 8;
}

int KeccakWidth1600_SpongeInitialize(KeccakWidth1600_SpongeInstance *instance, uint32 rate, uint32 capacity) {
  if (rate + capacity != 1600) return 1;
  if ((rate <= 0) || (rate > 1600) || ((rate % 8) != 0)) return 1;
  UnsafeMemory::Memset(instance->state, 0, 1600 / 8);
  instance->rate = rate;
  instance->byteIOIndex = 0;
  instance->squeezing = 0;
  return 0;
}

void SHA3Digester::Reset() {
  if (context_ != NULL) free(context_);
  context_ = calloc(1, sizeof(HASHCONTEXT));
  HASHCONTEXT* context = (HASHCONTEXT*)context_;
  switch (algorithm_) {
    case SHA3_224:
      KeccakWidth1600_SpongeInitialize(&context->sponge, 1152, 448);
      context->fixedOutputLength = 224;
      context->delimitedSuffix = 0x06;
      break;
    case SHA3_256:
      KeccakWidth1600_SpongeInitialize(&context->sponge, 1088, 512);
      context->fixedOutputLength = 256;
      context->delimitedSuffix = 0x06;
      break;
    case SHA3_384:
      KeccakWidth1600_SpongeInitialize(&context->sponge, 832, 768);
      context->fixedOutputLength = 384;
      context->delimitedSuffix = 0x06;
      break;
    default:
    case SHA3_512:
      KeccakWidth1600_SpongeInitialize(&context->sponge, 576, 1024);
      context->fixedOutputLength = 512;
      context->delimitedSuffix = 0x06;
      break;
  }
}

int KeccakWidth1600_SpongeAbsorbLastFewBits(KeccakWidth1600_SpongeInstance *instance, uint8 delimitedData) {
  uint32 rateInBytes = instance->rate / 8;

  if (delimitedData == 0)
    return 1;
  if (instance->squeezing)
    return 1; /* Too late for additional input */

  /* Last few bits, whose delimiter coincides with first bit of padding */
  KeccakP1600_AddByte(instance->state, delimitedData, instance->byteIOIndex);
  /* If the first bit of padding is at position rate-1, we need a whole new block for the second bit of padding */
  if ((delimitedData >= 0x80) && (instance->byteIOIndex == (rateInBytes - 1)))
    KeccakP1600_Permute_24rounds(instance->state);
  /* Second bit of padding */
  KeccakP1600_AddByte(instance->state, 0x80, rateInBytes - 1);
  KeccakP1600_Permute_24rounds(instance->state);
  instance->byteIOIndex = 0;
  instance->squeezing = 1;
  return 0;
}

int SpongeAbsorbLastFewBits(KeccakWidth1600_SpongeInstance *instance, uint8 delimitedData) {
  uint32 rateInBytes = instance->rate / 8;
  if (delimitedData == 0) return 1;
  if (instance->squeezing) return 1; /* Too late for additional input */
  /* Last few bits, whose delimiter coincides with first bit of padding */
  KeccakP1600_AddByte(instance->state, delimitedData, instance->byteIOIndex);
  /* If the first bit of padding is at position rate-1, we need a whole new block for the second bit of padding */
  if ((delimitedData >= 0x80) && (instance->byteIOIndex == (rateInBytes - 1)))
    KeccakP1600_Permute_24rounds(instance->state);
  /* Second bit of padding */
  KeccakP1600_AddByte(instance->state, 0x80, rateInBytes - 1);
  KeccakP1600_Permute_24rounds(instance->state);
  instance->byteIOIndex = 0;
  instance->squeezing = 1;
  return 0;
}

int KeccakWidth1600_SpongeSqueeze(KeccakWidth1600_SpongeInstance *instance, uint8 *data, size_t dataByteLen) {
  size_t i, j;
  uint32 partialBlock;
  uint32 rateInBytes = instance->rate / 8;
  uint8 *curData;

  if (!instance->squeezing)
    SpongeAbsorbLastFewBits(instance, 0x01);

  i = 0;
  curData = data;
  while (i < dataByteLen) {
    if ((instance->byteIOIndex == rateInBytes) && (dataByteLen >= (i + rateInBytes))) {
      for (j = dataByteLen - i; j >= rateInBytes; j -= rateInBytes) {
        KeccakP1600_Permute_24rounds(instance->state);
        KeccakP1600_ExtractBytes(instance->state, curData, 0, rateInBytes);
        curData += rateInBytes;
      }
      i = dataByteLen - j;
    } else {
      /* normal lane: using the message queue */
      if (instance->byteIOIndex == rateInBytes) {
        KeccakP1600_Permute_24rounds(instance->state);
        instance->byteIOIndex = 0;
      }
      partialBlock = (uint32)(dataByteLen - i);
      if (partialBlock + instance->byteIOIndex > rateInBytes)
        partialBlock = rateInBytes - instance->byteIOIndex;
      i += partialBlock;

      KeccakP1600_ExtractBytes(instance->state, curData, instance->byteIOIndex, partialBlock);
      curData += partialBlock;
      instance->byteIOIndex += partialBlock;
    }
  }
  return 0;
}

const Digester::Digest& SHA3Digester::GetDigest() {
  digest_.Clear();

  HASHCONTEXT* context = (HASHCONTEXT*)context_;
  uint8 hash[64];
  UnsafeMemory::Memset(hash, 0, 64);
  if (KeccakWidth1600_SpongeAbsorbLastFewBits(&context->sponge, context->delimitedSuffix) == 0) {
    KeccakWidth1600_SpongeSqueeze(&context->sponge, hash, context->fixedOutputLength / 8);
  }
  digest_.Append((const char*)hash, GetDigestLength());
  Reset();
  return digest_;
}

void SHA3Digester::UpdateImpl(const void* input, size_t input_length) {
  if (context_ == NULL || input == NULL || input_length == 0) return;
  HASHCONTEXT* context = (HASHCONTEXT*)context_;
  size_t j;
  uint32 partialBlock;
  uint32 rateInBytes = (&context->sponge)->rate / 8;
  if ((&context->sponge)->squeezing) return;
  size_t i = 0;
  const uint8 *curData = (const uint8 *)input;
  while (i < input_length) {
    if (((&context->sponge)->byteIOIndex == 0) && (input_length >= (i + rateInBytes))) {
      for (j = input_length - i; j >= rateInBytes; j -= rateInBytes) {
        KeccakP1600_AddBytes((&context->sponge)->state, curData, 0, rateInBytes);
        KeccakP1600_Permute_24rounds((&context->sponge)->state);
        curData += rateInBytes;
      }
      i = input_length - j;
    } else {
      partialBlock = (uint32)(input_length - i);
      if (partialBlock + (&context->sponge)->byteIOIndex > rateInBytes) {
        partialBlock = rateInBytes - (&context->sponge)->byteIOIndex;
      }
      i += partialBlock;

      KeccakP1600_AddBytes((&context->sponge)->state, curData, (&context->sponge)->byteIOIndex, partialBlock);
      curData += partialBlock;
      (&context->sponge)->byteIOIndex += partialBlock;
      if ((&context->sponge)->byteIOIndex == rateInBytes) {
        KeccakP1600_Permute_24rounds((&context->sponge)->state);
        (&context->sponge)->byteIOIndex = 0;
      }
    }
  }
}

//TODO remove this..
void SHA3Digester::Transform() {
  // NOOP
}

} // namespace fun
