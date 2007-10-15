#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/PayloadRaw.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/XMLNode.h>
#include <arc/loader/MCCLoader.h>
#include <arc/ws-addressing/WSA.h>
#include <arc/security/SecHandler.h>

#include "MCCSOAP.h"


Arc::Logger Arc::MCC_SOAP::logger(Arc::MCC::logger,"SOAP");

Arc::MCC_SOAP::MCC_SOAP(Arc::Config *cfg) : MCC(cfg) {
}

static Arc::MCC* get_mcc_service(Arc::Config *cfg,Arc::ChainContext*) {
    return new Arc::MCC_SOAP_Service(cfg);
}

static Arc::MCC* get_mcc_client(Arc::Config *cfg,Arc::ChainContext*) {
    return new Arc::MCC_SOAP_Client(cfg);
}

mcc_descriptors ARC_MCC_LOADER = {
    { "soap.service", 0, &get_mcc_service },
    { "soap.client", 0, &get_mcc_client },
    { NULL, 0, NULL }
};

using namespace Arc;


MCC_SOAP_Service::MCC_SOAP_Service(Arc::Config *cfg):MCC_SOAP(cfg) {
}

MCC_SOAP_Service::~MCC_SOAP_Service(void) {
}

MCC_SOAP_Client::MCC_SOAP_Client(Arc::Config *cfg):MCC_SOAP(cfg) {
}

MCC_SOAP_Client::~MCC_SOAP_Client(void) {
}

static MCC_Status make_raw_fault(Message& outmsg,const char* = NULL) {
  NS ns;
  SOAPEnvelope soap(ns,true);
  soap.Fault()->Code(SOAPFault::Receiver);
  std::string xml; soap.GetXML(xml);
  PayloadRaw* payload = new PayloadRaw;
  payload->Insert(xml.c_str());
  outmsg.Payload(payload);
  return MCC_Status(Arc::GENERIC_ERROR);
}

static MCC_Status make_soap_fault(Message& outmsg,const char* = NULL) {
  PayloadSOAP* soap = new PayloadSOAP(Arc::NS(),true);
  soap->Fault()->Code(SOAPFault::Receiver);
  outmsg.Payload(soap);
  return MCC_Status(Arc::GENERIC_ERROR);
}

static MCC_Status make_soap_fault(Message& outmsg,Message& oldmsg,const char* desc = NULL) {
  if(oldmsg.Payload()) {
    delete oldmsg.Payload();
    oldmsg.Payload(NULL);
  };
  return make_soap_fault(outmsg,desc);
}

MCC_Status MCC_SOAP_Service::process(Message& inmsg,Message& outmsg) {
  //Checking authentication and authorization; 
  //Each MCC/Service can define security handler chains in the configuration 
  // file, the chains are distinguished by "event" attribute;
  // Security SecHandlers in one handler chain are called one by one. For one 
  // type ("event") of handlers, each one should be configured carefully, 
  // because there should be some sequencial relationship between them (e.g. 
  // authentication should be put in front of authorization).
  // The h->Handle(msg) only return true/false; If any SecHandler in the 
  // handler chain produces some information which will be used by some 
  // following handler, the information should be stored in the 
  // msg.attributes(e.g. the Identity extracted from authentication will 
  // be used by authorization to make access control decision).
  
  std::list<ArcSec::SecHandler*> hlist=sechandlers_["incoming"];
  std::list<ArcSec::SecHandler*>::iterator it;
  for(it=hlist.begin(); it!=hlist.end(); it++){
    ArcSec::SecHandler* h = *it;
    if(h->Handle(&inmsg)) break;   
  }  
  // The "Handle" method only returns true/false; The MCC/Service doesn't 
  // care about security process. "SecHandler" uses msg.attributes to 
  // exchange security related information
  if((it==hlist.end()) && (hlist.size() > 0)){
    logger.msg(Arc::INFO, "UnAuthorized"); 
    return MCC_Status(Arc::GENERIC_ERROR);
  } //Do we need to add some status in MCC_Status
  else if (hlist.empty()) logger.msg(Arc::INFO, "No authorization requirement for SOAP MCC");
  else logger.msg(Arc::INFO, "soap_Authorized"); 

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
  if(WSAHeader::Check(nextpayload)) {
    std::string endpoint_attr = WSAHeader(nextpayload).To();
    nextinmsg.Attributes()->set("SOAP:ENDPOINT",endpoint_attr);
    nextinmsg.Attributes()->set("ENDPOINT",endpoint_attr);
  };
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
  return MCC_Status(Arc::STATUS_OK);
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
  logger.msg(Arc::DEBUG, xml.c_str()); 
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
  if(!ret) return make_soap_fault(outmsg,nextoutmsg);
  if(!nextoutmsg.Payload()) return make_soap_fault(outmsg,nextoutmsg);
  MessagePayload* retpayload = nextoutmsg.Payload();
  if(!retpayload) return make_soap_fault(outmsg,nextoutmsg);
  PayloadSOAP* outpayload  = new PayloadSOAP(*retpayload);
  if(!outpayload) return make_soap_fault(outmsg,nextoutmsg);
  if(!(*outpayload)) {
    delete outpayload;
    return make_soap_fault(outmsg,nextoutmsg);
  };
  outmsg = nextoutmsg;
  outmsg.Payload(outpayload);
  delete retpayload;
  return MCC_Status(Arc::STATUS_OK);
}

