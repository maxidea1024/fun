#pragma once

#define WEB_API /* TEMP */

//TODO templates 쪽으로 옮겨주는게 좋을듯...
#ifndef FORWARD_VA_ARGS
# define FORWARD_VA_ARGS(...)  fun::Forward<decltype(__VA_ARGS__)>(__VA_ARGS__)
#endif

#include "fun/http/version.h"
#include "fun/http/protocol.h"
#include "fun/http/method.h"
#include "fun/http/headers.h"
#include "fun/http/status_code.h"
#include "fun/http/content_type.h"
