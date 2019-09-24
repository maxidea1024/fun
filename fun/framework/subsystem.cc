#include "fun/framework/subsystem.h"

namespace fun {
namespace framework {

Subsystem::Subsystem() {
  // NOOP
}

Subsystem::~Subsystem() {
  // NOOP
}

void Subsystem::Reinitialize(Application& app) {
  Uninitialize();

  Initialize(app);
}

void Subsystem::DefineOptions(OptionSet& options) {
  FUN_UNUSED(options);
}

} // namespace framework
} // namespace fun
