#ifndef __ARC_MATCH_FUNCTION_H__
#define __ARC_MATCH_FUNCTION_H__

#include "Function.h"
#include <arc/ArcRegex.h>

namespace Arc {

#define NAME_REGEXP_STRING_MATCH "know-arc:function:regexp-string-match"
#define NAME_ANYURI_REGEXP_MATCH "know-arc:function:anyURI-regexp-match"
#define NAME_X500NAME_REGEXP_MATCH "know-arc:function:x500Name-regexp-match"

class MatchFunction : public Function {
public:
  MatchFunction(std::string functionName, std::string argumentType);

public:
  virtual bool evaluate(AttributeValue* arg0, AttributeValue* arg1);
   //help function specific for existing policy expression because of no exiplicit function defined in policy
  static std::string getFunctionName(std::string datatype);

private:
  std::string fnName;
  std::string argType;
};

} // namespace Arc

#endif /* __ARC_MATCH_FUNCTION_H__ */

