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
      if(!deleg_service_->UpdateCredentials(cred,*inpayload,*outpayload)) {
        logger.msg(Arc::ERROR, "Can not store proxy certificate");
        return make_soap_fault(outmsg);
      }
      std::cout<<"Delegated credentials:\n"<<cred<<std::endl;

      //Need some way to make other services get the proxy credential

    };

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
}

Service_Delegation::~Service_Delegation(void) {
  if(deleg_service_) delete deleg_service_;
}

} // namespace ArcSec

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "delegation.service", "HED:SERVICE", 0, &ArcSec::get_service },
    { NULL, NULL, 0, NULL }
};

