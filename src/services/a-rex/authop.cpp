#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Thread.h>
#include <arc/Utils.h>
#include <arc/message/PayloadSOAP.h>
#include "arex.h"

namespace ARex {


bool ARexConfigContext::CheckOperationAllowed(OperationType op, ARexConfigContext* config) {
  // TODO: very simplified code below. Proper way to identify how client was identified and 
  // which authentication information matched authorization rules LegacySecAttr must be used.
  if(!config)
    return false;

  bool has_tls_identity = false;
  bool has_token_identity = false;
  std::list<std::string> scopes;

  for(std::list<Arc::MessageAuth*>::iterator a = config->beginAuth();a!=config->endAuth();++a) {
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
    std::list<std::string> const * allowed_scopes = NULL;
    switch(op) {
      case OperationServiceInfo:
        allowed_scopes = &(config->GmConfig().TokenScopes("info"));
        break;
      case OperationJobInfo:
        allowed_scopes = &(config->GmConfig().TokenScopes("jobinfo"));
        break;
      case OperationJobCreate:
        allowed_scopes = &(config->GmConfig().TokenScopes("jobcreate"));
        break;
      case OperationJobCancel:
        allowed_scopes = &(config->GmConfig().TokenScopes("jobcancel"));
        break;
      case OperationJobDelete:
        allowed_scopes = &(config->GmConfig().TokenScopes("jobdelete"));
        break;
      case OperationDataInfo:
        allowed_scopes = &(config->GmConfig().TokenScopes("datainfo"));
        break;
      case OperationDataWrite:
        allowed_scopes = &(config->GmConfig().TokenScopes("datawrite"));
        break;
      case OperationDataRead:
        allowed_scopes = &(config->GmConfig().TokenScopes("dataread"));
        break;
      default:
        break;
    }
    // No assigned scopes means no limitation
    if(!allowed_scopes) return true;
    if(allowed_scopes->empty()) return true;
    for(std::list<std::string>::iterator scopeIt = scopes.begin(); scopeIt != scopes.end(); ++scopeIt) {
      if(std::find(allowed_scopes->begin(), allowed_scopes->end(), *scopeIt) != allowed_scopes->end()) return true;
    }
    return false;
  }

  if(has_tls_identity)
    return true; // X.509 authorization has no per-operation granularity

  return false;
}


} // namespace ARex 
