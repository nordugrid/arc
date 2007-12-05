#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "MatchFunction.h"
#include "../attr/StringAttribute.h"
#include "../attr/DateTimeAttribute.h"
#include "../attr/X500NameAttribute.h"
#include "../attr/AnyURIAttribute.h"

namespace ArcSec {

std::string MatchFunction::getFunctionName(std::string datatype){
  std::string ret;
  if (datatype ==  StringAttribute::getIdentifier()) ret = NAME_REGEXP_STRING_MATCH;
  else if(datatype == AnyURIAttribute::getIdentifier()) ret = NAME_ANYURI_REGEXP_MATCH;
  else if(datatype == X500NameAttribute::getIdentifier()) ret = NAME_X500NAME_REGEXP_MATCH;
  return ret;
}

MatchFunction::MatchFunction(std::string functionName, std::string argumentType) : Function(functionName, argumentType) {
  fnName = functionName;
  argType = argumentType;

}

bool MatchFunction::evaluate(AttributeValue* arg0, AttributeValue* arg1){
  //TODO
  //arg0 is the attributevalue in policy
  //arg1 is the attributevalue in request
  std::string label = arg0->encode();
  std::string value = arg1->encode();
  Arc::RegularExpression regex(label);
  if(regex.isOk()){
    std::list<std::string> unmatched, matched;
    if(regex.match(value, unmatched, matched))
      return true;
  }
  else 
    std::cerr<<"Bad Regex label"<<std::endl;
  return false;
}

}
