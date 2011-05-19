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
  LegacyMapCP(const LegacyMap::cfgfile& file, Arc::Logger& logger, AuthUser& auth):ConfigParser(file.filename,logger),file_(file),map_(auth),is_block_(false) {
  };

  virtual ~LegacyMapCP(void) {
  };

  std::string LocalID(void) {
    if(!map_) return "";
    return map_.unix_name();
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
      if(*block == bname) {
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
  //AuthUser& auth_;
  UnixMap map_;
  bool is_block_;
};


bool LegacyMap::Handle(Arc::Message* msg) const {
  if(blocks_.size()<=0) {
    logger.msg(Arc::ERROR, "LegacyMap: no configurations blocks defined");
    return false;
  };
  Arc::SecAttr* sattr = msg->Auth()->get("ARCLEGACY");
  if(!sattr) sattr = msg->AuthContext()->get("ARCLEGACY");
  if(!sattr) {
    logger.msg(Arc::ERROR, "LegacyPDP: there is no ARCLEGACY Sec Attribute defined. Probably ARC Legacy Sec Handler is not configured or failed.");
    return false;
  };
  LegacySecAttr* lattr = dynamic_cast<LegacySecAttr*>(sattr);
  if(!lattr) {
    logger.msg(Arc::ERROR, "LegacyPDP: ARC Legacy Sec Attribute not recognized.");
    return false;
  };
  // Populate with collected info
  AuthUser auth(*msg);
  auth.add_groups(lattr->GetGroups());
  auth.add_vos(lattr->GetVOs());
  for(std::list<cfgfile>::const_iterator block = blocks_.begin();
                              block!=blocks_.end();++block) {
    LegacyMapCP parser(*block,logger,auth);
    if(!parser) return false;
    if(!parser.Parse()) return false;
    std::string id = parser.LocalID();
    if(!id.empty()) {
      logger.msg(Arc::INFO,"Grid identity is mapped to local identity '%s'",id);
      msg->Attributes()->set("SEC:LOCALID",id);
      break;
    };
  };
  return true;
}


} // namespace ArcSHCLegacy

