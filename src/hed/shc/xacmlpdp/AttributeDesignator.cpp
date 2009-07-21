#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "AttributeDesignator.h"

using namespace Arc;
using namespace ArcSec;

AttributeDesignator::AttributeDesignator(Arc::XMLNode& node, AttributeFactory* attr_factory) 
    : present(false), attrfactory(attr_factory) {
  std::string name = node.Name();
  size_t found = name.find("AttributeDesignator");
  target = name.substr(0, found);

  id = (std::string)(node.Attribute("AttributeId")); 
  if(id.empty()) {std::cerr<<"Required AttributeId does not exist in AttributeDesignator"<<std::endl; exit(0);}

  std::string tp = (std::string)(node.Attribute("DataType"));
  if(tp.empty()) {std::cerr<<"Required DataType does not exist in AttributeDesignator"<<std::endl; exit(0);}

  type = tp;
  
#if 0
  found = tp.find_last_of("#");
  if(found!=std::string::npos) {
    type = tp.substr(found+1);
  }
  else {
    found=tp.find_last_of(":"); //urn:oasis:names:tc:xacml:1.0:data-type:rfc822Name
    type = tp.substr(found+1);
  }
#endif

  issuer = (std::string)(node.Attribute("Issuer"));

  if(target=="Subject") {
    category = (std::string)(node.Attribute("SubjectCategory"));
    if(category.empty()) category = "urn:oasis:names:tc:xacml:1.0:subject-category:access-subject";
  }
  
  std::string must = node.Attribute("MustBePresent");
  if(!must.empty()) present = true;
}

AttributeDesignator::~AttributeDesignator() {
}

std::list<AttributeValue*> AttributeDesignator::evaluate(EvaluationCtx* ctx) {
  std::list<AttributeValue*> res;
  if(target == "Subject") { res = ctx->getSubjectAttributes(id, type, issuer, category, attrfactory); } 
  else if(target == "Resource") { res = ctx->getResourceAttributes(id, type, issuer, attrfactory); }
  else if(target == "Action") { res = ctx->getActionAttributes(id, type, issuer, attrfactory); }
  else if(target == "Environment") { res = ctx->getContextAttributes(id, type, issuer, attrfactory); }

  if(present && (res.size() ==0)) {
    std::cerr<<"AttributeDesignator requires at least one attributes from request's"<<target<<std::endl;
  }

  return res;
}


