#include "ArcFnFactory.h"
#include "EqualFunction.h"
#include "../attr/StringAttribute.h"

using namespace Arc;

void ArcFnFactory::initFunctions(){
  /**Some Arc specified function types*/
  //fnmap.insert(pair<std::string, Function*>(StringFunction.identify, new StringFunction));
  //fnmap.insert(pair<std::string, Function*>(DateMathFunction.identify, new DateMathFunction));
  /** TODO:  other function type............. */
  std::string fnName = EqualFunction::getFunctionName(StringAttribute::getIdentifier());
  std::string argType = StringAttribute::getIdentifier();
  fnmap.insert(std::pair<std::string, Function*>(fnName, new EqualFunction(fnName, argType)));

  //! fnName = EqualFunction::getFunctionName(DateAttribute::identify());
  //! argType = DateAttribute::identity();
  //! fnmap.insert(std::pair<std::string, Function*>(fnName, new EqualFunction(fnName, argType)));
}

ArcFnFactory::ArcFnFactory(){
  initFunctions();
}

Function* ArcFnFactory::createFn(const std::string& type){
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
