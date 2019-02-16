#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

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


// EMI ES faults

//  InternalBaseFault
//    Message
//    Timestamp (dateTime) 0-1
//    Description 0-1
//    FailureCode (int) 0-1
void ARexService::ESInternalBaseFault(Arc::XMLNode fault,const std::string& message,const std::string& desc) {
  fault.Name("estypes:InternalBaseFault");
  fault.NewChild("estypes:Message") = message;
  fault.NewChild("estypes:Timestamp") = Arc::Time().str(Arc::ISOTime);
  if(!desc.empty()) fault.NewChild("estypes:Description") = desc;
  //fault.NewChild("estypes:FailureCode") = "0";
}

void ARexService::ESInternalBaseFault(Arc::SOAPFault& fault,const std::string& message,const std::string& desc) {
  ESInternalBaseFault(fault.Detail(true).NewChild("dummy"),message,desc);
}

void ARexService::ESVectorLimitExceededFault(Arc::XMLNode fault,unsigned long limit,const std::string& message,const std::string& desc) {
  ESInternalBaseFault(fault,message.empty()?"Limit of parallel requests exceeded":message,desc);
  fault.NewChild("estypes:ServerLimit") = Arc::tostring(limit);
  fault.Name("estypes:VectorLimitExceededFault");
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


ES_SIMPLE_FAULT(NotSupportedQueryDialectFault,esrinfo,"Query language not supported")
ES_SIMPLE_FAULT(NotValidQueryStatementFault,esrinfo,"Query is not valid for specified language")
ES_SIMPLE_FAULT(UnknownQueryFault,esrinfo,"Query is not recognized")
ES_SIMPLE_FAULT(InternalResourceInfoFault,esrinfo,"Internal failure retrieving resource information")
ES_SIMPLE_FAULT(ResourceInfoNotFoundFault,esrinfo,"Resource has no requested information")


ES_SIMPLE_FAULT(UnableToRetrieveStatusFault,esainfo,"Activity status is missing")
ES_SIMPLE_FAULT(UnknownAttributeFault,esainfo,"Activity has no such attribute")
ES_SIMPLE_FAULT(OperationNotAllowedFault,esainfo,"Requested operation not allowed")
ES_SIMPLE_FAULT(ActivityNotFoundFault,esainfo,"Activity with specified ID not found")
ES_SIMPLE_FAULT(InternalNotificationFault,esainfo,"Notofication fault")
ES_SIMPLE_FAULT(OperationNotPossibleFault,esainfo,"Can't perform this operation")
ES_SIMPLE_FAULT(InvalidActivityStateFault,esainfo,"Invalid activity state")
ES_SIMPLE_FAULT(InvalidActivityLimitFault,esainfo,"Invalid activity limit")
ES_SIMPLE_FAULT(InvalidParameterFault,esainfo,"Invalid parameter")

}

