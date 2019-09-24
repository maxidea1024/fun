#pragma once

namespace fun {
namespace net {

//TODO 이게 구지 extern일 필요가 있나??
extern const Uuid DUMP_PROTOCOL_VERSION;

class IDumpClientDelegate
{
public:
  virtual bool ShouldStop() = 0;

  virtual void OnException(HostId host_id, const Exception& e) = 0;

  virtual void OnComplete() = 0;

  virtual ~IDumpClientDelegate() {}
};


class DumpClient
{
public:
  NETENGINE_API static DumpClient* New(IDumpClientDelegate* Delegate);

public:
  enum class State {
    Connecting,
    Sending,
    Closing,
    Stopped
  };

public:
  virtual void Start(const String& server_ip, int32 server_port, const String& FilePath) = 0;

  virtual void Tick() = 0;

  virtual State GetState() = 0;

  virtual int32 GetSendProgress() = 0;

  virtual int32 GetSendTotal() = 0;

  virtual ~DumpClient() {}
};


class IDumpServerDelegate
{
public:
  virtual void OnStartServer(StartServerArgs& MutableParams) = 0;

  virtual bool ShouldStop() = 0;

  virtual CCriticalSection2& GetMutex() = 0;

  virtual void OnServerStartComplete(const ResultInfo* result_info) = 0;

  virtual String GetDumpFilePath(HostId client_id, const InetAddress& client_addr, const DateTime& DumpTime) = 0;

  virtual void OnTick() {}

  virtual ~IDumpServerDelegate() {}
};


class DumpServer
{
public:
  NETENGINE_API static DumpServer* New(IDumpServerDelegate* Delegate);

public:
  virtual void Serve() = 0;

  virtual ~DumpServer() {}
};

} // namespace net
} // namespace fun
