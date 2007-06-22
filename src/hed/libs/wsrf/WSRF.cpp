#include "../ws-addressing/WSA.h"
#include "WSRF.h"

namespace Arc {

void WSRF::set_namespaces(void) {
  NS ns;
  ns["wsa"]="http://www.w3.org/2005/08/addressing";
  ns["wsrf-bf"]="http://docs.oasis-open.org/wsrf/bf-2";
  ns["wsrf-r"]="http://docs.oasis-open.org/wsrf/r-2";
  ns["wsrf-rw"]="http://docs.oasis-open.org/wsrf/rw-2";
  soap_.Namespaces(ns);
}

WSRF::WSRF(SOAPEnvelope& soap,const std::string& action):
             soap_(soap),allocated_(false),valid_(false) {
  if(!soap_) return;
  set_namespaces();
  if(!action.empty()) if(WSAHeader(soap).Action() != action) return;
  valid_=true;
}


WSRF::WSRF(bool fault,const std::string& action):
            soap_(*(new SOAPEnvelope(NS(),fault))),
            allocated_(true),valid_(false) {
  set_namespaces();
  if(!action.empty()) WSAHeader(soap_).Action(action);
  valid_=true;
}

WSRF::~WSRF(void) {
  if(allocated_) delete (&soap_);
}

} // namespace Arc 

