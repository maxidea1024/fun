#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

// 현재 float계열 빼고는 정상적으로 동작하는듯 싶으나,
// 여전히 인자의 타입 안전성은 확보할 수 없다.

int32_t vsnprintf16(char16_t* out, uint32_t out_len, const char16_t* fmt, va_list args);
int32_t snprintf16(char16_t* out, uint32_t out_len, const char16_t* fmt, ...);
