#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/security/ArcPDP/attr/AttributeValue.h>

#include "XACMLEvaluationCtx.h"

using namespace Arc;
using namespace ArcSec;

Logger XACMLEvaluationCtx::logger(Arc::Logger::rootLogger, "XACMLEvaluationCtx");

XACMLEvaluationCtx::XACMLEvaluationCtx(Request* request) : EvaluationCtx(request), req(NULL) {
  req = request;
  XMLNode req_node = req->getReqNode();
}

XACMLEvaluationCtx::~XACMLEvaluationCtx(){

}

Request* XACMLEvaluationCtx::getRequest () const{
  return req;
}

std::list<AttributeValue*> XACMLEvaluationCtx::getAttributes(std::string& reqctxpath, 
       Arc::XMLNode& namespaceNode, std::string& data_type, AttributeFactory* attrfactory)  { 
  std::list<AttributeValue*> attrlist; 

  XMLNode req_node = req->getReqNode();
  NS nsList;
  nsList = namespaceNode.Namespaces();
  
  std::string path;
  //If the xPath string is a relative one
  if(reqctxpath.find_first_of("/") != 0) {
    std::string name = req_node.Name();
    std::string name_space = req_node.Namespace();
    if(name_space.empty())
      //no namespace in the request 
      path = "//" + name + "/";
    else {
      //namespaces are used in the request. lookup the correct prefix 
      //for using in the search string
      for (NS::const_iterator ns = nsList.begin(); ns != nsList.end(); ++ns) {
        std::string ns_val = ns->second;
        if(ns_val == name_space) {
          std::string ns_name = ns->first;
          if(ns_name.empty()) path = "//";
          else path = "//" + ns_name;
          path  = path + ":" + name + "/";
          break;
        }
      }
      if(path.empty()) std::cout<<"Failed to map a namespace into an XPath expression"<<std::endl;
    }
  }

  path = path + reqctxpath;

  std::size_t pos = path.rfind("/text()");
  if(pos!=std::string::npos)
    path = path.substr(0, pos);

  std::list<Arc::XMLNode> list = req_node.XPathLookup(path, nsList);
  std::list<Arc::XMLNode>::iterator it;

  for (it = list.begin(); it != list.end(); it++) {
    std::cout << (*it).FullName() << ":" << (std::string)(*it) << std::endl;
    AttributeValue* attr = NULL;
    std::string type;
    std::size_t f = data_type.find_last_of("#"); //http://www.w3.org/2001/XMLSchema#string
    if(f!=std::string::npos) {
      type = data_type.substr(f+1);
    }
    else {
      f=data_type.find_last_of(":"); //urn:oasis:names:tc:xacml:1.0:data-type:rfc822Name
      type = data_type.substr(f+1);
    }
    attr = attrfactory->createValue((*it), type);
    attrlist.push_back(attr);
  }

  return attrlist; 
}

std::list<AttributeValue*> XACMLEvaluationCtx::getSubjectAttributes(std::string& id, 
       std::string& type, std::string& issuer, std::string& category, AttributeFactory* attrfactory) {
  std::list<AttributeValue*> attrlist;
  XMLNode req_node = req->getReqNode();
  for(int i = 0;; i++) {
    XMLNode attr_nd = req_node["Subject"]["Attribute"][i];
    std::string sub_category = req_node["Subject"].Attribute("SubjectCategory");
    if(sub_category.empty()) sub_category = "urn:oasis:names:tc:xacml:1.0:subject-category:access-subject";
    if(!attr_nd) break;
    std::string attr_id = attr_nd.Attribute("AttributeId");
    std::string attr_type = attr_nd.Attribute("DataType");
    std::string attr_issuer = attr_nd.Attribute("Issuer");

std::cout<<attr_id<<"  "<<attr_type<<"  "<<attr_issuer<<std::endl;
std::cout<<id<<"  "<<type<<"  "<<issuer<<std::endl;

    if(attr_id.empty()) continue;
    if(attr_type.empty()) attr_type = "xs:string";
    if((id == attr_id) && (issuer.empty() || 
      (!attr_issuer.empty() && (attr_issuer==issuer)))) {
      //If category does not match
      if(!category.empty() && sub_category!=category) continue;
      //Create the object for attribute
      AttributeValue* attr = NULL;
      std::string tp;
      std::size_t f = attr_type.find_last_of("#"); //http://www.w3.org/2001/XMLSchema#string
      if(f!=std::string::npos) {
        tp = attr_type.substr(f+1);
      }
      else {
        f=attr_type.find_last_of(":"); //urn:oasis:names:tc:xacml:1.0:data-type:rfc822Name
        tp = attr_type.substr(f+1);
      }
      attr = attrfactory->createValue(attr_nd, tp);
      attrlist.push_back(attr);
    }
  }
  return attrlist;
}

std::list<AttributeValue*> XACMLEvaluationCtx::getAttributesHelper(std::string& id,
       std::string& type, std::string& issuer, AttributeFactory* attrfactory, const std::string& target_class) {
  std::list<AttributeValue*> attrlist;
  XMLNode req_node = req->getReqNode();
  for(int i = 0;; i++) {
    XMLNode attr_nd = req_node[target_class]["Attribute"][i];
    if(!attr_nd) break;
    std::string attr_id = attr_nd.Attribute("AttributeId");
    std::string attr_type = attr_nd.Attribute("DataType");
    std::string attr_issuer = attr_nd.Attribute("Issuer");

std::cout<<attr_id<<"  "<<attr_type<<"  "<<attr_issuer<<std::endl;
std::cout<<id<<"  "<<type<<"  "<<issuer<<std::endl;

    if(attr_id.empty()) continue;
    if(attr_type.empty()) attr_type = "xs:string";
    if((id == attr_id) && (issuer.empty() ||
      (!attr_issuer.empty() && (attr_issuer==issuer)))) {
      AttributeValue* attr = NULL;
      std::string tp;
      std::size_t f = attr_type.find_last_of("#"); //http://www.w3.org/2001/XMLSchema#string
      if(f!=std::string::npos) {
        tp = attr_type.substr(f+1);
      }
      else {
        f=attr_type.find_last_of(":"); //urn:oasis:names:tc:xacml:1.0:data-type:rfc822Name
        tp = attr_type.substr(f+1);
      }
      attr = attrfactory->createValue(attr_nd, tp);
      attrlist.push_back(attr);
    }
  }
  return attrlist;
}

std::list<AttributeValue*> XACMLEvaluationCtx::getResourceAttributes(std::string& id, 
       std::string& type, std::string& issuer, AttributeFactory* attrfactory) {
  return getAttributesHelper(id, type, issuer, attrfactory, std::string("Resource"));
}

std::list<AttributeValue*> XACMLEvaluationCtx::getActionAttributes(std::string& id, 
       std::string& type, std::string& issuer, AttributeFactory* attrfactory) {
  return getAttributesHelper(id, type, issuer, attrfactory, std::string("Action"));
}

std::list<AttributeValue*> XACMLEvaluationCtx::getContextAttributes(std::string& id, 
       std::string& type, std::string& issuer, AttributeFactory* attrfactory) {
  return getAttributesHelper(id, type, issuer, attrfactory, std::string("Environment"));
}

