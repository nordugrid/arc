#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <openssl/md5.h>

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
  delegation_cred_identity_ = (std::string)((*cfg)["DelegationCredIdentity"]);  

  if(delegation_type == "x509") {
    proxy_file_=(std::string)((*cfg)["ProxyPath"]);
    cert_file_=(std::string)((*cfg)["CertificatePath"]);
    if(cert_file_.empty()&&proxy_file_.empty()&&delegation_cred_identity_.empty()) {
      logger.msg(ERROR,"Missing CertificatePath element or ProxyPath element, or <DelegationCredIdentity/> is missing");
      return;
    };
    key_file_=(std::string)((*cfg)["KeyPath"]);
    if(key_file_.empty()&&proxy_file_.empty()&&delegation_cred_identity_.empty()) {
      logger.msg(ERROR,"Missing or empty KeyPath element, or <DelegationCredIdentity/> is missing");
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
  DelegationContext(void){ have_delegated_ = false; };
  virtual ~DelegationContext(void) { };
};

DelegationContext* DelegationSH::get_delegcontext(Arc::Message& msg) {
  DelegationContext* deleg_ctx=NULL;
  Arc::MessageContextElement* mcontext = (*msg.Context())["deleg.context"];
  if(mcontext) {
    try {
      deleg_ctx = dynamic_cast<DelegationContext*>(mcontext);
    } catch(std::exception& e) { };
  };
  if(deleg_ctx) return deleg_ctx;
  deleg_ctx = new DelegationContext();
  if(deleg_ctx) {
    msg.Context()->Add("deleg.context",deleg_ctx);
  } else {
    logger.msg(Arc::ERROR, "Failed to acquire delegation context");
  }
  return deleg_ctx;
}

//Generate hash value for a string
static unsigned long string_hash(const std::string& value){
  unsigned long ret=0;
  unsigned char md[16];
  MD5((unsigned char *)(value.c_str()),value.length(),&(md[0]));
  ret=(((unsigned long)md[0])|((unsigned long)md[1]<<8L)|
       ((unsigned long)md[2]<<16L)|((unsigned long)md[3]<<24L)
       )&0xffffffffL;
  return ret;
}

bool DelegationSH::Handle(Arc::Message* msg){
  if(delegation_type_ == delegation_x509) {
    try {
      PayloadSOAP* soap = dynamic_cast<PayloadSOAP*>(msg->Payload());

      if(delegation_role_ == delegation_service) {
        //Try to get the delegation service and delegation ID
        //information from incoming message
        logger.msg(Arc::INFO,"+++++++++ Delegation handler with service role starts to process +++++++++");
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
              std::string cred_identity = msg->Attributes()->get("TLS:IDENTITYDN");
              std::string cred_delegator_ip = msg->Attributes()->get("TCP:REMOTEHOST");
              if(!client_deleg.acquireDelegation(DELEG_ARC,delegation_cred,delegation_id,cred_identity,cred_delegator_ip)) {
                logger.msg(ERROR,"Can not get the delegation credential: %s from delegation service:%s",delegation_id.c_str(),ds_endpoint.c_str());
                return false;
              };
            }

            std::string cred_identity = msg->Attributes()->get("TLS:IDENTITYDN");
            unsigned long hash_value = string_hash(cred_identity);
            //Store the delegated credential (got from delegation service)
            //into local temporary path; this delegated credential will be 
            //used by the client functionality in this service (intemidiate 
            //service) to contact the next service. 
            std::string deleg_cred_path="/tmp/";
            char text[8];
            sprintf(text, "%08lx", hash_value);
            deleg_cred_path.append(text).append(".pem");
            logger.msg(INFO,"Delegated credential identity: %s",cred_identity.c_str());
            logger.msg(INFO,"The delegated credential got from delegation service is stored into path: %s",deleg_cred_path.c_str());
            std::ofstream proxy_f(deleg_cred_path.c_str());
            proxy_f.write(delegation_cred.c_str(),delegation_cred.size());
            proxy_f.close();

            //Remove the delegation information inside the payload 
            //since this information ('DelegationService' and 'DelegationID') 
            //is only supposed to be consumer by this security handler, not 
            //the hosted service itself.
            if((*inpayload)["DelegationService"]) ((*inpayload)["DelegationService"]).Destroy();
            if((*inpayload)["DelegationID"]) ((*inpayload)["DelegationID"]).Destroy();

          } else {
            logger.msg(ERROR,"The endpoint of delgation service should be configured");
            return false;
          }
        };
        logger.msg(Arc::INFO,"+++++++++ Delegation handler with service role ends +++++++++");
        return true;
      }
      else if(delegation_role_ == delegation_client) {
        //Create one more level of delegation
        Arc::MCCConfig ds_client_cfg;
        //Use delegation credential (one option is to use the one got and stored 
        //in the delegation handler with 'service' delegation role, note in this 
        //case the service implementation should configure the client interface 
        //(the client interface which is called by the service implementation to
        //contact another service) with the 'Identity' of the credential on which 
        //this service will act on behalf, then the delegation handler with 'client' 
        //delegation role (configured in this client interface's configuration) will
        //get the delegated credential from local temporary path (this path is 
        //decided according to the 'Identity'); the other option is cofigure the credential
        //(EEC credential or delegated credential) in this 'client' role delegation
        //handler's configuration. What can be concluded here is: the former option
        //applies to intermediate service; the later option applies to the client
        //utilities. 
        //By creating one more level of delegation, the delegated credential
        //will be used by the next intermediate service to act on behalf of
        //the EEC credential's holder

        //Store delegation context into message context
        DelegationContext* deleg_ctx = get_delegcontext(*msg);
        if(!deleg_ctx) {
          logger.msg(Arc::ERROR, "Can't create delegation context");
          return false;
        }
        //Credential delegation will only be triggered once for each connection
        if(deleg_ctx->have_delegated_) return true;

        logger.msg(Arc::INFO,"+++++++++ Delegation handler with client role starts to process +++++++++");
        std::string proxy_path;
        if(!delegation_cred_identity_.empty()) {
          unsigned long hash_value = string_hash(delegation_cred_identity_);
          proxy_path="/tmp/";
          char text[8];
          sprintf(text, "%08lx", hash_value);
          proxy_path.append(text).append(".pem");
          logger.msg(INFO,"Delegated credential identity: %s",delegation_cred_identity_.c_str());
          logger.msg(INFO,"The delegated credential got from path: %s",proxy_path.c_str());
          ds_client_cfg.AddProxy(proxy_path);
        }else if(!proxy_file_.empty()) {        
          ds_client_cfg.AddProxy(proxy_file_);
        }else if(!cert_file_.empty()&&!key_file_.empty()) {
          ds_client_cfg.AddCertificate(cert_file_); 
          ds_client_cfg.AddPrivateKey(key_file_); 
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
        //client side. If the client functionality is hosted/called by
        //some intermediate service, this handler (delegation handler with
        //client role) should be configured into the 'incoming' message of 
        //the hosted service; if the client functionality is called by
        //some independent client utility, this handler should be configured 
        //into the 'incoming' message of the client itself.
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

        //Treat the delegation information as 'out-of-bound' information
        //to SOAP message
        Arc::PayloadSOAP* outpayload = NULL;
        try {
          outpayload = dynamic_cast<Arc::PayloadSOAP*>(msg->Payload());
        } catch(std::exception& e) { };
        if(!outpayload) {
          logger.msg(Arc::ERROR, "output is not SOAP");
          return false;
        };
        outpayload->NewChild("deleg:DelegationService")=ds_endpoint_;
        outpayload->NewChild("deleg:DelegationID")=delegation_id;

        //Set the 'have_delegated_' value of DelegationContext to
        //be true, so that the delegation process will only be triggered
        //once for each communication.
        deleg_ctx->have_delegated_=true;      

        logger.msg(Arc::INFO, "Succeeded to send DelegationService: %s and DelegationID: %s info to peer service",ds_endpoint_.c_str(),delegation_id.c_str());
        logger.msg(Arc::INFO,"+++++++++ Delegation handler with service role ends +++++++++");
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


