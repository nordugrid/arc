#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

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

// A-REX faults

static const std::string BES_FACTORY_FAULT_URL("http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/Fault");

static void SetFaultResponse(Arc::SOAPFault& fault) {
  // Fetch top element of SOAP message - should be better way
  Arc::XMLNode fault_node = fault;
  Arc::SOAPEnvelope res(fault_node.Parent().Parent()); // Fault->Body->Envelope
  Arc::WSAHeader(res).Action(BES_FACTORY_FAULT_URL);
}

void ARexService::GenericFault(Arc::SOAPFault& fault) {
  Arc::XMLNode fault_node = fault;
  Arc::SOAPEnvelope res(fault_node.Parent().Parent()); // Fault->Body->Envelope
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


// EMI ES faults

void ARexService::ESInternalBaseFault(Arc::XMLNode fault,const std::string& message,const std::string& desc) {
  fault.Name("estypes:InternalBaseFault");
  fault.NewChild("estypes:Message") = message;
  fault.NewChild("estypes:Timestamp") = Arc::Time().str(Arc::ISOTime);
  if(!desc.empty()) fault.NewChild("estypes:Description") = desc;
  //fault.NewChild("estypes:FailureCode") = 0;
}

void ARexService::ESInternalBaseFault(Arc::SOAPFault& fault,const std::string& message,const std::string& desc) {
  ESInternalBaseFault(fault.Detail(true).NewChild("dummy"),message,desc);
}

void ARexService::ESVectorLimitExceededFault(Arc::XMLNode fault,unsigned long limit,const std::string& message,const std::string& desc) {
  ESInternalBaseFault(fault,message.empty()?"Limit of parallel requests exceeded":message,desc);
  fault.NewChild("estypes:ServerLimit") = Arc::tostring(limit);
  fault.Name("estypes:AccessControlFault");
}

void ARexService::ESVectorLimitExceededFault(Arc::SOAPFault& fault,unsigned long limit,const std::string& message,const std::string& desc) {
  ESVectorLimitExceededFault(fault.Detail(true).NewChild("dummy"),limit,message,desc);
}

#define ES_SIMPLE_FAULT(FAULTNAME,NAMESPACE,MESSAGE) \
void ARexService::ES##FAULTNAME(Arc::XMLNode fault, \
                           const std::string& message,const std::string& desc) { \
  ESInternalBaseFault(fault,message.empty()?(MESSAGE):message,desc); \
  fault.Name(#NAMESPACE ":" #FAULTNAME); \
} \
 \
void ARexService::ES##FAULTNAME(Arc::SOAPFault& fault, \
                           const std::string& message,const std::string& desc) { \
  ES##FAULTNAME(fault.Detail(true).NewChild("dummy"),message,desc); \
}

ES_SIMPLE_FAULT(AccessControlFault,estypes,"Access denied")

ES_SIMPLE_FAULT(UnsupportedCapabilityFault,escreate,"Unsupported capability")

ES_SIMPLE_FAULT(InvalidActivityDescriptionSemanticFault,escreate,"Invalid activity description semantics")

ES_SIMPLE_FAULT(InvalidActivityDescriptionFault,escreate,"Invalid activity description")

ES_SIMPLE_FAULT(InternalResourceInfoFault,esrinfo,"Internal failure retrieving resource information")

ES_SIMPLE_FAULT(InvalidActivityIDFault,esainfo,"Invalid activity ID")

ES_SIMPLE_FAULT(UnknownActivityIDFault,esainfo,"Unknown activity ID")

ES_SIMPLE_FAULT(InvalidActivityStateFault,esainfo,"Invalid activity state")

}

