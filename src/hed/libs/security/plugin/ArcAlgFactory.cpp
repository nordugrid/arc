#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/ClassLoader.h>

#include <arc/ArcConfig.h>
#include "ArcAlgFactory.h"
#include <arc/security/ArcPDP/alg/DenyOverridesAlg.h>
#include <arc/security/ArcPDP/alg/PermitOverridesAlg.h>

static Arc::LoadableClass* get_alg_factory (void**) {
    return new ArcSec::ArcAlgFactory();
}

loader_descriptors __arc_algfactory_modules__  = {
    { "alg.factory", 0, &get_alg_factory },
    { NULL, 0, NULL }
};


using namespace Arc;
using namespace ArcSec;

void ArcAlgFactory::initCombiningAlgs(){
  /**Some Arc specified algorithm types*/
  algmap.insert(std::pair<std::string, CombiningAlg*>("Deny-Overrides", new DenyOverridesCombiningAlg()));
  algmap.insert(std::pair<std::string, CombiningAlg*>("Permit-Overrides", new PermitOverridesCombiningAlg()));
  /** TODO:  other algorithm type............. */

}

ArcAlgFactory::ArcAlgFactory(){
  initCombiningAlgs();
}

CombiningAlg* ArcAlgFactory::createAlg(const std::string& type){
  AlgMap::iterator it; 
  if((it=algmap.find(type)) != algmap.end()){
    return (*it).second;
  }
  else return NULL;
}

ArcAlgFactory::~ArcAlgFactory(){
  AlgMap::iterator it;
  for(it = algmap.begin(); it != algmap.end(); it++){
    CombiningAlg * alg = (*it).second;
    algmap.erase(it);
    if(alg) delete alg;;
  }
}
