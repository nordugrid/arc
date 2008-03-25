#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "AttributeDesignator.h"

using namespace Arc;
using namespace ArcSec;

AttributeDesignator::AttributeDesignator(Arc::XMLNode& node) : present(false) {
  std::string name = node.Name();
  size_t found = name.find_first_of("AttributeDesignator");
  target = name.substr(0, found);

  id = (std::string)(node.Attribute("AttributeId")); 
  if(id.empty()) {std::cerr<<"Required AttributeId does not exist in AttributeDesignator"<<std::endl; exit(0);}

  std::string tp = (std::string)(node.Attribute("DataType"));
  if(tp.empty()) {std::cerr<<"Required DataType does not exist in AttributeDesignator"<<std::endl; exit(0);}
  size_t found = tp.find_last_of("#");
  type = tp.substr(found+1);

  issuer = (std::string)(node.Attribute("Issuer"));

  if(target=="Subject") {
    std::string category = node.Attribute("SubjectCategory");
    if(category.empty()) category = "urn:oasis:names:tc:xacml:1.0:subject-category:access-subject";
  }
  
  std::string must = node.Attribute("MustBePresent");
  if(!must.empty()) present = true;
  
}

AttributeDesignator::~AttributeDesignator() {
}

std::list<AttributeValue*> AttributeDesignator::evaluate(EvaluationCtx* ctx) {
  std::list<AttributeValue*> res;
  if(target == "Subject") { res = ctx->getSubjectAttributes(id, type, issuer, category); } 
  else if(target == "Resource") { res = ctx->getResourceAttributes(id, type, issuer); }
  else if(target == "Action") { res = ctx->getActionAttributes(id, type, issuer); }
  else if(target == "Environment") { res = ctx->getContextAttributes(id, type, issuer); }

  if(present && (res.size() ==0)) {
    std::cerr<<"AttributeDesignator requires at least one attributes from request's"<<target<<std::endl;
  }

  return res;
}


