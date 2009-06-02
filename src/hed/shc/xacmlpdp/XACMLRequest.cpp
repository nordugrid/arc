#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/security/ClassLoader.h>

#include <fstream>
#include "XACMLRequest.h"
//#include "XACMLRequestItem.h"

/** get_request (in charge of class-loading of XACMLRequest) can only accept two types of argument: NULL, XMLNode*/
Arc::Plugin* ArcSec::XACMLRequest::get_request(Arc::PluginArgument* arg) {
  //std::cout<<"Argument type of XACMLRequest:"<<typeid(arg).name()<<std::endl;
  if(arg==NULL) return NULL;
  else {
    /*
    {
      std::string xml;
      ((Arc::XMLNode*)arg)->GetXML(xml);
      std::cout<<"node inside XACMLRequest:"<<xml<<std::endl;
    };
    */
    Arc::ClassLoaderPluginArgument* clarg =
            arg?dynamic_cast<Arc::ClassLoaderPluginArgument*>(arg):NULL;
    if(!clarg) return NULL;
    Arc::XMLNode* xarg = (Arc::XMLNode*)(*clarg);
    if(xarg==NULL) { return new ArcSec::XACMLRequest(); } // ???
    ArcSec::Source source(*xarg);
    return new ArcSec::XACMLRequest(source);
  }
}

//loader_descriptors __xacml_request_modules__  = {
//    { "xacml.request", 0, &get_xacmlrequest },
//    { NULL, 0, NULL }
//};


using namespace Arc;
using namespace ArcSec;

void XACMLRequest::make_request(){

  std::string xml;
  reqnode.GetDoc(xml);
  std::cout<<xml<<std::endl;

  //Parse the XMLNode structure, and generate the RequestAttribute object
  XMLNode node = reqnode["Request"];
  XMLNode nd, cnd;

  //Parse the <Subject> part
  //"Catagory" need to be processed
  std::string type;
  int i;
  nd = node["Subject"];
  if(!nd) {std::cerr<<"There is no subject element in request"<<std::endl; exit(0); }
  Subject sub;
  for(i = 0;; i++) {
    cnd = nd["Attribute"][i];
    if(!cnd) break;
    sub.push_back(new RequestAttribute(cnd, attrfactory));
  }

  //Parse the <Resource> part
  nd = node["Resource"];
  Resource res;
  for(i = 0;; i++) {
    cnd = nd["Attribute"][i];
    if(!cnd) break;
    res.push_back(new RequestAttribute(cnd, attrfactory));
  }

  //Parse the <Action> part
  nd = node["Action"];
  Action act;
  for(i = 0;; i++) {
    cnd = nd["Attribute"][i];
    if(!cnd) break;
    act.push_back(new RequestAttribute(cnd, attrfactory));
  }

  //Parse the <Environment> part
  nd = node["Environment"];
  Context env;
  for(i = 0;; i++) {
    cnd = nd["Attribute"][i];
    if(!cnd) break;
    env.push_back(new RequestAttribute(cnd, attrfactory));
  }

}




















/*
XACMLRequest::XACMLRequest(const char* filename) : Request(filename) {
  std::string str;
  std::string xml_str = "";
  std::ifstream f(filename);

  while (f >> str) {
    xml_str.append(str);
    xml_str.append(" ");
  }
  f.close();

  Arc::XMLNode node(xml_str);
  NS ns;
  ns["ra"]="urn:oasis:names:tc:xacm:2.0:context:schema:os";
  node.Namespaces(ns);
  node.New(reqnode);
}

XACMLRequest::XACMLRequest (const XMLNode* node) : Request(node) {
  node->New(reqnode);
}
*/

XACMLRequest::XACMLRequest (const Source& req) : Request(req) {
  req.Get().New(reqnode);
  NS ns;
  ns["ra"]="urn:oasis:names:tc:xacm:2.0:context:schema:os";
  reqnode.Namespaces(ns);
}

XACMLRequest::XACMLRequest () {
  NS ns;
  ns["ra"]="urn:oasis:names:tc:xacm:2.0:context:schema:os";
  XMLNode request(ns,"ra:Request");
  request.New(reqnode);
}

XACMLRequest::~XACMLRequest(){
  while(!(rlist.empty())){
    delete rlist.back();
    rlist.pop_back();
  }
}
