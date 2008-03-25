#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <iostream>
#include <arc/security/ArcPDP/attr/AttributeValue.h>
#include "XACMLRule.h"
#include <list>

#include <arc/security/ArcPDP/fn/EqualFunction.h>

Arc::Logger ArcSec::XACMLRule::logger(Arc::Logger::rootLogger, "XACMLRule");

using namespace Arc;
using namespace ArcSec;

XACMLRule::XACMLRule(XMLNode& node, EvaluatorContext* ctx) : Policy(node), target(NULL) {
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
  if((bool)targetnode) target = new XACMLTarget(targetnode, ctx);
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
    MatchResult matchres = target.match(ctx);
    if(matchres == NO_MATCH)  return result;
    else if(matchres == INDETERMINATE) {result = DECISION_INDETERMINATE; return result;}
  }

  //TODO: evaluate the "Condition"

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
}
