#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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

UsernameTokenSH::UsernameTokenSH(Config *cfg,ChainContext* ctx):SecHandler(cfg){
  process_type_=process_none;
  std::string process_type = (std::string)((*cfg)["Process"]);
  if(process_type == "extract") {
    logger.msg(ERROR,"Username Token extraction is not supported yet");
    return;
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
    logger.msg(ERROR,"Username Token extraction is not supported yet");
    return false;
  } else if(process_type_ == process_generate) {
    try {
      PayloadSOAP* soap = dynamic_cast<PayloadSOAP*>(msg->Payload());
      UsernameToken ut(*soap,username_,password_,std::string(""),
         (password_type_==password_digest)?(UsernameToken::PasswordDigest):(UsernameToken::PasswordText));
      if(!ut) {
        logger.msg(ERROR,"Failed to generate Username Token");
        return false;
      };
    } catch(std::exception) {
      logger.msg(ERROR,"Message is not SOAP");
      return false;
    }
  } else {
    logger.msg(ERROR,"Username Token handler is not configured");
    return false;
  } 
  return true;
}

}


