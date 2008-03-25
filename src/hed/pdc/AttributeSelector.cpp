#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "AttributeSelector.h"

using namespace Arc;
using namespace ArcSec;

AttributeSelector::AttributeSelector(Arc::XMLNode& node) : present(false) {
  std::string tp = (std::string)(node.Attribute("DataType"));
  if(tp.empty()) {std::cerr<<"Required DataType does not exist in AttributeSelector"<<std::endl; exit(0);}
  size_t found = tp.find_last_of("#");
  type = tp.substr(found+1);

  reqctxpath = (std::string)(node.Attribute("RequestContextPath"));
  if(reqctxpath.empty()) {std::cerr<<"Required RequestContextPath does not exist in AttributeSelector"<<std::endl; exit(0);} 
  
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
  
  res = ctx->getAttributes(reqctxpath, policyroot, type, xpathver);
  
  if(present && (res.size() ==0)) {
    std::cerr<<"AttributeSelector requires at least one attributes from request's"<<target<<std::endl;
  }

  return res;
}


