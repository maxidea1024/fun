#include "fun/zookeeper/zookeeper.h"

#define THREADED  //!!!
#include "ThirdParty/zookeeper-3.3.0/src/c/include/zookeeper.h"

#if FUN_PLATFORM_WINDOWS_FAMILY && defined(_MSC_VER)
#ifdef _DEBUG
#pragma comment(lib, "ThirdParty/zookeeper-3.3.0/src/c/x64/Debug/zookeeper.lib")
#else
#pragma comment(lib, \
                "ThirdParty/zookeeper-3.3.0/src/c/x64/Release/zookeeper.lib")
#endif
#endif

#include <mutex>  // once_flag

// TODO context Pooling...

namespace fun {
namespace zookeeper {

#define ZK_SUCCEEDED(rc) ((rc) == ZOK)
#define ZK_FAILED(rc) ((rc) != ZOK)

#define THROW_IF_FAILED(rc)                                          \
  if (ZK_FAILED(rc)) {                                               \
    /* TODO logging도 같이 지원하는게 좋으려나? */       \
    throw ZookeeperException(                                        \
        String::Format("zookeeper failed({}): {}", rc, zerror(rc))); \
  }

namespace {

NodeStat FromZooStat(const Stat& src) {
  NodeStat ret;
  ret.czxid = src.czxid;
  ret.mzxid = src.mzxid;
  ret.ctime = src.ctime;
  ret.mtime = src.mtime;
  ret.version = src.version;
  ret.cversion = src.cversion;
  ret.aversion = src.aversion;
  ret.ephemeral_owner = src.ephemeralOwner;
  ret.data_length = src.dataLength;
  ret.num_children = src.numChildren;
  ret.pzxid = src.pzxid;
  return ret;
}

Stat ToZooStat(const NodeStat& src) {
  Stat ret;
  ret.czxid = src.czxid;
  ret.mzxid = src.mzxid;
  ret.ctime = src.ctime;
  ret.mtime = src.mtime;
  ret.version = src.version;
  ret.cversion = src.cversion;
  ret.aversion = src.aversion;
  ret.ephemeralOwner = src.ephemeral_owner;
  ret.dataLength = src.data_length;
  ret.numChildren = src.num_children;
  ret.pzxid = src.pzxid;
  return ret;
}

String State2String(int state) {
  if (state == 0) {
    return StringLiteral("CLOSED_STATE");
  } else if (state == ZOO_CONNECTING_STATE) {
    return StringLiteral("CONNECTING_STATE");
  } else if (state == ZOO_ASSOCIATING_STATE) {
    return StringLiteral("ASSOCIATING_STATE");
  } else if (state == ZOO_CONNECTED_STATE) {
    return StringLiteral("CONNECTED_STATE");
  } else if (state == ZOO_EXPIRED_SESSION_STATE) {
    return StringLiteral("EXPIRED_SESSION_STATE");
  } else if (state == ZOO_AUTH_FAILED_STATE) {
    return StringLiteral("AUTH_FAILED_STATE");
  }

  return String::Format("INVALID_STATE:{0}", state);
}

String Type2String(int type) {
  if (type == ZOO_CREATED_EVENT) {
    return StringLiteral("CREATED_EVENT");
  } else if (type == ZOO_DELETED_EVENT) {
    return StringLiteral("DELETED_EVENT");
  } else if (type == ZOO_CHANGED_EVENT) {
    return StringLiteral("CHANGED_EVENT");
  } else if (type == ZOO_CHILD_EVENT) {
    return StringLiteral("CHILD_EVENT");
  } else if (type == ZOO_SESSION_EVENT) {
    return StringLiteral("SESSION_EVENT");
  } else if (type == ZOO_NOTWATCHING_EVENT) {
    return StringLiteral("NOTWATCHING_EVENT");
  }

  return String::Format("UNKNOWN_EVENT_TYPE: {0}", type);
}

inline int NodesTypeToZkFlags(NodeTypes node_types) {
  int flags = 0;
  if (node_types & NodeType::Ephemeral) {
    flags |= ZOO_EPHEMERAL;
  }
  if (node_types & NodeType::Sequence) {
    flags |= ZOO_SEQUENCE;
  }
  return flags;
}

// TODO rc(resultcode)가 ZOK가 아니면 로그를 남기도록 하자.

struct VoidCompletionContext {
  Function<void(bool)> callback;

