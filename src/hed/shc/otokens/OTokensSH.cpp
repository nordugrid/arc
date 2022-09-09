#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <arc/StringConv.h>
#include <arc/otokens/otokens.h>
#include <arc/message/SecAttr.h>
#include <arc/external/cJSON/cJSON.h>

#include "OTokensSH.h"

static Arc::Logger logger(Arc::Logger::rootLogger, "OTokensSH");

Arc::Plugin* ArcSec::OTokensSH::get_sechandler(Arc::PluginArgument* arg) {
  ArcSec::SecHandlerPluginArgument* shcarg =
          arg?dynamic_cast<ArcSec::SecHandlerPluginArgument*>(arg):NULL;
  if(!shcarg) return NULL;
  ArcSec::OTokensSH* plugin = new ArcSec::OTokensSH((Arc::Config*)(*shcarg),(Arc::ChainContext*)(*shcarg),arg);
  if(!plugin) return NULL;
  if(!(*plugin)) { delete plugin; plugin = NULL; };
  return plugin;
}

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
    { "otokens.handler", "HED:SHC", NULL, 0, &ArcSec::OTokensSH::get_sechandler},
    { NULL, NULL, NULL, 0, NULL }
};

namespace ArcSec {
using namespace Arc;


class OTokensSecAttr: public SecAttr {
 public:
  OTokensSecAttr(Arc::Message* msg);
  virtual ~OTokensSecAttr(void);
  virtual operator bool(void) const;
  virtual std::string get(const std::string& id) const;
  virtual std::list<std::string> getAll(const std::string& id) const;
  virtual std::map< std::string,std::list<std::string> > getAll() const;
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

OTokensSecAttr::OTokensSecAttr(Arc::Message* msg):valid_(false) {
  static const char tokenid[] = "bearer ";
  if(msg) {
    logger.msg(DEBUG, "OTokens: Attr: message");
    MessageAttributes* attrs = msg->Attributes();
    if(attrs) {
      AttributeIterator it = attrs->getAll();
      while(true) {
        logger.msg(DEBUG, "OTokens: Attr: %s = %s", it.key(), *it);
        ++it;
        if(!it.hasMore()) break;
      }
      token_ = attrs->get("HTTP:authorization");
      logger.msg(DEBUG, "OTokens: Attr: token: %s", token_);
      if(strnicmp(token_.c_str(), tokenid, sizeof(tokenid)-1) == 0) {
        token_.erase(0, sizeof(tokenid)-1);
        logger.msg(DEBUG, "OTokens: Attr: token: bearer: %s", token_);
        valid_ = jwse_.Input(token_);
        if(valid_) {
          // Additionally require signature protected by safely obtained key.
          // So far only accepting keys fetched from issuer through https.
          valid_ = (jwse_.InputKeyOrigin() == JWSE::ExternalSafeKey);
        };
      };
    };
  };
}

OTokensSecAttr::~OTokensSecAttr() {
}


std::string OTokensSecAttr::get(const std::string& id) const {
  std::list<std::string> items = getAll(id);
  if(items.empty())
    return std::string();
  return *(items.begin());
}

std::list<std::string> OTokensSecAttr::getAll(const std::string& id) const {
  // Implement for many claims
  std::list<std::string> items;
  if(!valid_)
    return items;
  if(id == "") { // whole token requested
    items.push_back(token_);
    return items;
  }
  if(id == "iss+sub") { // special name for combination of issuer and subject
    std::string issuer = get("iss");
    std::string subject = get("sub");
    if(issuer.empty() || subject.empty()) return items;
    items.push_back(issuer + "/" + subject);
    return items;
  };
  cJSON const * obj = jwse_.Claim(id.c_str());
  if(obj) {
    if(obj->type == cJSON_String) {
      if(obj->valuestring)
        items.push_back(obj->valuestring);
    }
    else if(obj->type == cJSON_Array) {
      obj = obj->child;
      for( ;obj; obj = obj->next) {
        if(obj->type == cJSON_String) {
          if(obj->valuestring) {
            items.push_back(obj->valuestring);
          }
        }
      }
    }
  }
  return items;
}

std::map< std::string,std::list<std::string> > OTokensSecAttr::getAll() const {
  std::map< std::string,std::list<std::string> > all;
  if(!valid_)
    return all;

  std::set<std::string> names = jwse_.ClaimNames();
  for(std::set<std::string>::iterator itName = names.begin(); itName != names.end(); ++itName) {
    all[*itName] = getAll(*itName);
  }
  return all;
}

OTokensSecAttr::operator bool() const {
  return valid_;
}

OTokensSH::OTokensSH(Config *cfg,ChainContext*,Arc::PluginArgument* parg):SecHandler(cfg,parg),valid_(false){
  valid_ = true;
}

OTokensSH::~OTokensSH(){
}

SecHandlerStatus OTokensSH::Handle(Arc::Message* msg) const {
  logger.msg(DEBUG, "OTokens: Handle");
  if(msg) {
    logger.msg(DEBUG, "OTokens: Handle: message");
    OTokensSecAttr* sattr = new OTokensSecAttr(msg);
    if(!*sattr) {
      logger.msg(ERROR, "Failed to create OTokens security attributes");
      delete sattr;
      sattr = NULL;
    } else {
      logger.msg(DEBUG, "OTokens: Handle: attributes created: subject = %s", sattr->get("iss+sub"));
      msg->Auth()->set("OTOKENS",sattr);
      msg->Attributes()->set("OTOKENS:IDENTITYDN",sattr->get("iss+sub"));
    };
  };
  return true;
}


} // namespace ArcSec

