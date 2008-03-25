#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "XACMLPolicy.h"
#include "XACMLRule.h"
#include <list>

static Arc::Logger logger(Arc::Logger::rootLogger, "XACMLTarget");

using namespace Arc;
using namespace ArcSec;

XACMLTargetMatch::XACMLTargetMatch(XMLNode& node, EvaluatorContext* ctx) : matchnode(node), function(NULL){
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
       attrfactory->createValue();
    }
    else if(MatchXMLName(cnd, "AttributeSelector")) {

    }
    else if(MatchXMLName(cnd, "AttributeDesignator")) {

    }
  }

}

XACMLTargetMatch::~XACMLTargetMatch() {

}

MatchResult XACMLTargetMatch::match(EvaluationCtx* ctx) {


}