  VoidCompletionContext(const Function<void(bool)>& callback)
      : callback(callback) {}
};

struct StatCompletionContext {
  Function<void(bool, const NodeStat&)> callback;

  StatCompletionContext(const Function<void(bool, const NodeStat&)>& callback)
      : callback(callback) {}
};

struct DataAsStringCompletionContext {
  Function<void(bool, const ByteArray&, const NodeStat&)> callback;

  DataAsStringCompletionContext(
      const Function<void(bool, const ByteArray&, const NodeStat&)>& callback)
      : callback(callback) {}
};

struct DataAsRawCompletionContext {
  Function<void(bool, const Array<uint8>&, const NodeStat&)> callback;

  DataAsRawCompletionContext(const Function<void(bool, const Array<uint8>&,
                                                 const NodeStat&)>& callback)
      : callback(callback) {}
};

struct StringCompletionContext {
  Function<void(bool, const ByteArray&)> callback;

  StringCompletionContext(
      const Function<void(bool, const ByteArray&)>& callback)
      : callback(callback) {}
};

struct StringsCompletionContext {
  Function<void(bool, const Array<ByteArray>&)> callback;

  StringsCompletionContext(
      const Function<void(bool, const Array<ByteArray>&)>& callback)
      : callback(callback) {}
};

struct StringStatCompletionContext {
  Function<void(bool, const ByteArray&, const NodeStat&)> callback;

  StringStatCompletionContext(
      const Function<void(bool, const ByteArray&, const NodeStat&)>& callback)
      : callback(callback) {}
};

struct StringsStatCompletionContext {
  Function<void(bool, const Array<ByteArray>&, const NodeStat&)> callback;

