#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>

#include <arc/security/ArcPDP/attr/AttributeValue.h>

#include "XACMLPolicy.h"
#include "XACMLRule.h"
#include "XACMLCondition.h"

static Arc::Logger logger(Arc::Logger::rootLogger, "XACMLCondition");

using namespace Arc;
using namespace ArcSec;

XACMLCondition::XACMLCondition(Arc::XMLNode& node, EvaluatorContext* ctx) : condition_node(node) {
  XMLNode cnd;
  std::string name;
  for(int i = 0;;i++ ) {
    cnd = node.Child(i);
    if(!cnd) break;
    name = cnd.Name();
    if(name == "Apply") {
      apply_list.push_back(new XACMLApply(cnd, ctx));
    }
  }
}

XACMLCondition::~XACMLCondition() {
  while(!(apply_list.empty())) {
    XACMLApply* apply = apply_list.back();
    apply_list.pop_back();
    delete apply;
  }
}

std::list<AttributeValue*> XACMLCondition::evaluate(EvaluationCtx* ctx) {
  std::list<AttributeValue*> res_list;
  std::list<XACMLApply*>::iterator i;
  for(i = apply_list.begin(); i!= apply_list.end(); i++) {
    res_list = (*i)->evaluate(ctx);
    if(!res_list.empty()) break;
    //Suppose only one <Apply/> exists under <Condition/>
    //TODO: process the situation about more than one <Apply/> exist under <Condition/>
  }
  return res_list;
}

