#ifndef __ARC_EUQAL_FUNCTION_H__
#define __ARC_EQUAL_FUNCTION_H__

#include "Function.h"

namespace Arc {

  static std::string NAME_STRING_EQUAL = FUNCTION_NS + "string-equal";
  static std::string NAME_BOOLEAN_EQUAL = FUNCTION_NS + "boolean-equal";
  static std::string NAME_INTEGER_EQUAL = FUNCTION_NS + "integer-equal";
  static std::string NAME_DOUBLE_EQUAL = FUNCTION_NS + "double-equal";
  static std::string NAME_DATE_EQUAL = FUNCTION_NS + "date-equal";
  static std::string NAME_TIME_EQUAL = FUNCTION_NS + "time-equal";
  static std::string NAME_DATETIME_EQUAL = FUNCTION_NS + "dateTime-equal";
  static std::string NAME_DAYTIME_DURATION_EQUAL = FUNCTION_NS + "dayTimeDuration-equal";
  static std::string NAME_YEARMONTH_DURATION_EQUAL = FUNCTION_NS + "yearMonthDuration-equal";
  static std::string NAME_ANYURI_EQUAL = FUNCTION_NS + "anyURI-equal";       
  static std::string NAME_X500NAME_EQUAL = FUNCTION_NS + "x500Name-equal";
  static std::string NAME_RFC822NAME_EQUAL = FUNCTION_NS + "rfc822Name-equal";
  static std::string NAME_HEXBINARY_EQUAL = FUNCTION_NS + "hexBinary-equal";
  static std::string NAME_BASE64BINARY_EQUAL = FUNCTION_NS + "base64Binary-equal";       
  static std::string NAME_IPADDRESS_EQUAL = FUNCTION_NS + "ipAddress-equal";
  static std::string NAME_DNSNAME_EQUAL = FUNCTION_NS + "dnsName-equal";

class EqualFunction : public Function {
public:
  EqualFunction(std::string functionName, std::string argumentType);
  virtual ~EqualFunction();

public:
  virtual bool evaluate(){};

  //help function specific for existing policy expression because of no exiplicit function defined in policy
  std::string getFunctionName(std::string datatype) const;

private:
  std::string fnName; 
  std::string argType; 

};

} // namespace Arc

#endif /* __ARC_EQUAL_FUNCTION_H__ */

