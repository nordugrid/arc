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
    return new LegacyPDP((Arc::Config*)(*pdparg),arg);
}

static bool match_lists(const std::list<std::string>& list1, const std::list<std::string>& list2, std::string& matched, Arc::Logger& logger) {
  for(std::list<std::string>::const_iterator l1 = list1.begin(); l1 != list1.end(); ++l1) {
    for(std::list<std::string>::const_iterator l2 = list2.begin(); l2 != list2.end(); ++l2) {
      if((*l1) == (*l2)) {
        matched = *l1;
        return true;
      };
    };
  };
  return false;
}

static bool match_lists(const std::list<std::pair<bool,std::string> >& list1, const std::list<std::string>& list2, std::string& matched, Arc::Logger& logger) {
  for(std::list<std::pair<bool,std::string> >::const_iterator l1 = list1.begin(); l1 != list1.end(); ++l1) {
    for(std::list<std::string>::const_iterator l2 = list2.begin(); l2 != list2.end(); ++l2) {
      if((l1->second) == (*l2)) {
        matched = l1->second;
        return l1->first;
      };
    };
  };
  return false;
}

class LegacyPDPCP: public ConfigParser {
 public:
  LegacyPDPCP(LegacyPDP::cfgfile& file, Arc::Logger& logger):ConfigParser(file.filename,logger),file_(file) {
  };

  virtual ~LegacyPDPCP(void) {
  };

 protected:
  virtual bool BlockStart(const std::string& id, const std::string& name) {
    std::string bname = id;
    if(!name.empty()) bname = bname+":"+name;
    for(std::list<LegacyPDP::cfgblock>::iterator block = file_.blocks.begin();
                                 block != file_.blocks.end();++block) {
      if(block->name == bname) {
        block->exists = true;
      };
    };
    return true;
  };

  virtual bool BlockEnd(const std::string& id, const std::string& name) {
    return true;
  };

  virtual bool ConfigLine(const std::string& id, const std::string& name, const std::string& cmd, const std::string& line) {
    //if(group_matched_) return true;
    if((cmd != "allowaccess") && (cmd != "denyaccess")) return true;

    std::string bname = id;
    if(!name.empty()) bname = bname+":"+name;
    for(std::list<LegacyPDP::cfgblock>::iterator block = file_.blocks.begin();
                                   block != file_.blocks.end();++block) {
      if(block->name == bname) {
        block->limited = true;
        std::list<std::string> groups;
        Arc::tokenize(line,groups," ");
        for(std::list<std::string>::const_iterator group = groups.begin(); group != groups.end(); ++group) {
          block->groups.push_back(std::pair<bool,std::string>(cmd == "allowaccess",*group));
        };
      };
    };
    return true;
  };

 private:
  LegacyPDP::cfgfile& file_;
};

LegacyPDP::LegacyPDP(Arc::Config* cfg,Arc::PluginArgument* parg):PDP(cfg,parg),attrname_("ARCLEGACYPDP"),srcname_("ARCLEGACY") {
  any_ = false;
  Arc::XMLNode group = (*cfg)["Group"];
  while((bool)group) {
    groups_.push_back(std::pair<bool,std::string>(true,(std::string)group));
    ++group;
  };
  Arc::XMLNode vo = (*cfg)["VO"];
  while((bool)vo) {
    vos_.push_back((std::string)vo);
    ++vo;
  };
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
      //blocks_.clear();
      return;
    };
    cfgfile file(filename);
    Arc::XMLNode name = block["BlockName"];
    while((bool)name) {
      std::string blockname = (std::string)name;
      if(blockname.empty()) {
        logger.msg(Arc::ERROR, "BlockName is empty");
        //blocks_.clear();
        return;
      };
      //file.blocknames.push_back(blockname);
      file.blocks.push_back(blockname);
      ++name;
    };
    LegacyPDPCP parser(file,logger);
    if((!parser) || (!parser.Parse())) {
      logger.msg(Arc::ERROR, "Failed to parse configuration file %s",filename);
      return;
    };
    for(std::list<cfgblock>::const_iterator b = file.blocks.begin();
                         b != file.blocks.end(); ++b) {
      if(!(b->exists)) {
        logger.msg(Arc::ERROR, "Block %s not found in configuration file %s",b->name,filename);
        return;
      };
      if(!(b->limited)) {
        any_ = true;
      } else {
        groups_.insert(groups_.end(),b->groups.begin(),b->groups.end());
      };
    };
    //blocks_.push_back(file);
    ++block;
  };
}

