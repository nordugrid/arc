#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Thread.h>
#include <arc/Utils.h>
#include <arc/message/PayloadSOAP.h>
#include "arex.h"

namespace ARex {


bool ARexService::CheckOperationAllowed(OperationType op, ARexConfigContext* config) const {
  // TODO: very simplified code below. Proper way to identify how client was identified and 
  // which authentication information matched authorization rules LegacySecAttr must be used.
  if(!config)
    return false;

  bool has_tls_identity = false;
  bool has_token_identity = false;
  std::list<std::string> scopes;

  for(std::list<Arc::MessageAuth*>::iterator a = config.beginAuth();a!=config.endAuth();++a) {
    if(*a) {
      Arc::SecAttr* sattr = NULL;
      if(sattr = (*a)->get("TLS")) {
        has_tls_identity = !sattr->get("SUBJECT").empty();
      } else if(sattr = (*a)->get("OTOKENS")) {
        scopes = sattr->getAll("scope");
        has_token_identity = true;
      }
    }
  }

  if(has_token_identity) {
    switch(op) {
      case OperationInfo:
        if(std::find(scopes.begin(), scopes.end(), "computing.read") != scopes.end())
          return true;
        break;
      case OperationCreate:
        if(std::find(scopes.begin(), scopes.end(), "computing.create") != scopes.end())
          return true;
        break;
      case OperationModify:
        if(std::find(scopes.begin(), scopes.end(), "computing.modify") != scopes.end())
          return true;
        break;
      case OperationDelete:
        if(std::find(scopes.begin(), scopes.end(), "computing.cancel") != scopes.end())
          return true;
        break;
    }
  }

  if(has_tls_identity)
    return true; // X.509 authorization has no per-operation granularity

  return false;
}


} // namespace ARex 
