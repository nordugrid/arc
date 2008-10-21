#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <iostream>

#include <arc/loader/ClassLoader.h>

#include "ArcRequest.h"
#include "ArcRequestItem.h"

/** get_request (in charge of class-loading of ArcRequest) can only accept two types of argument: NULL, XMLNode*/
static Arc::LoadableClass* get_request(void* arg) {
  //std::cout<<"Argument type of ArcRequest:"<<typeid(arg).name()<<std::endl;
  if(arg==NULL) { return new ArcSec::ArcRequest(); }
  else {
    /*
    {
      std::string xml;
      ((Arc::XMLNode*)arg)->GetXML(xml);
      std::cout<<"node inside ArcRequest:"<<xml<<std::endl;
    };
    */
    ArcSec::Source source(*(Arc::XMLNode*)arg);
    return new ArcSec::ArcRequest(source);
  }
}

loader_descriptors __arc_request_modules__  = {
    { "arc.request", 0, &get_request },
    { NULL, 0, NULL }
};


using namespace Arc;
using namespace ArcSec;

ReqItemList ArcRequest::getRequestItems () const {
  return rlist;
}

void ArcRequest::setRequestItems (ReqItemList sl){
  rlist = sl;
}

void ArcRequest::addRequestItem(Attrs& sub, Attrs& res, Attrs& act, Attrs& ctx){
  //Add a new RequestItem into reqnode 
  XMLNode request = reqnode;
  XMLNode requestitem = request.NewChild("ra:RequestItem");

  XMLNode subject = requestitem.NewChild("ra:Subject");
  int i;
  int size = sub.size();
  if(size>1) {
    for(i = 0; i < size; i++ )
    {
      XMLNode subjectattr = subject.NewChild("ra:Attribute");
      subjectattr = sub[i].value;
      XMLNode subjectattr_attr = subjectattr.NewAttribute("ra:Type");
      subjectattr_attr = sub[i].type;
    }
  }
  else{
    XMLNode subject_attr = subject.NewAttribute("ra:Type");
    subject = sub[0].value;
    subject_attr = sub[0].type;
  }

  XMLNode resource = requestitem.NewChild("ra:Resource");
  size = res.size();
  if(size>1) {
    for(i = 0; i< size; i++)
    {
      XMLNode resourceattr = resource.NewChild("ra:Attribute");
      resourceattr = res[i].value;
      XMLNode resourceattr_attr = resourceattr.NewAttribute("ra:Type");
      resourceattr_attr = res[i].type;
    }
  }
  else{
    XMLNode resource_attr = resource.NewAttribute("ra:Type");
    resource = res[0].value;
    resource_attr = res[0].type;
  }

  XMLNode action = requestitem.NewChild("ra:Action");
  size = act.size();
  if(size>1) {
    for(i = 0; i < size; i++)
    {
      XMLNode actionattr = action.NewChild("ra:Attribute");
      actionattr = act[i].value;
      XMLNode actionattr_attr = actionattr.NewAttribute("ra:Type");
      actionattr_attr = act[i].type;
    }
  }
  else{
    XMLNode action_attr = action.NewAttribute("ra:Type");
    action = act[0].value;
    action_attr = act[0].type;
  }

  XMLNode context = requestitem.NewChild("ra:Context");
  size = ctx.size();
  if(size>1) {
    for(i = 0; i < size; i++)
    {
      XMLNode contextattr = context.NewChild("ra:Attribute");
      contextattr = ctx[i].value;
      XMLNode contextattr_attr = contextattr.NewAttribute("ra:Type");
      contextattr_attr = ctx[i].type;
    }
  }
  else{
    XMLNode context_attr = context.NewAttribute("ra:Type");
    context = ctx[0].value;
    context_attr = ctx[0].type;
  }

  std::string xml;
  reqnode.GetDoc(xml);
  std::cout<<xml<<std::endl; 

}

void ArcRequest::make_request(){
  Arc::NS nsList;

  nsList.insert(std::pair<std::string, std::string>("request","http://www.nordugrid.org/schemas/request-arc"));

  std::list<XMLNode>::iterator itemit;
    std::list<XMLNode> itemlist = reqnode.XPathLookup("//request:RequestItem", nsList);
    for ( itemit=itemlist.begin() ; itemit != itemlist.end(); itemit++ ){
      XMLNode itemnd=*itemit;
      //Generate a new ArcRequestItem, which will generate RequestAttribute object 
      rlist.push_back(new ArcRequestItem(itemnd, attrfactory));
    }
}

std::string ArcRequest::getEvalName() const{
  return "arc.evaluator";
}

ArcRequest::ArcRequest (const Source& req) : Request(req) {
  req.Get().New(reqnode);
  NS ns;
  ns["ra"]="http://www.nordugrid.org/schemas/request-arc";
  reqnode.Namespaces(ns);
}

ArcRequest::ArcRequest () {
  NS ns;
  ns["ra"]="http://www.nordugrid.org/schemas/request-arc";
  XMLNode request(ns,"ra:Request");
  request.New(reqnode);
}

ArcRequest::~ArcRequest(){
  while(!(rlist.empty())){
    delete rlist.back();
    rlist.pop_back();
  }
}
