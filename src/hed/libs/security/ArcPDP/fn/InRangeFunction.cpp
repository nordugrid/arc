#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "InRangeFunction.h"
#include "../attr/DateTimeAttribute.h"

namespace ArcSec {

std::string InRangeFunction::getFunctionName(std::string datatype){
  std::string ret;
  if (datatype ==  DateTimeAttribute::getIdentifier()) ret = NAME_TIME_IN_RANGE;
  return ret;
}

InRangeFunction::InRangeFunction(std::string functionName, std::string argumentType) : Function(functionName, argumentType) {
  fnName = functionName;
  argType = argumentType;

}

bool InRangeFunction::evaluate(AttributeValue* arg0, AttributeValue* arg1){
  //TODO
  //arg0 is the attributevalue in policy
  //arg1 is the attributevalue in request
  if(fnName == NAME_TIME_IN_RANGE){
    PeriodAttribute* v0;
    DateTimeAttribute* v1;
    try{
      v0 = dynamic_cast<PeriodAttribute*>(arg0);
      v1 = dynamic_cast<DateTimeAttribute*>(arg1);
    } catch(std::exception&){ };
    if(v1->inrange(v0))
      return true;
  }
  return false;
}

}
