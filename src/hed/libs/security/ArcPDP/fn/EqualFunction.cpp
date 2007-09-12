#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "EqualFunction.h"
#include "../attr/StringAttribute.h"
#include "../attr/DateTimeAttribute.h"
#include "../attr/X500NameAttribute.h"
#include "../attr/AnyURIAttribute.h"

namespace Arc {

std::string EqualFunction::getFunctionName(std::string datatype){
  std::string ret;
  if (datatype ==  StringAttribute::getIdentifier()) ret = NAME_STRING_EQUAL;
 /* else if(datatype == BooleanAttribute::identify()) ret = NAME_BOOLEAN_EQUAL;
  else if(datatype == IntegerAttribute::identify()) ret = NAME_INTEGER_EQUAL;
  else if(datatype == DoubleAttribute::identify()) ret = NAME_DOUBLE_EQUAL;*/
  else if(datatype == DateAttribute::getIdentifier()) ret = NAME_DATE_EQUAL;
  else if(datatype == TimeAttribute::getIdentifier()) ret = NAME_TIME_EQUAL;
  else if(datatype == DateTimeAttribute::getIdentifier()) ret = NAME_DATETIME_EQUAL;
  else if(datatype == DurationAttribute::getIdentifier()) ret = NAME_DURATION_EQUAL;
  else if(datatype == PeriodAttribute::getIdentifier()) ret = NAME_PERIOD_EQUAL; 
  //else if(datatype == DayTimeDurationAttribute::identify()) ret = NAME_DAYTIME_DURATION_EQUAL;
  //else if(datatype == YearMonthDurationAttribute::identify()) ret = NAME_YEARMONTH_DURATION_EQUAL;
  else if(datatype == AnyURIAttribute::getIdentifier()) ret = NAME_ANYURI_EQUAL;
  else if(datatype == X500NameAttribute::getIdentifier()) ret = NAME_X500NAME_EQUAL;
  /*else if(datatype == RFC822NameAttribute::identify()) ret = NAME_RFC822NAME_EQUAL;
  else if(datatype == HexBinaryAttribute::identify()) ret = NAME_HEXBINARY_EQUAL;
  else if(datatype == Base64BinaryAttribute::identify()) ret = BASE64BINARY_EQUAL;
  else if(datatype == IPAddressAttribute::identify()) ret = NAME_IPADDRESS_EQUAL;
  else if(datatype == DNSName::identify()) ret = NAME_DNSNAME_EQUAL;
  */
  return ret;
}

EqualFunction::EqualFunction(std::string functionName, std::string argumentType) : Function(functionName, argumentType) {
  fnName = functionName;
  argType = argumentType;

}

bool EqualFunction::evaluate(AttributeValue* arg0, AttributeValue* arg1){
  //TODO
  return arg0->equal(arg1);
}

}
