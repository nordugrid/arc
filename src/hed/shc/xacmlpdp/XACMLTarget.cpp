#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "XACMLPolicy.h"
#include "XACMLRule.h"
#include <list>

static Arc::Logger logger(Arc::Logger::rootLogger, "XACMLTarget");

using namespace Arc;
using namespace ArcSec;

XACMLTargetMatch::XACMLTargetMatch(XMLNode& node, EvaluatorContext* ctx) : matchnode(node), 
  function(NULL), selector(NULL), designator(NULL){
  attrfactory = (AttributeFactory*)(*ctx);
  fnfactory = (FnFactory*)(*ctx); 

  matchId = (std::string)(node.Attribute("MatchId"));
  //get the suffix of xacml-formated matchId, like
  //"urn:oasis:names:tc:xacml:1.0:function:string-equal",
  //and use it as the function name
  size_t found = matchId.find_last_of(":");
  std::string funcname = matchId.substr(found+1);

  //If matchId does not exist, compose the DataType and "equal" function
  //e.g. if the DataType of <AttributeValue> inside this <Match> is "string", then 
  //suppose the match function is "string-equal"
  std::string datatype = (std::string)(node["AttributeValue"].Attribute("DataType"));
  if(funcname.empty()) funcname = EqualFunction::getFunctionName(datatype); 
  
  //create the Function based on the function name
  function = fnfactory->createFn(funcname);

  //create the AttributeValue, AttributeDesignator and AttributeSelector
  XMLNode cnd;
  AttributeValue attrval;

  for(int i = 0;;i++ ) {
    cnd = node.Child(i);
    if(!cnd) break;
    if(MatchXMLName(cnd, "AttributeValue")) {
       function = attrfactory->createValue(cnd, );
    }
    else if(MatchXMLName(cnd, "AttributeSelector")) {
       selector = new AttributeSelector(cnd, attrfactory);
    }
    else if(MatchXMLName(cnd, "AttributeDesignator")) {
       designator = new AttributeDesignator(cnd);
    }
  }

}

XACMLTargetMatch::~XACMLTargetMatch() {
  if(function != NULL) delete function;
  if(selector != NULL) delete selector;
  if(designator != NULL) delete designator;
}

MatchResult XACMLTargetMatch::match(EvaluationCtx* ctx) {
  std::list<AttributeValue*> attrlist;
  if(selector != NULL) attrlist = selector->evaluate(ctx);
  else if(designator != NULL) attrlist = designator->evaluate(ctx);

  bool evalres = false;
  std::list<AttributeValue*>::iterator i;
  for(i = attrlist.begin(); i != attrlist.end(); i++) {
    evalres = function.evaluate(attrval, (*i));
    if(evalres) return MATCH;
  }
  return NO_MATCH;
}


XACMLTargetMatchGroup::XACMLTargetMatchGroup(XMLNode& node, EvaluatorContext* ctx) : matchgrpnode(node) {
  XMLNode cnd;
  std::string name;
  for(int i = 0;;i++ ) {
    cnd = node.Child(i);
    if(!cnd) break;
    name = cnd.Name();
    if(name.find("Match") != npos)
      matches.push_back(new XACMLTarget(cnd, ctx));
    }
  }
}

XACMLTargetMatchGroup::~XACMLTargetMatchGroup() {
  while(!(matches.empty())) {
    XACMLTargetMatch* tm = matches.pop_back();
    delete tm;
  }
}

MatchResult XACMLTargetMatchGroup::match(EvaluationCtx* ctx) {
  MatchResult res;
  std::list<XACMLTargetMatch*>::iterator i;
  for(i = matches.begin(); i!= matches.end(); i++) {
    res = (*i)->match(ctx);
    if(res == MATCH) break;
  }
  return res;
}


XACMLTargetSection::XACMLTargetSection(Arc::XMLNode& node, EvaluatorContext* ctx) : sectionnode(node) {
  XMLNode cnd;
  std::string name;
  for(int i = 0;;i++ ) {
    cnd = node.Child(i);
    if(!cnd) break;
    name = cnd.Name();
    if(name == "Subject" || name == "Resource" || 
      name == "Action" || name == "Environment" || 
      name == "AnySubject" || name == "AnyResource" || 
      name == "AnyAction" || name == "AnyEnvironment") {
      groups.push_back(new XACMLTargetGroup(cnd, ctx));
    }
    if(name == "AnySubject" || name == "AnyResource" ||
      name == "AnyAction" || name == "AnyEnvironment") break;
  }

}

XACMLTargetSection::~XACMLTargetSection() {
  while(!(groups.empty())) {
    XACMLTargetMatchGroup* grp = groups.pop_back();
    delete grp;
  }
}

MatchResult XACMLTargetSection::match(EvaluationCtx* ctx) {
  MatchResult res;
  std::list<XACMLTargetMatchGroup*>::iterator i;
  for(i = groups.begin(); i!= groups.end(); i++) {
    res = (*i)->match(ctx);
    if(res == MATCH) break;
  }
  return res;
}


XACMLTarget::XACMLTarget(Arc::XMLNode& node, EvaluatorContext* ctx) : targetnode(node) {
  XMLNode cnd;
  std::string name;
  for(int i = 0;;i++ ) {
    cnd = node.Child(i);
    if(!cnd) break;
    name = cnd.Name();
    if(name == "Subjects" || name == "Resources" ||
      name == "Actions" || name == "Environments") {
      sections.push_back(new XACMLTargetSection(cnd, ctx));
    }
  }

}

XACMLTarget::~XACMLTarget() {
  while(!(sections.empty())) {
    XACMLTargetMatchSection* section = sections.pop_back();
    delete section;
  }
}

MatchResult XACMLTarget::match(EvaluationCtx* ctx) {
  MatchResult res;
  std::list<XACMLTargetMatchSection*>::iterator i;
  for(i = sections.begin(); i!= sections.end(); i++) {
    res = (*i)->match(ctx);
    if(res != MATCH) break;
  }
  return res;

}

