#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/security/ClassLoader.h>
#include <arc/security/ArcPDP/fn/EqualFunction.h>
#include <arc/security/ArcPDP/fn/MatchFunction.h>
#include <arc/security/ArcPDP/fn/InRangeFunction.h>
#include <arc/security/ArcPDP/attr/StringAttribute.h>
#include <arc/security/ArcPDP/attr/DateTimeAttribute.h>
#include <arc/security/ArcPDP/attr/X500NameAttribute.h>
#include <arc/security/ArcPDP/attr/AnyURIAttribute.h>

#include "XACMLFnFactory.h"

using namespace Arc;

namespace ArcSec {

Arc::Plugin* get_xacmlpdp_fn_factory (Arc::PluginArgument*) {
    return new ArcSec::XACMLFnFactory();
}

void XACMLFnFactory::initFunctions(){
  //EqualFunctions
  std::string fnName = EqualFunction::getFunctionName(StringAttribute::getIdentifier());
  std::string argType = StringAttribute::getIdentifier();
  fnmap.insert(std::pair<std::string, Function*>(fnName, new EqualFunction(fnName, argType)));

  fnName = EqualFunction::getFunctionName(DateTimeAttribute::getIdentifier());
  argType = DateTimeAttribute::getIdentifier();
  fnmap.insert(std::pair<std::string, Function*>(fnName, new EqualFunction(fnName, argType)));

  fnName = EqualFunction::getFunctionName(DateAttribute::getIdentifier());
  argType = DateAttribute::getIdentifier();
  fnmap.insert(std::pair<std::string, Function*>(fnName, new EqualFunction(fnName, argType)));

  fnName = EqualFunction::getFunctionName(TimeAttribute::getIdentifier());
  argType = TimeAttribute::getIdentifier();
  fnmap.insert(std::pair<std::string, Function*>(fnName, new EqualFunction(fnName, argType)));

  fnName = EqualFunction::getFunctionName(DurationAttribute::getIdentifier());
  argType = DurationAttribute::getIdentifier();
  fnmap.insert(std::pair<std::string, Function*>(fnName, new EqualFunction(fnName, argType)));

  fnName = EqualFunction::getFunctionName(PeriodAttribute::getIdentifier());
  argType = PeriodAttribute::getIdentifier();
  fnmap.insert(std::pair<std::string, Function*>(fnName, new EqualFunction(fnName, argType)));

  fnName = EqualFunction::getFunctionName(X500NameAttribute::getIdentifier());
  argType = X500NameAttribute::getIdentifier();
  fnmap.insert(std::pair<std::string, Function*>(fnName, new EqualFunction(fnName, argType)));

  fnName = EqualFunction::getFunctionName(AnyURIAttribute::getIdentifier());
  argType = AnyURIAttribute::getIdentifier();
  fnmap.insert(std::pair<std::string, Function*>(fnName, new EqualFunction(fnName, argType)));

  //MatchFunctions
  fnName = MatchFunction::getFunctionName(StringAttribute::getIdentifier());
  argType = StringAttribute::getIdentifier();
  fnmap.insert(std::pair<std::string, Function*>(fnName, new MatchFunction(fnName, argType)));

  fnName = MatchFunction::getFunctionName(X500NameAttribute::getIdentifier());
  argType = X500NameAttribute::getIdentifier();
  fnmap.insert(std::pair<std::string, Function*>(fnName, new MatchFunction(fnName, argType)));

  fnName = MatchFunction::getFunctionName(AnyURIAttribute::getIdentifier());
  argType = AnyURIAttribute::getIdentifier();
  fnmap.insert(std::pair<std::string, Function*>(fnName, new MatchFunction(fnName, argType)));

  //InRangeFunctions
  fnName = InRangeFunction::getFunctionName(StringAttribute::getIdentifier());
  argType = StringAttribute::getIdentifier();
  fnmap.insert(std::pair<std::string, Function*>(fnName, new InRangeFunction(fnName, argType)));

  fnName = InRangeFunction::getFunctionName(PeriodAttribute::getIdentifier());
  argType = PeriodAttribute::getIdentifier();
  fnmap.insert(std::pair<std::string, Function*>(fnName, new InRangeFunction(fnName, argType)));

  /** TODO:  other function type............. */
}

XACMLFnFactory::XACMLFnFactory(){
  initFunctions();
}

Function* XACMLFnFactory::createFn(const std::string& type){
  FnMap::iterator it; 
  if((it=fnmap.find(type)) != fnmap.end()) 
    return (*it).second;
  else { //Default: string-equal
    std::string tp("string-equal");
    if((it=fnmap.find(tp)) != fnmap.end())
      return (*it).second;
  }
  return NULL;
}

XACMLFnFactory::~XACMLFnFactory(){
  FnMap::iterator it;
  for(it = fnmap.begin(); it != fnmap.end(); it = fnmap.begin()){
    Function* fn = (*it).second;
    fnmap.erase(it);
    if(fn) delete fn;
  }
}

} // namespace ArcSec

