#include "ArcAlgFactory.h"
#include "DenyOverridesAlg.h"
#include "PermitOverridesAlg.h"

using namespace Arc;

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
    delete (*it).second;
    algmap.erase(it);
  }
}
