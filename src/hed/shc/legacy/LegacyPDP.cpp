#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//#include <iostream>

//#include <arc/StringConv.h>
//#include <arc/Utils.h>

#include "LegacySecAttr.h"

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
}

LegacyPDP::~LegacyPDP() {
}

static bool match_lists(const std::list<std::string>& list1, const std::list<std::string>& list2) {
  for(std::list<std::string>::const_iterator l1 = list1.begin(); l1 != list1.end(); ++l1) {
    for(std::list<std::string>::const_iterator l2 = list2.begin(); l2 != list2.end(); ++l2) {
      if((*l1) == (*l2)) return true;
    };
  };
  return false;
}

bool LegacyPDP::isPermitted(Arc::Message *msg) const {
  Arc::SecAttr* sattr = msg->Auth()->get("ARCLEGACY");
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
  if(match_lists(groups_,groups)) return true;
  if(match_lists(vos_,vos)) return true;
  return false;
}


} // namespace ArcSHCLegacy

