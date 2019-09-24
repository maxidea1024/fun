#include "CorePrivatePCH.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

void RpcStub::ShowUnknownHostIdWarning(HostId remote_id) {
  // LOG(Warning) << Substitute("경고: ProcessReceivedMessage에서 unknown
  // host_id $0!", (int32)remote_id); LOG(Warning) <<
  // String::Format("ProcessReceivedMessage에서 unknown host_id %d!",
  // (int32)remote_id);
}

RpcStub::RpcStub() {
  // TODO 아래 두 필드는 제거하는게 좋을듯...
  bHolsterMoreCallback_FORONETHREADEDMODEL = false;
  bPostponeThisCallback_FORONETHREADEDMODEL = false;

  core_ = nullptr;
  notify_call_from_stub_enabled_ = false;
  engine_specific_only_ = false;
  stub_profiling_enabled_ = false;
}

RpcStub::~RpcStub() {
  if (core_) {
    LOG(LogNetEngine, Warning,
        "RPC stub which is still in use by FunNet core cannot be destroyed! "
        "Destroy NetClient or NetServer instance first.");
  }
}

// TODO 아래 두 함수는 제거하는게 좋을듯...
//꼭 지원하겠다면, ERpcStubResult::* 로 처리하는게 바람직할듯 싶다.
void RpcStub::HolsterMoreCallbackUntilNextTick() {
  bHolsterMoreCallback_FORONETHREADEDMODEL = true;
}

void RpcStub::PostponeThisCallback() {
  bPostponeThisCallback_FORONETHREADEDMODEL = true;
}

}  // namespace net
}  // namespace fun
