#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sys/types.h>

#include <arc/XMLNode.h>
#include <arc/message/MCCLoader.h>

#include "ArcAuthZ.h"

Arc::Plugin* ArcSec::ArcAuthZ::get_sechandler(Arc::PluginArgument* arg) {
    ArcSec::SecHandlerPluginArgument* shcarg =
            arg?dynamic_cast<ArcSec::SecHandlerPluginArgument*>(arg):NULL;
    if(!shcarg) return NULL;
    return new ArcSec::ArcAuthZ((Arc::Config*)shcarg,(Arc::ChainContext*)(*shcarg));
}

/*
sechandler_descriptors ARC_SECHANDLER_LOADER = {
    { "arc.authz", 0, &get_sechandler},
    { NULL, 0, NULL }
};
*/

using namespace Arc;
namespace ArcSec {

ArcAuthZ::PDPDesc::PDPDesc(const std::string& action_,PDP* pdp_):pdp(pdp_),action(breakOnDeny) {
  if(strcasecmp("breakOnAllow",action_.c_str()) == 0) { action=breakOnAllow; }
  else if(strcasecmp("breakOnDeny",action_.c_str()) == 0) { action=breakOnDeny; }
  else if(strcasecmp("breakAlways",action_.c_str()) == 0) { action=breakAlways; }
  else if(strcasecmp("breakNever",action_.c_str()) == 0) { action=breakNever; };
}

ArcAuthZ::ArcAuthZ(Config *cfg,ChainContext* ctx):SecHandler(cfg){
  pdp_factory = (PluginsFactory*)(*ctx);
  if(pdp_factory) {
    for(int n = 0;;++n) {
      XMLNode p = (*cfg)["Plugins"][n];
      if(!p) break;
      std::string name = (*cfg)["Plugins"][n]["Name"];
      if(name.empty()) continue; // Nameless plugin?
      pdp_factory->load(name,PDPPluginKind);
    };
  };
  if(!MakePDPs(cfg)) {
    for(pdp_container_t::iterator p = pdps_.begin();p!=pdps_.end();++p) {
      if(p->second.pdp) delete p->second.pdp;
    };
    logger.msg(ERROR, "ArcAuthZ: failed to initiate all PDPs - this instance will be non-functional"); 
  };
}

ArcAuthZ::~ArcAuthZ() {
  for(pdp_container_t::iterator p = pdps_.begin();p!=pdps_.end();++p) {
    if(p->second.pdp) delete p->second.pdp;
  };
}

/**Producing PDPs */
bool ArcAuthZ::MakePDPs(Config* cfg) {
  /**Creating the PDP plugins*/
    XMLNode cn;
    cn=(*cfg)["PDP"]; //need some polishing

    for(int n = 0;;++n) {
        XMLNode can = cn[n];
        if(!can) break;
        Arc::Config cfg_(can);
        std::string name = can.Attribute("name");
        logger.msg(DEBUG, "PDP: %s (%d)", name, n);
        PDP* pdp = NULL;
        PDPPluginArgument arg(&cfg_);
        pdp = pdp_factory->GetInstance<PDP>(PDPPluginKind,name,&arg);
        if(!pdp) { 
            logger.msg(ERROR, "PDP: %s can not be loaded", name); 
            return false; 
        };
        if(pdps_.insert(std::make_pair(name,PDPDesc(can.Attribute("action"),pdp))).second == false) {
            logger.msg(ERROR, "PDP: %s name is duplicate", name); 
            return false; 
        };
    } 
    return true;
}

bool ArcAuthZ::Handle(Arc::Message* msg){
  pdp_container_t::iterator it;
  bool r = false;
  for(it=pdps_.begin();it!=pdps_.end();it++){
    r = it->second.pdp->isPermitted(msg);
    if((r == true) && (it->second.action == PDPDesc::breakOnAllow)) break;
    if((r == false) && (it->second.action == PDPDesc::breakOnDeny)) break;
    if(it->second.action == PDPDesc::breakAlways) break;
  }
  return r;
}

} // namespace ArcSec

