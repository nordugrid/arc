#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <arc/loader/SecHandlerLoader.h>
#include <arc/loader/Loader.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/XMLNode.h>
#include <arc/ws-security/X509Token.h>
#include <arc/xmlsec/XmlSecUtils.h>

#include "X509TokenSH.h"

static Arc::Logger logger(Arc::Logger::rootLogger, "X509TokenSH");

ArcSec::SecHandler* ArcSec::X509TokenSH::get_sechandler(Arc::Config *cfg, Arc::ChainContext* ctx) {
    return new ArcSec::X509TokenSH(cfg,ctx);
}

/*
sechandler_descriptors ARC_SECHANDLER_LOADER = {
    { "x509token.creator", 0, &get_sechandler},
    { NULL, 0, NULL }
};
*/

namespace ArcSec {
using namespace Arc;

X509TokenSH::X509TokenSH(Config *cfg,ChainContext*):SecHandler(cfg){
  if(!init_xmlsec()) return;
  process_type_=process_none;
  std::string process_type = (std::string)((*cfg)["Process"]);
  if(process_type == "generate") { 
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
    process_type_=process_generate;
  } else if(process_type == "extract") {
    //If ca file does not exist, we can only verify the signature by
    //using the certificate in the incoming wssecurity; we can not authenticate
    //the the message because we can not check the certificate chain without 
    //trusted ca.
    ca_file_=(std::string)((*cfg)["CACertificatePath"]);
    ca_dir_=(std::string)((*cfg)["CACertificatesDir"]);
    if(ca_file_.empty() && ca_dir_.empty()) {
      logger.msg(INFO,"Missing or empty CertificatePath or CACertificatesDir element; will only check the signature, will not do message authentication");
    };
    process_type_=process_extract;
  } else {
    logger.msg(ERROR,"Processing type not supported: %s",process_type);
    return;
  };
}

X509TokenSH::~X509TokenSH() {
  final_xmlsec();
}

bool X509TokenSH::Handle(Arc::Message* msg){
  if(process_type_ == process_extract) {
    try {
      PayloadSOAP* soap = dynamic_cast<PayloadSOAP*>(msg->Payload());
      X509Token xt(*soap);
      if(!xt) {
        logger.msg(ERROR,"Failed to parse X509 Token from incoming SOAP");
        return false;
      };
      if(!xt.Authenticate()) {
        logger.msg(ERROR, "Failed to verify X509 Token inside the incoming SOAP");
        return false;
      };
      if((!ca_file_.empty() || !ca_dir_.empty()) && !xt.Authenticate(ca_file_, ca_dir_)) {
        logger.msg(ERROR, "Failed to authenticate X509 Token inside the incoming SOAP");
        return false;
      };
      logger.msg(INFO, "Succeeded to authenticate X509Token");
    } catch(std::exception) {
      logger.msg(ERROR,"Incoming Message is not SOAP");
      return false;
    }  
  } else if(process_type_ == process_generate) {
    try {
      PayloadSOAP* soap = dynamic_cast<PayloadSOAP*>(msg->Payload());
      X509Token xt(*soap, cert_file_, key_file_);
      if(!xt) {
        logger.msg(ERROR,"Failed to generate X509 Token for outgoing SOAP");
        return false;
      };
      //Reset the soap message
      (*soap) = xt;
    } catch(std::exception) {
      logger.msg(ERROR,"Outgoing Message is not SOAP");
      return false;
    }
  } else {
    logger.msg(ERROR,"X509 Token handler is not configured");
    return false;
  } 
  return true;
}

}


