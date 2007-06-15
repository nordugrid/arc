#include <sys/types.h>

#include "../../libs/loader/SecHandlerLoader.h"
#include "../../../libs/common/XMLNode.h"

#include "SimpleListAuthZ.h"

static Arc::SecHandler* get_sechandler(Arc::Config *cfg, Arc::ChainContext* ctx) {
    return new Arc::SimpleListAuthZ(cfg,ctx);
}

sechandler_descriptor __arc_sechandler_modules__[] = {
    { "simplelist.authz", 0, &get_sechandler},
    { NULL, 0, NULL }
};

using namespace Arc;

SimpleListAuthZ::SimpleListAuthZ(Arc::Config *cfg,ChainContext* ctx):SecHandler(cfg){
  
  pdp_factory = (PDPFactory*)ctx;
  if(pdp_factory) {
    for(int n = 0;;++n) {
      std::string name = (*cfg)["Plugins"][n]["Name"];
      if(name.empty()) break;
      pdp_factory->load_all_instances(name);
    };
  };
  MakePDPs(cfg);
}

SimpleListAuthZ::~SimpleListAuthZ() {
  for(pdp_container_t::iterator p = pdps_.begin();p!=pdps_.end();++p) {
    if(p->second) delete p->second;
  };
}

/**Producing PDPs */
void* SimpleListAuthZ::MakePDPs(Arc::Config* cfg) {
    /**Creating the PDP plugins*/
 /*   XMLNode cn, an, can;
    for(int i=0;;++i){
       cn = cfg->Child(i);
       if(MatchXMLName(cn, "PDP")){
         for(int j=0;;++j){
	    can=cn[j];
	    if(!can) break;
	    std::string name = can.Attribute("name");
	    if(name.empty()) {
	       logger.msg(ERROR, "PDP has no name attribute defined");
               continue;
	    }
            PDP* pdp = pdp_factory->get_instance(name,&cfg_,ctx);
    	    pdps_[name]=pdp;
         }
	break;
        }
    }*/
    XMLNode cn;
    cn=(*cfg)["PDP"]; //need some polishing
    for(int n = 0;;++n) {
       XMLNode can = cn[n];
       if(!can) break;
       Arc::Config cfg_(cn);
       std::string name = cn.Attribute("name");
       logger.msg(LogMessage(ERROR, "PDP name:"+name));
       PDP* pdp = pdp_factory->get_instance(name,&cfg_,NULL);
       if(!pdp) continue;
       pdps_[name]=pdp;
    } 
}

bool SimpleListAuthZ::Handle(Message* msg){
  std::string subject=msg->Attributes()->get("TLS:PEERDN");
  pdp_container_t::iterator it;
  for(it=pdps_.begin();it!=pdps_.end();it++){
     if(it->second->isPermitted(subject))
	return true;  
  }
  if(it==pdps_.end())
    return false;
}
