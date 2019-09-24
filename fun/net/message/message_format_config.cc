#include "fun/net/message/message_format_config.h"

namespace fun {
namespace net {

int32 MessageFormatConfig::MessageMinLength = 128;
int32 MessageFormatConfig::MessageMaxLength = 1024 * 1024 * 10; //TODO NetConfig::MessageMaxLength로 통합하는게 좋을듯...
int32 MessageFormatConfig::MaxContainerElementCount = 65536;
int32 MessageFormatConfig::DefaultRecursionLimit = 128;

} // namespace net
} // namespace fun
