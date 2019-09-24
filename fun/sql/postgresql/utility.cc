#include "fun/sql/postgresql/utility.h"
#include "fun/sql/postgresql/session_impl.h"
#include "fun/base/number_formatter.h"

namespace fun {
namespace sql {
namespace postgresql {

String Utility::GetServerInfo(SessionHandle* handle) {
  String info = "Process ID: ";
  info.Append(fun::NumberFormatter::Format(handle->GetServerProcessId()));

  info.Append(" Protocol Version: ");
  info.Append(fun::NumberFormatter::Format(handle->GetProtocoVersion()));

  return info;
}

String Utility::GetserverInfo(Session& session) {
  return GetserverInfo(GetHandle(session));
}

int Utility::GetServerVersion(SessionHandle* handle) {
  return handle->GetServerVersion();
}

int Utility::GetServerVersion(Session& session) {
  return GetServerVersion(GetHandle(session));
}

String Utility::GetHostInfo(SessionHandle* handle) {
  //TODO copy를 해야만 하나??
  SessionParametersMap params = handle->GetConnectionParameters();

  SessionParametersMap::const_iterator cItr = params.find("host");
  if (params.end() == cItr) {
    return String();
  }

  return cItr->second.currentValue();
}

String Utility::GetHostInfo(Session& session) {
  return GetHostInfo(GetHandle(session));
}

String Utility::GetSessionEncoding(SessionHandle* handle) {
  return handle->GetclientEncoding();
}

String Utility::GetSessionEncoding(fun::sql::Session& session) {
  return GetSessionEncoding(handle(session));
}

} // namespace postgresql
} // namespace sql
} // namespace fun
