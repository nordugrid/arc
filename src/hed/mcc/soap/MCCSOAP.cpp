#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/PayloadRaw.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/SecAttr.h>
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

namespace Arc {

class SOAPSecAttr: public SecAttr {
 friend class MCC_SOAP_Service;
 friend class MCC_SOAP_Client;
 public:
  SOAPSecAttr(PayloadSOAP& payload);
  virtual ~SOAPSecAttr(void);
  virtual operator bool(void) const;
  virtual bool Export(Format format,XMLNode &val) const;
 protected:
  std::string action_;
  std::string object_;
  std::string context_;
  virtual bool equal(const SecAttr &b) const;
};

SOAPSecAttr::SOAPSecAttr(PayloadSOAP& payload) {
  action_=payload.Child().Name();
  context_=payload.Child().Namespace();
  if(WSAHeader::Check(payload)) object_ = WSAHeader(payload).To();
}

SOAPSecAttr::~SOAPSecAttr(void) {
}

SOAPSecAttr::operator bool(void) const {
  return !action_.empty();
}

bool SOAPSecAttr::equal(const SecAttr &b) const {
  try {
    const SOAPSecAttr& a = (const SOAPSecAttr&)b;
    return ((action_ == a.action_) && (context_ == a.context_));
  } catch(std::exception&) { };
  return false;
}

bool SOAPSecAttr::Export(Format format,XMLNode &val) const {
  if(format == UNDEFINED) {
  } else if(format == ARCAuth) {
    NS ns;
    ns["ra"]="http://www.nordugrid.org/schemas/request-arc";
    val.Namespaces(ns); val.Name("ra:Request");
    XMLNode item = val.NewChild("ra:RequestItem");
    if(!object_.empty()) {
      XMLNode object = item.NewChild("ra:Resource");
      object=object_;
      object.NewAttribute("Type")="string";
      object.NewAttribute("AttributeId")="http://www.nordugrid.org/schemas/policy-arc/types/soap/endpoint";
    };
    if(!action_.empty()) {
      XMLNode action = item.NewChild("ra:Action");
      action=action_;
      action.NewAttribute("Type")="string";
      action.NewAttribute("AttributeId")="http://www.nordugrid.org/schemas/policy-arc/types/soap/operation";
    };
    if(!context_.empty()) {
      XMLNode context = item.NewChild("ra:Context").NewChild("ra:ContextAttribute");
      context=context_;
      context.NewAttribute("Type")="string";
      context.NewAttribute("AttributeId")="http://www.nordugrid.org/schemas/policy-arc/types/soap/namespace";
    };
    return true;
  } else {
  };
  return false;
}

MCC_SOAP_Service::MCC_SOAP_Service(Arc::Config *cfg):MCC_SOAP(cfg) {
}

MCC_SOAP_Service::~MCC_SOAP_Service(void) {
}

MCC_SOAP_Client::MCC_SOAP_Client(Arc::Config *cfg):MCC_SOAP(cfg) {
}

MCC_SOAP_Client::~MCC_SOAP_Client(void) {
}

static MCC_Status make_raw_fault(Message& outmsg,const char* reason = NULL) 
{
  NS ns;
  SOAPEnvelope soap(ns,true);
  soap.Fault()->Code(SOAPFault::Receiver);
  std::string xml; soap.GetXML(xml);
  if(reason != NULL) soap.Fault()->Reason(0, reason);
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
  // Extracting payload
  MessagePayload* inpayload = inmsg.Payload();
  if(!inpayload) {
    logger.msg(WARNING, "empty input payload");
    return make_raw_fault(outmsg);
  }
  // Converting payload to SOAP
  PayloadSOAP nextpayload(*inpayload);
  if(!nextpayload) {
    logger.msg(WARNING, "incoming message is not SOAP");
    return make_raw_fault(outmsg);
  }
  // Creating message to pass to next MCC and setting new payload.. 
  // Using separate message. But could also use same inmsg.
  // Just trying to keep it intact as much as possible.
  Message nextinmsg = inmsg;
  nextinmsg.Payload(&nextpayload);
  if(WSAHeader::Check(nextpayload)) {
    std::string endpoint_attr = WSAHeader(nextpayload).To();
    nextinmsg.Attributes()->set("SOAP:ENDPOINT",endpoint_attr);
    nextinmsg.Attributes()->set("ENDPOINT",endpoint_attr);
  };
  SOAPSecAttr* sattr = new SOAPSecAttr(nextpayload);
  nextinmsg.Auth()->set("SOAP",sattr);
  // Checking authentication and authorization; 
  if(!ProcessSecHandlers(nextinmsg,"incoming")) {
    logger.msg(ERROR, "Security check failed in SOAP MCC for incoming message");
    return make_raw_fault(outmsg, "Can not pass the authentication or authorization checking from service side in soap level.");
  };
  // Call next MCC 
  MCCInterface* next = Next();
  if(!next) {
    logger.msg(WARNING, "empty next chain element");
    return make_raw_fault(outmsg);
  }
  Message nextoutmsg = outmsg; nextoutmsg.Payload(NULL);
  MCC_Status ret = next->process(nextinmsg,nextoutmsg); 
  // Do checks and extract SOAP response
  if(!ret) {
    logger.msg(WARNING, "next element of the chain returned error status");
    return make_raw_fault(outmsg);
  }
  if(!nextoutmsg.Payload()) {
    logger.msg(WARNING, "next element of the chain returned empty payload");
    return make_raw_fault(outmsg);
  }
  PayloadSOAP* retpayload = NULL;
  try {
    retpayload = dynamic_cast<PayloadSOAP*>(nextoutmsg.Payload());
  } catch(std::exception& e) { };
  if(!retpayload) {
    logger.msg(WARNING, "next element of the chain returned invalid payload");
    delete nextoutmsg.Payload(); 
    return make_raw_fault(outmsg); 
  };
  if(!(*retpayload)) { delete retpayload; return make_raw_fault(outmsg); };
  //Checking authentication and authorization; 
  if(!ProcessSecHandlers(nextoutmsg,"outgoing")) {
    logger.msg(ERROR, "Security check failed in SOAP MCC for outgoing message");
    delete retpayload; return make_raw_fault(outmsg);
  };
  // Convert to Raw - TODO: more efficient conversion
  PayloadRaw* outpayload = new PayloadRaw;
  std::string xml; retpayload->GetXML(xml);
  outpayload->Insert(xml.c_str());
  outmsg = nextoutmsg;
  // Specifying attributes for binding to underlying protocols - HTTP so far
  if(retpayload->Version() == SOAPEnvelope::Version_1_2) {
    // TODO: For SOAP 1.2 Content-Type is not sent in case of error - probably harmless
    outmsg.Attributes()->set("HTTP:Content-Type","application/soap+xml");
  } else {
    outmsg.Attributes()->set("HTTP:Content-Type","text/xml");
  };
  if(retpayload->Fault() != NULL) {
    // Maybe MCC_Status should be used instead ?
    outmsg.Attributes()->set("HTTP:CODE","500"); // TODO: For SOAP 1.2 :Sender fault must generate 400
    outmsg.Attributes()->set("HTTP:REASON","SOAP FAULT");
    // CONFUSED: SOAP 1.2 says that SOAP message is sent in response only if
    // HTTP code is 200 "Only if status line is 200, the SOAP message serialized according 
    // to the rules for carrying SOAP messages in the media type given by the Content-Type 
    // header field ...". But it also associates SOAP faults with HTTP error codes. So it 
    // looks like in case of SOAP fault SOAP fault messages is not sent. That sounds 
    // stupid - not implementing.
  };
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
  //Checking authentication and authorization;
  if(!ProcessSecHandlers(inmsg,"outgoing")) {
    logger.msg(ERROR, "Security check failed in SOAP MCC for outgoing message");
    return make_soap_fault(outmsg);
  };
  // Converting payload to Raw
  PayloadRaw nextpayload;
  std::string xml; inpayload->GetXML(xml);
  nextpayload.Insert(xml.c_str());
  // Creating message to pass to next MCC and setting new payload.. 
  Message nextinmsg = inmsg;
  nextinmsg.Payload(&nextpayload);
  // Specifying attributes for binding to underlying protocols - HTTP so far
  std::string soap_action = inmsg.Attributes()->get("SOAP:ACTION");
  if(soap_action.empty()) soap_action=WSAHeader(*inpayload).Action();
  if(inpayload->Version() == SOAPEnvelope::Version_1_2) {
    std::string mime_type("application/soap+xml");
    if(!soap_action.empty()) mime_type+=" ;action=\""+soap_action+"\"";
    nextinmsg.Attributes()->set("HTTP:Content-Type",mime_type);
  } else {
    nextinmsg.Attributes()->set("HTTP:Content-Type","text/xml");
    nextinmsg.Attributes()->set("HTTP:SOAPAction",soap_action);
  };
  // Call next MCC 
  MCCInterface* next = Next();
  if(!next) return make_soap_fault(outmsg);
  Message nextoutmsg = outmsg; nextoutmsg.Payload(NULL);
  MCC_Status ret = next->process(nextinmsg,nextoutmsg); 
  // Do checks and create SOAP response
  if(!ret) return make_soap_fault(outmsg,nextoutmsg);
  if(!nextoutmsg.Payload()) return make_soap_fault(outmsg,nextoutmsg);
  MessagePayload* retpayload = nextoutmsg.Payload();
  if(!retpayload) return make_soap_fault(outmsg,nextoutmsg);
  PayloadSOAP* outpayload  = new PayloadSOAP(*retpayload);
  if(!outpayload) return make_soap_fault(outmsg,nextoutmsg);
  if(!(*outpayload)) {
    delete outpayload; return make_soap_fault(outmsg,nextoutmsg);
  };
  outmsg = nextoutmsg;
  outmsg.Payload(outpayload);
  delete retpayload;
  //Checking authentication and authorization; 
  if(!ProcessSecHandlers(outmsg,"incoming")) {
    logger.msg(ERROR, "Security check failed in SOAP MCC for incoming message");
    delete outpayload; return make_soap_fault(outmsg);
  };
  return MCC_Status(Arc::STATUS_OK);
}

} // namespace Arc
