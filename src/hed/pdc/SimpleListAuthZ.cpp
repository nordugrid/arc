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
      if(p->second) delete p->second;
    };
    logger.msg(ERROR, "SimpleListAuthZ: failed to initiate all PDPs - this instance will be non-functional"); 
  };
}

SimpleListAuthZ::~SimpleListAuthZ() {
  for(pdp_container_t::iterator p = pdps_.begin();p!=pdps_.end();++p) {
    if(p->second) delete p->second;
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
        logger.msg(DEBUG, "PDP: %s (%d)", name.c_str(), n);
        PDP* pdp = NULL;
        pdp = pdp_factory->get_instance(name,&cfg_,NULL);
        if(!pdp) { 
            logger.msg(ERROR, "PDP: %s can not be loaded", name.c_str()); 
            return false; 
        };
        if(pdps_.insert(std::make_pair(name,pdp)).second == false) {
            logger.msg(ERROR, "PDP: %s name is duplicate", name.c_str()); 
            return false; 
        };
    } 
    return true;
}

bool SimpleListAuthZ::Handle(Arc::Message* msg){
  //std::string subject=msg->Attributes()->get("TLS:PEERDN");
  pdp_container_t::iterator it;
  for(it=pdps_.begin();it!=pdps_.end();it++){
     if(it->second->isPermitted(msg))
        return true;  
  }
  return false;
}
