#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <sys/types.h>
#include <pwd.h>

#include <arc/loader/Loader.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/Thread.h>

#include "delegation.h"

namespace ArcSec {

#define ARC_DELEGATION_NAMESPACE "http://www.nordugrid.org/schemas/delegation"

static Arc::LogStream logcerr(std::cerr);

static Arc::Plugin* get_service(Arc::PluginArgument* arg) {
    Arc::ServicePluginArgument* srvarg =
            arg?dynamic_cast<Arc::ServicePluginArgument*>(arg):NULL;
    if(!srvarg) return NULL;
    return new Service_Delegation((Arc::Config*)(*srvarg));
}

Arc::MCC_Status Service_Delegation::make_soap_fault(Arc::Message& outmsg) {
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
  Arc::SOAPFault* fault = outpayload?outpayload->Fault():NULL;
  if(fault) {
    fault->Code(Arc::SOAPFault::Sender);
    fault->Reason("Failed processing delegation request");
  };
  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

class Service_Delegation::CredentialCache {
 public:
  time_t start_;
  std::string credential_;
  std::string id_;
  std::string credential_identity_;
  std::string credential_delegator_ip_;
  CredentialCache(void):start_(time(NULL)) {
  };
  CredentialCache(const std::string& cred):start_(time(NULL)),credential_(cred) {
  };
  CredentialCache& operator=(const std::string& cred) {
    credential_=cred; start_=time(NULL); return *this;
  };
}; 

Arc::MCC_Status Service_Delegation::process(Arc::Message& inmsg,Arc::Message& outmsg) {
  std::string method = inmsg.Attributes()->get("HTTP:METHOD");

  if(!ProcessSecHandlers(inmsg,"incoming")) {
    logger_.msg(Arc::ERROR, "Security Handlers processing failed");
    return Arc::MCC_Status();
  };

  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);
  // Identify which of served endpoints request is for.
  // Delegation can only accept POST method
  if(method == "POST") {
    logger.msg(Arc::DEBUG, "process: POST");
    // Both input and output are supposed to be SOAP
    // Extracting payload
    Arc::PayloadSOAP* inpayload = NULL;
    try {
      inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if(!inpayload) {
      logger.msg(Arc::ERROR, "input is not SOAP");
      return make_soap_fault(outmsg);
    };
    // Analyzing request
    if((*inpayload)["DelegateCredentialsInit"]) {
      if(!deleg_service_->DelegateCredentialsInit(*inpayload,*outpayload)) {
        logger.msg(Arc::ERROR, "Can not generate X509 request");
        return make_soap_fault(outmsg);
      }
    } else if((*inpayload)["UpdateCredentials"]) {
      std::string cred;
      std::string identity;
      if(!deleg_service_->UpdateCredentials(cred,identity,*inpayload,*outpayload)) {
        logger.msg(Arc::ERROR, "Can not store proxy certificate");
        return make_soap_fault(outmsg);
      }
      std::cout<<"Delegated credentials:\n"<<cred<<std::endl;
      CredentialCache* cred_cache = NULL;
      std::string id = (std::string)((*inpayload)["UpdateCredentials"]["DelegatedToken"]["Id"]);
      std::string credential_delegator_ip = inmsg.Attributes()->get("TCP:REMOTEHOST");
      cred_cache = new CredentialCache(cred);
      cred_cache->credential_delegator_ip_ = credential_delegator_ip;
      cred_cache->credential_identity_ = identity;
      cred_cache->id_ = id;
      id2cred_.insert(std::pair<std::string,CredentialCache*>(id,cred_cache));  
      identity2cred_.insert(std::pair<std::string,CredentialCache*>(identity,cred_cache)); 

      //Need some way to make other services get the proxy credential

    } else if((*inpayload)["AcquireCredentials"]) {
      CredentialCache* cred_cache = NULL;
      std::string cred;
      std::string id = (std::string)((*inpayload)["AcquireCredentials"]["DelegatedTokenLookup"]["Id"]);
      std::string cred_identity = (std::string)((*inpayload)["AcquireCredentials"]["DelegatedTokenLookup"]["CredIdentity"]);
      std::string cred_delegator_ip = (std::string)((*inpayload)["AcquireCredentials"]["DelegatedTokenLookup"]["CredDelegatorIP"]);
      if(!id.empty()) cred_cache = id2cred_.find(id)->second;
      else if(!cred_identity.empty() && !cred_delegator_ip.empty()) {
        Identity2CredMapReturn ret;
        Identity2CredMapIterator it;
        ret = identity2cred_.equal_range(cred_identity);
        for(it = ret.first; it!=ret.second; ++it) {
          cred_cache = (*it).second;
          if(!(cred_cache->credential_delegator_ip_).empty()) break;
        }
      }
      if(!cred_cache) {
        logger_.msg(Arc::ERROR, "Can not find the corresponding credential from credential cache");
        return make_soap_fault(outmsg);
      }; 
      cred = cred_cache->credential_;
      Arc::NS ns; ns["deleg"]=ARC_DELEGATION_NAMESPACE;
      Arc::XMLNode cred_resp = (*outpayload).NewChild("deleg:AcquireCredentialsResponse");
      Arc::XMLNode token = cred_resp.NewChild("deleg:DelegatedToken");
      token.NewChild("deleg:Id") = cred_cache->id_;
      token.NewChild("deleg:Value") = cred_cache->credential_;
      token.NewAttribute("deleg:Format") = std::string("x509");
 
      std::cout<<"Delegated credentials:\n"<<cred<<std::endl;
    }

    //Compose response message
    outmsg.Payload(outpayload);

    if(!ProcessSecHandlers(outmsg,"outgoing")) {
      logger_.msg(Arc::ERROR, "Security Handlers processing failed");
      delete outmsg.Payload(NULL);
      return Arc::MCC_Status();
    };

    return Arc::MCC_Status(Arc::STATUS_OK);
  }
  else {
    delete inmsg.Payload();
    logger.msg(Arc::DEBUG, "process: %s: not supported",method);
    return Arc::MCC_Status();
  }
  return Arc::MCC_Status();
}

Service_Delegation::Service_Delegation(Arc::Config *cfg):Service(cfg), 
    logger_(Arc::Logger::rootLogger, "Delegation_Service"), deleg_service_(NULL) {

  logger_.addDestination(logcerr);
  ns_["delegation"]="http://www.nordugrid.org/schemas/delegation";

  deleg_service_ = new Arc::DelegationContainerSOAP;
  max_crednum_ = 1000;
  max_credlife_ = 43200;
}

Service_Delegation::~Service_Delegation(void) {
  if(deleg_service_) delete deleg_service_;
}

} // namespace ArcSec

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "delegation.service", "HED:SERVICE", 0, &ArcSec::get_service },
    { NULL, NULL, 0, NULL }
};

