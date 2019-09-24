#include "ReportError.h"
#include "fun/net/net.h"

namespace fun {

void ErrorReporter::Report(const String& text) {
  LOG(LogNetEngine, Error, "NetError: %s", *text);
}

}  // namespace fun
