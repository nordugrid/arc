#include "ArcAlgFactory.h"

void ArcAlgFactory::initCombiningAlgs(){
  /**Some Arc specified algorithm types*/
  algmap.insert(pair<std::string, CombiningAlg*>(DenyOveride.identify, new DenyOverridesPolicyAlg));
  algmap.insert(pair<std::string, CombiningAlg*>(PermitOveride.identify, new PermitOverridesPolicyAlg));
  /** TODO:  other algorithm type............. */

}

ArcAlgFactory::ArcAlgFactory(){
  initCombiningAlgs();
}

CombiningAlg* ArcAlgFactory::createCombiningAlg(const std::string& type){
  AlgMap::iterator it; 
  if((it=algmap.find(type)) != algmap.end())
    return (*it).second;
  else return NULL;
}

ArcAlgFactory::~ArcAlgFactory(){
  AlgMap::iterator it;
  for(it = algmap.begin(); it != algmap.end(); it++){
    delete (*it).second;
    algmap.erase(it);
  }
}
