#pragma once

#include "fun/base/exception.h"
#include "fun/framework/framework.h"

namespace fun {
namespace framework {

FUN_DECLARE_EXCEPTION(FUN_FRAMEWORK_API, OptionException, fun::DataException);
FUN_DECLARE_EXCEPTION(FUN_FRAMEWORK_API, UnknownOptionException,
                      OptionException);
FUN_DECLARE_EXCEPTION(FUN_FRAMEWORK_API, AmbiguousOptionException,
                      OptionException);
FUN_DECLARE_EXCEPTION(FUN_FRAMEWORK_API, MissingOptionException,
                      OptionException);
FUN_DECLARE_EXCEPTION(FUN_FRAMEWORK_API, MissingArgumentException,
                      OptionException);
FUN_DECLARE_EXCEPTION(FUN_FRAMEWORK_API, InvalidArgumentException,
                      OptionException);
FUN_DECLARE_EXCEPTION(FUN_FRAMEWORK_API, UnexpectedArgumentException,
                      OptionException);
FUN_DECLARE_EXCEPTION(FUN_FRAMEWORK_API, IncompatibleOptionsException,
                      OptionException);
FUN_DECLARE_EXCEPTION(FUN_FRAMEWORK_API, DuplicateOptionException,
                      OptionException);
FUN_DECLARE_EXCEPTION(FUN_FRAMEWORK_API, EmptyOptionException, OptionException);

}  // namespace framework
}  // namespace fun
