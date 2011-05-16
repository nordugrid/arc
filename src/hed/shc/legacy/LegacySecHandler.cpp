#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/Utils.h>

#include "LegacySecAttr.h"
#include "auth.h"
#include "ConfigParser.h"

#include "LegacySecHandler.h"

namespace ArcSHCLegacy {

Arc::Plugin* LegacySecHandler::get_sechandler(Arc::PluginArgument* arg) {
  ArcSec::SecHandlerPluginArgument* shcarg =
            arg?dynamic_cast<ArcSec::SecHandlerPluginArgument*>(arg):NULL;
  if(!shcarg) return NULL;
  LegacySecHandler* plugin = new LegacySecHandler((Arc::Config*)(*shcarg),(Arc::ChainContext*)(*shcarg));
  if(!plugin) return NULL;
  if(!(*plugin)) { delete plugin; plugin = NULL; };
  return plugin;
}

LegacySecHandler::LegacySecHandler(Arc::Config *cfg,Arc::ChainContext* ctx):SecHandler(cfg) {
  Arc::XMLNode conf_file = (*cfg)["ConfigFile"];
  while((bool)conf_file) {
    std::string filename = (std::string)conf_file;
    if(!filename.empty()) {
      conf_files_.push_back(filename);
    };
    ++conf_file;
  };
  if(conf_files_.size() <= 0) {
    logger.msg(Arc::ERROR, "LegacySecHandler: configuration file not specified");
  };
}

LegacySecHandler::~LegacySecHandler(void) {
}

class LegacySHCP: public ConfigParser {
 public:
  LegacySHCP(const std::string& filename, Arc::Logger& logger, AuthUser& auth, LegacySecAttr& sattr):ConfigParser(filename,logger),auth_(auth),sattr_(sattr) {
  };

  virtual ~LegacySHCP(void) {
  };

 protected:
  virtual bool BlockStart(const std::string& id, const std::string& name) {
    group_match_ = AAA_NO_MATCH;
    vo_match_ = false;
    return true;
  };

  virtual bool BlockEnd(const std::string& id, const std::string& name) {
    if(id == "group") {
      if((group_match_ == AAA_POSITIVE_MATCH)  && !name.empty()) {
        auth_.add_group(name);
        sattr_.AddGroup(name);
      };
    } else if(id == "vo") {
      if(vo_match_ && !name.empty()) {
        auth_.add_vo(name);
        sattr_.AddVO(name);
      };
    };
    return true;
  };

  virtual bool ConfigLine(const std::string& id, const std::string& name, const std::string& cmd, const std::string& line) {
    if(id == "group") {
      if(group_match_ == AAA_NO_MATCH) {
        group_match_ = auth_.evaluate((cmd + " " + line).c_str());
      };
    } else if(id == "vo") {
      if(!vo_match_) {
        if(cmd == "file") {
          if(!line.empty()) {
            // Because file=filename looks exactly like 
            // matching rule evaluate() can be used
            int r = auth_.evaluate((cmd + " " + line).c_str());
            vo_match_ = (r == AAA_POSITIVE_MATCH);
          };
        };
      };
    };
    return true;
  };

 private:
  AuthUser& auth_;
  LegacySecAttr& sattr_;
  int group_match_;
  bool vo_match_;
};


bool LegacySecHandler::Handle(Arc::Message* msg) const {
  if(conf_files_.size() <= 0) {
    logger.msg(Arc::ERROR, "LegacySecHandler: configuration file not specified");
    return false;
  };
  AuthUser auth(*msg);
  Arc::AutoPointer<LegacySecAttr> sattr(new LegacySecAttr(logger));
  for(std::list<std::string>::const_iterator conf_file = conf_files_.begin();
                             conf_file != conf_files_.end();++conf_file) {
    LegacySHCP parser(*conf_file,logger,auth,*sattr);
    if(!parser) return false;
    if(!parser.Parse()) return false;
  };
  // Pass all matched groups and VOs to Message in SecAttr
  // TODO: maybe assign to context
  msg->Auth()->set("ARCLEGACY",sattr.Release());
  return true;
}


} // namespace ArcSHCLegacy

