#include <examples/ace/ttcp/common.h>

#include <fun_check.h>

int main(int argc, char* argv[]) {
  Options options;
  if (parseCommandLine(argc, argv, &options)) {
    if (options.transmit) {
      transmit(options);
    } else if (options.receive) {
      receive(options);
    } else {
      fun_check(0);
    }
  }
}
