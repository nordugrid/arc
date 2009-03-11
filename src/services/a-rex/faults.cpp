#include "arex.h"
#include "tools.h"

  /*
  NotAuthorizedFault

  NotAcceptingNewActivitiesFault

  UnsupportedFeatureFault
    Feature (string, unbounded)

  CantApplyOperationToCurrentStateFault
    ActivityStatus (ActivityStatusType)
    Message (xsd:string)

  OperationWillBeAppliedEventuallyFault
    ActivityStatus (ActivityStatusType)
    Message (xsd:string)

  UnknownActivityIdentifierFault
    Message (string)

  InvalidRequestMessageFaultType
    InvalidElement (string,unbounded)
    Message (string)

  */

namespace ARex {

static const std::string BES_FACTORY_FAULT_URL("http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/Fault");

static void SetFaultResponse(Arc::SOAPFault& fault) {
  // Fetch top element of SOAP message - should be better way
  Arc::SOAPEnvelope res(((Arc::XMLNode)fault).Parent().Parent()); // Fault->Body->Envelope
  Arc::WSAHeader(res).Action(BES_FACTORY_FAULT_URL);
}

void ARexService::GenericFault(Arc::SOAPFault& fault) {
  Arc::SOAPEnvelope res(((Arc::XMLNode)fault).Parent().Parent()); // Fault->Body->Envelope
  Arc::WSAHeader(res).Action("");
}

void ARexService::NotAuthorizedFault(Arc::XMLNode fault) {
  fault.Name("bes-factory:NotAuthorizedFault");
}

void ARexService::NotAuthorizedFault(Arc::SOAPFault& fault) {
  NotAuthorizedFault(fault.Detail(true).NewChild("dummy"));
  SetFaultResponse(fault);
}

void ARexService::NotAcceptingNewActivitiesFault(Arc::XMLNode fault) {
  fault.Name("bes-factory:NotAcceptingNewActivitiesFault");
  return;
}

void ARexService::NotAcceptingNewActivitiesFault(Arc::SOAPFault& fault) {
  NotAcceptingNewActivitiesFault(fault.Detail(true).NewChild("dummy"));
  SetFaultResponse(fault);
}

void ARexService::UnsupportedFeatureFault(Arc::XMLNode fault,const std::string& feature) {
  fault.Name("bes-factory:UnsupportedFeatureFault");
  if(!feature.empty()) fault.NewChild("bes-factory:Feature")=feature;
  return;
}

void ARexService::UnsupportedFeatureFault(Arc::SOAPFault& fault,const std::string& feature) {
  UnsupportedFeatureFault(fault.Detail(true).NewChild("dummy"),feature);
  SetFaultResponse(fault);
}

void ARexService::CantApplyOperationToCurrentStateFault(Arc::XMLNode fault,const std::string& gm_state,bool failed,const std::string& message) {
  fault.Name("bes-factory:CantApplyOperationToCurrentStateFault");
  addActivityStatus(fault,gm_state,"",failed);
  fault.NewChild("bes-factory:Message")=message;
  return;
}

void ARexService::CantApplyOperationToCurrentStateFault(Arc::SOAPFault& fault,const std::string& gm_state,bool failed,const std::string& message) {
  CantApplyOperationToCurrentStateFault(fault.Detail(true).NewChild("dummy"),gm_state,failed,message);
  SetFaultResponse(fault);
}

void ARexService::OperationWillBeAppliedEventuallyFault(Arc::XMLNode fault,const std::string& gm_state,bool failed,const std::string& message) {
  fault.Name("bes-factory:OperationWillBeAppliedEventuallyFault");
  addActivityStatus(fault,gm_state,"",failed);
  fault.NewChild("bes-factory:Message")=message;
  return;
}

void ARexService::OperationWillBeAppliedEventuallyFault(Arc::SOAPFault& fault,const std::string& gm_state,bool failed,const std::string& message) {
  OperationWillBeAppliedEventuallyFault(fault.Detail(true).NewChild("dummy"),gm_state,failed,message);
  SetFaultResponse(fault);
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

