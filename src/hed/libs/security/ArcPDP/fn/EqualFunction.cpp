#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "EqualFunction.h"
#include "../attr/StringAttribute.h"
#include "../attr/DateTimeAttribute.h"
#include "../attr/X500NameAttribute.h"
#include "../attr/AnyURIAttribute.h"

namespace ArcSec {

std::string EqualFunction::getFunctionName(std::string datatype){
  std::string ret;
  if (datatype ==  StringAttribute::getIdentifier()) ret = NAME_STRING_EQUAL;
  //else if(datatype == BooleanAttribute::getIdentify()) ret = NAME_BOOLEAN_EQUAL;
  //else if(datatype == IntegerAttribute::getIdentify()) ret = NAME_INTEGER_EQUAL;
  //else if(datatype == DoubleAttribute::getIdentify()) ret = NAME_DOUBLE_EQUAL;
  else if(datatype == DateAttribute::getIdentifier()) ret = NAME_DATE_EQUAL;
  else if(datatype == TimeAttribute::getIdentifier()) ret = NAME_TIME_EQUAL;
  else if(datatype == DateTimeAttribute::getIdentifier()) ret = NAME_DATETIME_EQUAL;
  else if(datatype == DurationAttribute::getIdentifier()) ret = NAME_DURATION_EQUAL;
  else if(datatype == PeriodAttribute::getIdentifier()) ret = NAME_PERIOD_EQUAL; 
  //else if(datatype == DayTimeDurationAttribute::getIdentify()) ret = NAME_DAYTIME_DURATION_EQUAL;
  //else if(datatype == YearMonthDurationAttribute::getIdentify()) ret = NAME_YEARMONTH_DURATION_EQUAL;
  else if(datatype == AnyURIAttribute::getIdentifier()) ret = NAME_ANYURI_EQUAL;
  else if(datatype == X500NameAttribute::getIdentifier()) ret = NAME_X500NAME_EQUAL;
  //else if(datatype == RFC822NameAttribute::getIdentify()) ret = NAME_RFC822NAME_EQUAL;
  //else if(datatype == HexBinaryAttribute::getIdentify()) ret = NAME_HEXBINARY_EQUAL;
  //else if(datatype == Base64BinaryAttribute::getIdentify()) ret = BASE64BINARY_EQUAL;
  //else if(datatype == IPAddressAttribute::getIdentify()) ret = NAME_IPADDRESS_EQUAL;
  //else if(datatype == DNSName::getIdentify()) ret = NAME_DNSNAME_EQUAL;
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
