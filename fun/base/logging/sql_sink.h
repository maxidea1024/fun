#include "fun/sql/sql.h"
#include "fun/framework/layered_configuration.h"

namespace fun {
namespace sql {

class FUN_BASE_API SqlSink : public LogSink {
 public:
  SqlSink();
  SqlSink(const String& connector, const String& connect, const String& name = "-");

  void Open() override;
  void Close() override;

  void Log(const LogMessage& msg) override;

  void SetProperty(const String& name, const String& value) override;
  String GetProperty(const String& name) const override;
  int32 Wait();

  static void RegisterSink();

  static const String PROP_CONNECT;
  static const String PROP_CONNECTOR;
  static const String PROP_NAME;
  static const String PROP_TABLE;
  static const String PROP_ARCHIVE_TABLE;
  static const String PROP_MAX_AGE;
  static const String PROP_ASYNC;
  static const String PROP_TIMEOUT;
  static const String PROP_THROW;

 protected:
  ~SqlSink();

 private:
  void InitLogStatement();
  void InitArchiveStatements();
  void LogAsync(const LogMessage& msg);
  void LogSync(const LogMessage& msg);
  bool IsTrue(const String& value) const;

  String connector_;
  String connect_;
  SharedPtr<Session> session_;
  SharedPtr<Statement> log_statement_;
  String name_;
  String table_;
  int32 timeout_;
  bool throw_;
  bool async_;

  String source_;
  long pid_;
  String thread_;
  long tid_;
  int32 priority_;
  String text_;
  DateTime datetime_;

  SharedPtr<ArchiveStrategy> archive_strategy_;
};


//
// inlines
//

FUN_ALWAYS_INLINE vint32 SqlSink::Wait() {
  if (async_ && log_statement_) {
    return log_statement_->Wait(timeout_);
  }

  return 0;
}

FUN_ALWAYS_INLINE bool SqlSink::IsTrue(const String& value) const {
  //TODO 좀 이상한데??
  return  icompare(value, "true") == 0 ||
          icompare(value, "t") == 0 ||
          icompare(value, "yes") == 0 ||
          icompare(value, "y") == 0;
}

} // namespace sql
} // namespace fun
