#pragma once

#include "Net/Core/NetCore.h"
#include "Net/Message/NetMessage.h"

#define FUN_NETX_API /**/

#include "CriticalSection2.h"

#include "Enums.h"
#include "ConnectParam.h"
#include "HostIdArray.h"
#include "NetPeerInfo.h"
#include "NetClientInfo.h"
#include "MessageSummary.h"

#include "result_info.h"

//TODO 아래 파일들은 rpc.h로 합쳐주어도 좋을듯 싶은데...??
#include "rpc_header.h"
#include "RpcHint.h"
#include "rpc_call_opt.h"
#include "RpcProxy.h"
#include "RpcStub.h"

#include "received_msg.h"
#include "Compressor.h" //Core쪽으로 옮겨주는게 좋을듯 싶은데???

//#if FUN_64_BIT
//# define FUN_ALIGNED_VOLATILE  volatile alignas(8)
//#else
//# define FUN_ALIGNED_VOLATILE  volatile alignas(4)
//#endif

#ifdef FUN_64_BIT
#define FUN_ALIGNED_VOLATILE  __declspec(align(8)) volatile
#else
#define FUN_ALIGNED_VOLATILE  __declspec(align(4)) volatile
#endif

#include "ThreadPool.h" // core의 thread-pool 하고 둘중 정리된 하나로 통합하자.

#include "NetInterface.h"
#include "LanInterface.h"

#include "Misc/Timer.h"
