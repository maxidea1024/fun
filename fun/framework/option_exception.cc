#include "fun/framework/option_exception.h"

namespace fun {
namespace framework {

FUN_IMPLEMENT_EXCEPTION(OptionException, "Option exception")
FUN_IMPLEMENT_EXCEPTION(UnknownOptionException, "Unknown option specified")
FUN_IMPLEMENT_EXCEPTION(AmbiguousOptionException, "Ambiguous option specified")
FUN_IMPLEMENT_EXCEPTION(MissingOptionException, "Required option not specified")
FUN_IMPLEMENT_EXCEPTION(MissingArgumentException, "Missing option argument")
FUN_IMPLEMENT_EXCEPTION(InvalidArgumentException, "Invalid option argument")
FUN_IMPLEMENT_EXCEPTION(UnexpectedArgumentException, "Unexpected option argument")
FUN_IMPLEMENT_EXCEPTION(IncompatibleOptionsException, "Incompatible options")
FUN_IMPLEMENT_EXCEPTION(DuplicateOptionException, "Option must not be given more than once")
FUN_IMPLEMENT_EXCEPTION(EmptyOptionException, "Empty option specified")

} // namespace framework
} // namespace fun
