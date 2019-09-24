// Based on the FreeBSD random number generator.
// src/lib/libc/stdlib/random.c,v 1.25
//
// Copyright (c) 1983, 1993
// The Regents of the University of California.  All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 4. Neither the name of the University nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

#include "fun/base/random.h"

// For platform cryptogrphical random generation
#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/windows_less.h"
#include <wincrypt.h>
#elif FUN_PLATFORM_UNIX_FAMILY
#include <fcntl.h>
#include <unistd.h>
#endif

#include <ctime>

#if defined(_WIN32_WCE) && _WIN32_WCE < 0x800
#include "wce_time.h"
#endif

#if !FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/digest/sha1_digester.h"
#endif

namespace fun {

#define TYPE_0    0   // linear congruential
#define BREAK_0   8
#define DEG_0     0
#define SEP_0     0

#define TYPE_1    1   // x**7 + x**3 + 1
#define BREAK_1   32
#define DEG_1     7
#define SEP_1     3

#define TYPE_2    2   // x**15 + x + 1
#define BREAK_2   64
#define DEG_2     15
#define SEP_2     1

#define TYPE_3    3   // x**31 + x**3 + 1
#define BREAK_3   128
#define DEG_3     31
#define SEP_3     3

#define TYPE_4    4   // x**63 + x + 1
#define BREAK_4   256
#define DEG_4     63
#define SEP_4     1


Random::Random(int32 state_size) {
  fun_check(BREAK_0 <= state_size && state_size <= BREAK_4);

  buffer_ = new char[state_size];

#if defined(_WIN32_WCE) && _WIN32_WCE < 0x800
  InitState((uint32)wceex_time(NULL), buffer_, state_size);
#else
  InitState((uint32)std::time(NULL), buffer_, state_size);
#endif
}

Random::~Random() {
  delete[] buffer_;
}

void Random::SetSeed(uint32 x) {
  int32 i, lim;

  state_[0] = x;

  if (rand_type_ == TYPE_0) {
    lim = NSHUFF;
  } else {
    for (i = 1; i < rand_deg_; ++i) {
      state_[i] = GoodRand(state_[i - 1]);
    }

    fptr_ = &state_[rand_sep_];
    rptr_ = &state_[0];
    lim = 10 * rand_deg_;
  }

  for (i = 0; i < lim; ++i) {
    Next();
  }
}

void Random::Randomize() {
  size_t len;

  if (rand_type_ == TYPE_0) {
    len = sizeof(state_[0]);
  } else {
    len = rand_deg_ * sizeof(state_[0]);
  }

  GenerateCryptRandomData((char*)state_, len);
}

void Random::InitState(uint32 s, char* arg_state, int32 n) {
  uint32* uint_arg_state = (uint32*)arg_state;

  if (n < BREAK_0) {
    fun_bugcheck_msg("not enough state");
    return;
  }

  if (n < BREAK_1) {
    rand_type_ = TYPE_0;
    rand_deg_ = DEG_0;
    rand_sep_ = SEP_0;
  } else if (n < BREAK_2) {
    rand_type_ = TYPE_1;
    rand_deg_ = DEG_1;
    rand_sep_ = SEP_1;
  } else if (n < BREAK_3) {
    rand_type_ = TYPE_2;
    rand_deg_ = DEG_2;
    rand_sep_ = SEP_2;
  } else if (n < BREAK_4) {
    rand_type_ = TYPE_3;
    rand_deg_ = DEG_3;
    rand_sep_ = SEP_3;
  } else {
    rand_type_ = TYPE_4;
    rand_deg_ = DEG_4;
    rand_sep_ = SEP_4;
  }

  state_ = uint_arg_state + 1;    // first location
  end_ptr_ = &state_[rand_deg_];  // must set end_ptr before seed

  SetSeed(s);

  if (rand_type_ == TYPE_0) {
    uint_arg_state[0] = rand_type_;
  } else {
    uint_arg_state[0] = MAX_TYPES * (int32)(rptr_ - state_) + rand_type_;
  }
}

uint32 Random::Next() const {
  uint32 i;
  uint32 *f, *r;

  if (rand_type_ == TYPE_0) {
    i = state_[0];
    state_[0] = i = GoodRand(i) & 0x7FFFFFFF;
  } else {
    // Use local variables rather than static variables for speed.
    f = fptr_;
    r = rptr_;
    *f += *r;
    i = (*f >> 1) & 0x7FFFFFFF; // chucking least random bit
    if (++f >= end_ptr_) {
      f = state_;
      ++r;
    } else if (++r >= end_ptr_) {
      r = state_;
    }

    fptr_ = f;
    rptr_ = r;
  }

  return i;
}

int32 Random::GenerateCryptRandomData(char* buffer, size_t length) {
  int32 n = 0;

#if FUN_PLATFORM_WINDOWS_FAMILY

  HCRYPTPROV hprovider = 0;
  CryptAcquireContext(&hprovider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
  CryptGenRandom(hprovider, (DWORD)length, (BYTE*)buffer);
  CryptReleaseContext(hprovider, 0);
  n = static_cast<int32>(length);

#else

 #if FUN_PLATFORM_UNIX_FAMILY
  int fd = open("/dev/urandom", O_RDONLY, 0);
  if (fd >= 0) {
    n = read(fd, buffer, length);
    close(fd);
  }
 #endif

  // workaround (fake)
  if (n <= 0) {
    n = 0;

    // 변수 x는 랜덤을 생성하기 위한 재료로만 사용되므로, 값이 깨져도
    // 무방하므로 mutex등으로 보호할 필요가 없음.
    static uint32 x = 0;
    Random rnd1(256);
    Random rnd2(64);
    x += rnd1.Next();

    Sha1Digester digester;

    // 현재 시각을 재료로 첨가
    uint32 t = (uint32)std::time(NULL);
    digester.Update(&t, sizeof(t));

    // this 포인터를 재료로 첨가
    void* p = this;
    digester.Update(&p, sizeof(p));

    // buffer 포인터를 재료로 첨가
    digester.Update(buffer, length);

    // 스택 버퍼를 재료로 첨가
    uint32 junk[32];
    digester.Update(junk, sizeof(junk));

    // 스택 버퍼의 포인터도 재료로 첨가
    p = &junk[0];
    digester.Update(&p, sizeof(p));

    while (n < length) {
      // random shuffling
      for (int32 i = 0; i < 100; ++i) {
        uint32 r = rnd2.Next();
        digester.Update(&r, sizeof(r));
        digester.Update(&x, sizeof(x));
        x += rnd1.Next();
      }

      // digesting via sha1
      auto digest = digester.GetDigest();
      for (int32 i = 0; i < digest.Count() && n < length; ++i, ++n) {
        digester.Update(digest[i]);
        *buffer++ = digest[i];
      }
    }
  }
#endif

  return n;
}

} // namespace fun
