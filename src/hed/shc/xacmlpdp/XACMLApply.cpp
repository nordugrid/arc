#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>

#include <arc/security/ArcPDP/attr/AttributeValue.h>

#include "XACMLPolicy.h"
#include "XACMLRule.h"
#include "XACMLApply.h"

static Arc::Logger logger(Arc::Logger::rootLogger, "XACMLApply");

using namespace Arc;
using namespace ArcSec;

XACMLApply::XACMLApply(XMLNode& node, EvaluatorContext* ctx) : applynode(node), function(NULL) {
  attrfactory = (AttributeFactory*)(*ctx);
  fnfactory = (FnFactory*)(*ctx); 

  functionId = (std::string)(node.Attribute("FunctionId"));
  //get the suffix of xacml-formatted FunctionId, like
  //"urn:oasis:names:tc:xacml:1.0:function:and",
  //and use it as the function name
  std::size_t found = functionId.find_last_of(":");
  std::string funcname = functionId.substr(found+1);

  if(funcname.empty()) { logger.msg(ERROR, "Can not create function: FunctionId does not exist"); return; }; 
  
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
      AttributeValue* attrval = attrfactory->createValue(attrval_nd, attrval_type);
      attrval_list[i] = attrval;
    }
    else if(name.find("AttributeSelector") != std::string::npos) {
      AttributeSelector* selector = new AttributeSelector(cnd, attrfactory);
      selector_list[i] = selector;
    }
    else if(name.find("AttributeDesignator") != std::string::npos) {
      AttributeDesignator* designator = new AttributeDesignator(cnd, attrfactory);
      designator_list[i] = designator;
    }
    else if(name == "Apply") {
      XACMLApply* apply = new XACMLApply(cnd, ctx);
      sub_apply_list[i] = apply;
    }
  }
}

XACMLApply::~XACMLApply() {
  std::map<int, AttributeValue*>::iterator attrval_it;
  std::map<int, AttributeSelector*>::iterator selector_it;
  std::map<int, AttributeDesignator*>::iterator designator_it;
  std::map<int, XACMLApply*>::iterator apply_it;
  attrval_it = attrval_list.begin();
  selector_it = selector_list.begin();
  designator_it = designator_list.begin();
  apply_it = sub_apply_list.begin();
 
  for(;attrval_it != attrval_list.end();) { 
    AttributeValue* attrval = (*attrval_it).second;
    attrval_list.erase(attrval_it);
    delete attrval;
    attrval_it = attrval_list.begin();
  }
  for(;selector_it != selector_list.end();) {  
    AttributeSelector* selector = (*selector_it).second;
    selector_list.erase(selector_it);
    delete selector;
    selector_it = selector_list.begin();
  }
  for(;designator_it != designator_list.end();) {
    AttributeDesignator* designator = (*designator_it).second;
    designator_list.erase(designator_it);
    delete designator;
    designator_it = designator_list.begin();
  }
  for(;apply_it != sub_apply_list.end();) {
    XACMLApply* apply = (*apply_it).second;
    sub_apply_list.erase(apply_it);
    delete apply;
    apply_it = sub_apply_list.begin();
  }
}

std::list<AttributeValue*> XACMLApply::evaluate(EvaluationCtx* ctx) {
  std::list<AttributeValue*> list;
  std::list<AttributeValue*> attrlist;
  std::list<AttributeValue*> attrlist_to_remove;

  std::map<int, AttributeValue*>::iterator attrval_it;
  std::map<int, AttributeSelector*>::iterator selector_it;
  std::map<int, AttributeDesignator*>::iterator designator_it;
  std::map<int, XACMLApply*>::iterator apply_it;
  for(int i=0;;i++) {
    attrval_it = attrval_list.find(i);
    selector_it = selector_list.find(i);
    designator_it = designator_list.find(i);
    apply_it = sub_apply_list.find(i);

    if((attrval_it == attrval_list.end()) &&
       (selector_it == selector_list.end()) && 
       (designator_it == designator_list.end()) &&
       (apply_it == sub_apply_list.end()))
      break;

    if(attrval_it != attrval_list.end()) {
      attrlist.push_back((*attrval_it).second);
    }
    if(selector_it != selector_list.end()) {
      list = (*selector_it).second->evaluate(ctx);
      attrlist.insert(attrlist.end(), list.begin(), list.end());
      attrlist_to_remove.insert(attrlist_to_remove.end(), list.begin(), list.end());
    }
    if(designator_it != designator_list.end()) {
      list = (*designator_it).second->evaluate(ctx);
      attrlist.insert(attrlist.end(), list.begin(), list.end());
      attrlist_to_remove.insert(attrlist_to_remove.end(), list.begin(), list.end());
    }
    if(apply_it != sub_apply_list.end()) {
      list = (*apply_it).second->evaluate(ctx);
      attrlist.insert(attrlist.end(), list.begin(), list.end());
      attrlist_to_remove.insert(attrlist_to_remove.end(), list.begin(), list.end());
    }
  }

  //Evaluate
  std::list<AttributeValue*> res;
  try{
    std::cout<<"There are "<<attrlist.size()<<" attribute values to be evaluated"<<std::endl;
    res = function->evaluate(attrlist, false);
  } catch(std::exception&) { };

  while(!(attrlist_to_remove.empty())) {
    //Note that the attributes which are directly parsed 
    //from policy should not be deleted here.
    //Instead, they should be deleted by deconstructor of XACMLApply
    AttributeValue* val = attrlist_to_remove.back();
    attrlist_to_remove.pop_back();
    delete val;
  }

  return res;
}

