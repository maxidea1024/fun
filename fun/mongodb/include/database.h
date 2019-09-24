#pragma once

#include "fun/mongodb/conn.h"
#include "fun/mongodb/document.h"

namespace fun {
namespace mongodb {

class FUN_MONGODB_API Database {
 public:
  explicit Database(const String& name);
  virtual ~Database();

  bool Authenticate(Connection& conn,
                    const String& user_name,
                    const String& password,
                    const String& method = AUTH_SCRAM_SHA1);

  int64 Count(Connection& conn, const String& collection_name) const;

  QueryRequestPtr CreateCommand() const;
  QueryRequestPtr CreateCountRequest(const String& collection_name) const;
  InsertRequestPtr CreateInsertRequest(const String& collection_name) const;
  DeleteRequestPtr CreateDeleteRequest(const String& collection_name) const;
  QueryRequestPtr CreateQueryRequest(const String& collection_name) const;

  DocumentPtr EnsureIndex(Connection& conn,
                          const String& collection,
                          const String& index_name,
                          DocumentPtr keys,
                          bool unique = false,
                          bool background = false,
                          int32 version = 0,
                          int32 ttl = 0);

  DocumentPtr GetLastErrorDoc(Connection& conn) const;
  String GetLastError(Connection& conn) const;

  /// Default authentication mechanism prior to MongoDB 3.0.
  static const String AUTH_MONGODB_CR;
  /// Default authentication mechanism for MongoDB 3.0.
  static const String AUTH_SCRAM_SHA1;

 protected:
  bool AuthCR(Connection& conn, const String& user_name, const String& password);
  bool AuthSCRAM(Connection& conn, const String& user_name, const String& password);

 private:
  String db_name_;
};


//
// inlines
//

inline QueryRequestPtr Database::CreateCommand() const {
  QueryRequestPtr cmd(CreateQueryRequest("$cmd"));
  cmd->SetLimit(1);
  return cmd;
}

inline InsertRequestPtr Database::CreateInsertRequest(const String& collection_name) const {
  return new InsertRequest(db_name_ + '.' + collection_name);
}

inline InsertRequestPtr Database::CreateDeleteRequest(const String& collection_name) const {
  return new DeleteRequest(db_name_ + '.' + collection_name);
}

inline QueryRequestPtr Database::CreateQueryRequest(const String& collection_name) const {
  return new QueryRequest(db_name_ + '.' + collection_name);
}

inline UpdateRequestPtr Database::CreateUpdateRequest(const String& collection_name) const {
  return new UpdateRequest(db_name_ + '.' + collection_name);
}

} // namespace mongodb
} // namespace fun
