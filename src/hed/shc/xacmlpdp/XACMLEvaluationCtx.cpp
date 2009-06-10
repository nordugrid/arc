#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/security/ArcPDP/attr/AttributeValue.h>

#include "XACMLEvaluationCtx.h"

using namespace Arc;
using namespace ArcSec;

Logger XACMLEvaluationCtx::logger(Arc::Logger::rootLogger, "XACMLEvaluationCtx");

#if 0
ArcRequestTuple::ArcRequestTuple() : RequestTuple() {
  NS ns;
  ns["ra"]="http://www.nordugrid.org/schemas/request-arc";
  XMLNode tupledoc(ns,"ra:RequestItem");
  tupledoc.New(tuple);
}

RequestTuple* ArcRequestTuple::duplicate(const RequestTuple* req_tpl) {  
  XMLNode root = tuple;
  int n;
  //ArcRequestTuple* tpl = dynamic_cast<ArcRequestTuple*>(req_tpl);

  //Reconstruct the XMLNode by using the information inside req_tpl

  //Reconstruct subject 
  Subject::iterator sit;
  Subject req_sub = req_tpl->sub;
  XMLNode subject;
  if(!req_sub.empty())
    subject = root.NewChild("ra:Subject");
  n = req_sub.size();
  for(sit = req_sub.begin(); sit != req_sub.end(); sit++){
    //Record the object of the Attribute
    RequestAttribute* attr = new RequestAttribute;
    attr->duplicate(*(*sit));
    sub.push_back(attr); 
     
    //Record the xml node of the Attribute
    XMLNode subjectattr = subject.NewChild("ra:Attribute");
    subjectattr = ((*sit)->getAttributeValue())->encode();
    XMLNode subjectattr_attr = subjectattr.NewAttribute("ra:Type");
    subjectattr_attr = ((*sit)->getAttributeValue())->getType();
    subjectattr_attr = subjectattr.NewAttribute("ra:AttributeId");
    subjectattr_attr = ((*sit)->getAttributeValue())->getId();

/*  AttributeValue *attrval;
    attrval = (*sit)->getAttributeValue();
    if(attrval) std::cout<< "Attribute Value:"<< (attrval->encode()).c_str() << std::endl;
*/           
  }
  
  //Reconstruct resource
  Resource::iterator rit;
  Resource req_res = req_tpl->res;
  XMLNode resource;
  if(!req_res.empty())
    resource = root.NewChild("ra:Resource");
  n = req_res.size();
  for(rit = req_res.begin(); rit != req_res.end(); rit++){
    RequestAttribute* attr = new RequestAttribute;
    attr->duplicate(*(*rit));
    res.push_back(attr);

    XMLNode resourceattr = resource.NewChild("ra:Attribute");
    resourceattr = ((*rit)->getAttributeValue())->encode();
    XMLNode resourceattr_attr = resourceattr.NewAttribute("ra:Type");
    resourceattr_attr = ((*rit)->getAttributeValue())->getType();
    resourceattr_attr = resourceattr.NewAttribute("ra:AttributeId");
    resourceattr_attr = ((*rit)->getAttributeValue())->getId();
  }

  //Reconstruct action
  Action::iterator ait;
  Action req_act = req_tpl->act;
  XMLNode action;
  if(!req_act.empty())
    action = root.NewChild("ra:Action");
  n = req_act.size();
  for(ait = req_act.begin(); ait != req_act.end(); ait++){
    RequestAttribute* attr = new RequestAttribute;
    attr->duplicate(*(*ait));
    act.push_back(attr);

    XMLNode actionattr = action.NewChild("ra:Attribute");
    actionattr = ((*ait)->getAttributeValue())->encode();
    XMLNode actionattr_attr = actionattr.NewAttribute("ra:Type");
    actionattr_attr = ((*ait)->getAttributeValue())->getType();
    actionattr_attr = actionattr.NewAttribute("ra:AttributeId");
    actionattr_attr = ((*ait)->getAttributeValue())->getId();
  }

  //Reconstruct context
  Context::iterator cit;
  Context req_ctx = req_tpl->ctx;
  XMLNode context;
  if(!req_ctx.empty())
    context = root.NewChild("ra:Context");
  n = req_ctx.size();
  for(cit = req_ctx.begin(); cit != req_ctx.end(); cit++){
    RequestAttribute* attr = new RequestAttribute;
    attr->duplicate(*(*cit));
    ctx.push_back(attr);

    XMLNode contextattr = context.NewChild("ra:Attribute");
    contextattr = ((*cit)->getAttributeValue())->encode();
    XMLNode contextattr_attr = contextattr.NewAttribute("ra:Type");
    contextattr_attr = ((*cit)->getAttributeValue())->getType();
    contextattr_attr = contextattr.NewAttribute("ra:AttributeId");
    contextattr_attr = ((*cit)->getAttributeValue())->getId();
  }

  return this;
}

