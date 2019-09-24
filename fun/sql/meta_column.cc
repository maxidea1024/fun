#include "fun/sql/meta_column.h"

namespace fun {
namespace sql {

MetaColumn::MetaColumn()
  : length_(),
    precision_(),
    position_(),
    type_(),
    nullable_() {}

MetaColumn::MetaColumn( size_t position,
                        const String& name,
                        ColumnDataType type,
                        size_t length,
                        size_t precision,
                        bool nullable)
  : name_(name),
    length_(length),
    precision_(precision),
    position_(position),
    type_(type),
    nullable_(nullable) {}

MetaColumn::~MetaColumn() {}

} // namespace sql
} // namespace fun
