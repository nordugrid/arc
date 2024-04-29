#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/ws-addressing/WSA.h>

#include "arex.h"
#include "tools.h"


namespace ARex {

  /*
  UnknownActivityIdentifierFault
    Message (string)

  InvalidRequestMessageFaultType
    InvalidElement (string,unbounded)
    Message (string)

  */

// A-REX faults

static const std::string BES_FACTORY_FAULT_URL("http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/Fault");

static void SetFaultResponse(Arc::SOAPFault& fault) {
  // Fetch top element of SOAP message - should be better way
  Arc::XMLNode fault_node = fault;
  Arc::SOAPEnvelope res(fault_node.Parent().Parent()); // Fault->Body->Envelope
  Arc::WSAHeader(res).Action(BES_FACTORY_FAULT_URL);
}

void ARexService::UnknownActivityIdentifierFault(Arc::XMLNode fault,const std::string& message) {
  fault.Name("bes-factory:UnknownActivityIdentifierFault");
  fault.NewChild("bes-factory:Message")=message;
  return;
}

void ARexService::UnknownActivityIdentifierFault(Arc::SOAPFault& fault,const std::string& message) {
  UnknownActivityIdentifierFault(fault.Detail(true).NewChild("dummy"),message);
  SetFaultResponse(fault);
}

void ARexService::InvalidRequestMessageFault(Arc::XMLNode fault,const std::string& element,const std::string& message) {
  fault.Name("bes-factory:InvalidRequestMessageFaultType");
  if(!element.empty()) fault.NewChild("bes-factory:InvalidElement")=element;
  fault.NewChild("bes-factory:Message")=message;
  return;
}

void ARexService::InvalidRequestMessageFault(Arc::SOAPFault& fault,const std::string& element,const std::string& message) {
  InvalidRequestMessageFault(fault.Detail(true).NewChild("dummy"),element,message);
  SetFaultResponse(fault);
}


}