void ArcRequestTuple::erase() {
  while(!(sub.empty())){
    delete sub.back();
    sub.pop_back();
  }

  while(!(res.empty())){
    delete res.back();
    res.pop_back();
  }

  while(!(act.empty())){
    delete act.back();
    act.pop_back();
  }

  while(!(ctx.empty())){
    delete ctx.back();
    ctx.pop_back();
  }
}

ArcRequestTuple::~ArcRequestTuple() {
  while(!(sub.empty())){
    sub.pop_back();
  }

  while(!(res.empty())){
    res.pop_back();
  }

  while(!(act.empty())){
    act.pop_back();
  }

  while(!(ctx.empty())){
    ctx.pop_back();
  }
}
#endif

XACMLEvaluationCtx::XACMLEvaluationCtx(Request* request) : req(NULL), EvaluationCtx(req) {
  req = request;
}

XACMLEvaluationCtx::~XACMLEvaluationCtx(){
#if 0
  while(!(reqtuples.empty())) {
    delete reqtuples.back();
    reqtuples.pop_back();
  } 
#endif
}

Request* XACMLEvaluationCtx::getRequest () const{
  return req;
}

std::list<AttributeValue*> XACMLEvaluationCtx::getAttributes(std::string& reqctxpath, 
       Arc::XMLNode& policy, std::string& data_type, AttributeFactory* attrfactory)  { 
  std::list<AttributeValue*> attrlist; 

  XMLNode req_node = req->getReqNode();
  NS nsList;
  //nsList["xacml-context"] = "urn:oasis:names:tc:xacm:2.0:context:schema:os";

  XMLNode ns_nd = ((bool)policy) ? policy : req_node;

  std::string path;
  //If the xPath string is a relative one
  if(reqctxpath.find_first_of("/") != 0) {
    std::string name = req_node.Name();
    std::string name_space = req_node.Namespace();
    if(name_space.empty()) 
      path = "/" + name + "/";
    else {
      for(int k = 0; k<policy.AttributesSize(); k++) {
        XMLNode attr = policy.Attribute(k);
        std::string attr_val = (std::string)attr;
        if(attr_val == name_space) {
          std::string attr_name = attr.Name();
          if(attr_name.empty()) path = "/";
          else path = "/" + attr_name;
          path  = path + ":" + name + "/";
          nsList[attr_name] = attr_val;
          break;
        }
      }
      if(path.empty()) std::cout<<"Failed to map a namespace into an XPath expression"<<std::endl;
    }
  }

  path = path + reqctxpath;

  std::list<Arc::XMLNode> list = req_node.XPathLookup(path, nsList);
  std::list<Arc::XMLNode>::iterator it;

  for (it = list.begin(); it != list.end(); it++) {
    std::cout << (*it).FullName() << ":" << (std::string)(*it) << std::endl;
    AttributeValue* attr = NULL;
    std::size_t f = data_type.find_last_of("#");
    std::string type = data_type.substr(f+1);
    attr = attrfactory->createValue((*it), type);
    attrlist.push_back(attr);
  }

  return attrlist; 
}
