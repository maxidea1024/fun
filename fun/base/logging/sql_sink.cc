﻿// TODO ...

#include "Poco/DateTime.h"
#include "Poco/Format.h"
#include "Poco/Instantiator.h"
#include "Poco/LoggingFactory.h"
#include "Poco/NumberFormatter.h"
#include "Poco/NumberParser.h"
#include "Poco/SQL/SessionFactory.h"
#include "Poco/SQL/SqlSink.h"

namespace fun {
namespace sql {

using namespace Keywords;

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
    : name_("_"),
      table_("FUN_LOG"),
      timeout_(1000),
      throw_(true),
      async_(true),
      pid_(),
      tid_(),
      priority_() {}

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
      priority_() {
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
        "Connector and connect string must be non-IsEmpty.");
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

  if (Wait() == 0 && !log_statement_->Done() &&
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
  priority_ = msg.GetPriority();
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

void SqlSink::setProperty(const String& name, const String& value) {
  if (name == PROP_NAME) {
    name_ = value;
    if (name_.IsEmpty()) {
      name_ = "-";
    }

  } else if (name == PROP_CONNECTOR) {
    connector_ = value;
    Close();
    Open();

  } else if (name == PROP_CONNECT) {
    connect_ = value;
    Close();
    Open();

  } else if (name == PROP_TABLE) {
    table_ = value;
    InitLogStatement();

  } else if (name == PROP_ARCHIVE_TABLE) {
    if (value.IsEmpty()) {
      archive_strategy_ = nullptr;
    } else if (archive_strategy_) {
      archive_strategy_->SetDestination(value);
    } else {
      archive_strategy_ =
          new ArchiveByAgeStrategy(connector_, connect_, table_, value);
    }
  } else if (name == PROP_MAX_AGE) {
    if (value.IsEmpty() || "forever" == value) {
      archive_strategy_ = nullptr;
    } else if (archive_strategy_) {
      archive_strategy_->SetThreshold(value);
    } else {
      ArchiveByAgeStrategy* p =
          new ArchiveByAgeStrategy(connector_, connect_, table_);
      p->SetThreshold(value);
      archive_strategy_ = p;
    }
  } else if (name == PROP_ASYNC) {
    async_ = IsTrue(value);
    InitLogStatement();
  } else if (name == PROP_TIMEOUT) {
    if (value.IsEmpty() || '0' == value[0]) {
      timeout_ = Statement::WAIT_FOREVER;
    } else {
      timeout_ = NumberParser::parse(value);
    }
  } else if (name == PROP_THROW) {
    throw_ = IsTrue(value);
  } else {
    LogSink::SetProperty(name, value);
  }
}

String SqlSink::GetProperty(const String& name) const {
  if (name == PROP_NAME) {
    if (name_ != "-") {
      return name_;
    } else {
      return "";
    }
  } else if (name == PROP_CONNECTOR) {
    return connector_;
  } else if (name == PROP_CONNECT) {
    return connect_;
  } else if (name == PROP_TABLE) {
    return table_;
  } else if (name == PROP_ARCHIVE_TABLE) {
    return archive_strategy_ ? archive_strategy_->GetDestination() : "";
  } else if (name == PROP_MAX_AGE) {
    return archive_strategy_ ? archive_strategy_->GetThreshold() : "forever";
  } else if (name == PROP_TIMEOUT) {
    return NumberFormatter::Format(timeout_);
  } else if (name == PROP_THROW) {
    if (throw_) {
      return "true";
    } else {
      return "false";
    }
  } else {
    return LogSink::GetProperty(name);
  }
}

void SqlSink::InitLogStatement() {
  log_statement_ = new Statement(*session_);

  String sql =
      String::Format("INSERT INTO %s VALUES (?,?,?,?,?,?,?,?)", table_);

  *log_statement_ << sql, use(source_), use(name_), use(pid_), use(thread_),
      use(tid_), use(priority_), use(text_), use(datetime_);

  if (async_) {
    log_statement_->SetAsync();
  }
}

void SqlSink::RegisterSink() {
  LoggingFactory::DefaultFactory().RegisterSinkClass(
      "SqlSink", new Instantiator<SqlSink, LogSink>);
}

}  // namespace sql
}  // namespace fun
