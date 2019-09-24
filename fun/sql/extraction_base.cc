#include "fun/sql/extraction_base.h"

namespace fun {
namespace sql {

ExtractionBase::ExtractionBase( const String& type, uint32 limit,
                                uint32 position,
                                bool bulk)
  : type_(type),
    extractor_(nullptr),
    limit_(limit),
    position_(position),
    bulk_(bulk),
    empty_string_is_null_(false),
    force_empty_string_(false) {}

ExtractionBase::~ExtractionBase() {}

} // namespace sql
} // namespace fun
