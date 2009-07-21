#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <arc/security/ClassLoader.h>

#include "XACMLRequest.h"

Arc::Logger ArcSec::XACMLRequest::logger(Arc::Logger::rootLogger, "XACMLRequest");

static Arc::NS reqns("request", "urn:oasis:names:tc:xacml:2.0:context:schema:os");

/** get_request (in charge of class-loading of XACMLRequest) can only accept two types of argument: NULL, XMLNode*/
Arc::Plugin* ArcSec::XACMLRequest::get_request(Arc::PluginArgument* arg) {
  if(arg==NULL) return NULL;
  else {
    Arc::ClassLoaderPluginArgument* clarg =
            arg?dynamic_cast<Arc::ClassLoaderPluginArgument*>(arg):NULL;
    if(!clarg) return NULL;
    Arc::XMLNode* xarg = (Arc::XMLNode*)(*clarg);
    if(xarg==NULL) { return new ArcSec::XACMLRequest(); } // ???
    ArcSec::Source source(*xarg);
    return new ArcSec::XACMLRequest(source);
  }
}

using namespace Arc;
using namespace ArcSec;

void XACMLRequest::make_request(){
  //Parse the XMLNode structure, and generate the RequestAttribute object
  if((!reqnode) || (reqnode.Size() == 0)) {
    logger.msg(ERROR,"Request is empty");
    return;
  }
  std::list<XMLNode> r = reqnode.XPathLookup("//request:Request", reqns);
  if(r.empty()) {
    logger.msg(ERROR,"Can not find <Request/> element with proper namespace");
    return;
  }
  XMLNode node = *(r.begin());

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
}
