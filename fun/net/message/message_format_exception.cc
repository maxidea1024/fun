#include "fun/net/message_format_exception.h"

namespace fun {
namespace net {

/*
//TODO
void MessageFormatException::CheckCollectionElementCount(int32 count) {
  if (count < 0) {
  }
  else if (count >
}
*/

MessageFormatException MessageFormatException::Misuse(const String& message) {
  return MessageFormatException(message);
}

MessageFormatException MessageFormatException::MoreDataAvailable() {
  return MessageFormatException(
      "Completed reading a message while more data was available in the "
      "stream.");
}

MessageFormatException MessageFormatException::TruncatedMessage() {
  return MessageFormatException(
      "While reading a message, the input ended unexpectedly "
      "in the middle of a field.  This could mean either that the "
      "input has been truncated or that an embedded message "
      "misreported its own length.");
}

MessageFormatException MessageFormatException::NegativeSize() {
  return MessageFormatException(
      "Encountered an embedded string or message which claimed to have "
      "negative size.");
}

MessageFormatException MessageFormatException::MalformedVarint() {
  return MessageFormatException("Encountered a malformed varint.");
}

MessageFormatException MessageFormatException::InvalidTag() {
  return MessageFormatException("message contained an invalid tag (zero).");
}

MessageFormatException MessageFormatException::InvalidFieldId(int32 field_id) {
  return MessageFormatException(
      String::Format("Field-id `{0}` is an invalid value.  The field-id must "
                     "be in the range of 1 to 2 ^ 32-1.",
                     field_id));
}

MessageFormatException MessageFormatException::MessageInLengthLimited(
    int32 length, int32 limit) {
  // TODO ���ڷ� �Ѱܹ��� �Ķ���͵� ���.
  return MessageFormatException(
      "message was too large.  May be malicious.  "
      "Set IMessageIn.MaximumMessageLength property or "
      "MessageFormatConfig.message_max_length to increase the size limit.");
}

MessageFormatException MessageFormatException::MessageOutLengthLimited(
    int32 length, int32 limit) {
  // TODO ���ڷ� �Ѱܹ��� �Ķ���͵� ���.
  return MessageFormatException(
      "message was too large.  May be malicious.  "
      "Set IMessageOut.MaximumMessageLength property or "
      "MessageFormatConfig.message_max_length to increase the size limit.");
}

MessageFormatException MessageFormatException::RecursionLimitExceeded() {
  return MessageFormatException(
      "message had too many levels of nesting.  May be malicious.  "
      "Use IMessageIn.SetRecursionLimit() to increase the depth limit.");
}

MessageFormatException MessageFormatException::UnderflowRecursionDepth() {
  return MessageFormatException(
      "Recursion depth is mismatched.  May be over call "
      "DecreaseRecursionDepth()");
}

MessageFormatException MessageFormatException::RequiredFieldIsMissing(
    const String& field_name, const String& struct_name) {
  return MessageFormatException(
      String::Format("The `{0}` field was missing even though it was specified "
                     "as a required field in structure `{1}`.",
                     field_name, struct_name));
}

}  // namespace net
}  // namespace fun
