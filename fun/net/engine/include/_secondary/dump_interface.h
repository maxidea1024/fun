//@deprecated 사용할지 여부 좀더 고민..
#pragma once

namespace fun {
namespace net {

extern const GUID DUMP_PROTOCOL_VERSION;

class IDumpClientDelegate
{
 public:
  virtual ~IDumpClientDelegate() {}

  virtual bool ShouldStop() = 0;

  virtual void OnException(const Exception& e) = 0;

  virtual void OnComplete() = 0;
};

class DumpClient
{
public:
  NETENGINE_API static DumpClient* New(IDumpClientDelegate* delegate);

  enum class State {
    Connecting,
    Sending,
    Closing,
    Stopped
  };

  virtual ~DumpClient() {}

  virtual void Start( const String& server_ip,
                      int32 server_port,
                      const String& file_path) = 0;

  virtual void Tick() = 0;

  virtual State GetState() = 0;

  virtual int32 GetSendProgress() = 0;

  virtual int32 GetSendTotal() = 0;
};

class IDumpServerDelegate
{
 public:
  virtual ~IDumpServerDelegate() {}

  virtual void OnStartServer(StartServerArgs& args) = 0;

  virtual bool ShouldStop() = 0;

  virtual CCriticalSection2* GetMutex() = 0;

  virtual void OnServerStartComplete(const ResultInfo* result_info) = 0;

  virtual String GetDumpFilePath( HostId client_id,
                                  const InetAddress& client_addr,
                                  const DateTime& dump_time) = 0;

  virtual void OnTick() {}
};

class DumpServer
{
public:
  NETENGINE_API static DumpServer* New(IDumpServerDelegate* delegate);

  virtual ~DumpServer() {}

  virtual void RunMainLoop() = 0;
};

} // namespace net
} // namespace fun