LegacyPDP::~LegacyPDP() {
}

class LegacyPDPAttr: public Arc::SecAttr {
 public:
  LegacyPDPAttr(bool decision):decision_(decision) { };
  LegacyPDPAttr(bool decision, const std::list<std::string>& mvoms,
                               const std::list<std::string>& mvo,
                               const std::list<std::string>& mscitokens):
        decision_(decision), voms(mvoms), vo(mvo), scitokens(mscitokens) { };
  virtual ~LegacyPDPAttr(void);

  // Common interface
  virtual operator bool(void) const;
  virtual bool Export(Arc::SecAttrFormat format,Arc::XMLNode &val) const;
  virtual std::string get(const std::string& id) const;
  virtual std::list<std::string> getAll(const std::string& id) const;

  // Specific interface
  bool GetDecision(void) const { return decision_; };

 protected:
  bool decision_;
  virtual bool equal(const SecAttr &b) const;
  std::list<std::string> voms;
  std::list<std::string> vo;
  std::list<std::string> scitokens; // 
};

LegacyPDPAttr::~LegacyPDPAttr(void) {
}

LegacyPDPAttr::operator bool(void) const {
  return true;
}

bool LegacyPDPAttr::Export(Arc::SecAttrFormat format,Arc::XMLNode &val) const {
  return true;
}

std::string LegacyPDPAttr::get(const std::string& id) const {
  if(id == "VOMS") {
    if(!voms.empty()) return *voms.begin();
  } else if(id == "VO") {
    if(!vo.empty()) return *vo.begin();
  } else if(id == "SCITOKENS") {
    if(!scitokens.empty()) return *scitokens.begin();
  }
  return "";
}

std::list<std::string> LegacyPDPAttr::getAll(const std::string& id) const {
  if(id == "VOMS") return voms;
  if(id == "VO") return vo;
  if(id == "SCITOKENS") return scitokens;
  return std::list<std::string>();
}

bool LegacyPDPAttr::equal(const SecAttr &b) const {
  const LegacyPDPAttr& a = dynamic_cast<const LegacyPDPAttr&>(b);
  if (!a) return false;
  return (decision_ == a.decision_);
}

ArcSec::PDPStatus LegacyPDP::isPermitted(Arc::Message *msg) const {
  if(any_) return true; // No need to perform anything if everyone is allowed
  Arc::SecAttr* sattr = msg->Auth()->get(srcname_);
  if(!sattr) {
    // Only if information collection is done per context.
    // Check if decision is already made.
    Arc::SecAttr* dattr = msg->AuthContext()->get(attrname_);
    if(dattr) {
      LegacyPDPAttr* pattr = dynamic_cast<LegacyPDPAttr*>(dattr);
      if(pattr) {
        // Decision is already made in this context
        return pattr->GetDecision();
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
  const std::list<std::string>& groups(lattr->GetGroups());
  const std::list<std::string>& vos(lattr->GetVOs());
  bool decision = false;
  std::string match;
  if(match_lists(groups_,groups,match,logger)) {
    decision = true;
    const std::list<std::string>& matched_voms = lattr->GetGroupVOMS(match);
    const std::list<std::string>& matched_vo = lattr->GetGroupVO(match);
    const std::list<std::string>& matched_scitokens = lattr->GetGroupScitokens(match);
    msg->AuthContext()->set(attrname_,new LegacyPDPAttr(decision, matched_voms, matched_vo, matched_scitokens));
  } else if(match_lists(vos_,vos,match,logger)) {
    decision = true;
    const std::list<std::string> matched_voms;
    const std::list<std::string> matched_scitokens;
    std::list<std::string> matched_vo;
    matched_vo.push_back(match);
    msg->AuthContext()->set(attrname_,new LegacyPDPAttr(decision, matched_voms, matched_vo, matched_scitokens));
  } else {
    msg->AuthContext()->set(attrname_,new LegacyPDPAttr(decision));
  };
  return decision;
}


} // namespace ArcSHCLegacy

