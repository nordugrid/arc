#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/Utils.h>

#include "LegacySecAttr.h"
#include "unixmap.h"
#include "ConfigParser.h"

#include "LegacyMap.h"

namespace ArcSHCLegacy {

Arc::Plugin* LegacyMap::get_sechandler(Arc::PluginArgument* arg) {
  ArcSec::SecHandlerPluginArgument* shcarg =
            arg?dynamic_cast<ArcSec::SecHandlerPluginArgument*>(arg):NULL;
  if(!shcarg) return NULL;
  LegacyMap* plugin = new LegacyMap((Arc::Config*)(*shcarg),(Arc::ChainContext*)(*shcarg));
  if(!plugin) return NULL;
  if(!(*plugin)) { delete plugin; plugin = NULL; };
  return plugin;
}

LegacyMap::LegacyMap(Arc::Config *cfg,Arc::ChainContext* ctx):SecHandler(cfg) {
  Arc::XMLNode block = (*cfg)["ConfigBlock"];
  while((bool)block) {
    std::string filename = (std::string)(block["ConfigFile"]);
    if(filename.empty()) {
      logger.msg(Arc::ERROR, "Configuration file not specified n ConfigBlock");
      blocks_.clear();
      return;
    };
    cfgfile file(filename);
    Arc::XMLNode name = block["BlockName"];
    while((bool)name) {
      std::string blockname = (std::string)name;
      if(blockname.empty()) {
        logger.msg(Arc::ERROR, "BlockName is empty");
        blocks_.clear();
        return;
      };
      file.blocknames.push_back(blockname);
      ++name;
    };
    blocks_.push_back(file);
    ++block;
  };
}

LegacyMap::~LegacyMap(void) {
}

class LegacyMapCP: public ConfigParser {
 public:
  LegacyMapCP(const LegacyMap::cfgfile& file, Arc::Logger& logger, AuthUser& auth, LegacySecAttr& sattr):ConfigParser(file.filename,logger),file_(file),auth_(auth),map_(auth),sattr_(sattr),is_block_(false) {
  };

  virtual ~LegacyMapCP(void) {
  };

 protected:
  virtual bool BlockStart(const std::string& id, const std::string& name) {
    if(map_) return true; // already mapped
    std::string bname = id;
    if(!name.empty()) bname = bname+"/"+name;
    if(file_.blocknames.empty()) {
      is_block_ = true;
      return true;
    };
    for(std::list<std::string>::const_iterator block = file_.blocknames.begin();
                                 block != file_.blocknames.end();++block) {
      if(*block == name) {
        is_block_ = true;
        break;
      };
    };
    return true;
  };

  virtual bool BlockEnd(const std::string& id, const std::string& name) {
    is_block_ = false;
    return true;
  };

  virtual bool ConfigLine(const std::string& id, const std::string& name, const std::string& cmd, const std::string& line) {
    if(!is_block_) return true;
    if(map_) return true; // already mapped
    if(cmd == "unixmap") {
      //# unixmap [unixname][:unixgroup] rule
      //unixmap="nobody:nogroup all"
      map_.mapname(line.c_str());
    } else if(cmd == "unixgroup") {
      //# unixgroup group rule
      //unixgroup="users simplepool /etc/grid-security/pool/users"
      map_.mapgroup(line.c_str());
    } else if(cmd == "unixvo") {
      //# unixvo vo rule
      //unixvo="ATLAS unixuser atlas:atlas"
      map_.mapvo(line.c_str());
    };
    return true;
  };

 private:
  const LegacyMap::cfgfile& file_;
  AuthUser& auth_;
  UnixMap map_;
  LegacySecAttr& sattr_;
  bool is_block_;
};


bool LegacyMap::Handle(Arc::Message* msg) const {
  if(blocks_.size()<=0) {
    logger.msg(Arc::ERROR, "LegacyMap: no configurations blocks defined");
    return false;
  };
  AuthUser auth(*msg);
  Arc::AutoPointer<LegacySecAttr> sattr(new LegacySecAttr(logger));
  for(std::list<cfgfile>::const_iterator block = blocks_.begin();
                              block!=blocks_.end();++block) {
    LegacyMapCP parser(block->filename,logger,auth,*sattr);
    if(!parser) return false;
    if(!parser.Parse()) return false;
  };
  // Pass all matched groups and VOs to Message in SecAttr
  // TODO: maybe assign to context
  msg->Auth()->set("ARCLEGACY",sattr.Release());
  return true;
}


} // namespace ArcSHCLegacy

