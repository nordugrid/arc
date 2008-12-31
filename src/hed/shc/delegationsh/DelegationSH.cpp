#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <arc/ArcConfig.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/MCC.h>
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
  ds_endpoint_ = (std::string)((*cfg)["DelegationServiceEndpoint"]);
  peers_endpoint_ =  (std::string)((*cfg)["PeerServiceEndpoint"]);// And this value will be parsed from main chain later 
  delegation_id_ = (std::string)((*cfg)["DelegationID"]);

  if(delegation_type == "x509") {
    proxy_file_=(std::string)((*cfg)["ProxyPath"]);
    cert_file_=(std::string)((*cfg)["CertificatePath"]);
    if(cert_file_.empty()&&proxy_file_.empty()) {
      logger.msg(ERROR,"Missing CertificatePath element or ProxyPath element");
      return;
    };
    key_file_=(std::string)((*cfg)["KeyPath"]);
    if(key_file_.empty()&&proxy_file_.empty()) {
      logger.msg(ERROR,"Missing or empty KeyPath element");
      return;
    };
    ca_file_=(std::string)((*cfg)["CACertificatePath"]);
    ca_dir_=(std::string)((*cfg)["CACertificatesDir"]);
    if(ca_file_.empty() && ca_dir_.empty()) {
      logger.msg(INFO,"Missing or empty CertificatePath or CACertificatesDir element");
      return;
    }
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
  }
}

DelegationSH::~DelegationSH() {
}

class DelegationContext:public Arc::MessageContextElement{
 public:
  bool have_delegated_;
  std::string delegation_endpoint_;
  std::string delegation_id_;
  std::string deleg_cred_;
  DelegationContext(const std::string& delegation_endpoint,const std::string& delegation_id,const std::string& deleg_cred);
  std::string acquire_credential_path();
  virtual ~DelegationContext(void) { };
};

DelegationContext::DelegationContext(const std::string& delegation_endpoint,const std::string& delegation_id,const std::string& deleg_cred):delegation_endpoint_(delegation_endpoint),delegation_id_(delegation_id),deleg_cred_(deleg_cred),have_delegated_(false) {
  std::string path="/tmp/";
  path.append(delegation_endpoint_).append(delegation_id_);
   
}

std::string DelegationContext::acquire_credential_path() {
  std::string path="/tmp/";
  path.append(delegation_endpoint_).append(delegation_id_);
  return path;
} 

DelegationContext* DelegationSH::get_delegcontext(Arc::Message& msg, const std::string& ds_endpoint, const std::string& delegation_id, const std::string& deleg_cred) {
  DelegationContext* deleg_ctx=NULL;
  Arc::MessageContextElement* mcontext = (*msg.Context())["deleg.context"];
  if(mcontext) {
    try {
      deleg_ctx = dynamic_cast<DelegationContext*>(mcontext);
    } catch(std::exception& e) { };
  };
  if(deleg_ctx) return deleg_ctx;
  deleg_ctx = new DelegationContext(ds_endpoint,delegation_id,deleg_cred);
  if(deleg_ctx) {
    deleg_ctx->have_delegated_ = true;
    msg.Context()->Add("deleg.context",deleg_ctx);
  } else {
    logger.msg(Arc::ERROR, "Failed to acquire delegation context");
  }
  return deleg_ctx;
}

