#include "ArcFnFactory.h"

void ArcFnFactory::initFunctions(){
  /**Some Arc specified function types*/
  fnmap.insert(pair<std::string, Function*>(StringFunction.identify, new StringFunction));
  fnmap.insert(pair<std::string, Function*>(DateMathFunction.identify, new DateMathFunction));
  /** TODO:  other function type............. */

}

ArcFnFactory::ArcFnFactory(){
  initFunctions();
}

Function* ArcFnFactory::createFunction(const std::string& type){
  FnMap::iterator it; 
  if((it=fnmap.find(type)) != fnmap.end())
    return (*it).second;
  else return NULL;
}

ArcFnFactory::~ArcFnFactory(){
  FnMap::iterator it;
  for(it = fnmap.begin(); it != fnmap.end(); it++){
    delete (*it).second;
    fnmap.erase(it);
  }
}
