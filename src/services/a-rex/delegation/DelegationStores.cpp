#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/delegation/DelegationInterface.h>

#include "DelegationStore.h"

#include "DelegationStores.h"

namespace ARex {

  DelegationStores::DelegationStores(DelegationStore::DbType db_type):db_type_(db_type) {
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
    DelegationStore* store = new DelegationStore(path,db_type_);
    stores_.insert(std::pair<std::string,DelegationStore*>(path,store));
    return *store;
  }

  bool DelegationStores::MatchNamespace(const Arc::SOAPEnvelope& in) {
    return Arc::DelegationContainerSOAP().MatchNamespace(in);
  }

  bool DelegationStores::Process(const std::string& path,const Arc::SOAPEnvelope& in,Arc::SOAPEnvelope& out,const std::string& client,std::string& credentials) {
    return operator[](path).Process(credentials,in,out,client);
  }

  bool DelegationStores::GetRequest(const std::string& path,std::string& id,const std::string& client,std::string& request) {
    return operator[](path).GetRequest(id,client,request);
  }

  bool DelegationStores::PutDeleg(const std::string& path,const std::string& id,const std::string& client,const std::string& credentials) {
    return operator[](path).PutDeleg(id,client,credentials);
  }


} // namespace ARex

