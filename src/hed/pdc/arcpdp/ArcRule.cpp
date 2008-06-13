#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <fstream>
#include <iostream>
#include <list>

#include <arc/security/ArcPDP/attr/AttributeValue.h>
#include <arc/security/ArcPDP/fn/EqualFunction.h>
#include <arc/security/ArcPDP/fn/MatchFunction.h>
#include <arc/security/ArcPDP/fn/InRangeFunction.h>

#include "ArcRule.h"

Arc::Logger ArcSec::ArcRule::logger(Arc::Logger::rootLogger, "ArcRule");

using namespace Arc;
using namespace ArcSec;

void ArcRule::getItemlist(XMLNode& nd, OrList& items, const std::string& itemtype,
    const std::string& type_attr, const std::string& function_attr){
  XMLNode tnd;
  for(int i=0; i<nd.Size(); i++){
    for(int j=0;;j++){
      std::string type = type_attr;
      std::string funcname = function_attr;

      tnd = nd[itemtype][j];
      if(!tnd) break;
      if(!((std::string)(tnd.Attribute("Type"))).empty())
        type = (std::string)(tnd.Attribute("Type"));
      if(!((std::string)(tnd.Attribute("Function"))).empty())
        funcname = (std::string)(tnd.Attribute("Function"));

      if(!(type.empty())&&(tnd.Size()==0)){
        AndList item;
        std::string function;
        if(funcname.empty()) function = EqualFunction::getFunctionName(type);
        else if(funcname == "Match" || funcname == "match" || funcname == "MATCH")
          function = MatchFunction::getFunctionName(type);
        else if(funcname == "InRange" || funcname == "inrange" || funcname == "INRANGE" || funcname == "Inrange")
          function = InRangeFunction::getFunctionName(type);
        else std::cout<<"Function Name is wrong"<<std::endl;
        item.push_back(Match(attrfactory->createValue(tnd, type), fnfactory->createFn(function)));
        items.push_back(item);
      }
      else if((type.empty())&&(tnd.Size()>0)){
        AndList item;
        int size = tnd.Size();
        for(int k=0; k<size; k++){
          XMLNode snd = tnd.Child(k);
          type = (std::string)(snd.Attribute("Type"));
          // The priority of function definition: Subelement.Attribute("Function")
          // > Element.Attribute("Function") > Subelement.Attribute("Type") + "equal"
          if(!((std::string)(snd.Attribute("Function"))).empty())
            funcname = (std::string)(snd.Attribute("Function"));
          std::string function;
          if(funcname.empty()) function = EqualFunction::getFunctionName(type);
          else if(funcname == "Match" || funcname == "match" || funcname == "MATCH")
            function = MatchFunction::getFunctionName(type);
          else if(funcname == "InRange" || funcname == "inrange" || funcname == "INRANGE" || funcname == "Inrange")
            function = InRangeFunction::getFunctionName(type);
          else std::cout<<"Function Name is wrong"<<std::endl;
          item.push_back(Match(attrfactory->createValue(snd, type), fnfactory->createFn(function)));
        }
        items.push_back(item);
      }
      else if(!(type.empty())&&(tnd.Size()>0)){
        AndList item;
        int size = tnd.Size();
        for(int k=0; k<size; k++){
          XMLNode snd = tnd.Child(k);
          if(!((std::string)(snd.Attribute("Function"))).empty())
            funcname = (std::string)(snd.Attribute("Function"));
          std::string function;
          if(funcname.empty()) function = EqualFunction::getFunctionName(type);
          else if(funcname == "Match" || funcname == "match" || funcname == "MATCH")
            function = MatchFunction::getFunctionName(type);
          else if(funcname == "InRange" || funcname == "inrange" || funcname == "INRANGE" || funcname == "Inrange")
            function = InRangeFunction::getFunctionName(type);
          else std::cout<<"Function Name is wrong"<<std::endl;
          item.push_back(Match(attrfactory->createValue(snd, type), fnfactory->createFn(function)));
        }
        items.push_back(item);
      }
      else{
        //std::cerr <<"Error definition in policy"<<std::endl;
        //logger.msg(Arc::ERROR, "Error definition in policy"); 
        return;
      }
    }

    for(int l=0;;l++){
      tnd = nd["GroupIdRef"][l];
      if(!tnd) break;
      std::string location = (std::string)(tnd.Attribute("Location"));
 
      //Open the reference file and parse it to get external item information
      std::string xml_str = "";
      std::string str;
      std::ifstream f(location.c_str());
      while (f >> str) {
        xml_str.append(str);
        xml_str.append(" ");
      }
      f.close();

      XMLNode root = XMLNode(xml_str);
      XMLNode subref = root.Child();

      XMLNode snd;
      std::string itemgrouptype = itemtype + "Group";

      for(int k=0;;k++){
        snd = subref[itemgrouptype][k];
        if(!snd) break;

        //If the reference ID in the policy file matches the ID in the external file, 
        //try to get the subject information from the external file
        if((std::string)(snd.Attribute("GroupId")) == (std::string)tnd){
          getItemlist(snd, items, itemtype, type_attr, function_attr);
        }
      }
    }
  }
  return;
}

