#include "EqualFunction.h"
 
std::string EqualFunction::getFunctionName(std::string datatype){
  std::string ret;
  switch(datetype){
    case StringAttribute.identifier:
      ret = NAME_STRING_EQUAL;
    case BooleanAttribute.identifier:
      ret = NAME_BOOLEAN_EQUAL;
    case IntegerAttribute.identifier:
      ret = NAME_INTEGER_EQUAL;
    case DoubleAttribute.identifier:
      ret = NAME_DOUBLE_EQUAL;
    case DateAttribute.identifier:
      ret = NAME_DATE_EQUAL;
    case TimeAttribute.identifier:
      ret = NAME_TIME_EQUAL;
    case DateTimeAttribute.identifier:
      ret = NAME_DATETIME_EQUAL;
    case DayTimeDurationAttribute.identifier:
      ret = NAME_DAYTIME_DURATION_EQUAL;
    case YearMonthDurationAttribute.identifier:
      ret = NAME_YEARMONTH_DURATION_EQUAL;
    case AnyURIAttribute.identifier:
      ret = NAME_ANYURI_EQUAL;
    case X500NameAttribute.identifier:
      ret = NAME_X500NAME_EQUAL;
    case RFC822NameAttribute.identifier:
      ret = NAME_RFC822NAME_EQUAL;
    case HexBinaryAttribute.identifier:
      ret = NAME_HEXBINARY_EQUAL;
    case Base64BinaryAttribute.identifier:
      ret = BASE64BINARY_EQUAL;
    case IPAddressAttribute.identifier:
      ret = NAME_IPADDRESS_EQUAL;
    case DNSName.identifier:
      ret = NAME_DNSNAME_EQUAL;
  }
  return ret;
}

EqualFunction::EqualFunction(std::string functionName, std::string argumentType){
  fnName = functionName;
  argType = argumentType;

}

boolean EqualFunction::evaluate(AttributeValue* arg0, AttributeValue* arg1){
  //TODO
  return arg0->equals(*arg1);
}
