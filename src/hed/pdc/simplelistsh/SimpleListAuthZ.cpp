#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sys/types.h>

#include <arc/loader/SecHandlerLoader.h>
#include <arc/loader/Loader.h>
#include <arc/XMLNode.h>

#include "SimpleListAuthZ.h"

static ArcSec::SecHandler* get_sechandler(Arc::Config *cfg, Arc::ChainContext* ctx) {
    return new ArcSec::SimpleListAuthZ(cfg,ctx);
}

sechandler_descriptors ARC_SECHANDLER_LOADER = {
    { "simplelist.authz", 0, &get_sechandler},
    { NULL, 0, NULL }
};

using namespace Arc;
using namespace ArcSec;

SimpleListAuthZ::PDPDesc::PDPDesc(const std::string& action_,PDP* pdp_):pdp(pdp_),action(breakOnAllow) {
  if(strcasecmp("breakOnAllow",action_.c_str()) == 0) { action=breakOnAllow; }
  else if(strcasecmp("breakOnDeny",action_.c_str()) == 0) { action=breakOnDeny; }
  else if(strcasecmp("breakAlways",action_.c_str()) == 0) { action=breakAlways; }
  else if(strcasecmp("breakNever",action_.c_str()) == 0) { action=breakNever; };
}

SimpleListAuthZ::SimpleListAuthZ(Config *cfg,ChainContext* ctx):SecHandler(cfg){
  pdp_factory = (PDPFactory*)(*ctx);
  if(pdp_factory) {
    for(int n = 0;;++n) {
      XMLNode p = (*cfg)["Plugins"][n];
      if(!p) break;
      std::string name = (*cfg)["Plugins"][n]["Name"];
      if(name.empty()) continue; // Nameless plugin?
      pdp_factory->load_all_instances(name);
    };
  };
  if(!MakePDPs(cfg)) {
    for(pdp_container_t::iterator p = pdps_.begin();p!=pdps_.end();++p) {
      if(p->second.pdp) delete p->second.pdp;
    };
    logger.msg(ERROR, "SimpleListAuthZ: failed to initiate all PDPs - this instance will be non-functional"); 
  };
}

SimpleListAuthZ::~SimpleListAuthZ() {
  for(pdp_container_t::iterator p = pdps_.begin();p!=pdps_.end();++p) {
    if(p->second.pdp) delete p->second.pdp;
  };
}

/**Producing PDPs */
bool SimpleListAuthZ::MakePDPs(Config* cfg) {
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
        pdp = pdp_factory->get_instance(name,&cfg_,NULL);
        if(!pdp) { 
            logger.msg(ERROR, "PDP: %s can not be loaded", name); 
            return false; 
        };
        std::string pdp_id = can.Attribute("id");
        if(pdp_id.empty()) { logger.msg(ERROR, "PDP should have ID"); return false; }
        pdp->SetId(pdp_id);
        if(pdps_.insert(std::make_pair(name,PDPDesc(can.Attribute("action"),pdp))).second == false) {
            logger.msg(ERROR, "PDP: %s name is duplicate", name); 
            return false; 
        };
    } 
    return true;
}

bool SimpleListAuthZ::Handle(Arc::Message* msg){
  pdp_container_t::iterator it;
  bool r = false;
  for(it=pdps_.begin();it!=pdps_.end();it++){
    r = it->second.pdp->isPermitted(msg);
    if((r == true) && (it->second.action == PDPDesc::breakOnAllow)) return true;
    if((r == false) && (it->second.action == PDPDesc::breakOnDeny)) return false;
    if(it->second.action == PDPDesc::breakAlways) return r;
  }
  return r;
}

