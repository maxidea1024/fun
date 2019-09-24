#include "fun/sql/sql_sink.h"
#include "fun/base/date_time.h"
#include "fun/base/format.h"
#include "fun/base/instantiator.h"
#include "fun/base/logging_factory.h"
#include "fun/base/number_formatter.h"
#include "fun/base/number_parser.h"
#include "fun/sql/session_factory.h"

namespace fun {
namespace sql {

const String SqlSink::PROP_CONNECTOR("connector");
const String SqlSink::PROP_CONNECT("connect");
const String SqlSink::PROP_NAME("name");
const String SqlSink::PROP_TABLE("table");
const String SqlSink::PROP_ARCHIVE_TABLE("archive");
const String SqlSink::PROP_MAX_AGE("keep");
const String SqlSink::PROP_ASYNC("async");
const String SqlSink::PROP_TIMEOUT("timeout");
const String SqlSink::PROP_THROW("throw");

SqlSink::SqlSink()
    : name_("-"),
      table_("FUN_LOG"),
      timeout_(1000),
      throw_(true),
      async_(true),
      pid_(),
      tid_(),
      level_() {}

SqlSink::SqlSink(const String& connector, const String& connect,
                 const String& name)
    : connector_(connector),
      connect_(connect),
      name_(name),
      table_("FUN_LOG"),
      timeout_(1000),
      throw_(true),
      async_(true),
      pid_(),
      tid_(),
      level_() {
  Open();
}

SqlSink::~SqlSink() {
  try {
    Close();
  } catch (...) {
    fun_unexpected();
  }
}

void SqlSink::Open() {
  if (connector_.IsEmpty() || connect_.IsEmpty()) {
    throw IllegalStateException(
        "Connector and connect string must be non-empty.");
  }

  session_ = new Session(connector_, connect_);
  InitLogStatement();
}

void SqlSink::Close() { Wait(); }

void SqlSink::Log(const LogMessage& msg) {
  if (async_) {
    LogAsync(msg);
  } else {
    LogSync(msg);
  }
}

void SqlSink::LogAsync(const LogMessage& msg) {
  fun_check_ptr(log_statement_);
  if (0 == wait() && !log_statement_->IsDone() &&
      !log_statement_->IsInitialized()) {
    if (throw_) {
      throw TimeoutException(
          "Timed out waiting for previous statement completion");
    } else {
      return;
    }
  }

  if (!session_ || !session_->IsConnected()) {
    Open();
  }

  LogSync(msg);
}

void SqlSink::LogSync(const LogMessage& msg) {
  if (archive_strategy_) {
    archive_strategy_->Archive();
  }

  source_ = msg.GetSource();
  pid_ = msg.GetPid();
  thread_ = msg.GetThread();
  tid_ = msg.GetTid();
  level_ = msg.GetLevel();
  text_ = msg.GetText();
  datetime_ = msg.GetTime();
  if (source_.IsEmpty()) {
    source_ = name_;
  }

  try {
    log_statement_->Execute();
  } catch (Exception&) {
    if (throw_) {
      throw;
    }
  }
}

void SqlSink::SetProperty(const String& name, const String& value) {
  if (icompare(name, PROP_NAME) == 0) {
    name_ = value;
    if (name_.IsEmpty()) {
      name_ = "-";
    }
  } else if (icompare(name, PROP_CONNECTOR) == 0) {
    connector_ = value;
    Close();
    Open();
  } else if (icompare(name, PROP_CONNECT) == 0) {
    connect_ = value;
    Close();
    Open();
  } else if (icompare(name, PROP_TABLE) == 0) {
    table_ = value;
    InitLogStatement();
  } else if (icompare(name, PROP_ARCHIVE_TABLE) == 0) {
    if (value.IsEmpty()) {
      archive_strategy_ = nullptr;
    } else if (archive_strategy_) {
      archive_strategy_->SetDestination(value);
    } else {
      archive_strategy_ =
          new ArchiveByAgeStrategy(connector_, connect_, table_, value);
    }
  } else if (icompare(name, PROP_MAX_AGE) == 0) {
    if (value.IsEmpty() || icompare(value, "forever") == 0) {
      archive_strategy_ = nullptr;
    } else if (archive_strategy_) {
      archive_strategy_->SetThreshold(value);
    } else {
      ArchiveByAgeStrategy* p =
          new ArchiveByAgeStrategy(connector_, connect_, table_);
      p->SetThreshold(value);
      archive_strategy_ = p;
    }
  } else if (icompare(name, PROP_ASYNC) == 0) {
    async_ = IsTrue(value);
    InitLogStatement();
  } else if (icompare(name, PROP_TIMEOUT) == 0) {
    if (value.IsEmpty() || '0' == value[0]) {
      timeout_ = Statement::WAIT_FOREVER;
    } else {
      timeout_ = NumberParser::parse(value);
    }
  } else if (icompare(name, PROP_THROW) == 0) {
    throw_ = IsTrue(value);
  } else {
    LogSink::SetProperty(name, value);
  }
}

String SqlSink::GetProperty(const String& name) const {
  if (icompare(name, PROP_NAME) == 0) {
    if (name_ != "-") {
      return name_;
    } else {
      return "";
    }
  } else if (icompare(name, PROP_CONNECTOR) == 0) {
    return connector_;
  } else if (icompare(name, PROP_CONNECT) == 0) {
    return connect_;
  } else if (icompare(name, PROP_TABLE) == 0) {
    return table_;
  } else if (icompare(name, PROP_ARCHIVE_TABLE) == 0) {
    return archive_strategy_ ? archive_strategy_->GetDestination() : "";
  } else if (icompare(name, PROP_MAX_AGE) == 0) {
    return archive_strategy_ ? archive_strategy_->GetThreshold() : "forever";
  } else if (icompare(name, PROP_TIMEOUT) == 0) {
    return NumberFormatter::Format(timeout_);
  } else if (icompare(name, PROP_THROW) == 0) {
    return throw_ ? "true" : "false";
  } else {
    return LogSink::GetProperty(name);
  }
}

void SqlSink::InitLogStatement() {
  using namespace Keywords;

  log_statement_ = new Statement(*session_);

  String sql;
  fun::Format(sql, "INSERT INTO %s VALUES (?,?,?,?,?,?,?,?)", table_);

  *log_statement_ << sql, use(source_), use(name_), use(pid_), use(thread_),
      use(tid_), use(level_), use(text_), use(datetime_);

  if (async_) {
    log_statement_->SetAsync();
  }
}

void SqlSink::RegisterSink() {
  fun::LoggingFactory::DefaultFactory().RegisterSinkClass(
      "SqlSink", new fun::Instantiator<SqlSink, fun::LogSink>);
}

}  // namespace sql
}  // namespace fun
