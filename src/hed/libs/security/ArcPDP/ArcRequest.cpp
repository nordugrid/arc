#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include "ArcRequest.h"
#include "ArcRequestItem.h"

using namespace Arc;
using namespace ArcSec;

ReqItemList ArcRequest::getRequestItems () const {
  return rlist;
}

void ArcRequest::setRequestItems (ReqItemList sl){
  rlist = sl;
}

void ArcRequest::addRequestItem(Attrs& sub, Attrs& res, Attrs& act, Attrs& ctx){
  NS ns;
  ns["ra"]="http://www.nordugrid.org/ws/schemas/request-arc";
  XMLNode reqdoc(ns);
  XMLNode request = reqdoc.NewChild("ra:Request");
  request.Namespaces(ns);
  XMLNode requestitem = request.NewChild("ra:RequestItem");

  int n;
  XMLNode subject = requestitem.NewChild("ra:Subject");
  n = sub.size();
  Attrs::iterator it;
  if(n>1) {
    for(it = sub.begin(); it != sub.end(); it++)
    {
      XMLNode subjectattr = subject.NewChild("ra:Attribute");
      subjectattr = (*it).value;
      XMLNode subjectattr_attr = subjectattr.NewAttribute("ra:Type");
      subjectattr_attr = (*it).type;
    }
  }
  else{
    it = sub.begin();
    XMLNode subject_attr = subject.NewAttribute("ra:Type");
    subject = (*it).value;
    subject_attr = (*it).type;
  }

  XMLNode resource = requestitem.NewChild("ra:Resource");
  n = res.size();
  if(n>1) {
    for(it = res.begin(); it != res.end(); it++)
    {
      XMLNode resourceattr = resource.NewChild("ra:Attribute");
      resourceattr = (*it).value;
      XMLNode resourceattr_attr = resourceattr.NewAttribute("ra:Type");
      resourceattr_attr = (*it).type;
    }
  }
  else{
    it = res.begin();
    XMLNode resource_attr = resource.NewAttribute("ra:Type");
    resource = (*it).value;
    resource_attr = (*it).type;
  }

  XMLNode action = requestitem.NewChild("ra:Action");
  n = act.size();
  if(n>1) {
    for(it = act.begin(); it != act.end(); it++)
    {
      XMLNode actionattr = action.NewChild("ra:Attribute");
      actionattr = (*it).value;
      XMLNode actionattr_attr = actionattr.NewAttribute("ra:Type");
      actionattr_attr = (*it).type;
    }
  }
  else{
    it = act.begin();
    XMLNode action_attr = action.NewAttribute("ra:Type");
    action = (*it).value;
    action_attr = (*it).type;
  }

  XMLNode context = requestitem.NewChild("ra:Context");
  n = ctx.size();
  if(n>1) {
    for(it = ctx.begin(); it != ctx.end(); it++)
    {
      XMLNode contextattr = context.NewChild("ra:Attribute");
      contextattr = (*it).value;
      XMLNode contextattr_attr = contextattr.NewAttribute("ra:Type");
      contextattr_attr = (*it).type;
    }
  }
  else{
    it = ctx.begin();
    XMLNode context_attr = context.NewAttribute("ra:Type");
    context = (*it).value;
    context_attr = (*it).type;
  }

  rlist.push_back(new ArcRequestItem(requestitem, attrfactory));
}

void ArcRequest::make_request(XMLNode& node){
  Arc::NS nsList;

  nsList.insert(std::pair<std::string, std::string>("request","http://www.nordugrid.org/ws/schemas/request-arc"));
//  std::list<XMLNode> reqlist = node.XPathLookup("//request:Request", nsList);

  std::list<XMLNode>::iterator itemit;
//  if(!(reqlist.empty())){
    std::list<XMLNode> itemlist = node.XPathLookup("//request:RequestItem", nsList);
    for ( itemit=itemlist.begin() ; itemit != itemlist.end(); itemit++ ){
      XMLNode itemnd=*itemit;
      rlist.push_back(new ArcRequestItem(itemnd, attrfactory));
    }
//  }
}

ArcRequest::ArcRequest(const std::string& filename, AttributeFactory* attrfactory) : Request(filename), attrfactory(attrfactory) {
  std::string str;
  std::string xml_str = "";
  std::ifstream f(filename.c_str());

  //std::cout<<filename<<std::endl;
  while (f >> str) {
    xml_str.append(str);
    xml_str.append(" ");
  }
  f.close();

  Arc::XMLNode node(xml_str);
  make_request(node);
}

ArcRequest::ArcRequest (XMLNode& node, AttributeFactory* attrfactory) : Request(node), attrfactory(attrfactory) {
  make_request(node);
}

ArcRequest::~ArcRequest(){
  while(!(rlist.empty())){
    delete rlist.back();
    rlist.pop_back();
  }
}
