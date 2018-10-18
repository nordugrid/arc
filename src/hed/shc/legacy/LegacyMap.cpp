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
  LegacyMap* plugin = new LegacyMap((Arc::Config*)(*shcarg),(Arc::ChainContext*)(*shcarg),arg);
  if(!plugin) return NULL;
  if(!(*plugin)) { delete plugin; plugin = NULL; };
  return plugin;
}

LegacyMap::LegacyMap(Arc::Config *cfg,Arc::ChainContext* ctx,Arc::PluginArgument* parg):SecHandler(cfg,parg),attrname_("ARCLEGACYMAP"),srcname_("ARCLEGACY") {
  Arc::XMLNode attrname = (*cfg)["AttrName"];
  if((bool)attrname) {
    attrname_ = (std::string)attrname;
  };
  Arc::XMLNode srcname = (*cfg)["SourceAttrName"];
  if((bool)srcname) {
    srcname_ = (std::string)srcname;
  };
  Arc::XMLNode block = (*cfg)["ConfigBlock"];
  while((bool)block) {
    std::string filename = (std::string)(block["ConfigFile"]);
    if(filename.empty()) {
      logger.msg(Arc::ERROR, "Configuration file not specified in ConfigBlock");
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
    if(!name.empty()) bname = bname+":"+name;
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
    // Now we have only one command left "unixgroupmap" and it is not indicated at all.
    //# [unixgroupmap] rule [=] authgroup args
    if(map_.mapgroup(cmd.c_str(), line.c_str()) == AAA_FAILURE) {
      logger_.msg(Arc::ERROR, "Failed processing user mapping command: %s %s", cmd, line);
      return false;
    };
    return true;
  };

 private:
  const LegacyMap::cfgfile& file_;
  //AuthUser& auth_;
  UnixMap map_;
  bool is_block_;
};

class LegacyMapAttr: public Arc::SecAttr {
 public:
  LegacyMapAttr(const std::string& id):id_(id) { };
  virtual ~LegacyMapAttr(void);

  // Common interface
  virtual operator bool(void) const;
  virtual bool Export(Arc::SecAttrFormat format,Arc::XMLNode &val) const;
  virtual std::string get(const std::string& id) const;
  virtual std::list<std::string> getAll(const std::string& id) const;

  // Specific interface
  const std::string GetID(void) const { return id_; };

 protected:
  std::string id_;
  virtual bool equal(const SecAttr &b) const;
};

LegacyMapAttr::~LegacyMapAttr(void) {
}

LegacyMapAttr::operator bool(void) const {
  return true; 
}

bool LegacyMapAttr::Export(Arc::SecAttrFormat format,Arc::XMLNode &val) const {
  return true; 
}

std::string LegacyMapAttr::get(const std::string& id) const {
  return ""; 
}

std::list<std::string> LegacyMapAttr::getAll(const std::string& id) const {
  return std::list<std::string>(); 
}

bool LegacyMapAttr::equal(const SecAttr &b) const {
  const LegacyMapAttr& a = dynamic_cast<const LegacyMapAttr&>(b);
  if (!a) return false;
  return (id_ == a.id_);
}

ArcSec::SecHandlerStatus LegacyMap::Handle(Arc::Message* msg) const {
  if(blocks_.size()<=0) {
    logger.msg(Arc::ERROR, "LegacyMap: no configurations blocks defined");
    return false;
  };
  Arc::SecAttr* sattr = msg->Auth()->get(srcname_);
  if(!sattr) {
    // Only if information collection is done per context.
    // Check if decision is already made.
    Arc::SecAttr* dattr = msg->AuthContext()->get(attrname_);
    if(dattr) {
      LegacyMapAttr* mattr = dynamic_cast<LegacyMapAttr*>(dattr);
      if(mattr) {
        // Mapping already was done in this context
        std::string id = mattr->GetID();
        if(!id.empty()) {
          msg->Attributes()->set("SEC:LOCALID",id);
        };
        return true;
      };
    };
  };
  if(!sattr) sattr = msg->AuthContext()->get(srcname_);
  if(!sattr) {
    logger.msg(Arc::ERROR, "LegacyPDP: there is no %s Sec Attribute defined. Probably ARC Legacy Sec Handler is not configured or failed.",srcname_);
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
  std::string id;
  for(std::list<cfgfile>::const_iterator block = blocks_.begin();
                              block!=blocks_.end();++block) {
    LegacyMapCP parser(*block,logger,auth);
    if(!parser) return false;
    if(!parser.Parse()) return false;
    id = parser.LocalID();
    if(!id.empty()) {
      logger.msg(Arc::INFO,"Grid identity is mapped to local identity '%s'",id);
      msg->Attributes()->set("SEC:LOCALID",id);
      break;
    };
  };
  // Store decision even if no id was selected
  msg->AuthContext()->set(attrname_,new LegacyMapAttr(id));
  return true;
}


} // namespace ArcSHCLegacy

