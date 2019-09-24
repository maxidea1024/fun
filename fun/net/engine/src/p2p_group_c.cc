#include "fun/net/net.h"
#include "P2PGroup_C.h"

namespace fun {
namespace net {

void P2PGroup_C::GetInfo(P2PGroupInfo& info) const {
  info.group_id = group_id;

  info.members_.Clear(members_.Count());
  for (const auto& pair : members) {
    info.members_.Add(pair.key);
  }
}

} // namespace net
} // namespace fun
