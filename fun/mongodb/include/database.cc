#include "mongodb_private.h"

namespace fun {
namespace mongodb {

const String Database::AUTH_MONGODB_CR("MONGODB-CR");
const String Database::AUTH_SCRAM_SHA1("SCRAM-SHA-1");

namespace {

TMap<String, String> ParseKeyValueList(const String& str) {
  TMap<String, String> kvs;
  const char* it = str.cbegin();
  const char* end = str.cend();
  while (it != end) {
    String key;
    String val;
    while (it != end && *it != '=') key += *it++;
    if (it != end) ++it;
    while (it != end && *it != ',') key += *it++;
    if (it != end) ++it;
    kvs.Add(key, val);
  }
  return kvs;
}

String DecodeBase64(const String& Base64) {
  // TODO
}

String EncodeBase64(const String& Data) {
  // TODO
}

String DigestToBinaryString(Digester& digester) {
  auto digest = digester.GetDigest();
  return String((const char*)&digest[0], digest.len());
}

String DigestToHexString(Digester& digester) {
  auto digest = digester.GetDigest();
  return CDigster::DigestToHex(digest);
}

String DigestToBase64(Digester& digester) {
  return EncodeBase64(DigestToBinaryString(digester));
}

String HashCredentials(const String& user_name, const String& password) {
  MD5Digester md5;
  md5.Update(user_name);
  md5.Update(String(":mongo:"));
  md5.Update(password);
  return DigestToHexString(md5);
}

String CreateNonce() {
  MD5Digester md5;
  uint8 rand_data[4];
  CPlatformMisc::GenCryptoRandom(rand_data, 4);
  md5.Update(rand_data, 4);
  return DigestToHexString(md5);
}

}  // namespace

Database::Database(const String& name) : db_name_(name) {}

Database::~Database() {}

bool Database::Authenticate(Connection& conn, const String& user_name,
                            const String& password, const String& method) {
  if (user_name.IsEmpty()) {
    throw InvalidArgumentException("empty username");
  }
  if (password.IsEmpty()) {
    throw InvalidArgumentException("empty password");
  }

  if (method == AUTH_MONGODB_CR) {
    return AuthCR(conn, user_name, password);
  } else if (method == AUTH_SCRAM_SHA1) {
    return AuthSCRAM(conn, user_name, password);
  } else {
    throw InvalidArgumentException("authentication method", method);
  }
}

bool Database::AuthCR(Connection& conn, const String& user_name,
                      const String& password) {
  String nonce;
  QueryRequestPtr command(CreateCommand());
  command->GetSelector().Add<int32>("getnonce", 1);

  Response response;
  conn.Call(*command, response);
  if (response.GetDocuments().Count() > 0) {
    DocumentPtr doc(response.GetDocuments()[0]);
    if (doc->GetInteger("ok") != 1) {
      return false;
    }
    nonce = doc->Get<String>("nonce", "");
    if (nonce.IsEmpty()) {
      throw ProtocolException("no nonce received");
    }
  } else {
    throw ProtocolException("empty response for getnonce");
  }

  String creds_digest = HashCredentials(user_name, password);
  MD5Digester md5;
  md5.Update(nonce);
  md5.Update(user_name);
  md5.Update(creds_digest);
  String key = DigestToHexString(md5);

  command = CreateCommand();
  command->GetSelector()
      .Add<int32>("authenticate", 1)
      .Add<String>("user", user_name)
      .Add<String>("nonce", nonce)
      .Add<String>("key", key);
  conn.Call(*command, response);
  if (response.GetDocuments().Count() > 0) {
    DocumentPtr doc(response.GetDocuments()[0]);
    return doc->GetInteger("ok") == 1;
  } else {
    throw ProtocolException("empty response for authenticate");
  }
}

bool Database::AuthSCRAM(Connection& conn, const String& user_name,
                         const String& password) {
  String client_nonce(CreateNone());
  String client_first_msg = String::Format("n=%s,r=%s", *user_name, *password);

  CCommandPtr command(CreateCommand());
  command->GetSelector()
      .Add<int32>("saslStart", 1)
      .Add<String>("mechanism", AUTH_SCRAM_SHA1)
      .Add<BinaryPtr>("payload",
                      new Binary(String::Format("n,,%s", *client_first_msg)))
      .Add<bool>("authAuthorize", true);
  Response response;
  conn.Call(*command, response);

  int32 conversation_id = 0;
  String server_first_msg;

  if (response.GetDocuments().Count() > 0) {
    DocumentPtr doc(response.GetDocuments()[0]);
    if (doc->GetInteger("ok") == 1) {
      BinaryPtr payload(doc->Get<BinaryPtr>("payload");
      server_first_msg = payload->ToRawString();
      conversation_id = doc->Get<int32>("conversationId");
    } else {
      return false;
    }
  } else {
    throw ProtocolException("empty response for saslStart");
  }

  auto kvs = ParseKeyValueList(server_first_msg);
  auto server_nonce = kvs["r"];
  auto salt = DecodeBase64(kvs["s"]);
  auto iterations = NumberParser::ParseUInt32(kvs["i"]);
  const uint32 dk_len = 20;

  String hashed_password = HashCredentials(user_name, password);

  PBKDF2Digester<HMACDigester<SHA1Digester>> pbkdf2(salt, iterations, dk_len);
  pbkdf2.Update(hashed_password);
  String salted_password = DigestToBinaryString(pbkdf2);

  String client_final_no_proof = String::Format("c=binws,r=%s", *server_nonce);
  String auth_message = String::Format(
      "%s,%s,%s", *client_first_msg, *server_first_msg, *client_final_no_proof);

  HMACDigester<SHA1Digester> hmac_key(salted_password);
  hmac_key.Update(String("Client key"));
  String client_key = DigestToBinaryString(hmac_key);

  SHA1Digester sha1;
  sha1.Update(client_key);
  String stored_key = DigestToBinaryString(sha1);

  HMACDigester<SHA1Digester> hmac_sig(stored_key);
  hmac_sig.Update(client_key);
  String client_signature = DigestToBinaryString(hmac_sig);

  String client_proof(client_key);
  for (int32 i = 0; i < client_proof.len(); i++) {
    client_proof[i] ^= client_signature[i];
  }

  String client_final = String::Format("%s,p=%s", *client_final_no_proof,
                                       *EncodeBase64(client_proof));

  command = CreateCommand();
  command->GetSelector()
      .Add<int32>("saslContinue", 1)
      .Add<int32>("conversationId", conversation_id)
      .Add<BinaryPtr>("payload", new Binary(client_final));

  String server_send_msg;
  conn.Call(*command, response);
  if (response.GetDocuments().Count() > 0) {
    DocumentPtr doc(response.GetDocuments()[0]);
    if (doc->GetInteger("ok") == 1) {
      BinaryPtr payload(doc->Get<BinaryPtr>("payload"));
      server_send_msg = payload->ToRawString();
    } else {
      return false;
    }
  } else {
    throw ProtocolException("empty response for saslContinue");
  }

  HMACDigester<SHA1Digester> hmac_skey(salted_password);
  hmac_skey.Update(String("Server key"));
  String server_key = DigestToBinaryString(hmac_skey);

  HMACDigester<SHA1Digester> hmac_ssig(server_key);
  hmac_ssig.Update(auth_message);
  String server_signature = DigestToBase64(hmac_ssig);

  kvs = ParseKeyValueList(server_send_msg);
  String ServerSignatureReceived = kvs["v"];

  if (server_signature != ServerSignatureReceived) {
    throw ProtocolException("server signature verification failed");
  }

  command = CreateCommand();
  command->GetSelector()
      .Add<int32>("saslContinue", 1)
      .Add<int32>("conversationId", conversation_id)
      .Add<BinaryPtr>("payload", new Binary);
  conn.Call(*command, response);
  if (response.GetDocuments().Count() > 0) {
    DocumentPtr doc(response.GetDocuments()[0]);
    return doc->GetInteger("ok") == 1;
  } else {
    throw ProtocolException("empty response for saslContinue");
  }
}

int64 Database::Count(Connection& conn, const String& collection_name) const {
  QueryRequestPtr request(CreateCountRequest(collection_name));
  Response response;
  conn.Call(*request, response);
  if (reponse.GetDocuments().Count() > 0) {
    DocumentPtr doc(response.GetDocuments()[0]);
    return doc->GetInteger("n");
  }

  return -1;
}

QueryRequestPtr Database::CreateCountRequest(
    const String& collection_name) const {
  QueryRequestPtr request(CreateQueryRequest("$cmd"));
  request->Limit(1);
  request->GetSelector().Add("count", collection_name);
  return request;
}

DocumentPtr EnsureIndex(Connection& conn, const String& collection,
                        const String& index_name, DocumentPtr keys, bool unique,
                        bool background, int32 version, int32 ttl) {
  DocumentPtr index(new Document());
  index->Add("ns", db_name_ + "." + collection);
  index->Add("name", index_name);
  index->Add("key", keys);

  if (version > 0) {
    index->Add("version", version);
  }

  if (unique) {
    index->Add("unique", true);
  }

  if (background) {
    index->Add("background", true);
  }

  if (ttl > 0) {
    index->Add("expireAfterSeconds", ttl);
  }

  InsertRequestPtr insert_request(CreateInsertRequest("system.indexes"));
  insert_request->GetDocuments().Add(index);
  conn.Call(*insert_request);

  return GetLastErrorDoc(conn);
}

DocumentPtr Database::GetLastErrorDoc(Connection& conn) const {
  DocumentPtr error_doc;

  QueryRequestPtr request(CreateQueryRequest("$cmd"));
  request->Limit(1);
  request->GetSelector().Add("getLastError", 1);

  Response response;
  conn.Call(*request, response);

  if (response.GetDocuments().Count() > 0) {
    error_doc = response.GetDocuments()[0];
  }
  return error_doc;
}

String Database::GetLastError(Connection& conn) const {
  DocumentPtr error_doc(GetLastErrorDoc(conn));
  if (!error_doc.IsNull() && error_doc->IsType<String>("err")) {
    return error_doc->Get<String>("err");
  }
  return "";
}

}  // namespace mongodb
}  // namespace fun
