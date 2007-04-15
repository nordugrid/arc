#include "../../PayloadRaw.h"
#include "../../SOAPMessage.h"
#include "../../PayloadSOAP.h"
#include "../../../libs/common/XMLNode.h"
#include "../../libs/loader/Loader.h"
#include "../../libs/loader/MCCLoader.h"

#include "MCCSOAP.h"


static Arc::MCC* get_mcc_service(Arc::Config *cfg) {
    return new Arc::MCC_SOAP_Service(cfg);
}

static Arc::MCC* get_mcc_client(Arc::Config *cfg) {
    return new Arc::MCC_SOAP_Client(cfg);
}

mcc_descriptor __arc_mcc_modules__[] = {
    { "soap.service", 0, &get_mcc_service },
    { "soap.client", 0, &get_mcc_client }
};

using namespace Arc;


MCC_SOAP_Service::MCC_SOAP_Service(Arc::Config *cfg):MCC(cfg) {
}

MCC_SOAP_Service::~MCC_SOAP_Service(void) {
}

MCC_SOAP_Client::MCC_SOAP_Client(Arc::Config *cfg):MCC(cfg) {
}

MCC_SOAP_Client::~MCC_SOAP_Client(void) {
}

static MCC_Status make_raw_fault(Message& outmsg,const char* desc = NULL) {
  SOAPMessage soap(XMLNode::NS(),true);
  soap.Fault()->Code(SOAPMessage::SOAPFault::Receiver);
  std::string xml; soap.GetXML(xml);
  PayloadRaw* payload = new PayloadRaw;
  payload->Insert(xml.c_str());
  outmsg.Payload(payload);
  return MCC_Status(-1);
}

static MCC_Status make_soap_fault(Message& outmsg,const char* desc = NULL) {
  PayloadSOAP* soap = new PayloadSOAP(XMLNode::NS(),true);
  soap->Fault()->Code(SOAPMessage::SOAPFault::Receiver);
  outmsg.Payload(soap);
  return MCC_Status(-1);
}

MCC_Status MCC_SOAP_Service::process(Message& inmsg,Message& outmsg) {
  // Extracting payload
  MessagePayload* inpayload = inmsg.Payload();
  if(!inpayload) return make_raw_fault(inmsg);
  // Converting payload to SOAP
  PayloadSOAP nextpayload(*inpayload);
  if(!nextpayload) return make_raw_fault(outmsg);
  // Creating message to pass to next MCC and setting new payload.. 
  // Using separate message. But could also use same inmsg.
  // Just trying to keep intact as much as possible.
  Message nextinmsg = inmsg;
  nextinmsg.Payload(&nextpayload);
  // Call next MCC 
  MCCInterface* next = Next();
  if(!next) return make_raw_fault(outmsg);
  Message nextoutmsg;
  MCC_Status ret = next->process(nextinmsg,nextoutmsg); 
  // Do checks and extract SOAP response
  if(!ret) return make_raw_fault(outmsg);
  if(!nextoutmsg.Payload()) return make_raw_fault(outmsg);
  PayloadSOAP* retpayload = NULL;
  try {
    retpayload = dynamic_cast<PayloadSOAP*>(nextoutmsg.Payload());
  } catch(std::exception& e) { };
  if(!retpayload) { delete nextoutmsg.Payload(); return make_raw_fault(outmsg); };
  if(!(*retpayload)) { delete retpayload; return make_raw_fault(outmsg); };
  // Convert to Raw - TODO: more efficient conversion
  PayloadRaw* outpayload = new PayloadRaw;
  std::string xml; retpayload->GetXML(xml);
  outpayload->Insert(xml.c_str());
  outmsg = nextoutmsg;
  delete outmsg.Payload(outpayload);
  return MCC_Status();
}

MCC_Status MCC_SOAP_Client::process(Message& inmsg,Message& outmsg) {
  // Extracting payload
  if(!inmsg.Payload()) return make_soap_fault(outmsg);
  PayloadSOAP* inpayload = NULL;
  try {
    inpayload = dynamic_cast<PayloadSOAP*>(inmsg.Payload());
  } catch(std::exception& e) { };
  if(!inpayload) return make_soap_fault(outmsg);
  // Converting payload to Raw
  PayloadRaw nextpayload;
  std::string xml; inpayload->GetXML(xml);
  nextpayload.Insert(xml.c_str());
  // Creating message to pass to next MCC and setting new payload.. 
  Message nextinmsg = inmsg;
  nextinmsg.Payload(&nextpayload);
  // Call next MCC 
  MCCInterface* next = Next();
  if(!next) return make_soap_fault(outmsg);
  Message nextoutmsg;
  MCC_Status ret = next->process(nextinmsg,nextoutmsg); 
  // Do checks and create SOAP response
  if(!ret) return make_soap_fault(outmsg);
  if(!nextoutmsg.Payload()) return make_soap_fault(outmsg);
  MessagePayload* retpayload = nextoutmsg.Payload();
  if(!retpayload) return make_soap_fault(outmsg);
  PayloadSOAP* outpayload  = new PayloadSOAP(*retpayload);
  delete retpayload;
  if(!outpayload) return make_soap_fault(outmsg);
  if(!(*outpayload)) { delete outpayload; return make_soap_fault(outmsg); };
  outmsg = nextoutmsg;
  delete outmsg.Payload(outpayload);
  return MCC_Status();
}

