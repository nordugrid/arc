#include "../../PayloadRaw.h"
#include "../../SOAPMessage.h"
#include "../../PayloadSOAP.h"
#include "../../../libs/common/XMLNode.h"
#include "../../libs/loader/Loader.h"

#include "MCCSOAP.h"


static Arc::MCC* get_mcc_instance(Arc::Config *cfg) {
    return new Arc::MCCSOAP(cfg);
}

mcc_descriptor descriptor = {
	"soap",
    0,
    &get_mcc_instance
};

using namespace Arc;


MCCSOAP::MCCSOAP(Arc::Config *cfg):MCC(cfg),next_(NULL) {
  // So far nothing to configure except next
}

MCCSOAP::~MCCSOAP(void) {
  // and nothing to destroy
}

static MCC_Status make_soap_fault(Message& outmsg,const char* desc = NULL) {
  SOAPMessage soap(XMLNode::NS(),true);
  soap.Fault()->Code(SOAPMessage::SOAPFault::Receiver);
  std::string xml; soap.GetXML(xml);
  PayloadRaw* payload = new PayloadRaw;
  payload->Insert(xml.c_str());
  outmsg.Payload(payload);
  return MCC_Status(-1);
}

MCC_Status MCCSOAP::process(Message& inmsg,Message& outmsg) {
  // Extracting payload
  MessagePayload* inpayload = inmsg.Payload();
  if(!inpayload) return make_soap_fault(inmsg);
  // Converting payload to SOAP
  PayloadSOAP nextpayload(*inpayload);
  if(!nextpayload) return make_soap_fault(outmsg);
  // Creating message to pass to next MCC and setting new payload.. 
  // Using separate message. But could also use same inmsg.
  // Just trying to keep intact as much as possible.
  Message nextinmsg = inmsg;
  nextinmsg.Payload(&nextpayload);
  // Call next MCC 
  MCCInterface* next = Next();
  if(!next) return make_soap_fault(outmsg);
  Message nextoutmsg;
  MCC_Status ret = next->process(nextinmsg,nextoutmsg); 
  // Do checks and extract SOAP response
  if(!ret) return make_soap_fault(outmsg);
  if(!nextoutmsg.Payload()) return make_soap_fault(outmsg);
  PayloadSOAP* retpayload = NULL;
  try {
    retpayload = dynamic_cast<PayloadSOAP*>(nextoutmsg.Payload());
  } catch(std::exception& e) { };
  if(!retpayload) return make_soap_fault(outmsg);
  if(!(*retpayload)) return make_soap_fault(outmsg);
  // Convert to Raw - TODO: more efficient conversion
  PayloadRaw* outpayload = new PayloadRaw;
  std::string xml; retpayload->GetXML(xml);
  outpayload->Insert(xml.c_str());
  outmsg = nextoutmsg;
  delete outmsg.Payload(outpayload);
  return MCC_Status();
}


