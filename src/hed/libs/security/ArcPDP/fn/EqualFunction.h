#ifndef __ARC_SEC_EQUAL_FUNCTION_H__
#define __ARC_SEC_EQUAL_FUNCTION_H__

#include <arc/security/ArcPDP/fn/Function.h>

namespace ArcSec {

#define NAME_STRING_EQUAL "string-equal"
#define NAME_BOOLEAN_EQUAL "boolean-equal"
#define NAME_INTEGER_EQUAL "integer-equal"
#define NAME_DOUBLE_EQUAL "double-equal"
#define NAME_DATE_EQUAL "date-equal"
#define NAME_TIME_EQUAL "time-equal"
#define NAME_DATETIME_EQUAL "datetime-equal"
#define NAME_DURATION_EQUAL "duration-equal"
#define NAME_PERIOD_EQUAL "period-equal"
//#define NAME_DAYTIME_DURATION_EQUAL "dayTimeDuration-equal"
//#define NAME_YEARMONTH_DURATION_EQUAL "yearMonthDuration-equal"
#define NAME_ANYURI_EQUAL "anyURI-equal"    
#define NAME_X500NAME_EQUAL "x500Name-equal"
#define NAME_RFC822NAME_EQUAL "rfc822Name-equal"
#define NAME_HEXBINARY_EQUAL "hexBinary-equal"
#define NAME_BASE64BINARY_EQUAL "base64Binary-equal"       
#define NAME_IPADDRESS_EQUAL "ipAddress-equal"
#define NAME_DNSNAME_EQUAL "dnsName-equal"

///Evaluate whether the two values are equal
class EqualFunction : public Function {
public:
  EqualFunction(std::string functionName, std::string argumentType);

public:
  virtual bool evaluate(AttributeValue* arg0, AttributeValue* arg1);
  /**help function to get the FunctionName*/
  static std::string getFunctionName(std::string datatype);

private:
  std::string fnName; 
  std::string argType; 

};

} // namespace ArcSec

#endif /* __ARC_SEC_EQUAL_FUNCTION_H__ */