ArcRule::ArcRule(XMLNode* node, EvaluatorContext* ctx) : Policy(node) {
  rulenode = *node;
  evalres.node = rulenode;
  evalres.effect = "Not_applicable";

  attrfactory = (AttributeFactory*)(*ctx);
  fnfactory = (FnFactory*)(*ctx);
  
  XMLNode nd, tnd;

  id = (std::string)(rulenode.Attribute("RuleId"));
  description = (std::string)(rulenode["Description"]);
  if((std::string)(rulenode.Attribute("Effect"))=="Permit")
    effect="Permit";
  else if((std::string)(rulenode.Attribute("Effect"))=="Deny")
    effect="Deny";
  //else
    //std::cerr<< "Invalid Effect" <<std::endl; 
    //logger.msg(Arc::ERROR, "Invalid Effect");

  std::string type,funcname;
  //Parse the "Subjects" part of a Rule
  nd = rulenode["Subjects"];
  type = (std::string)(nd.Attribute("Type"));
  funcname = (std::string)(nd.Attribute("Function"));
  getItemlist(nd, subjects, "Subject", type, funcname);  

  //Parse the "Resources" part of a Rule. The "Resources" does not include "Sub" item, 
  //so it is not such complicated, but we can handle it the same as "Subjects" 
  nd = rulenode["Resources"];
  type = (std::string)(nd.Attribute("Type"));
  funcname = (std::string)(nd.Attribute("Function"));
  getItemlist(nd, resources, "Resource", type, funcname);

  //Parse the "Actions" part of a Rule
  nd = rulenode["Actions"];
  type = (std::string)(nd.Attribute("Type"));
  funcname = (std::string)(nd.Attribute("Function"));
  getItemlist(nd, actions, "Action", type, funcname);

  //Parse the "Condition" part of a Rule
  nd = rulenode["Conditions"];
  type = (std::string)(nd.Attribute("Type"));
  funcname = (std::string)(nd.Attribute("Function"));
  getItemlist(nd, conditions, "Condition", type, funcname);
 
}

static ArcSec::MatchResult itemMatch(ArcSec::OrList items, std::list<ArcSec::RequestAttribute*> req){

  ArcSec::OrList::iterator orit;
  ArcSec::AndList::iterator andit;
  std::list<ArcSec::RequestAttribute*>::iterator reqit;

  //Go through each <Subject>/<Resource>/<Action>/<Context>
  //For example, go through each <Subject> element in one rule, 
  //once one <Subject> element is satisfied, skip out.
  for( orit = items.begin(); orit != items.end(); orit++ ){

    int all_fraction_matched = 0;
    //For example, go through each <Attribute> element in one <Subject>, 
    //all of the <Attribute> elements should be satisfied.
    for(andit = (*orit).begin(); andit != (*orit).end(); andit++){
      bool one_req_matched = false;

      //go through each <Attribute> element in one <Subject> in Request.xml, 
      //all of the <Attribute> should be satisfied.
      for(reqit = req.begin(); reqit != req.end(); reqit++){
        //evaluate two "AttributeValue*" based on "Function" definition in "Rule"
        bool res = false;
        try{
          res = ((*andit).second)->evaluate((*andit).first, (*reqit)->getAttributeValue());
        } catch(std::exception&) { };
        if(res)
          one_req_matched = true;
      }
      // if one of the Attribute in one Request's Subject does not match any of the 
      // Rule.Subjects.SubjectA.Attributes, then skip to the next: Rule.Subjects.SubjectB
      if(!one_req_matched) break;
      else all_fraction_matched +=1;
    }
    //One Rule.Subjects.Subject is satisfied (all of the Attribute are satisfied) 
    //by the RequestTuple.Subject
    if(all_fraction_matched == int((*orit).size())){
      return MATCH;
    }
  }
  return NO_MATCH;
}

MatchResult ArcRule::match(EvaluationCtx* ctx){
  ArcSec::RequestTuple* evaltuple = ctx->getEvalTuple();  

  if(
      (
        subjects.empty() ||
        (itemMatch(subjects, evaltuple->sub)==MATCH)
      ) &&
      (
        resources.empty() ||
        (itemMatch(resources, evaltuple->res)==MATCH)
      ) &&
      (
        actions.empty() || 
        (itemMatch(actions, evaltuple->act)==MATCH)
      ) &&
      (
        conditions.empty() || 
        (itemMatch(conditions, evaltuple->ctx)==MATCH)
      )
    )
    return MATCH;
  else return NO_MATCH;
}

Result ArcRule::eval(EvaluationCtx*){// ctx){
  Result result = DECISION_NOT_APPLICABLE;
  //TODO
  // need to evaluate the "Condition"
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

std::string ArcRule::getEffect(){
  return effect;
}

EvalResult& ArcRule::getEvalResult(){
  return evalres;
}

ArcRule::~ArcRule(){
  while(!(subjects.empty())){
    AndList list = subjects.back();
    while(!(list.empty())){
      Match match = list.back();
      if(match.first){
        delete match.first;
      }
      list.pop_back();
    }
    subjects.pop_back();
  }

  while(!(resources.empty())){
    AndList list = resources.back();
    while(!(list.empty())){
      Match match = list.back();
      if(match.first)
        delete match.first;
      list.pop_back();
    }
    resources.pop_back();
  }

  while(!(actions.empty())){
    AndList list = actions.back();
    while(!(list.empty())){
      Match match = list.back();
      if(match.first)
        delete match.first;
      list.pop_back();
    }
    actions.pop_back();
  }

  while(!(conditions.empty())){
    AndList list = conditions.back();
    while(!(list.empty())){
      Match match = list.back();
      if(match.first)
        delete match.first;
      list.pop_back();
    }
    conditions.pop_back();
  }
}
