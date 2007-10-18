#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/ClassLoader.h>

#include <fstream>
#include "ArcRequest.h"
#include "ArcRequestItem.h"

/** get_request (in charge of class-loading of ArcRequest) can only accept two types of argument: NULL, XMLNode*/

static Arc::LoadableClass* get_request(void** arg) {
    //std::cout<<"Argument type of ArcRequest:"<<typeid(arg).name()<<std::endl;
    if(arg==NULL) return new ArcSec::ArcRequest();
    else{
    /*std::cout<<"Inside ArcRequest"<<std::endl;
    std::string xml;
    Arc::XMLNode node(*((Arc::XMLNode*)arg));
    node.GetXML(xml);
    std::cout<<"node inside ArcRequest:"<<xml<<std::endl;*/
    return new ArcSec::ArcRequest((Arc::XMLNode*) arg);
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

  nsList.insert(std::pair<std::string, std::string>("request","http://www.nordugrid.org/ws/schemas/request-arc"));
//  std::list<XMLNode> reqlist = node.XPathLookup("//request:Request", nsList);

  std::list<XMLNode>::iterator itemit;
//  if(!(reqlist.empty())){
    std::list<XMLNode> itemlist = reqnode.XPathLookup("//request:RequestItem", nsList);
    for ( itemit=itemlist.begin() ; itemit != itemlist.end(); itemit++ ){
      XMLNode itemnd=*itemit;
      rlist.push_back(new ArcRequestItem(itemnd, attrfactory));
    }
//  }
}

ArcRequest::ArcRequest(const char* filename) : Request(filename) {
  std::string str;
  std::string xml_str = "";
  std::ifstream f(filename);

  //std::cout<<filename<<std::endl;
  while (f >> str) {
    xml_str.append(str);
    xml_str.append(" ");
  }
  f.close();

  Arc::XMLNode node(xml_str);
  NS ns;
  ns["ra"]="http://www.nordugrid.org/ws/schemas/request-arc";
  node.Namespaces(ns);
  node.New(reqnode);
}

ArcRequest::ArcRequest (const XMLNode* node) : Request(node) {
  node->New(reqnode);
}

ArcRequest::ArcRequest () {
  NS ns;
  ns["ra"]="http://www.nordugrid.org/ws/schemas/request-arc";
  XMLNode request(ns,"ra:Request");
  request.New(reqnode);
}

ArcRequest::~ArcRequest(){
  while(!(rlist.empty())){
    delete rlist.back();
    rlist.pop_back();
  }
}