bool DelegationSH::Handle(Arc::Message* msg){
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
          if(!ds_endpoint.empty()) {
            logger.msg(Arc::INFO, "Delegation service: %s",ds_endpoint.c_str());
            Arc::MCCConfig ds_client_cfg;
            //Use service's credential to acquire delegation credential which
            //will be used by this service to act on behalf of the original
            //EEC credential's holder.
            if(!cert_file_.empty())ds_client_cfg.AddCertificate(cert_file_);
            if(!key_file_.empty())ds_client_cfg.AddPrivateKey(key_file_);
            if(!proxy_file_.empty())ds_client_cfg.AddProxy(proxy_file_);
            if(!ca_dir_.empty()) ds_client_cfg.AddCADir(ca_dir_);
            if(!ca_file_.empty())ds_client_cfg.AddCAFile(ca_file_);
            Arc::URL ds_url(ds_endpoint);
            Arc::ClientX509Delegation client_deleg(ds_client_cfg,ds_url);
            std::string delegation_cred;
            if(!delegation_id.empty()) {
              if(!client_deleg.acquireDelegation(DELEG_ARC,delegation_cred,delegation_id)) {
                logger.msg(ERROR,"Can not get the delegation credential: %s from delegation service:%s",delegation_id.c_str(),ds_endpoint.c_str());
                return false;
              };
            } else {
              std::string cred_identity = msg->Attributes()->get("TCP:REMOTEHOST");
              std::string cred_delegator_ip = msg->Attributes()->get("TLS:IDENTITYDN");
              if(!client_deleg.acquireDelegation(DELEG_ARC,delegation_cred,delegation_id,cred_identity,cred_delegator_ip)) {
                logger.msg(ERROR,"Can not get the delegation credential: %s from delegation service:%s",delegation_id.c_str(),ds_endpoint.c_str());
                return false;
              };
            }
            //Store delegation credential into message context memory
            DelegationContext* deleg_ctx = get_delegcontext(*msg,ds_endpoint,delegation_id,delegation_cred);
            if(!deleg_ctx) {
              logger.msg(Arc::ERROR, "Can't create delegation context");
              return false;
            };
            return true; 
          } else {
            logger.msg(ERROR,"The endpoint of delgation service should be configured");
            return false;
          }
        };
      }
      else if(delegation_role_ == delegation_client) {
        //Create one more level of delegation
        Arc::MCCConfig ds_client_cfg;
        //Use delegation credential (one option is to use the one got and stored 
        //in the delegation handler with 'service' delegation role, note in this 
        //case the service should be configured with delegation handler with both 
        //'client' and 'service' role; the other option is cofigure the credential
        //(EEC credential or delegated credential) in this 'client' role delegation
        //handler's configuration. which can be concluded here is: the former option
        //applies to intermediate service; the later option applies to the client
        //utilities) to create one more level of delegation, this delegation credential
        //will be used by the next intermediate service to act on behalf of
        //the EEC credential's holder
        if(!proxy_file_.empty()) {        
          ds_client_cfg.AddProxy(proxy_file_);
        }
        else if(!cert_file_.empty()&&!key_file_.empty()) {
          ds_client_cfg.AddCertificate(cert_file_); 
          ds_client_cfg.AddPrivateKey(key_file_); 
        }
        else {
          //Parse the delegation credential from message context
          std::string proxy_path;
          DelegationContext* deleg_ctx = get_delegcontext(*msg);
          if(!deleg_ctx) {
            logger.msg(Arc::ERROR, "Can't acquire delegation context");
            return false;
          };   
          ds_client_cfg.AddProxy(proxy_path);
        }
        if(!ca_dir_.empty()) ds_client_cfg.AddCADir(ca_dir_);
        if(!ca_file_.empty())ds_client_cfg.AddCAFile(ca_file_);
        //Note here the delegation service endpoint to which this service will
        //create the delegation credential ('n+1' level delegation) could be
        //different with the delegation service endpoint from which this service
        //acquire the delegation credential ('n' level delegation)

        Arc::URL ds_url(ds_endpoint_);
        Arc::ClientX509Delegation client_deleg(ds_client_cfg,ds_url);
        std::string delegation_id;
        //delegation_id will be used later to interact           
        //with the next intermediate service.
        if(!client_deleg.createDelegation(DELEG_ARC,delegation_id)) {
          logger.msg(ERROR,"Can not create delegation crendential to delgation service: %s",ds_endpoint_.c_str());
          return false;
        };

        //Send the endpoint of delegation service and delegation ID to 
        //the peer service side, on which the delegation handler with 
        //'delegation_service' role will get the endpoint of delegation
        //service and delegation ID to aquire delegation credential.
        //
        //The delegation service and delegation ID
        //information will be sent to the service side by the 
        //client side, so the handler should be configured into 
        //the 'outgoing' message.
        //
        //The credential used to send delegation service and delegation ID
        //information is the same credential which is used to interact with
        //the peer service. And the target service is the peer service.
        Arc::MCCConfig peers_client_cfg;
        if(!cert_file_.empty())peers_client_cfg.AddCertificate(cert_file_);
        if(!key_file_.empty())peers_client_cfg.AddPrivateKey(key_file_);
        if(!proxy_file_.empty())peers_client_cfg.AddProxy(proxy_file_);
        if(!ca_dir_.empty()) peers_client_cfg.AddCADir(ca_dir_);
        if(!ca_file_.empty())peers_client_cfg.AddCAFile(ca_file_);
        Arc::URL peers_url(peers_endpoint_);
        Arc::ClientSOAP client_peer(peers_client_cfg,peers_url);
        Arc::NS delegation_ns; delegation_ns["deleg"]="urn:delegation"; //
        Arc::PayloadSOAP req(delegation_ns);
        req.NewChild("DelegationService")=ds_endpoint_;
        req.NewChild("DelegationID")=delegation_id;
        Arc::PayloadSOAP* resp = NULL;
        Arc::MCC_Status status = client_peer.process(&req,&resp);
        if(!status) {
          logger.msg(Arc::ERROR, "SOAP invokation failed");
          throw std::runtime_error("SOAP invokation failed");
        }
        if(resp == NULL) {
          logger.msg(Arc::ERROR,"There is no SOAP response");
          throw std::runtime_error("There is no SOAP response");
        }
        if(resp) delete resp;
        return true;     
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


