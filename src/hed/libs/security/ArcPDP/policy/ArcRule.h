#ifndef __ARC_ARCRULE_H__
#define __ARC_ARCRULE_H__

#include "Policy.h"
#include "../fn/Function.h"
#include <list>
#include "../alg/CombiningAlg.h"
#include <arc/XMLNode.h>


namespace Arc {

/**"Match" is the definition about the basic element in Rule. The AttributeValue in Request should satisfy the Attribute in ArcRule under the match-making Function.
Usually, there is only explicit definition about AttributeValue in ArcRule (e.g. <Subject Type="X500DN">/O=Grid/OU=KnowARC/CN=ABC</Subject>), no explicit definition about Function. The solution is just using string "equal" plus XML value of attribute "Type" (e.g. "X500-equal"). If there is explicit function difinition in policy (which will make policy definition more complicated, and more like XACML), we can use it.*/

typedef std::pair<Arc::AttributeValue*, Arc::Function*> Match;
typedef std::list<Arc::Match> AndList;
typedef std::list<Arc::AndList> OrList;


/**ArcRule class to parsing Arc specific rule format*/
class ArcRule : public Policy {

public:
  ArcRule(XMLNode& node);  

  virtual std::string getEffect();

  virtual Result eval(EvaluationCtx* ctx);

  virtual MatchResult match(EvaluationCtx* ctx);

private:
  void getItemlist(XMLNode& nd, OrList& items, const std::string& itemtype, const std::string& type_attr, const std::string&
function_attr);

private:
  std::string effect;
  std::string id;
  std::string version;
  std::string description;
  
  OrList subjects;
  OrList resources;
  OrList actions;
  OrList conditions;

protected:
  static Logger logger;
};

} // namespace Arc

#endif /* __ARC_ARCRULE_H__ */

