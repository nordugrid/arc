#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/SecHandlerLoader.h>
#include <arc/loader/Loader.h>
#include <arc/Logger.h>

#include "IdentityMap.h"

static ArcSec::SecHandler* get_sechandler(Arc::Config *cfg, Arc::ChainContext* ctx) {
    return new ArcSec::IdentityMap(cfg,ctx);
}

sechandler_descriptors ARC_SECHANDLER_LOADER = {
    { "identity.map", 0, &get_sechandler},
    { NULL, 0, NULL }
};

namespace ArcSec {

IdentityMap::IdentityMap(Arc::Config *cfg,Arc::ChainContext* ctx):ArcSec::SecHandler(cfg){
  Arc::PDPFactory* pdp_factory = (Arc::PDPFactory*)(*ctx);
  if(pdp_factory) {
    Arc::XMLNode plugins = (*cfg)["Plugins"];
    for(int n = 0;;++n) {
      Arc::XMLNode p = plugins[n];
      if(!p) break;
      std::string name = p["Name"];
      if(name.empty()) continue; // Nameless plugin?
      pdp_factory->load_all_instances(name);
    };
    Arc::XMLNode pdps = (*cfg)["PDP"];
    for(int n = 0;;++n) {
      Arc::XMLNode p = pdps[n];
      if(!p) break;
      std::string name = p.Attribute("name");
      if(name.empty()) continue; // Nameless?
      std::string local_id = p["LocalId"];
      if(local_id.empty()) continue; // No mapping?
      Arc::Config cfg_(p);
      ArcSec::PDP* pdp = pdp_factory->get_instance(name,&cfg_,NULL);
      if(!pdp) {
        logger.msg(Arc::ERROR, "PDP: %s can not be loaded", name.c_str());
        continue;
      };
      map_pair_t m;
      m.pdp=pdp;
      m.uid=new std::string(local_id);
      maps_.push_back(m);
    };
  };
}

IdentityMap::~IdentityMap(void) {
  for(std::list<map_pair_t>::iterator p = maps_.begin();p!=maps_.end();++p) {
    if(p->pdp) delete p->pdp;
    if(p->uid) delete p->uid;
  };
}

bool IdentityMap::Handle(Arc::Message* msg){
  for(std::list<map_pair_t>::iterator p = maps_.begin();p!=maps_.end();++p) {
    if(p->pdp->isPermitted(msg)) {
      msg->Attributes()->set("SEC:LOCALID",*(p->uid));
      return true;  
    };
  }
  return true;
}

}

