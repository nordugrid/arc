#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "LegacySecAttr.h"
#include "ConfigParser.h"
#include "auth.h"

#include "LegacyPDP.h"

namespace ArcSHCLegacy {

Arc::Plugin* LegacyPDP::get_pdp(Arc::PluginArgument *arg) {
    ArcSec::PDPPluginArgument* pdparg =
            arg?dynamic_cast<ArcSec::PDPPluginArgument*>(arg):NULL;
    if(!pdparg) return NULL;
    return new LegacyPDP((Arc::Config*)(*pdparg));
}

LegacyPDP::LegacyPDP(Arc::Config* cfg):PDP(cfg) {
  Arc::XMLNode group = (*cfg)["Group"];
  while((bool)group) {
    groups_.push_back((std::string)group);
    ++group;
  };
  Arc::XMLNode vo = (*cfg)["VO"];
  while((bool)vo) {
    vos_.push_back((std::string)vo);
    ++vo;
  };
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

LegacyPDP::~LegacyPDP() {
}

static bool match_lists(const std::list<std::string>& list1, const std::list<std::string>& list2, Arc::Logger& logger) {
  for(std::list<std::string>::const_iterator l1 = list1.begin(); l1 != list1.end(); ++l1) {
    for(std::list<std::string>::const_iterator l2 = list2.begin(); l2 != list2.end(); ++l2) {
      if((*l1) == (*l2)) return true;
    };
  };
  return false;
}

class LegacyPDPCP: public ConfigParser {
 public:
  LegacyPDPCP(const LegacyPDP::cfgfile& file, Arc::Logger& logger, AuthUser& auth):ConfigParser(file.filename,logger),file_(file),group_matched_(false),group_processed_(false) {
    auth.get_groups(groups_);
  };

  virtual ~LegacyPDPCP(void) {
  };

  bool Matched(void) {
    return group_matched_;
  };

  bool Processed(void) {
    return group_processed_;
  };

 protected:
  virtual bool BlockStart(const std::string& id, const std::string& name) {
    return true;
  };

  virtual bool BlockEnd(const std::string& id, const std::string& name) {
    return true;
  };

  virtual bool ConfigLine(const std::string& id, const std::string& name, const std::string& cmd, const std::string& line) {
    if(group_matched_) return true;
    if(cmd != "groupcfg") return true;
    std::string bname = id;
    if(!name.empty()) bname = bname+"/"+name;
    for(std::list<std::string>::const_iterator block = file_.blocknames.begin();
                                 block != file_.blocknames.end();++block) {
      if(*block == bname) {
        std::list<std::string> groups;
        Arc::tokenize(line,groups," \t","\"","\"");
        if(!groups.empty()) group_processed_ = true;
        if(match_lists(groups_,groups,logger_)) {
          group_matched_ = true;
        };
        break;
      };
    };
    return true;
  };

 private:
  const LegacyPDP::cfgfile& file_;
  bool group_matched_;
  bool group_processed_;
  std::list<std::string> groups_;
};

bool LegacyPDP::isPermitted(Arc::Message *msg) const {
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
  const std::list<std::string>& groups(lattr->GetGroups());
  const std::list<std::string>& vos(lattr->GetVOs());
  if(match_lists(groups_,groups,logger)) return true;
  if(match_lists(vos_,vos,logger)) return true;

  // Populate with collected info
  AuthUser auth(*msg);
  auth.add_groups(lattr->GetGroups());
  auth.add_vos(lattr->GetVOs());
  bool processed = false;
  for(std::list<cfgfile>::const_iterator block = blocks_.begin();
                              block!=blocks_.end();++block) {
    LegacyPDPCP parser(*block,logger,auth);
    if(!parser) return false;
    if(!parser.Parse()) return false;
    if(parser.Matched()) return true;
    if(parser.Processed()) processed = true;
  };

  if(groups_.empty() && vos_.empty() && !processed) return true;
  return false;
}


} // namespace ArcSHCLegacy

