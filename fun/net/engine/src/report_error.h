////@deprecated

namespace fun {
namespace net {

class ErrorReporter {
 public:
  static void Report(const String& text);
};

//class CReportSocketDelegate : public IInternalSocketDelegate {
//public:
//  virtual void OnSocketWarning(InternalSocket* socket, const String& msg) override {}
//};
//
//
///**
// * 클라, 서버 모두 쓴다.
// */
//class CErrorReporter_Indeed : public Singleton<CErrorReporter_Indeed> {
//private:
//  enum class ELogSocketState {
//    Init,
//    Connecting,
//    Sending,
//    Finished,
//  };
//
//  static const char* LogServerName;
//  static int32 LogServerPort;
//  static const ANSICHAR* LogHttpMsgFmt;
//  static const ANSICHAR* LogHttpMsgFmt2;
//
//  ELogSocketState LogSocketState;
//  uint32 LastReportedTime;
//
//  // 유저가 쌓은 메시지
//  String text;
//
//  InternalSocketPtr LogSocket;
//  CReportSocketDelegate LogSocketDelegate;
//
//  // overlapped IO 중이므로 이 버퍼를 유지하고 있어야 한다.
//  ByteArray LogHttpTxt;
//
//  int32 LogSendProgress;
//
//  void Heartbeat_LogSocket_SendingCase();
//  void Heartbeat_LogSocket_ConnectingCase();
//
//  void Heartbeat_LogSocket_IssueSend();
//  void Heartbeat_LogSocket_NoneCase();
//
//  bool Heartbeat_LogSocket_IssueConnect();
//
//  void IssueCloseAndWaitUntilNoIssueGuaranteed();
//
//  CCriticalSection2 CS;
//  FUN_ALIGNED_VOLATILE bool should_stop_thread_;
//
//  void RequestReport(const String& text);
//
//  CThread Worker;
//  static void StaticWorkerProc(void* context);
//  void WorkerProc();
//
//public:
//  CErrorReporter_Indeed();
//  ~CErrorReporter_Indeed();
//
//  void Heartbeat_LogSocket();
//
//  static void Report(const String& text);
//};
//
//class CErrorReporter_Slient : public Singleton<CErrorReporter_Slient> {
//public:
//  static void Report(const String& text) {}
//};
//
//#ifdef NO_USE_ERROR_REPORTER
//typedef CErrorReporter_Slient ErrorReporter;
//#else
//typedef CErrorReporter_Indeed ErrorReporter;
//#endif
//

} // namespace net
} // namespace fun
