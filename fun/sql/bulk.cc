#include "fun/sql/bulk.h"

namespace fun {
namespace sql {

Bulk::Bulk(const Limit& limit) : limit_(limit.value(), false, false) {}

Bulk::Bulk(uint32 value) : limit_(value, false, false) {}

Bulk::~Bulk() {}

}  // namespace sql
}  // namespace fun