  StringsStatCompletionContext(
      const Function<void(bool, const Array<ByteArray>&, const NodeStat&)>&
          callback)
      : callback(callback) {}
};

void VoidCompletionCallback(int rc, const void* data) {
  auto context = (VoidCompletionContext*)data;
  if (context->callback) {
    context->callback(ZK_SUCCEEDED(rc));
  }
  delete context;
}

void StatCompletionCallback(int rc, const struct Stat* stat, const void* data) {
  auto context = (StatCompletionContext*)data;
  if (context->callback) {
    context->callback(ZK_SUCCEEDED(rc), FromZooStat(*stat));
  }
  delete context;
}

void DataAsStringCompletionCallback(int rc, const char* value, int value_len,
                                    const struct Stat* stat, const void* data) {
  auto context = (DataAsStringCompletionContext*)data;
  if (context->callback) {
    context->callback(ZK_SUCCEEDED(rc), ByteArray(value, value_len),
                      FromZooStat(*stat));
  }
  delete context;
}

//제거해도 될듯...
void DataAsRawCompletionCallback(int rc, const char* value, int value_len,
                                 const struct Stat* stat, const void* data) {
  Array<uint8> data;
  if (ZK_SUCCEEDED(rc)) {
    data.ResizeUninitialized(value_len);
    UnsafeMemory::Memcpy(data.MutableData(), value, value_len);
  }

  auto context = (DataAsRawCompletionContext*)data;
  if (context->callback) {
    context->callback(ZK_SUCCEEDED(rc), data, FromZooStat(*stat));
  }
  delete context;
}

void StringCompletionCallback(int rc, const char* str, const void* data) {
  auto context = (StringCompletionContext*)data;
  if (context->callback) {
    context->callback(ZK_SUCCEEDED(rc), str);
  }
  delete context;
}

void StringsCompletionCallback(int rc, const struct String_vector* strings,
                               const void* data) {
  Array<ByteArray> string_list;
  string_list.Reserve(strings->count);
  for (int32 i = 0; i < strings->count; i++) {
    string_list.Add(strings->data[i]);
  }

  auto context = (StringsCompletionContext*)data;
  if (context->callback) {
    context->callback(ZK_SUCCEEDED(rc), string_list);
  }
  delete context;
}

void StringStatCompletionCallback(int rc, const char* string,
                                  const struct Stat* stat, const void* data) {
  auto context = (StringStatCompletionContext*)data;
  if (context->callback) {
    context->callback(ZK_SUCCEEDED(rc), string, FromZooStat(*stat));
  }
  delete context;
}

void StringsStatCompletionCallback(int rc, const struct String_vector* strings,
                                   const struct Stat* stat, const void* data) {
  Array<ByteArray> string_list;
  string_list.Reserve(strings->count);
  for (int32 i = 0; i < strings->count; ++i) {
    string_list.Add(strings->data[i]);
  }

  auto context = (StringsStatCompletionContext*)data;
  if (context->callback) {
    context->callback(ZK_SUCCEEDED(rc), string_list, FromZooStat(*stat));
  }
  delete context;
}

void GlobalWatchFunc(zhandle_t* handle, int type, int state, const char* path,
                     void* context) {
  auto self = static_cast<Client*>(context);
  self->WatchHandler_INTERNAL(type, state, path);
}

static std::once_flag ONCE_FLAG_SET_DEBUG_LEVEL;

void set_default_debug_level() {
  std::call_once(ONCE_FLAG_SET_DEBUG_LEVEL,
                 [] { zoo_set_debug_level(ZOO_LOG_LEVEL_WARN); });
}

void LogCallback(const char* message) {
  LOG(LogZookeeper, Trace, "zookeeper-log: {}", message);
}

}  // namespace

Client::Client() : zk_handle_(nullptr), global_watcher_(nullptr) {}

Client::Client(const ByteArray& server_host, IWatcher* global_watcher,
               const Timespan& timeout)
    : zk_handle_(nullptr), global_watcher_(nullptr) {
  Connect(server_host, global_watcher, timeout);
}

Client::~Client() {
  //소멸자에서 예외를 전파 시키면 다소 어수선해지므로, 여기에서 소거 시켜버림.

  try {
    Disconnect();
  } catch (std::exception&) {
    // NOOP?
  }
}

void Client::Connect(const ByteArray& server_host, IWatcher* global_watcher,
                     const Timespan& timeout) {
  Disconnect();

  global_watcher_ = global_watcher;

  set_default_debug_level();

  zk_handle_ =
      zookeeper_init(server_host.ConstData(), GlobalWatchFunc,
                     (int32)timeout.TotalMilliseconds(), nullptr, this, 0);
  if (zk_handle_ == nullptr) {
    char error[1024];
    strerror_s(error, errno);
    throw ZookeeperException(String(error));
  }

  zoo_set_log_callback((zhandle_t*)zk_handle_, &LogCallback);
}

void Client::Disconnect() {
  if (zk_handle_) {
    auto rc = zookeeper_close((zhandle_t*)zk_handle_);
    THROW_IF_FAILED(rc);
  }
}

bool Client::IsConnected() {
  return zk_handle_ && zoo_state((zhandle_t*)zk_handle_) == ZOO_CONNECTED_STATE;
}

bool Client::IsExpired() {
  return zk_handle_ &&
         zoo_state((zhandle_t*)zk_handle_) == ZOO_EXPIRED_SESSION_STATE;
}

void Client::WatchHandler_INTERNAL(int type, int state, const char* path) {
  LOG(LogZookeeper, Trace, "zookeeper-event: {}.{}: {}", *Type2String(type),
      *State2String(state), *String(path));

  // call global watcher
  if (global_watcher_ == nullptr) {
    return;
  }

  if (type == ZOO_SESSION_EVENT) {
    if (state == ZOO_EXPIRED_SESSION_STATE) {
      global_watcher_->OnSessionExpired();
    } else if (state == ZOO_CONNECTED_STATE) {
      global_watcher_->OnConnected();
    } else if (state == ZOO_CONNECTING_STATE) {
      global_watcher_->OnConnecting();
    } else {
      // TODO:
      fun_check(!"don't know how to process other session event yet");
    }
  } else if (type == ZOO_CREATED_EVENT) {
    global_watcher_->OnCreated(path);
  } else if (type == ZOO_DELETED_EVENT) {
    global_watcher_->OnDeleted(path);
  } else if (type == ZOO_CHANGED_EVENT) {
    global_watcher_->OnChanged(path);
  } else if (type == ZOO_CHILD_EVENT) {
    global_watcher_->OnChildChanged(path);
  } else if (type == ZOO_NOTWATCHING_EVENT) {
    global_watcher_->OnNotWatching(path);
  } else {
    fun_check(!"unknown zookeeper event type");
  }
}

ByteArray Client::Create(const ByteArray& path, const ByteArray& value,
                         const NodeTypes node_types) {
  ByteArray path_buf(path.Len() + 64, NoInit);

  auto zc = zoo_create((zhandle_t*)zk_handle_, path.ConstData(),
                       value.ConstData(), value.Len(), &ZOO_OPEN_ACL_UNSAFE,
                       NodesTypeToZkFlags(node_types), path_buf.MutableData(),
                       path_buf.Len());
  THROW_IF_FAILED(zc);

  path_buf.TrimToNulTerminator();
  return MoveTemp(path_buf);
}

bool Client::CreateAsync(const ByteArray& path,
                         const Function<void(bool, const ByteArray&)>& callback,
                         const ByteArray& value, const NodeTypes node_types) {
  auto zc = zoo_acreate(
      (zhandle_t*)zk_handle_, path.ConstData(), value.ConstData(), value.Len(),
      &ZOO_OPEN_ACL_UNSAFE, NodesTypeToZkFlags(node_types),
      StringCompletionCallback, new StringCompletionContext(callback));
  THROW_IF_FAILED(zc);
  return true;
}

ByteArray Client::Create2(NodeStat& out_stat, const ByteArray& path,
                          const ByteArray& value, const NodeTypes node_types) {
  ByteArray path_buf(path.Len() + 64, NoInit);

  Stat zoo_stat;
  auto zc = zoo_create2((zhandle_t*)zk_handle_, path.ConstData(),
                        value.ConstData(), value.Len(), &ZOO_OPEN_ACL_UNSAFE,
                        NodesTypeToZkFlags(node_types), path_buf.MutableData(),
                        path_buf.Len(), &zoo_stat);

  THROW_IF_FAILED(zc);

  out_stat = FromZooStat(zoo_stat);

  path_buf.TrimToNulTerminator();
  return MoveTemp(path_buf);
}

bool Client::Create2Async(
    const ByteArray& path,
    const Function<void(bool, const ByteArray&, const NodeStat&)>& callback,
    const ByteArray& value, const NodeTypes node_types) {
  auto zc = zoo_acreate2(
      (zhandle_t*)zk_handle_, path.ConstData(), value.ConstData(), value.Len(),
      &ZOO_OPEN_ACL_UNSAFE, NodesTypeToZkFlags(node_types),
      StringStatCompletionCallback, new StringStatCompletionContext(callback));
  THROW_IF_FAILED(zc);
  return true;
}

ByteArray Client::CreateIfNotExists(const ByteArray& path,
                                    const ByteArray& value,
                                    const NodeTypes node_types) {
  ByteArray path_buf(path.Len() + 64, NoInit);

  auto zc = zoo_create((zhandle_t*)zk_handle_, path.ConstData(),
                       value.ConstData(), value.Len(), &ZOO_OPEN_ACL_UNSAFE,
                       NodesTypeToZkFlags(node_types), path_buf.MutableData(),
                       path_buf.Len());

  if (zc == ZNODEEXISTS) {
    fun_check(!(node_types & NodeType::Sequence));
    return path;
  }

  THROW_IF_FAILED(zc);

  path_buf.TrimToNulTerminator();
  return MoveTemp(path_buf);
}

bool Client::Delete(const ByteArray& path) {
  NodeStat NodeStat;
  if (!Exists(path, false, &NodeStat)) {
    return false;
  }

  auto zc =
      zoo_delete((zhandle_t*)zk_handle_, path.ConstData(), NodeStat.version);
  THROW_IF_FAILED(zc);
  return true;
}

bool Client::DeleteAsync(const ByteArray& path,
                         const Function<void(bool)>& callback) {
  NodeStat node_stat;
  if (!Exists(path, false, &node_stat)) {
    return false;
  }

  auto zc =
      zoo_adelete((zhandle_t*)zk_handle_, path.ConstData(), node_stat.version,
                  VoidCompletionCallback, new VoidCompletionContext(callback));
  THROW_IF_FAILED(zc);
  return true;
}

bool Client::DeleteIfExists(const ByteArray& path) {
  NodeStat node_stat;
  if (!Exists(path, false, &node_stat)) {
    return false;
  }

  auto zc =
      zoo_delete((zhandle_t*)zk_handle_, path.ConstData(), node_stat.version);
  if (zc == ZNONODE) {
    return false;
  }

  THROW_IF_FAILED(zc);

  return true;
}

bool Client::Exists(const ByteArray& path, bool watch,
                    NodeStat* out_node_stat) {
  Stat zoo_stat;
  auto zc = zoo_exists((zhandle_t*)zk_handle_, path.ConstData(), watch,
                       out_node_stat != nullptr ? (&zoo_stat) : nullptr);
  if (zc == ZNONODE) {
    return false;
  }

  THROW_IF_FAILED(zc);

  if (out_node_stat != nullptr) {
    *out_node_stat = FromZooStat(zoo_stat);
  }

  return true;
}

bool Client::ExistsAsync(const ByteArray& path,
                         const Function<void(bool, const NodeStat&)>& callback,
                         bool watch) {
  auto zc =
      zoo_aexists((zhandle_t*)zk_handle_, path.ConstData(), watch,
                  StatCompletionCallback, new StatCompletionContext(callback));

  if (zc == ZNONODE) {
    return false;
  }

  THROW_IF_FAILED(zc);

  return true;
}

bool Client::GetStat(const ByteArray& path, NodeStat& out_node_stat) {
  Stat zoo_stat;
  auto zc =
      zoo_exists((zhandle_t*)zk_handle_, path.ConstData(), false, &zoo_stat);

  THROW_IF_FAILED(zc);

  out_node_stat = FromZooStat(zoo_stat);
  return true;
}

bool Client::GetStatAsync(
    const ByteArray& path,
    const Function<void(bool, const NodeStat&)>& callback) {
  auto zc =
      zoo_aexists((zhandle_t*)zk_handle_, path.ConstData(), false,
                  StatCompletionCallback, new StatCompletionContext(callback));
  THROW_IF_FAILED(zc);
  return true;
}

bool Client::Set(const ByteArray& path, const ByteArray& value) {
  NodeStat node_stat;
  if (!GetStat(path, node_stat)) {
    return false;
  }

  auto zc = zoo_set((zhandle_t*)zk_handle_, path.ConstData(), value.ConstData(),
                    value.Len(), NodeStat.version);
  THROW_IF_FAILED(zc);
  return true;
}

bool Client::SetAsync(const ByteArray& path, const ByteArray& value,
                      const Function<void(bool, const NodeStat&)>& callback) {
  NodeStat node_stat;
  if (!GetStat(path, node_stat)) {
    return false;
  }

  auto zc =
      zoo_aset((zhandle_t*)zk_handle_, path.ConstData(), value.ConstData(),
               value.Len(), NodeStat.version, StatCompletionCallback,
               new StatCompletionContext(callback));
  THROW_IF_FAILED(zc);
  return true;
}

bool Client::Get(const ByteArray& path, ByteArray& out_value, bool watch) {
  NodeStat node_stat;
  if (!GetStat(path, node_stat)) {
    return false;
  }

  // just in time.
  out_value.ResizeUninitialized(node_stat.dataLength);

  int buffer_len = out_value.Len();
  auto zoo_stat = ToZooStat(node_stat);
  auto zc = zoo_get((zhandle_t*)zk_handle_, path.ConstData(), watch,
                    out_value.MutableData(), &buffer_len, &zoo_stat);

  THROW_IF_FAILED(zc);

  out_value.Truncate(buffer_len);
  return true;
}

bool Client::GetAsync(
    const ByteArray& path,
    const Function<void(bool, const ByteArray&, const NodeStat&)>& callback,
    bool watch) {
  auto zc = zoo_aget((zhandle_t*)zk_handle_, path.ConstData(), watch,
                     DataAsStringCompletionCallback,
                     new DataAsStringCompletionContext(callback));
  THROW_IF_FAILED(zc);
  return true;
}

bool Client::GetChildren(const ByteArray& parent_path,
                         Array<ByteArray>& out_children, bool watch) {
  struct String_vector ChildrenVec;
  auto zc = zoo_get_children((zhandle_t*)zk_handle_, parent_path.ConstData(),
                             watch, &ChildrenVec);

  THROW_IF_FAILED(zc);

  out_children.Empty(ChildrenVec.count);  // just in time
  for (int32 i = 0; i < ChildrenVec.count; ++i) {
    out_children.Add(ChildrenVec.data[i]);
  }

  return true;
}

bool Client::GetChildrenAsync(
    const ByteArray& parent_path,
    const Function<void(bool, const Array<ByteArray>&)>& callback, bool watch) {
  auto zc = zoo_aget_children((zhandle_t*)zk_handle_, parent_path.ConstData(),
                              watch, StringsCompletionCallback,
                              new StringsCompletionContext(callback));
  THROW_IF_FAILED(zc);
  return true;
}

bool Client::GetChildren2(const ByteArray& parent_path,
                          Array<ByteArray>& out_children, NodeStat& out_stat,
                          bool watch) {
  struct String_vector ChildrenVec;
  Stat zoo_stat;
  auto zc = zoo_get_children2((zhandle_t*)zk_handle_, parent_path.ConstData(),
                              watch, &ChildrenVec, &zoo_stat);

  THROW_IF_FAILED(zc);

  out_children.Empty(ChildrenVec.count);  // just in time
  for (int32 i = 0; i < ChildrenVec.count; ++i) {
    out_children.Add(ChildrenVec.data[i]);
  }

  out_stat = FromZooStat(zoo_stat);

  return true;
}

bool Client::GetChildren2Async(
    const ByteArray& parent_path,
    const Function<void(bool, const Array<ByteArray>&, const NodeStat&)>&
        callback,
    bool watch) {
  auto zc = zoo_aget_children2((zhandle_t*)zk_handle_, parent_path.ConstData(),
                               watch, StringsStatCompletionCallback,
                               new StringsStatCompletionContext(callback));
  THROW_IF_FAILED(zc);
  return true;
}

ByteArray Client::RecursiveCreate(const ByteArray& path, const ByteArray& value,
                                  const NodeTypes node_types) {
  int32 pos = 0;
  do {
    pos = path.IndexOf('/', CaseSensitivity::CaseSensitive, pos + 1);
    if (pos == INVALID_INDEX) {
      break;
    }

    CreateIfNotExists(path.Mid(0, pos));
    pos = pos + 1;
  } while (true);

  return CreateIfNotExists(path, value, node_types);
}

bool Client::SyncAsync(const ByteArray& path,
                       const Function<void(bool, const ByteArray&)>& callback) {
  auto zc = zoo_async((zhandle_t*)zk_handle_, path.ConstData(),
                      StringCompletionCallback,
                      new StringCompletionContext(callback));
  THROW_IF_FAILED(zc);
  return true;
}

}  // namespace zookeeper
}  // namespace fun
