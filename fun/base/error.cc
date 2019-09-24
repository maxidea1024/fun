#include "fun/base/error.h"
//#include "fun/base/unicode_converter.h"
// TODO 왜 포함해야하지??
#include "fun/base/string/string.h"
#include "fun/base/windows_less.h"

#include <errno.h>
#include <string.h>
#include <string>

namespace fun {

#ifdef FUN_PLATFORM_WINDOWS_FAMILY

uint32 Error::Code() { return ::GetLastError(); }

String Error::Message(uint32 error_code) {
  String error_msg;
  DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS;
  LPWSTR msg_buf = 0;
  if (FormatMessageW(flags, 0, error_code, 0, (LPWSTR)&msg_buf, 0, NULL)) {
    // TODO
    // UnicodeConverter::ToUtf8(msg_buf, error_msg);
    // TODO 메모리 복사가 일어나므로, 한번에 처리할 수 있도록 하자.
    error_msg = UNICHAR_TO_UTF8_BUFFER(msg_buf).ConstData();
  }
  LocalFree(msg_buf);

  return error_msg;
}

#else

int32 Error::Code() { return errno; }

String Error::Message(int32 error_code) {
  char error_msg[512];
  return String(strerror_result(
      strerror_r(error_code, error_msg, sizeof(error_msg)), error_msg));
}

#endif

}  // namespace fun
