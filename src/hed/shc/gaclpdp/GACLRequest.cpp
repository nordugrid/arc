#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <iostream>

#include "GACLRequest.h"

/** get_request (in charge of class-loading of ArcRequest) can only accept two types of argument: NULL, XMLNode*/
Arc::Plugin* ArcSec::GACLRequest::get_request(Arc::PluginArgument* arg) {
  if(arg==NULL) return NULL;
  Arc::ClassLoaderPluginArgument* clarg =
          arg?dynamic_cast<Arc::ClassLoaderPluginArgument*>(arg):NULL;
  if(!clarg) return NULL;
  Arc::XMLNode* xarg = (Arc::XMLNode*)(*clarg);
  if(xarg == NULL) return new ArcSec::GACLRequest(arg);
  ArcSec::Source source(*xarg);
  return new ArcSec::GACLRequest(source,arg);
}

//loader_descriptors __arc_request_modules__  = {
//    { "gacl.request", 0, &ArcSec::GACLRequest::get_request },
//    { NULL, 0, NULL }
//};


using namespace Arc;
using namespace ArcSec;

GACLRequest::GACLRequest (const Source& req, Arc::PluginArgument* parg) : Request(req,parg) {
  req.Get().New(reqnode);
}

GACLRequest::GACLRequest (Arc::PluginArgument* parg) : Request(parg) {
}

GACLRequest::~GACLRequest(){
}
