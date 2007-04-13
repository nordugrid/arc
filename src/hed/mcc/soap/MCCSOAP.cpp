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

static Message make_soap_fault(Message basemsg,const char* desc = NULL) {
  Message msg = basemsg;
  SOAPMessage soap(XMLNode::NS(),true);
  soap.Fault()->Code(SOAPMessage::SOAPFault::Receiver);
  std::string xml; soap.GetXML(xml);
  PayloadRaw* payload = new PayloadRaw;
  payload->Insert(xml.c_str());
  msg.Payload(payload);
  return msg;
}

Message MCCSOAP::process(Message inmsg) {
  // Extracting payload
  MessagePayload* inpayload = inmsg.Payload();
  if(!inpayload) return make_soap_fault(inmsg);
  // Converting payload to SOAP
  PayloadSOAP nextpayload(*inpayload);
  if(!nextpayload) return make_soap_fault(inmsg);
  // Creating message to pass to next MCC
  Message nextmsg = inmsg;
  nextmsg.Payload(&nextpayload);
  // Call next MCC 
  if(!next_) return make_soap_fault(inmsg);
  Message retmsg = next_->process(nextmsg); 
  // Do checks and extract SOAP response
  if(!retmsg.Payload()) return make_soap_fault(inmsg);
  PayloadSOAP* retpayload = NULL;
  try {
    retpayload = dynamic_cast<PayloadSOAP*>(retmsg.Payload());
  } catch(std::exception& e) { };
  if(!retpayload) return make_soap_fault(inmsg);
  if(!(*retpayload)) return make_soap_fault(inmsg);
  // Convert to Raw - TODO: more effective conversion
  PayloadRaw* outpayload = new PayloadRaw;
  std::string xml; retpayload->GetXML(xml);
  outpayload->Insert(xml.c_str());
  Message outmsg = retmsg;
  delete outmsg.Payload(outpayload);
  return outmsg;
}
