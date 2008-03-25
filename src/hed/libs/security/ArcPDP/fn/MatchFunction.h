#ifndef __ARC_SEC_MATCH_FUNCTION_H__
#define __ARC_SEC_MATCH_FUNCTION_H__

#include <arc/security/ArcPDP/fn/Function.h>
#include <arc/ArcRegex.h>

namespace ArcSec {

#define NAME_REGEXP_STRING_MATCH "regexp-string-match"
#define NAME_ANYURI_REGEXP_MATCH "anyURI-regexp-match"
#define NAME_X500NAME_REGEXP_MATCH "x500Name-regexp-match"

///Evaluate whether arg1 (value in regular expression) matched arg0 (lable in regular expression)
class MatchFunction : public Function {
public:
  MatchFunction(std::string functionName, std::string argumentType);

public:
  virtual bool evaluate(AttributeValue* arg0, AttributeValue* arg1);
   /**help function to get the FunctionName*/
  static std::string getFunctionName(std::string datatype);

private:
  std::string fnName;
  std::string argType;
};

} // namespace ArcSec

#endif /* __ARC_SEC_MATCH_FUNCTION_H__ */

