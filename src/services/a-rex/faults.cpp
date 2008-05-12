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

  InvalidActivityIdentifierFault
    Message (string)

  InvalidRequestMessageFaultType
    InvalidElement (string,unbounded)
    Message (string)

  */

namespace ARex {

void ARexService::NotAuthorizedFault(Arc::XMLNode fault) {
  fault.Name("bes-factory:NotAuthorizedFault");
  return;
}

void ARexService::NotAcceptingNewActivitiesFault(Arc::XMLNode fault) {
  fault.Name("bes-factory:NotAcceptingNewActivitiesFault");
  return;
}

void ARexService::UnsupportedFeatureFault(Arc::XMLNode fault,const std::string& feature) {
  fault.Name("bes-factory:UnsupportedFeatureFault");
  if(!feature.empty()) fault.NewChild("bes-factory:Feature")=feature;
  return;
}

void ARexService::CantApplyOperationToCurrentStateFault(Arc::XMLNode fault,const std::string& gm_state,bool failed,const std::string& message) {
  fault.Name("bes-factory:CantApplyOperationToCurrentStateFault");
  addActivityStatus(fault,gm_state,failed);
  fault.NewChild("bes-factory:Message")=message;
  return;
}

void ARexService::OperationWillBeAppliedEventuallyFault(Arc::XMLNode fault,const std::string& gm_state,bool failed,const std::string& message) {
  fault.Name("bes-factory:OperationWillBeAppliedEventuallyFault");
  addActivityStatus(fault,gm_state,failed);
  fault.NewChild("bes-factory:Message")=message;
  return;
}

void ARexService::InvalidActivityIdentifierFault(Arc::XMLNode fault,const std::string& message) {
  fault.Name("bes-factory:InvalidActivityIdentifierFault");
  fault.NewChild("bes-factory:Message")=message;
  return;
}

void ARexService::InvalidRequestMessageFaultType(Arc::XMLNode fault,const std::string& element,const std::string& message) {
  fault.Name("bes-factory:InvalidRequestMessageFaultType");
  if(!element.empty()) fault.NewChild("bes-factory:InvalidElement")=element;
  fault.NewChild("bes-factory:Message")=message;
  return;
}

}

