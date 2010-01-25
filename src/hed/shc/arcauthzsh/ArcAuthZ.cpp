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
  return new ArcSec::ArcAuthZ((Arc::Config*)(*shcarg),(Arc::ChainContext*)(*shcarg));
}

using namespace Arc;
namespace ArcSec {

ArcAuthZ::PDPDesc::PDPDesc(const std::string& action_,const std::string& id_,PDP* pdp_):pdp(pdp_),action(breakOnDeny),id(id_) {
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
  if(!MakePDPs(*cfg)) {
    for(pdp_container_t::iterator p = pdps_.begin();p!=pdps_.end();) {
      if(p->pdp) delete p->pdp;
      p = pdps_.erase(p);
    };
    logger.msg(ERROR, "ArcAuthZ: failed to initiate all PDPs - this instance will be non-functional"); 
  };
}

ArcAuthZ::~ArcAuthZ() {
  for(pdp_container_t::iterator p = pdps_.begin();p!=pdps_.end();) {
    if(p->pdp) delete p->pdp;
    p = pdps_.erase(p);
  };
}

/**Producing PDPs */
bool ArcAuthZ::MakePDPs(XMLNode cfg) {
  /**Creating the PDP plugins*/
  XMLNode cn;
  cn=cfg["PDP"]; //need some polishing

  for(;cn;++cn) {
    if(!cn) break;
    Arc::Config cfg_(cn);
    std::string name = cn.Attribute("name");
    if(name.empty()) {
      logger.msg(ERROR, "PDP: missing name attribute"); 
      return false; 
    };
    std::string id = cn.Attribute("id");
    logger.msg(VERBOSE, "PDP: %s (%s)", name, id);
    PDP* pdp = NULL;
    PDPPluginArgument arg(&cfg_);
    pdp = pdp_factory->GetInstance<PDP>(PDPPluginKind,name,&arg);
    if(!pdp) { 
      logger.msg(ERROR, "PDP: %s (%s) can not be loaded", name, id); 
      return false; 
    };
    pdps_.push_back(PDPDesc(cn.Attribute("action"),id,pdp));
  } 
  return true;
}

bool ArcAuthZ::Handle(Arc::Message* msg) const {
  pdp_container_t::const_iterator it;
  bool r = false;
  for(it=pdps_.begin();it!=pdps_.end();it++){
    r = it->pdp->isPermitted(msg);
    if((r == true) && (it->action == PDPDesc::breakOnAllow)) break;
    if((r == false) && (it->action == PDPDesc::breakOnDeny)) break;
    if(it->action == PDPDesc::breakAlways) break;
  }
  return r;
}

} // namespace ArcSec

