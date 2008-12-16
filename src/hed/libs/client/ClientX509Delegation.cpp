#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// This define is needed to have maximal values for types with fixed size
#define __STDC_LIMIT_MACROS
#include <stdlib.h>
#include <map>

#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/MCC.h>

#include "ClientX509Delegation.h"

namespace Arc {
  Logger ClientX509Delegation::logger(Logger::getRootLogger(), "ClientX509Delegation");

  ClientX509Delegation::ClientX509Delegation(const BaseConfig& cfg,
                                                 const URL& url)
    : soap_client_(NULL), delegator_(NULL) {
    soap_client_ = new ClientSOAP(cfg, url);

    //Use the certificate and key in the main chain to delegate
    std::string cert_file = cfg.cert;
    std::string privkey_file = cfg.key;
    delegator_ = new DelegationProviderSOAP(cert_file, privkey_file, &(std::cin));
  }

  ClientX509Delegation::~ClientX509Delegation() { 
    if(soap_client_) delete soap_client_;
    if(delegator_) delete delegator_;
  }

  MCC_Status ClientX509Delegation::process(PayloadSOAP *request, PayloadSOAP **response) {
    return(process("", request, response));
  }

  MCC_Status ClientX509Delegation::process(const std::string& action, 
                                           PayloadSOAP *request,
                                           PayloadSOAP **response) {

    MCCInterface* soap_entry = soap_client_->GetEntry();

    Arc::MessageContext context;
    if(!delegator_->DelegateCredentialsInit(*soap_entry,&context)) {
      logger.msg(Arc::ERROR, "DelegateCredentialsInit failed");
      return Arc::MCC_Status();
    };
    if(!delegator_->UpdateCredentials(*soap_entry,&context)) {
      logger.msg(Arc::ERROR, "UpdateCredentials failed");
      return Arc::MCC_Status();
    };

    return Arc::MCC_Status(Arc::STATUS_OK);
 
    //Send the message
    //Arc::MCC_Status status = soap_client_->process(action, request, response);             
    //return status;
  }

} // namespace Arc
