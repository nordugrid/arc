#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <arc/loader/SecHandlerLoader.h>
#include <arc/loader/Loader.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/XMLNode.h>
#include <arc/ws-security/UsernameToken.h>

#include "UsernameTokenSH.h"

static Arc::Logger logger(Arc::Logger::rootLogger, "UsernameTokenSH");

ArcSec::SecHandler* ArcSec::UsernameTokenSH::get_sechandler(Arc::Config *cfg, Arc::ChainContext* ctx) {
    return new ArcSec::UsernameTokenSH(cfg,ctx);
}

/*
sechandler_descriptors ARC_SECHANDLER_LOADER = {
    { "usernametoken.creator", 0, &get_sechandler},
    { NULL, 0, NULL }
};
*/

namespace ArcSec {
using namespace Arc;

UsernameTokenSH::UsernameTokenSH(Config *cfg,ChainContext*):SecHandler(cfg){
  process_type_=process_none;
  std::string process_type = (std::string)((*cfg)["Process"]);
  if(process_type == "extract") { 
    password_source_=(std::string)((*cfg)["PasswordSource"]);
    if(password_source_.empty()) {
      logger.msg(ERROR,"Missing or empty PasswordSource element");
      return;
    };
    process_type_=process_extract;
  } else if(process_type == "generate") {
    std::string pwd_encoding = (std::string)((*cfg)["PasswordEncoding"]);
    if(pwd_encoding == "digest") {
      password_type_=password_digest;
    } else if((pwd_encoding == "text") || pwd_encoding.empty()) {
      password_type_=password_text;
    } else {
      logger.msg(ERROR,"Password encoding type not supported: %s",pwd_encoding);
      return;
    };
    username_=(std::string)((*cfg)["Username"]);
    if(username_.empty()) {
      logger.msg(ERROR,"Missing or empty Username element");
      return;
    };
    password_=(std::string)((*cfg)["Password"]);
    process_type_=process_generate;
  } else {
    logger.msg(ERROR,"Processing type not supported: %s",process_type);
    return;
  };
}

UsernameTokenSH::~UsernameTokenSH() {
}

bool UsernameTokenSH::Handle(Arc::Message* msg){
  if(process_type_ == process_extract) {
    try {
      PayloadSOAP* soap = dynamic_cast<PayloadSOAP*>(msg->Payload());
      UsernameToken ut(*soap);
      if(!ut) {
        logger.msg(ERROR,"Failed to parse Username Token from incoming SOAP");
        return false;
      };
      std::string derived_key;
      std::ifstream stream(password_source_.c_str());  
      if(!ut.Authenticate(stream, derived_key)) {
        logger.msg(ERROR, "Failed to authenticate Username Token inside the incoming SOAP");
        stream.close(); return false;
      };
      logger.msg(INFO, "Succeeded to authenticate UsernameToken");
      stream.close();
    } catch(std::exception) {
      logger.msg(ERROR,"Incoming Message is not SOAP");
      return false;
    }  
  } else if(process_type_ == process_generate) {
    try {
      PayloadSOAP* soap = dynamic_cast<PayloadSOAP*>(msg->Payload());
      UsernameToken ut(*soap,username_,password_,std::string(""),
         (password_type_==password_digest)?(UsernameToken::PasswordDigest):(UsernameToken::PasswordText));
      if(!ut) {
        logger.msg(ERROR,"Failed to generate Username Token for outgoing SOAP");
        return false;
      };
    } catch(std::exception) {
      logger.msg(ERROR,"Outgoing Message is not SOAP");
      return false;
    }
  } else {
    logger.msg(ERROR,"Username Token handler is not configured");
    return false;
  } 
  return true;
}

}


