#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

// ���� float�迭 ����� ���������� �����ϴµ� ������,
// ������ ������ Ÿ�� �������� Ȯ���� �� ����.

int32_t vsnprintf16(char16_t* out, uint32_t out_len, const char16_t* fmt, va_list args);
int32_t snprintf16(char16_t* out, uint32_t out_len, const char16_t* fmt, ...);
