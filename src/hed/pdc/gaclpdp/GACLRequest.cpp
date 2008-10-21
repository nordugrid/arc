#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <iostream>

#include <arc/loader/ClassLoader.h>

#include "GACLRequest.h"
//#include "ArcRequestItem.h"

/** get_request (in charge of class-loading of ArcRequest) can only accept two types of argument: NULL, XMLNode*/
Arc::LoadableClass* ArcSec::GACLRequest::get_request(void* arg) {
  if(arg==NULL) {
    return new ArcSec::GACLRequest();
  } else {
    ArcSec::Source source(*((Arc::XMLNode*)arg));
    return new ArcSec::GACLRequest(source);
  }
}

//loader_descriptors __arc_request_modules__  = {
//    { "gacl.request", 0, &ArcSec::GACLRequest::get_request },
//    { NULL, 0, NULL }
//};


using namespace Arc;
using namespace ArcSec;

GACLRequest::GACLRequest (const Source& req) : Request(req) {
  req.Get().New(reqnode);
}

GACLRequest::GACLRequest () {
}

GACLRequest::~GACLRequest(){
}
