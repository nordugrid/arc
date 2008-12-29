#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <arc/message/PayloadSOAP.h>
#include <arc/XMLNode.h>
#include <arc/client/ClientInterface.h>
#include <arc/client/ClientX509Delegation.h>

#include "DelegationSH.h"

static Arc::Logger logger(Arc::Logger::rootLogger, "DelegationSH");

Arc::Plugin* ArcSec::DelegationSH::get_sechandler(Arc::PluginArgument* arg) {
    ArcSec::SecHandlerPluginArgument* shcarg =
            arg?dynamic_cast<ArcSec::SecHandlerPluginArgument*>(arg):NULL;
    if(!shcarg) return NULL;
    return new ArcSec::DelegationSH((Arc::Config*)(*shcarg),(Arc::ChainContext*)(*shcarg));
}

/*
sechandler_descriptors ARC_SECHANDLER_LOADER = {
    { "delegation.handler", 0, &get_sechandler},
    { NULL, 0, NULL }
};
*/

namespace ArcSec {
using namespace Arc;

DelegationSH::DelegationSH(Config *cfg,ChainContext*):SecHandler(cfg) {
  std::string delegation_type = (std::string)((*cfg)["Type"]);
  std::string delegation_role = (std::string)((*cfg)["Role"]);
  ds_endpoint_ = (std::string)((*cfg)["DSEndpoint"]);

  if(delegation_type == "x509") {
    cert_file_=(std::string)((*cfg)["CertificatePath"]);
    if(cert_file_.empty()) {
      logger.msg(ERROR,"Missing or empty CertificatePath element");
      return;
    };
    key_file_=(std::string)((*cfg)["KeyPath"]);
    if(key_file_.empty()) {
      logger.msg(ERROR,"Missing or empty KeyPath element");
      return;
    };
    ca_file_=(std::string)((*cfg)["CACertificatePath"]);
    ca_dir_=(std::string)((*cfg)["CACertificatesDir"]);
    if(ca_file_.empty() && ca_dir_.empty()) {
      logger.msg(INFO,"Missing or empty CertificatePath or CACertificatesDir element");
    delegation_type_=delegation_x509;
    if(delegation_role == "client") delegation_role_ = delegation_client;
    else if(delegation_role == "service") delegation_role_ = delegation_service;
    else {
      logger.msg(ERROR,"Delegation role not supported: %s",delegation_role);
      return;
    }
  } else if(delegation_type == "saml") {
    //TODO:
    
    
    delegation_type_=delegation_saml;
  } else {
    logger.msg(ERROR,"Delegation type not supported: %s",delegation_type);
    return;
  };
}

DelegationSH::~DelegationSH() {
}

class DelegationContext:public Arc::MessageContextElement{
 public:
  bool have_delegated_;
  std::string delegation_endpoint_;
  std::string delegation_id_;
  DelegationContext(const std::string& delegation_endpoint,const std::string& delegation_id):delegation_endpoint_(delegation_endpoint), delegation_id_(delegation_id){ };
  virtual ~DelegationContext(void) { };
};

bool DelegationSH::Handle(Arc::Message* msg){
  msg->Context() 
  if(delegation_type_ == delegation_x509) {
    try {
      PayloadSOAP* soap = dynamic_cast<PayloadSOAP*>(msg->Payload());

      if(delegation_role_ == delegation_service) {
        //Try to get the delegation service and delegation ID
        //information from incoming message
        std::string method = (*msg).Attributes()->get("HTTP:METHOD");
        if(method == "POST") {
          logger.msg(Arc::DEBUG, "process: POST");
          // Extracting payload
          Arc::PayloadSOAP* inpayload = NULL;
          try {
            inpayload = dynamic_cast<Arc::PayloadSOAP*>(msg->Payload());
          } catch(std::exception& e) { };
          if(!inpayload) {
            logger.msg(Arc::ERROR, "input is not SOAP");
            return false;
          };
          // Analyzing request
          std::string ds_endpoint = (std::string)((*inpayload)["DelegationService"]);
          std::string delegation_id = (std::string)((*inpayload)["DelegationID"]);
          if(!ds_endpoint.empty() && !delegation_id.empty()) {
            logger.msg(Arc::INFO, "Delegation service: %s",ds_endpoint.c_str());
            Arc::MCCConfig ds_client_cfg1;
            //Use service's credential to acquire delegation credential which
            //will be used by this service to act on behalf of the original
            //EEC credential's holder.
            ds_client_cfg1.AddCertificate(cert_file_);
            ds_client_cfg1.AddPrivateKey(key_file_);
            if(!ca_dir_.empty()) ds_client_cfg1.AddCADir(ca_dir_);
            if(!ca_file_.empty())ds_client_cfg1.AddCAFile(ca_file_);
            Arc::URL ds_url1(ds_endpoint);
            ClientX509Delegation client_deleg1(ds_client_cfg1,ds_url1);
            client_deleg1.acquireDelegation(DELEG_ARC,delegation_cred,delegation_id);
            //TODO:Store delegation credential into local temporary directory
            std::string proxy_path;            

            //Create one more level of delegation
            Arc::MCCConfig ds_client_cfg2;
            //Use delegation credential (the one got and stored in the above step)
            //to create one more level of delegation, this delegation credential 
            //will be used by the next intermediate service to act on behalf of
            //the EEC credential's holder 
            ds_client_cfg2.AddCertificate(cert_file_);
            ds_client_cfg2.AddPrivateKey(key_file_);
            if(!ca_dir_.empty()) ds_client_cfg2.AddCADir(ca_dir_);
            if(!ca_file_.empty())ds_client_cfg2.AddCAFile(ca_file_);
            //Note here the delegation service endpoint to which this service will 
            //create the delegation credential ('n+1' level delegation) could be 
            //different with the delegation service endpoint from which this service 
            //acquire the delegation credential ('n' level delegation)
            //
            Arc::URL ds_url2(ds_endpoint_);
            ClientX509Delegation client_deleg2(ds_client_cfg2,ds_url2);
            std::string delegation_id2;
            client_deleg2.createDelegation(DELEG_ARC,delegation_id2);
         

          };
          
        };
      }
      else if(delegation_role_ == delegation_client) {
        //Send the endpoint of delegation service and delegation ID to 
        //the service side, on which the delegation handler with 
        //'delegation_service' role will get the endpoint of delegation
        //service and delegation ID to aquire delegation credential.
                
     
      }

    } catch(std::exception) {
      logger.msg(ERROR,"Incoming Message is not SOAP");
      return false;
    }  
  } else if(delegation_type_ == delegation_saml) {
    try {
      PayloadSOAP* soap = dynamic_cast<PayloadSOAP*>(msg->Payload());

    } catch(std::exception) {
      logger.msg(ERROR,"Outgoing Message is not SOAP");
      return false;
    }
  } else {
    logger.msg(ERROR,"Delegation handler is not configured");
    return false;
  } 
  return true;
}

}


