#include "fun/sql/pooled_session_holder.h"

namespace fun {
namespace sql {

PooledSessionHolder::PooledSessionHolder(SessionPool& owner,
                                         SessionImpl::Ptr session_impl)
    : onwer_(owner), impl_(session_impl) {}

PooledSessionHolder::~PooledSessionHolder() {}

}  // namespace sql
}  // namespace fun
