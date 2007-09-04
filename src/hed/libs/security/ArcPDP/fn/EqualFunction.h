#ifndef __ARC_EQUAL_FUNCTION_H__
#define __ARC_EQUAL_FUNCTION_H__

#include "Function.h"

namespace Arc {

#define NAME_STRING_EQUAL "know-arc:function:string-equal"
#define NAME_BOOLEAN_EQUAL "know-arc:function:boolean-equal"
#define NAME_INTEGER_EQUAL "know-arc:function:integer-equal"
#define NAME_DOUBLE_EQUAL "know-arc:function:double-equal"
#define NAME_DATE_EQUAL "know-arc:function:date-equal"
#define NAME_TIME_EQUAL "know-arc:function:time-equal"
#define NAME_DATETIME_EQUAL "know-arc:function:dateTime-equal"
#define NAME_DAYTIME_DURATION_EQUAL "know-arc:function:dayTimeDuration-equal"
#define NAME_YEARMONTH_DURATION_EQUAL "know-arc:function:yearMonthDuration-equal"
#define NAME_ANYURI_EQUAL "know-arc:function:anyURI-equal"    
#define NAME_X500NAME_EQUAL "know-arc:function:x500Name-equal"
#define NAME_RFC822NAME_EQUAL "know-arc:function:rfc822Name-equal"
#define NAME_HEXBINARY_EQUAL "know-arc:function:hexBinary-equal"
#define NAME_BASE64BINARY_EQUAL "know-arc:function:base64Binary-equal"       
#define NAME_IPADDRESS_EQUAL "know-arc:function:ipAddress-equal"
#define NAME_DNSNAME_EQUAL "know-arc:function:dnsName-equal"

class EqualFunction : public Function {
public:
  EqualFunction(std::string functionName, std::string argumentType);

public:
  virtual bool evaluate(AttributeValue* arg0, AttributeValue* arg1);
  //help function specific for existing policy expression because of no exiplicit function defined in policy
  static std::string getFunctionName(std::string datatype);

private:
  std::string fnName; 
  std::string argType; 

};

} // namespace Arc

#endif /* __ARC_EQUAL_FUNCTION_H__ */

