#include "fun/sql/mysql/utility.h"
#include <mysql.h>

namespace fun {
namespace sql {
namespace mysql {

String Utility::GetServerInfo(MYSQL* handle) {
  String info(mysql_get_server_info(handle));
  return info;
}

String Utility::GetServerInfo(Session& session) {
  String info(mysql_get_server_info(GetHandle(session)));
  return info;
}

unsigned long Utility::GetServerVersion(MYSQL* handle) {
  return mysql_get_server_version(handle);
}

unsigned long Utility::GetServerVersion(Session& session) {
  return mysql_get_server_version(GetHandle(session));
}

String Utility::GetHostInfo(MYSQL* handle) {
  String info(mysql_get_host_info(handle));
  return info;
}

String Utility::GetHostInfo(Session& session) {
  String info(mysql_get_host_info(GetHandle(session)));
  return info;
}

}  // namespace mysql
}  // namespace sql
}  // namespace fun
