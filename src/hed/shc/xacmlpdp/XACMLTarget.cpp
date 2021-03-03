#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>

#include <arc/security/ArcPDP/attr/AttributeValue.h>
#include <arc/security/ArcPDP/attr/BooleanAttribute.h>
#include <arc/security/ArcPDP/fn/EqualFunction.h>

#include "XACMLPolicy.h"
#include "XACMLRule.h"
#include "XACMLTarget.h"

static Arc::Logger logger(Arc::Logger::rootLogger, "XACMLTarget");

using namespace Arc;
using namespace ArcSec;

XACMLTargetMatch::XACMLTargetMatch(XMLNode& node, EvaluatorContext* ctx) : matchnode(node), 
  attrval(NULL), function(NULL), designator(NULL), selector(NULL) {
  attrfactory = (AttributeFactory*)(*ctx);
  fnfactory = (FnFactory*)(*ctx); 

  matchId = (std::string)(node.Attribute("MatchId"));
  //get the suffix of xacml-formatted matchId, like
  //"urn:oasis:names:tc:xacml:1.0:function:string-equal",
  //and use it as the function name
  std::size_t found = matchId.find_last_of(":");
  std::string funcname = matchId.substr(found+1);

  //If matchId does not exist, compose the DataType and "equal" function
  //e.g. if the DataType of <AttributeValue> inside this <Match> is "string", then 
  //suppose the match function is "string-equal"
  std::string datatype = (std::string)(node["AttributeValue"].Attribute("DataType"));
  if(funcname.empty()) funcname = EqualFunction::getFunctionName(datatype); 
  
  //create the Function based on the function name
  function = fnfactory->createFn(funcname);
  if(!function) { logger.msg(ERROR, "Can not create function %s", funcname); return; }

  //create the AttributeValue, AttributeDesignator and AttributeSelector
  XMLNode cnd;

  XMLNode attrval_nd;
  std::string attrval_id;
  std::string attrval_type;
  for(int i = 0;;i++ ) {
    cnd = node.Child(i);
    if(!cnd) break;
    std::string name = cnd.Name();
    if(name.find("AttributeValue") != std::string::npos) {
       std::string data_type = cnd.Attribute("DataType");
       //<AttributeValue DataType="http://www.w3.org/2001/XMLSchema#string">
       //  http://www.med.example.com/schemas/record.xsd
       //</AttributeValue>
       attrval_nd = cnd;
       std::size_t f = data_type.find_last_of("#"); //http://www.w3.org/2001/XMLSchema#string
       if(f!=std::string::npos) {
         attrval_type = data_type.substr(f+1);
       }
       else {
         f=data_type.find_last_of(":"); //urn:oasis:names:tc:xacml:1.0:data-type:rfc822Name
         attrval_type = data_type.substr(f+1);
       }
    }
    else if(name.find("AttributeSelector") != std::string::npos) {
      selector = new AttributeSelector(cnd, attrfactory);
      attrval_id = (std::string)(cnd.Attribute("AttributeId"));   
    }
    else if(name.find("AttributeDesignator") != std::string::npos) {
      designator = new AttributeDesignator(cnd, attrfactory);
      attrval_id = (std::string)(cnd.Attribute("AttributeId"));
    }
  }
  //kind of hack here. Because in xacml, <AttributeValue/> (the policy side)
  //normally xml attribute "AttributeId" is absent, but in our implementation 
  //about comparing two attribute, "AttributeId" is required.
  attrval_nd.NewAttribute("AttributeId") = attrval_id;
  attrval = attrfactory->createValue(attrval_nd, attrval_type);
}

XACMLTargetMatch::~XACMLTargetMatch() {
  if(attrval != NULL) delete attrval;
  if(selector != NULL) delete selector;
  if(designator != NULL) delete designator;
}

MatchResult XACMLTargetMatch::match(EvaluationCtx* ctx) {
  std::list<AttributeValue*> attrlist;
  if(selector != NULL) attrlist = selector->evaluate(ctx);
  else if(designator != NULL) attrlist = designator->evaluate(ctx);

  AttributeValue* evalres = NULL;
  std::list<AttributeValue*>::iterator i;
  for(i = attrlist.begin(); i != attrlist.end(); i++) {
std::cout<<"Request side: "<<(*i)->encode()<<" Policy side:  "<<attrval->encode()<<std::endl;
    evalres = function->evaluate(attrval, (*i), false);
    BooleanAttribute bool_attr(true);
    if((evalres != NULL) && (evalres->equal(&bool_attr))) { 
      std::cout<<"Matched!"<<std::endl; 
      delete evalres; break; 
    }
    if(evalres) delete evalres;
  }
  while(!(attrlist.empty())) {
    AttributeValue* val = attrlist.back();
    attrlist.pop_back();
    delete val;
  }
  if(evalres) return MATCH;
  else return NO_MATCH;
}


XACMLTargetMatchGroup::XACMLTargetMatchGroup(XMLNode& node, EvaluatorContext* ctx) : matchgrpnode(node) {
  XMLNode cnd;
  std::string name;
  for(int i = 0;;i++ ) {
    cnd = node.Child(i);
    if(!cnd) break;
    name = cnd.Name();
    if(name.find("Match") != std::string::npos)
      matches.push_back(new XACMLTargetMatch(cnd, ctx));
  }
}

XACMLTargetMatchGroup::~XACMLTargetMatchGroup() {
  while(!(matches.empty())) {
    XACMLTargetMatch* tm = matches.back();
    matches.pop_back();
    delete tm;
  }
}

MatchResult XACMLTargetMatchGroup::match(EvaluationCtx* ctx) {
  MatchResult res = NO_MATCH;
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
      groups.push_back(new XACMLTargetMatchGroup(cnd, ctx));
    }
    if(name == "AnySubject" || name == "AnyResource" ||
      name == "AnyAction" || name == "AnyEnvironment") break;
  }

}

XACMLTargetSection::~XACMLTargetSection() {
  while(!(groups.empty())) {
    XACMLTargetMatchGroup* grp = groups.back();
    groups.pop_back();
    delete grp;
  }
}

MatchResult XACMLTargetSection::match(EvaluationCtx* ctx) {
  MatchResult res = NO_MATCH;
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
    XACMLTargetSection* section = sections.back();
    sections.pop_back();
    delete section;
  }
}

MatchResult XACMLTarget::match(EvaluationCtx* ctx) {
  MatchResult res = NO_MATCH;
  std::list<XACMLTargetSection*>::iterator i;
  for(i = sections.begin(); i!= sections.end(); i++) {
    res = (*i)->match(ctx);
    if(res != MATCH) break;
  }
  return res;
}

