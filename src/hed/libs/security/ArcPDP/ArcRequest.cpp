#include "ArcRequest.h"

ReqItemList ArcRequest::getRequestItems () const {
  return rlist;
}

void ArcRequest::setRequestItems (const ReqItemList& sl){
  rlist = sl;
}


static void make_request(const XMLNode& node){
  Arc::NS nsList;
  nsList.insert(std::pair<std::string, std::string>("request","http://www.nordugrid.org/ws/schemas/request-arc"));
  std::list<Arc::XMLNode> reqlist = node.XPathLookup("//request:Request", nsList);
 
  std::list<Arc::XMLNode>::iterator itemit;
  
  if(!(reqlist.empty())){
    std::list<Arc::XMLNode> itemlist = node.XPathLookup("//request:RequestItem", nsList);
    for ( itemit=itemlist.begin() ; itemit != itemlist.end(); itemit++ ){
      XMLNode itemnd=*itemit;
      rlist.push_back(new Arc::ArcRequestItem(itemnd));
    }
  }
}

ArcRequest::ArcRequest (const std::ifstream& input){
  std::string xml_str = "";
  while (input >> str) {
    xml_str.append(str);
    xml_str.append(" ");
  }
  Arc::XMLNode node(xml_str);
  make_request(node);
}

ArcRequest::ArcRequest (const Arc::XMLNode& node){
  make_request(node);
}

ArcRequest::~ArcRequest{
  while(!rlist.empty()){
    delete rlist.back();
    rlist.pop_back();
  }
}
