#pragma once

#include "fun/net/net.h"
#include "fun/base/function.h"

namespace fun {
namespace net {
namespace reactor {

// All client visible callbacks go here.

class Buffer;
class TcpConnection;

typedef SharedPtr<TcpConnection> TcpConnectionPtr;
typedef Function<void ()> TimerCallback;
typedef Function<void (const TcpConnectionPtr&)> ConnectionCallback;
typedef Function<void (const TcpConnectionPtr&)> CloseCallback;
typedef Function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
typedef Function<void (const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;

typedef Function<void (const TcpConnectionPtr&, Buffer*, const Timestamp&)> MessageCallback;


//
// Default callbacks
//

void DefaultConnectionCallback(const TcpConnectionPtr& conn);
void DefaultMessageCallback(const TcpConnectionPtr& conn,
                            Buffer* buffer,
                            const Timestamp& received_time);

} // namespace reactor
} // namespace net
} // namespace fun
