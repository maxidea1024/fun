#include "fun/base/logging/rotate_strategy.h"
//#include "fun/base/file_stream.h"
//#include "fun/base/date_time_parser.h"
//#include "fun/base/date_time_formatter.h"
//#include "fun/base/date_time_format.h"
//#include "fun/base/line_ending_converter.h"

namespace fun {

//
// RotateStrategy
//

RotateStrategy::RotateStrategy() {}

RotateStrategy::~RotateStrategy() {}

//
// RotateByIntervalStrategy
//

const String RotateByIntervalStrategy::ROTATE_TEXT(
    "# Log file created/rotated ");

RotateByIntervalStrategy::RotateByIntervalStrategy(const Timespan& span)
    : span_(span), last_rotated_at_(0) {
  if (span.TotalMicroseconds() <= 0) {
    throw InvalidArgumentException("time span must be greater than zero");
  }
}

RotateByIntervalStrategy::~RotateByIntervalStrategy() {}

bool RotateByIntervalStrategy::MustRotate(LogFile* file) {
  // TODO
  fun_check(0);

  /*
  if (last_rotated_at_ == 0 || file->GetSize() == 0) {
    if (file->GetSize() != 0) {
      fun::FileInputStream istr(file->GetPath());
      fun::InputLineEndingConverter converter(istr,
  fun::LineEnding::NEWLINE_LF); String tag; std::getline(converter, tag); if
  (tag.compare(0, ROTATE_TEXT.Len(), ROTATE_TEXT) == 0) { String timestamp(tag,
  ROTATE_TEXT.Len()); int tzd; last_rotated_at_ =
  DateTimeParser::Parse(DateTimeFormat::RFC1036_FORMAT, timestamp,
  tzd).timestamp(); } else { last_rotated_at_ = file->GetCreationDate();
      }
    } else {
      last_rotated_at_.Update();
      String tag(ROTATE_TEXT);
      DateTimeFormatter::append(tag, last_rotated_at_,
  DateTimeFormat::RFC1036_FORMAT); file->Write(tag);
    }
  }
  */

  Timestamp now;
  return span_ <= now - last_rotated_at_;
}

//
// RotateBySizeStrategy
//

RotateBySizeStrategy::RotateBySizeStrategy(uint64 size) : size_(size) {
  if (size == 0) {
    throw InvalidArgumentException("size must be greater than zero");
  }
}

RotateBySizeStrategy::~RotateBySizeStrategy() {}

bool RotateBySizeStrategy::MustRotate(LogFile* file) {
  return file->GetSize() >= size_;
}

}  // namespace fun
