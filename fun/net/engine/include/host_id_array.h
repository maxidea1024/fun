﻿#pragma once

#include "NetConfig.h"  // NetConfig::OrdinaryHeavyS2CMulticastCount
#include "fun/net/net.h"

namespace fun {
namespace net {

typedef Array<HostId,
              InlineAllocator<NetConfig::OrdinaryHeavyS2CMulticastCount>>
    HostIdArray;

// TODO 순서가 중요하다면, std::set을 사용해야함.
typedef Set<HostId> HostIdSet;

}  // namespace net
}  // namespace fun
