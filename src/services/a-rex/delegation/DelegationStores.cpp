#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/delegation/DelegationInterface.h>

#include "DelegationStore.h"

#include "DelegationStores.h"

namespace ARex {

  DelegationStores::DelegationStores(void) {
  }  

  DelegationStores::~DelegationStores(void) {
    Glib::Mutex::Lock lock(lock_);
    for(std::map<std::string,DelegationStore*>::iterator i = stores_.begin();
                   i != stores_.end(); ++i) {
      delete i->second;
    }
  }

  DelegationStore& DelegationStores::operator[](const std::string& path) {
    Glib::Mutex::Lock lock(lock_);
    std::map<std::string,DelegationStore*>::iterator i = stores_.find(path);
    if(i != stores_.end()) return *(i->second);
    DelegationStore* store = new DelegationStore(path);
    stores_.insert(std::pair<std::string,DelegationStore*>(path,store));
    return *store;
  }

  bool DelegationStores::MatchNamespace(const Arc::SOAPEnvelope& in) {
    return Arc::DelegationContainerSOAP().MatchNamespace(in);
  }

  bool DelegationStores::Process(const std::string& path,const Arc::SOAPEnvelope& in,Arc::SOAPEnvelope& out,const std::string& client,std::string& credentials) {
    return operator[](path).Process(credentials,in,out,client);
  }

  bool DelegationStores::DelegatedToken(const std::string& path,Arc::XMLNode token,const std::string& client,std::string& credentials) {
    return operator[](path).DelegatedToken(credentials,token,client);
  }

} // namespace ARex

