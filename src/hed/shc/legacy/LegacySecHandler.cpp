#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//#include <iostream>
//#include <globus_gsi_credential.h>

#include <arc/StringConv.h>
#include <arc/Utils.h>

#include "LegacySecAttr.h"
#include "auth.h"

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
  conf_file_ = (std::string)(*cfg)["ConfigFile"];
  if(conf_file_.empty()) {
    logger.msg(Arc::ERROR, "LegacySecHandler: configuration file not specified");
  };
}

LegacySecHandler::~LegacySecHandler(void) {
}

bool LegacySecHandler::Handle(Arc::Message* msg) const {
  if(conf_file_.empty()) {
    logger.msg(Arc::ERROR, "LegacySecHandler: configuration file not specified");
    return false;
  };
  AuthUser auth(*msg);
  std::ifstream f(conf_file_.c_str());
  if(!f) {
    logger.msg(Arc::ERROR, "LegacySecHandler: configuration file can not be read");
    return false;
  };
  bool is_group = false;
  bool is_vo = false;
  int group_match = AAA_NO_MATCH;
  bool vo_match = false;
  std::string block_name;
  // Match all [group] and [vo] blocks
  Arc::AutoPointer<LegacySecAttr> sattr(new LegacySecAttr(logger));
  while(!f.eof()) {
    if(!f) {
      logger.msg(Arc::ERROR, "LegacySecHandler: configuration file can not be read");
      return false;
    };
    std::string line;
    getline(f,line);
    line = Arc::trim(line);
    if(line.empty()) continue;
    if(line[0] == '#') continue;
    if(line[0] == ']') {
      if(line.length() < 2) return false;
      // End of block processing
      if(is_group && (group_match == AAA_POSITIVE_MATCH)  && !block_name.empty()) {
        auth.add_group(block_name.c_str());
        sattr->AddGroup(block_name);
      };
      if(is_vo && vo_match && !block_name.empty()) {
        auth.add_vo(block_name.c_str());
        sattr->AddVO(block_name);
      };
      line = line.substr(1,line.length()-2);
      block_name = "";
      std::string::size_type ps = line.find('/');
      if(ps != std::string::npos) {
        block_name = Arc::trim(line.substr(ps+1));
        line.resize(ps);
      };
      line = Arc::trim(line);
      is_group = (line == "group");
      is_vo = (line == "vo");
      group_match = AAA_NO_MATCH;
      vo_match = false;
      continue;
    };
    std::string cmd;
    std::string::size_type p = line.find('=');
    if(p == std::string::npos) {
      cmd = Arc::trim(line);
    } else {
      line[p] = ' ';
      cmd = Arc::trim(line.substr(0,p));
    };
    if(cmd == "name") {
      if(p != std::string::npos) {
        block_name = Arc::trim(Arc::trim(line.substr(p+1)),"\"");
      };
      continue;
    };
    if(is_group) {
      if(group_match == AAA_NO_MATCH) {
        group_match = auth.evaluate(line.c_str());
      };
    } else if(is_vo) {
      if(cmd == "file") {
        if(p != std::string::npos) {
          // std::string filename = Arc::trim(Arc::trim(line.substr(p+1)),"\"");
          // Because file=filename looks exactly like 
          // matching rule evaluate() can be used
          int r = auth.evaluate(line.c_str());
          if(r == AAA_POSITIVE_MATCH) vo_match = vo_match || true;
        };
      };
    };
  };
  // End of block processing
  if(is_group && (group_match == AAA_POSITIVE_MATCH) && !block_name.empty()) {
    auth.add_group(block_name.c_str());
    sattr->AddGroup(block_name);
  };
  if(is_vo && vo_match && !block_name.empty()) {
    auth.add_vo(block_name.c_str());
    sattr->AddVO(block_name);
  };
  // Pass all matched groups and VOs to Message in SecAttr
  // TODO: maybe assign to context
  msg->Auth()->set("ARCLEGACY",sattr.Release());
  return true;
}


} // namespace ArcSHCLegacy

