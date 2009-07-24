#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <iostream>
#include <arc/security/ArcPDP/attr/AttributeValue.h>
#include <arc/security/ArcPDP/attr/BooleanAttribute.h>
#include "XACMLRule.h"
#include <list>

#include <arc/security/ArcPDP/fn/EqualFunction.h>

Arc::Logger ArcSec::XACMLRule::logger(Arc::Logger::rootLogger, "XACMLRule");

using namespace Arc;
using namespace ArcSec;

XACMLRule::XACMLRule(XMLNode& node, EvaluatorContext* ctx) : Policy(node), 
  target(NULL), condition(NULL) {
  rulenode = node;
  evalres.node = node;
  evalres.effect = "Not_applicable";

  attrfactory = (AttributeFactory*)(*ctx);
  fnfactory = (FnFactory*)(*ctx);
  
  id = (std::string)(node.Attribute("RuleId"));
  description = (std::string)(node["Description"]);
  if((std::string)(node.Attribute("Effect"))=="Permit")
    effect="Permit";
  else if((std::string)(node.Attribute("Effect"))=="Deny")
    effect="Deny";
  else
    logger.msg(Arc::ERROR, "Invalid Effect");

  XMLNode targetnode = node["Target"];
  if(((bool)targetnode) && ((bool)(targetnode.Child()))) 
    target = new XACMLTarget(targetnode, ctx);

  XMLNode conditionnode = node["Condition"];
  if((bool)conditionnode) condition = new XACMLCondition(conditionnode, ctx);
}

MatchResult XACMLRule::match(EvaluationCtx* ctx){
  MatchResult res;
  if(target != NULL) res = target->match(ctx);
  else { logger.msg(Arc::ERROR, "No target available inside the rule"); res = INDETERMINATE; }
  return res;
}

Result XACMLRule::eval(EvaluationCtx* ctx){
  Result result = DECISION_NOT_APPLICABLE;
  if(target != NULL) {
    MatchResult matchres = target->match(ctx);
    if(matchres == NO_MATCH)  return result;
    else if(matchres == INDETERMINATE) {result = DECISION_INDETERMINATE; return result;}
  }

  //evaluate the "Condition"
  bool cond_res = false;
  if(condition != NULL) {
    std::list<AttributeValue*> res_list = condition->evaluate(ctx);
    AttributeValue* attrval = *(res_list.begin()); 
    //Suppose only one "bool" attribute value in the evaluation result.
    BooleanAttribute bool_attr(true);
    if(attrval->equal(&bool_attr))
      cond_res = true;
    if(attrval) delete attrval;
    if(!cond_res) { result = DECISION_INDETERMINATE; return result; } 
  }

  if (effect == "Permit") { 
    result = DECISION_PERMIT;
    evalres.effect = "Permit";
  }
  else if (effect == "Deny") {
    result = DECISION_DENY;
    evalres.effect = "Deny";
  }
  return result;
}

std::string XACMLRule::getEffect(){
  return effect;
}

EvalResult& XACMLRule::getEvalResult(){
  return evalres;
}

XACMLRule::~XACMLRule(){
  if(target != NULL) delete target;
  if(condition != NULL) delete condition;
}
