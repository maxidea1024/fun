#pragma once

#include "fun/mongodb/connection.h"
#include "fun/mongodb/document.h"

namespace fun {
namespace mongodb {

/**
 * Class for working with a MongoDB replica set.
 */
class FUN_MONGODB_API ReplicaSet {
 public:
  ReplicaSet(const fun::Array<InetAddress>& addresses)
      : addresses_(addresses) {}
  virtual ~ReplicaSet() {}

  SharedPtr<Connection> FindMaster() {
    ConnectionPtr master;
    for (const auto& address : addresses_) {
      master = IsMaster(address);
      if (master.IsValid()) {
        break;
      }
    }
    return master;
  }

 protected:
  ConnectionPtr IsMaster(const InetAddress& host) {
    ConnectionPtr conn(new Connection());
    try {
      conn->Connect(host);

      QueryRequest request("admin.$cmd");
      request.SetLimit(1);
      request.GetSelector().Add("isMaster", 1);

      Response response;
      conn->Call(request, response);

      if (response.GetDocuments().Count() > 0) {
        DocumentPtr doc(response.GetDocuments()[0]);
        if (doc->Get<bool>("ismaster")) {
          return conn;
        } else if (doc->Exists("primary")) {
          return IsMaster(InetAddress(doc->Get<String>("primary")));
        }
      }
    } catch (...) {
      conn.Reset();
    }
    return conn;
  }

 private:
  fun::Array<InetAddress> addresses_;
};

}  // namespace mongodb
}  // namespace fun
