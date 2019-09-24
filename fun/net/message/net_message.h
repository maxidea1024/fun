#pragma once

#define FUN_MESSAGEFORMAT_SUPPORT_STL 1

#if FUN_MESSAGEFORMAT_SUPPORT_STL
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>
#endif  // FUN_MESSAGEFORMAT_SUPPORT_STL

#include "fun/base/base_ordinal.h"

#define FUN_NET_API /* ... */

#define FUN_DO_CHECKED(x) \
  if (!(x)) {             \
    return false;         \
  }

#include "fun/net/message/flex_format.h"
#include "fun/net/message/generated_struct.h"
#include "fun/net/message/i_message_in.h"
#include "fun/net/message/i_message_out.h"
#include "fun/net/message/lite_format.h"
#include "fun/net/message/message_byte_counter.h"
#include "fun/net/message/message_field_types.h"
#include "fun/net/message/message_format.h"
#include "fun/net/message/message_format_config.h"
#include "fun/net/message/message_format_exception.h"
#include "fun/net/message/message_in.h"
#include "fun/net/message/message_out.h"
#include "fun/net/message/property_field.h"
