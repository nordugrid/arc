#ifndef __ARC_DELEGATION_STORES_H__
#define __ARC_DELEGATION_STORES_H__

#include <string>
#include <map>

#include <arc/Thread.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/XMLNode.h>

#include "DelegationStore.h"

namespace ARex {

/// Set of service storing delegated credentials
class DelegationStores {
 private:
  Glib::Mutex lock_;
  std::map<std::string,DelegationStore*> stores_;
  DelegationStore::DbType db_type_;
  DelegationStores(const DelegationStores&) { };
 public:
  DelegationStores(DelegationStore::DbType db_type = DelegationStore::DbSQLite);
  ~DelegationStores(void);
  void SetDbType(DelegationStore::DbType db_type) { db_type_ = db_type; };
  /// Returns or creates delegation storage associated with 'path'.
  DelegationStore& operator[](const std::string& path); 
  /// Check if SOAP request 'in' can be handled by this implementation.
  bool MatchNamespace(const Arc::SOAPEnvelope& in);
  /// Processes SOAP request 'in' using delegation storage associated with 'path'.
  /// Response is filled into 'out'. The 'client' is identifier of requestor
  /// used by service internally to recognize owner of stored credentials.
  /// If operation produces credentials token it is returned in 'credentials'.
  /// If operation is successful returns true.
  bool Process(const std::string& path,const Arc::SOAPEnvelope& in,Arc::SOAPEnvelope& out,const std::string& client,std::string& credentials);
  /// Provides delegation request from storage 'path' for cpecified 'id' and 'client'. If 'id' is empty
  /// then new storage slot is created and its identifier stored in 'id'.
  bool GetRequest(const std::string& path,std::string& id,const std::string& client,std::string& request);
  /// Stores delegated credentials corresponding to delegation request obtained by call to GetRequest().
  /// Only public part is expected in 'credentials'.
  bool PutDeleg(const std::string& path,const std::string& id,const std::string& client,const std::string& credentials);
  /// Stores full credentials into specified 'id' and 'client'. If 'id' is empty
  /// then new storage slot is created and its identifier stored in 'id'.
  bool PutCred(const std::string& path,std::string& id,const std::string& client,const std::string& credentials, const std::list<std::string>& meta = std::list<std::string>());
};

} // namespace ARex

#endif // __ARC_DELEGATION_STORE_H__

