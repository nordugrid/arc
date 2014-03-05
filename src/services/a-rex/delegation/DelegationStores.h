#ifndef __ARC_DELEGATION_STORES_H__
#define __ARC_DELEGATION_STORES_H__

#include <string>
#include <map>

#include <arc/Thread.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/XMLNode.h>

namespace ARex {

class DelegationStore;

/// Set of service storing delegated credentials
class DelegationStores {
 private:
  Glib::Mutex lock_;
  std::map<std::string,DelegationStore*> stores_;
 public:
  DelegationStores(void);
  DelegationStores(const DelegationStores&) { };
  ~DelegationStores(void);
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
  /// Stores delegated credentials token defined by 'token' into storage 'path'. 
  /// Extracted token is also returned in 'credentials'.
  /// If operation is successful returns true.
  bool DelegatedToken(const std::string& path,Arc::XMLNode token,const std::string& client,std::string& credentials);
};

} // namespace ARex

#endif // __ARC_DELEGATION_STORE_H__

