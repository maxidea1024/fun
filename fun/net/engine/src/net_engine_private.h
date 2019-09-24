#pragma once

//TODO AWS 위치한 서버에 접속시에 문제가 있음.  좀더 스터디한 다음에 적용하도록 하자.
#define FUN_DISABLE_IPV6  1


#include "CorePrivatePCH.h"

#include "Net/Engine/NetEngine.h"
#include "EnumsPrivate.h"

#include "Misc/Uri.h"

#include "SysUtil.h"

#include "Crypto.h"

#include "SendData.h"
#include "NetCore.h"

#include "InternalSocket.h"
#include "completion_port_.h"

#include "LocalEvent.h"
#include "FinalUserWorkItem.h"

#include "MessageStream.h"
#include "RpcCallOptionImpl.h"

#include "ISendDest_C.h"
#include "ISendDest_S.h"

#include "Relayer.h"

#include "UdpSendOpt.h"
#include "TcpSendOpt.h"

#include "IntervalAlarm.h"

#include "NetSettings.h"

#include "UseCount.h"
#include "UserTask.h"

#include "EngineMessageFieldTypes.h"

#include "NetUtilPrivate.h"

#include "Containers/Algo/UnionDuplicateds.h"
#include "Singleton2.h"

//TODO 임시로 사용함.  stack 깨지는것 검출용.
#include "stackguard.h"

namespace fun {

//NETENGINE_API DECLARE_LOG_CATEGORY_EXTERN(LogNetEngine, Info, All);
DECLARE_LOG_CATEGORY_EXTERN(LogNetEngine, Info, All);

//debugging
#define TRACE_SOURCE_LOCATION()         LOG(LogNetEngine,Warning,"%s(%d): %s", __FILE__,__LINE__,__FUNCTION__);
#define TRACE_SOURCE_LOCATION_MSG(msg)  LOG(LogNetEngine,Warning,"%s(%d): %s\n  %s", __FILE__,__LINE__,__FUNCTION__, msg);

} // namespace fun
