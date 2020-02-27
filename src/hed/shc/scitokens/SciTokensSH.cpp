#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <arc/StringConv.h>
#include <arc/scitokens/jwse.h>
#include <arc/message/SecAttr.h>
#include <arc/external/cJSON/cJSON.h>

#include "SciTokensSH.h"

static Arc::Logger logger(Arc::Logger::rootLogger, "SciTokensSH");

Arc::Plugin* ArcSec::SciTokensSH::get_sechandler(Arc::PluginArgument* arg) {
  ArcSec::SecHandlerPluginArgument* shcarg =
          arg?dynamic_cast<ArcSec::SecHandlerPluginArgument*>(arg):NULL;
  if(!shcarg) return NULL;
  ArcSec::SciTokensSH* plugin = new ArcSec::SciTokensSH((Arc::Config*)(*shcarg),(Arc::ChainContext*)(*shcarg),arg);
  if(!plugin) return NULL;
  if(!(*plugin)) { delete plugin; plugin = NULL; };
  return plugin;
}

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
    { "scitokens.handler", "HED:SHC", NULL, 0, &ArcSec::SciTokensSH::get_sechandler},
    { NULL, NULL, NULL, 0, NULL }
};

namespace ArcSec {
using namespace Arc;


class SciTokensSecAttr: public SecAttr {
 public:
  SciTokensSecAttr(Arc::Message* msg);
  virtual ~SciTokensSecAttr(void);
  virtual operator bool(void) const;
  virtual std::string get(const std::string& id) const;
 protected:
  bool valid_;
  Arc::JWSE jwse_;
  std::string token_;
};

int strnicmp(char const* left, char const* right, size_t len) {
  while(len > 0) {
    if(std::tolower(*left) != std::tolower(*right)) return *left - *right;
    if(*left == '\0') return 0;
    --len;
    ++left;
    ++right;
  };
  return 0;
}

SciTokensSecAttr::SciTokensSecAttr(Arc::Message* msg):valid_(false) {
  static const char tokenid[] = "bearer ";
  if(msg) {
    logger.msg(DEBUG, "SciTokens: Attr: message");
    MessageAttributes* attrs = msg->Attributes();
    if(attrs) {
      AttributeIterator it = attrs->getAll();
      while(true) {
        logger.msg(DEBUG, "SciTokens: Attr: %s = %s", it.key(), *it);
        ++it;
        if(!it.hasMore()) break;
      }
      token_ = attrs->get("HTTP:authorization");
      logger.msg(DEBUG, "SciTokens: Attr: token: %s", token_);
      if(strnicmp(token_.c_str(), tokenid, sizeof(tokenid)-1) == 0) {
        token_.erase(0, sizeof(tokenid)-1);
        logger.msg(DEBUG, "SciTokens: Attr: token: bearer: %s", token_);
        valid_ = jwse_.Input(token_);
      };
    };
  };
}

SciTokensSecAttr::~SciTokensSecAttr() {
}

std::string SciTokensSecAttr::get(const std::string& id) const {
  if(id == "") // whole token requested
    return token_;
  if(id == "iss+sub") { // special name for combination of issuer and subject
    std::string issuer = get("iss");
    std::string subject = get("sub");
    if(issuer.empty() || subject.empty()) return "";
    return issuer + "/" + subject;
  };
  cJSON const * obj = jwse_.Claim(id.c_str());
  if(!obj)
    return "";
  if(obj->type != cJSON_String)
    return "";
  if(!obj->valuestring)
    return "";
  return obj->valuestring;
}

SciTokensSecAttr::operator bool() const {
  return valid_;
}

SciTokensSH::SciTokensSH(Config *cfg,ChainContext*,Arc::PluginArgument* parg):SecHandler(cfg,parg),valid_(false){
  valid_ = true;
}

SciTokensSH::~SciTokensSH(){
}

SecHandlerStatus SciTokensSH::Handle(Arc::Message* msg) const {
  logger.msg(DEBUG, "SciTokens: Handle");
  if(msg) {
    logger.msg(DEBUG, "SciTokens: Handle: message");
    SciTokensSecAttr* sattr = new SciTokensSecAttr(msg);
    if(!*sattr) {
      logger.msg(ERROR, "Failed to create SciTokens security attributes");
      delete sattr;
      sattr = NULL;
    } else {
      logger.msg(DEBUG, "SciTokens: Handle: attributes created: subject = %s", sattr->get("iss+sub"));
      msg->Auth()->set("SCITOKENS",sattr);
      msg->Attributes()->set("SCITOKENS:IDENTITYDN",sattr->get("iss+sub"));
    };
  };
  return true;
}


} // namespace ArcSec

