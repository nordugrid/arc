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

ArcRule::ArcRule(const XMLNode node, EvaluatorContext* ctx) : Policy(node) {
  rulenode = node;
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

  //Set the initial value for id matching 
  sub_idmatched = ID_NO_MATCH;
  res_idmatched = ID_NO_MATCH;
  act_idmatched = ID_NO_MATCH;
  ctx_idmatched = ID_NO_MATCH;
 
}

static ArcSec::MatchResult itemMatch(ArcSec::OrList items, std::list<ArcSec::RequestAttribute*> req, Id_MatchResult& idmatched){

  ArcSec::OrList::iterator orit;
  ArcSec::AndList::iterator andit;
  std::list<ArcSec::RequestAttribute*>::iterator reqit;

  bool indeterminate = true;

  idmatched = ID_NO_MATCH;

  //Go through each <Subject> <Resource> <Action> or <Context> under 
  //<Subjects> <Resources> <Actions> or<Contexts>
  //For example, go through each <Subject> element under <Subjects> in a rule, 
  //once one <Subject> element is satisfied, skip out.
  for( orit = items.begin(); orit != items.end(); orit++ ){
    int all_fraction_matched = 0;
    int all_id_matched = 0;
    //For example, go through each <Attribute> element in one <Subject>, 
    //all of the <Attribute> elements should be satisfied 
    for(andit = (*orit).begin(); andit != (*orit).end(); andit++){
      bool one_req_matched = false;
      bool one_id_matched = false;

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

        //distinguish whether the "id" of the two <Attribute>s (from request and policy) are matched
        //here we distinguish three kinds of situation: 
        //1. All the <Attribute> id under one <Subject> (or other type) in the policy side is matched by 
        //<Attribute> id under one <Subject> in the request side;
        //2. Part of id is matched;
        //3. None of id is matched at all.
        if( ((*andit).first)->getId() == ((*reqit)->getAttributeValue())->getId() )
          one_id_matched = true;
      }
      // if any of the <Attribute> in one Request's <Subject> matches one of the 
      // Rule.Subjects.Subject.Attribute, count the match number. Later if all of the 
      // <Attribute>s under Rule.Subjects.Subject are matched, then the Rule.Subjects.Subject
      // is mathed.
      if(one_req_matched) all_fraction_matched +=1;

      //Similar to above, except only "id" is considered, not including the "value" of <Attribute> 
      if(one_id_matched) all_id_matched +=1;
    }
    //One Rule.Subjects.Subject is satisfied (all of the Attribute value and Attribute Id are matched) 
    //by the RequestTuple.Subject
    if(all_fraction_matched == int((*orit).size())){
      idmatched = ID_MATCH;
      return MATCH;
    }
    else if(all_id_matched == int((*orit).size())) { idmatched = ID_MATCH; indeterminate = false; /*break;*/ }
    //else if(all_id_matched > 0) { idmatched = ID_PARTIAL_MATCH; }
  }
  if(indeterminate) return INDETERMINATE;
  return NO_MATCH;
}

MatchResult ArcRule::match(EvaluationCtx* ctx){
  ArcSec::RequestTuple* evaltuple = ctx->getEvalTuple();  

  //Reset the value for id matching, since the Rule object could be 
  //used a number of times for match-making
  sub_idmatched = ID_NO_MATCH;
  res_idmatched = ID_NO_MATCH;
  act_idmatched = ID_NO_MATCH;
  ctx_idmatched = ID_NO_MATCH;

  MatchResult sub_matched, res_matched, act_matched, ctx_matched;
  sub_matched = itemMatch(subjects, evaltuple->sub, sub_idmatched);
  res_matched = itemMatch(resources, evaltuple->res, res_idmatched);
  act_matched = itemMatch(actions, evaltuple->act, act_idmatched);
  ctx_matched = itemMatch(conditions, evaltuple->ctx, ctx_idmatched);

  if(
      ( subjects.empty() || sub_matched==MATCH) &&
      ( resources.empty() || res_matched==MATCH) &&
      ( actions.empty() || act_matched==MATCH) &&
      ( conditions.empty() || ctx_matched==MATCH)
    )
    return MATCH;

  else if ( ( !(subjects.empty()) && sub_matched==INDETERMINATE ) || 
            ( !(resources.empty()) &&res_matched==INDETERMINATE ) || 
            ( !(actions.empty()) && act_matched==INDETERMINATE ) || 
            ( !(conditions.empty()) && ctx_matched==INDETERMINATE)
          )
    return INDETERMINATE;

  else return NO_MATCH;
}

Result ArcRule::eval(EvaluationCtx* ctx){
  Result result = DECISION_NOT_APPLICABLE;
  MatchResult match_res = match(ctx);

  if(match_res == MATCH) {
    if(effect == "Permit") {
      result = DECISION_PERMIT;
      evalres.effect = "Permit";
    }
    else if(effect == "Deny") {
      result = DECISION_DENY;
      evalres.effect = "Deny";
    }
    return result;
  }
  else if(match_res == INDETERMINATE) {
    if(effect == "Permit") evalres.effect = "Permit";
    else if(effect == "Deny") evalres.effect = "Deny";
    return DECISION_INDETERMINATE; 
  }
  else if(match_res == NO_MATCH){
    if(effect == "Permit") evalres.effect = "Permit";
    else if(effect == "Deny") evalres.effect = "Deny";
    return DECISION_NOT_APPLICABLE;
  }
}

std::string ArcRule::getEffect() const {
  return effect;
}

EvalResult& ArcRule::getEvalResult() {
  return evalres;
}

void ArcRule::setEvalResult(EvalResult& res){
  evalres = res;
}

ArcRule::operator bool(void) const {
  return true;
}

std::string ArcRule::getEvalName() const{
  return "arc.evaluator";
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
