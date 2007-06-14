#include <iostream>
#include <sys/types.h>

#include "../../libs/loader/HandlerLoader.h"
#include "../../../libs/common/XMLNode.h"
#include "../../../libs/common/Logger.h"

#include "SimpleListAuthZ.h"

//Arc::Logger &l = Arc::Logger::rootLogger;

static Arc::Handler* get_handler(Arc::Config *cfg, Arc::ChainContext* ctx) {
    return new Arc::SimpleListAuthZ(cfg);
}

handler_descriptor __arc_handler_modules__[] = {
    { "simplelist.authz", 0, &get_handler},
    { NULL, 0, NULL }
};

using namespace Arc;
SimpleListAuthZ::SimpleListAuthZ(Arc::Config *cfg):Handler(cfg){
  
std::cerr<<"SimpleListAuthZ: top = "<<cfg->Name()<<std::endl;
  pdp_factory = new Arc::PDPFactory(cfg);
  for(int n = 0;;++n) {
    std::string name = (*cfg)["Plugins"][n]["Name"];
std::cerr<<"SimpleListAuthZ: Name = "<<name<<std::endl;
    if(name.empty()) break;
    pdp_factory->load_all_instances(name);
  };
}

/**Producing PDPs */
void* SimpleListAuthZ::MakePDP(Arc::Config* cfg) {
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
               std::cerr << "PDP has no name attribute defined" << std::endl;
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
       std::cerr <<"PDP name:\n"<<name<<std::endl;
       PDP* pdp = pdp_factory->get_instance(name,&cfg_,NULL);
       if(!pdp) continue;
       pdps_[name]=pdp;
    } 
}

bool SimpleListAuthZ::SecHandle(Message* msg){
  std::string subject=msg->Attributes()->get("TLS:PEERDN");
  std::cerr <<"Get the DN:\n"<<subject<<std::endl;
  pdp_container_t::iterator it;
  for(it=pdps_.begin();it!=pdps_.end();it++){
     if(it->second->isPermitted(subject))
	return true;  
  }
  if(it==pdps_.end())
    return false;
}
