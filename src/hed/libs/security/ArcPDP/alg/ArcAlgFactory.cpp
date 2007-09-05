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
  std::cout<<"Produce Alg"<<std::endl;
  AlgMap::iterator it; 
  if((it=algmap.find(type)) != algmap.end()){
    std::cout<<"Alg is:"<<std::endl;
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
