#include "fun/net/net.h"
#include "ReportError.h"

namespace fun {

void ErrorReporter::Report(const String& text) {
  LOG(LogNetEngine,Error,"NetError: %s", *text);
}

} // namespace fun
