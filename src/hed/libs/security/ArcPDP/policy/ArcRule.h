#ifndef __ARC_SEC_ARCRULE_H__
#define __ARC_SEC_ARCRULE_H__

#include <arc/XMLNode.h>
#include <list>

#include <arc/security/ArcPDP/policy/Policy.h>
#include <arc/security/ArcPDP/fn/Function.h>
#include <arc/security/ArcPDP/alg/CombiningAlg.h>
#include <arc/security/ArcPDP/attr/AttributeFactory.h>
#include <arc/security/ArcPDP/fn/FnFactory.h>

#include <arc/security/ArcPDP/Evaluator.h>

namespace ArcSec {

/**"Match" is the definition about the basic element in Rule. The AttributeValue in Request should satisfy the Attribute in ArcRule under the match-making Function.
Usually, there is only explicit definition about AttributeValue in ArcRule (e.g. <Subject Type="X500DN">/O=Grid/OU=KnowARC/CN=ABC</Subject>), no explicit definition about Function. The solution is just using string "equal" plus XML value of attribute "Type" (e.g. "X500-equal"). If there is explicit function difinition in policy (which will make policy definition more complicated, and more like XACML), we can use it.*/

typedef std::pair<AttributeValue*, Function*> Match;
typedef std::list<Match> AndList;
typedef std::list<AndList> OrList;


/**ArcRule class to parsing Arc specific rule format*/
class ArcRule : public Policy {

public:
  ArcRule(Arc::XMLNode& node, EvaluatorContext* ctx);  

  virtual std::string getEffect();

  virtual Result eval(EvaluationCtx* ctx);

  virtual MatchResult match(EvaluationCtx* ctx);

  virtual ~ArcRule();

private:
  void getItemlist(Arc::XMLNode& nd, OrList& items, const std::string& itemtype, const std::string& type_attr, const std::string&
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

  AttributeFactory* attrfactory;
  FnFactory* fnfactory;


protected:
  static Arc::Logger logger;
};

} // namespace ArcSec

#endif /* __ARC_SEC_ARCRULE_H__ */

