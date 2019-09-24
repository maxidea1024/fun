#include "fun/net/reactor/http/http_response.h"

namespace fun {
namespace net {

void HttpResponse::AppendToBuffer(Buffer* output) const {
  char buf[32];

  // Status line
  snprintf(buf, sizeof buf, "HTTP/1.1 %d ", status_code_);
  output->Append(buf);
  output->Append(status_message_);
  output->Append("\r\n");

  // Headers
  if (close_connection_) {
    output->Append("Connection: close\r\n");
  } else {
    snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
    output->Append(buf);
    output->Append("Connection: Keep-Alive\r\n");
  }

  for (std::map<String, String>::const_iterator it = headers_.begin();
       it != headers_.end();
       ++it) {
    output->Append(it->first);
    output->Append(": ");
    output->Append(it->second);
    output->Append("\r\n");
  }

  // Separator
  output->Append("\r\n");

  // Body
  output->Append(body_);
}

} // namespace net
} // namespace fun
