#pragma once

#include "fun/base/flags.h"
#include "fun/zk/zk.h"

struct NodeStat;

namespace fun {
namespace zookeeper {

/**
 * Zookeeper 이벤트 watcher.
 */
class IWatcher {
 public:
  virtual ~IWatcher() {}

  /** Called when the client connects to the zookeeper server. */
  virtual void OnConnected() = 0;
  /** Called when the client is connected to the zookeeper server. */
  virtual void OnConnecting() = 0;
  /** Called when a client session has expired. */
  virtual void OnSessionExpired() = 0;

  /** Called when a new node is created. */
  virtual void OnCreated(const char* path) = 0;
  /** Called when a new node has been removed. */
  virtual void OnDeleted(const char* path) = 0;
  /** Called when the state of the node has changed. */
  virtual void OnChanged(const char* path) = 0;
  /** Called when a child node changes. */
  virtual void OnChildChanged(const char* path) = 0;
  /** Called when the watch state is released. */
  virtual void OnNotWatching(const char* path) = 0;
};

/**
 * Zookeeper 노드 stats
 */
struct CNodeStat {
  /** The zxid of the change that caused this znode to be created. */
  int64 czxid;

  /** The zxid of the change that last modified this znode. */
  int64 mzxid;

  /** The time in milliseconds from epoch when this znode was created. */
  int64 ctime;

  /** The time in milliseconds from epoch when this znode was last modified. */
  int64 mtime;

  /** The number of changes to the data of this znode. */
  int32 version;

  /** The number of changes to the children of this znode. */
  int32 cversion;

  /** The number of changes to the ACL of this znode. */
  int32 aversion;

  /** The session id of the owner of this znode if the znode is an ephemeral
   * node. If it is not an ephemeral node, it will be zero. */
  int64 ephemeral_owner;

  /** The length of the data field of this znode. */
  int32 data_length;

  /** The number of children of this znode. */
  int32 num_children;

  int64 pzxid;
};

/**
 * Zookeeper 노드 타입
 */
enum class NodeType {
  Persistent = 0,
  Ephemeral = 1,
  Sequence = 2,
};

DECLARE_FLAGS(NodeTypes, NodeType);
DECLARE_OPERATORS_FOR_FLAGS(NodeTypes);

// TODO 예외는 던지지 말도록 하자. 로깅만 처리하고, 결과를 반환하는 형태로
// 처리하도록 하자.

/**
 * Zookeeper session.
 */
class FUN_ZOOKEEPER_API Client : public Noncopyable {
 public:
  Client();

  /**
   * Constructor.
   *
   * \param TimeoutMS - 서버에 전달되는 timeout 값으로, client사이드에서
   * 사용되는 되는 값이 아님. Zookeeper에서 받아들일때 4000msec 밑으로는 설정이
   * 안된다. (즉, 최소 값이 4000임)
   */
  Client(const ByteArray& server_host_list, IWatcher* global_watcher = nullptr,
         const Timespan& timeout = Timespan::FromMilliseconds(5 * 1000));

  /**
   * Destructor
   */
  virtual ~Client();

  void Connect(const ByteArray& server_host_list,
               IWatcher* global_watcher = nullptr,
               const Timespan& timeout = Timespan::FromMilliseconds(5 * 1000));

  void Disconnect();

  bool IsConnected();

  bool IsExpired();

  // typedef Function<void(int32 RC, const ByteArray& CreatedNodeName)>
  // CCreateCallback; typedef Function<void(int32 RC, const ByteArray&
  // CreatedNodeName, const CNodeStat& NodeStat)> CCreate2Callback; typedef
  // Function<void(int32 RC, const CNodeStat& NodeStat)> CExistsCallback; typedef
  // Function<void(int32 RC, const CNodeStat& NodeStat)> CSetCallback;

  ByteArray Create(const ByteArray& path, const ByteArray& value = ByteArray(),
                   const NodeTypes node_types = NodeType::Persistent);

  bool CreateAsync(const ByteArray& path,
                   const Function<void(bool, const ByteArray&)>& callback,
                   const ByteArray& value = ByteArray(),
                   const NodeTypes node_types = NodeType::Persistent);

  ByteArray Create2(CNodeStat& out_stat, const ByteArray& path,
                    const ByteArray& value = ByteArray(),
                    const NodeTypes node_types = NodeType::Persistent);

  bool Create2Async(
      const ByteArray& path,
      const Function<void(bool, const ByteArray&, const CNodeStat&)>& callback,
      const ByteArray& value = ByteArray(),
      const NodeTypes node_types = NodeType::Persistent);

  ByteArray CreateIfNotExists(
      const ByteArray& path, const ByteArray& value = ByteArray(),
      const NodeTypes node_types = NodeType::Persistent);

  bool Delete(const ByteArray& path);

  bool DeleteAsync(const ByteArray& path, const Function<void(bool)>& callback);

  bool DeleteIfExists(const ByteArray& path);

  bool Exists(const ByteArray& path, bool watch = false,
              CNodeStat* out_node_stat = nullptr);

  bool ExistsAsync(const ByteArray& path,
                   const Function<void(bool, const CNodeStat&)>& callback,
                   bool watch = false);

  bool GetStat(const ByteArray& path, CNodeStat& out_node_stat);

  bool GetStatAsync(const ByteArray& path,
                    const Function<void(bool, const CNodeStat&)>& callback);

  bool Set(const ByteArray& path, const ByteArray& value);

  bool SetAsync(const ByteArray& path, const ByteArray& value,
                const Function<void(bool, const CNodeStat&)>& callback);

  bool Get(const ByteArray& path, ByteArray& out_value, bool watch = false);

  bool GetAsync(
      const ByteArray& path,
      const Function<void(bool, const ByteArray&, const CNodeStat&)>& callback,
      bool watch = false);

  bool GetChildren(const ByteArray& parent_path, Array<ByteArray>& out_children,
                   bool watch = false);

  bool GetChildrenAsync(
      const ByteArray& parent_path,
      const Function<void(bool, const Array<ByteArray>&)>& callback,
      bool watch = false);

  bool GetChildren2(const ByteArray& parent_path,
                    Array<ByteArray>& out_children, CNodeStat& out_stat,
                    bool watch = false);

  bool GetChildren2Async(const ByteArray& parent_path,
                         const Function<void(bool, const Array<ByteArray>&,
                                             const CNodeStat&)>& callback,
                         bool watch = false);

  ByteArray RecursiveCreate(const ByteArray& path,
                            const ByteArray& value = ByteArray(),
                            const NodeTypes node_types = NodeType::Persistent);

  bool SyncAsync(const ByteArray& path,
                 const Function<void(bool, const ByteArray&)>& callback);

  /**
   * \warning Do not call this funciton directly(internal usage only)
   */
  void WatchHandler_INTERNAL(int type, int state, const char* path);

 private:
  /** Zookeep handle */
  void* zoo_handle_;

  /** Global watcher object */
  IWatcher* global_watcher_;
};

}  // namespace zookeeper
}  // namespace fun
