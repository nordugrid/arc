#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "XACMLEvaluationCtx.h"
#include "AttributeSelector.h"

using namespace Arc;
using namespace ArcSec;

AttributeSelector::AttributeSelector(Arc::XMLNode& node, AttributeFactory* attr_factory) : present(false), 
    attrfactory(attr_factory) {
  std::string tp = (std::string)(node.Attribute("DataType"));
  if(tp.empty()) {std::cerr<<"Required DataType does not exist in AttributeSelector"<<std::endl; exit(0);}

  type = tp;

#if 0
  size_t found = tp.find_last_of("#");
  if(found!=std::string::npos) {
    type = tp.substr(found+1);
  }
  else {
    found=tp.find_last_of(":"); //urn:oasis:names:tc:xacml:1.0:data-type:rfc822Name
    type = tp.substr(found+1);
  }
#endif

  reqctxpath = (std::string)(node.Attribute("RequestContextPath"));
  if(reqctxpath.empty()) {std::cerr<<"Required RequestContextPath does not exist in AttributeSelector"<<std::endl; exit(0);} 

std::cout<<"=====!!!!!  "<<reqctxpath<<std::endl;

  
  std::string must = node.Attribute("MustBePresent");
  if(!must.empty()) present = true;

  //get the <Policy> node
  //TODO: need to duplicate node maybe
  policyroot = node.GetRoot(); 

}

AttributeSelector::~AttributeSelector() {
}

std::list<AttributeValue*> AttributeSelector::evaluate(EvaluationCtx* ctx) {
  std::list<AttributeValue*> res;
  res = ctx->getAttributes(reqctxpath, policyroot, type, attrfactory);
  
  if(present && (res.size() ==0)) {
    //std::cerr<<"AttributeSelector requires at least one attributes from request's"<<target<<std::endl;
  }

  return res;
}


