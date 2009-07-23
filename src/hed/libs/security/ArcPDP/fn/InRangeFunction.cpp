#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "InRangeFunction.h"
#include "../attr/BooleanAttribute.h"
#include "../attr/DateTimeAttribute.h"
#include "../attr/StringAttribute.h"

namespace ArcSec {

std::string InRangeFunction::getFunctionName(std::string datatype){
  std::string ret;
  if (datatype ==  DateTimeAttribute::getIdentifier()) ret = NAME_TIME_IN_RANGE;
  else if (datatype ==  StringAttribute::getIdentifier()) ret = NAME_STRING_IN_RANGE;
  return ret;
}

InRangeFunction::InRangeFunction(std::string functionName, std::string argumentType) : Function(functionName, argumentType) {
  fnName = functionName;
  argType = argumentType;
}

AttributeValue* InRangeFunction::evaluate(AttributeValue* arg0, AttributeValue* arg1, bool check_id){
  //TODO
  //arg0 is the attributevalue in policy
  //arg1 is the attributevalue in request
  if(check_id) { if(arg0->getId() != arg1->getId()) return new BooleanAttribute(false); }
  if(fnName == NAME_TIME_IN_RANGE){
    PeriodAttribute* v0;
    DateTimeAttribute* v1;
    try{
      v0 = dynamic_cast<PeriodAttribute*>(arg0);
      v1 = dynamic_cast<DateTimeAttribute*>(arg1);
    } catch(std::exception&){ };
    if(v1->inrange(v0))
      return new BooleanAttribute(true);
  }
  else if(fnName == NAME_STRING_IN_RANGE) {
    StringAttribute* v0;
    StringAttribute* v1;
    try{
      v0 = dynamic_cast<StringAttribute*>(arg0);
      v1 = dynamic_cast<StringAttribute*>(arg1);
    } catch(std::exception&){ };
    if(v1->inrange(v0))
      return new BooleanAttribute(true);
  }
  return new BooleanAttribute(false);
}

std::list<AttributeValue*> InRangeFunction::evaluate(std::list<AttributeValue*> args, bool check_id) {
  AttributeValue* arg0 = NULL;
  AttributeValue* arg1 = NULL;
  std::list<AttributeValue*>::iterator it = args.begin();
  arg0 = *it; it++;
  if(it!= args.end()) arg1 = *it;
  if(check_id) { 
    if(arg0->getId() != arg1->getId()) {
      std::list<AttributeValue*> ret;
      ret.push_back(new BooleanAttribute(false));
      return ret;
    }
  }
  AttributeValue* res = evaluate(arg0, arg1);
  std::list<AttributeValue*> ret;
  ret.push_back(res);
  return ret;
}

}
