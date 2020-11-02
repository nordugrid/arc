#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/PayloadRaw.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/SecAttr.h>
#include <arc/XMLNode.h>
#include <arc/loader/Plugin.h>
#include <arc/ws-addressing/WSA.h>

#include "MCCSOAP.h"


Arc::Logger ArcMCCSOAP::MCC_SOAP::logger(Arc::Logger::getRootLogger(), "MCC.SOAP");


ArcMCCSOAP::MCC_SOAP::MCC_SOAP(Arc::Config *cfg,PluginArgument* parg) : Arc::MCC(cfg,parg) {
}

static Arc::Plugin* get_mcc_service(Arc::PluginArgument* arg) {
    Arc::MCCPluginArgument* mccarg =
            arg?dynamic_cast<Arc::MCCPluginArgument*>(arg):NULL;
    if(!mccarg) return NULL;
    return new ArcMCCSOAP::MCC_SOAP_Service((Arc::Config*)(*mccarg),mccarg);
}

static Arc::Plugin* get_mcc_client(Arc::PluginArgument* arg) {
    Arc::MCCPluginArgument* mccarg =
            arg?dynamic_cast<Arc::MCCPluginArgument*>(arg):NULL;
    if(!mccarg) return NULL;
    return new ArcMCCSOAP::MCC_SOAP_Client((Arc::Config*)(*mccarg),mccarg);
}

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
    { "soap.service", "HED:MCC", NULL, 0, &get_mcc_service },
    { "soap.client",  "HED:MCC", NULL, 0, &get_mcc_client  },
    { NULL, NULL, NULL, 0, NULL }
};

namespace ArcMCCSOAP {

using namespace Arc;

class SOAPSecAttr: public SecAttr {
 friend class MCC_SOAP_Service;
 friend class MCC_SOAP_Client;
 public:
  SOAPSecAttr(PayloadSOAP& payload);
  virtual ~SOAPSecAttr(void);
  virtual operator bool(void) const;
  virtual bool Export(SecAttrFormat format,XMLNode &val) const;
  virtual std::string get(const std::string& id) const;
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

std::string SOAPSecAttr::get(const std::string& id) const {
  if(id == "ACTION") return action_;
  if(id == "OBJECT") return object_;
  if(id == "CONTEXT") return context_;
  return "";
}

bool SOAPSecAttr::equal(const SecAttr &b) const {
  try {
    const SOAPSecAttr& a = (const SOAPSecAttr&)b;
    return ((action_ == a.action_) && (context_ == a.context_));
  } catch(std::exception&) { };
  return false;
}

bool SOAPSecAttr::Export(SecAttrFormat format,XMLNode &val) const {
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
  } else if(format == XACML) {
    NS ns;
    ns["ra"]="urn:oasis:names:tc:xacml:2.0:context:schema:os";
    val.Namespaces(ns); val.Name("ra:Request");
    if(!object_.empty()) {
      XMLNode object = val.NewChild("ra:Resource");
      XMLNode attr = object.NewChild("ra:Attribute");
      attr.NewChild("ra:AttributeValue") = object_;
      attr.NewAttribute("DataType")="xs:string";
      attr.NewAttribute("AttributeId")="http://www.nordugrid.org/schemas/policy-arc/types/soap/endpoint";
    };
    if(!action_.empty()) {
      XMLNode action = val.NewChild("ra:Action");
      XMLNode attr = action.NewChild("ra:Attribute");
      attr.NewChild("ra:AttributeValue") = action_;
      attr.NewAttribute("DataType")="xs:string";
      attr.NewAttribute("AttributeId")="http://www.nordugrid.org/schemas/policy-arc/types/soap/operation";
    };
    if(!context_.empty()) {
      XMLNode context = val.NewChild("ra:Environment");
      XMLNode attr = context.NewChild("ra:Attribute");
      attr.NewChild("ra:AttributeValue") = context_;
      attr.NewAttribute("DataType")="xs:string";
      attr.NewAttribute("AttributeId")="http://www.nordugrid.org/schemas/policy-arc/types/soap/namespace";
    };
    return true;
  } else {
  };
  return false;
}

MCC_SOAP_Service::MCC_SOAP_Service(Config *cfg,PluginArgument* parg):MCC_SOAP(cfg,parg),_continueNonSoap(false) {
  std::string continueNonSoap = (*cfg)["ContinueNonSOAP"];
  if((continueNonSoap == "yes") || (continueNonSoap == "true") || (continueNonSoap == "1")) {
    _continueNonSoap = true;
  }
}

MCC_SOAP_Service::~MCC_SOAP_Service(void) {
}

MCC_SOAP_Client::MCC_SOAP_Client(Config *cfg,PluginArgument* parg):MCC_SOAP(cfg,parg) {
}

MCC_SOAP_Client::~MCC_SOAP_Client(void) {
}

static MCC_Status make_raw_fault(Message& outmsg, const char* reason1 = NULL, const char* reason2 = NULL, const char* reason3 = NULL) {
  NS ns;
  SOAPEnvelope soap(ns,true);
  soap.Fault()->Code(SOAPFault::Receiver);
  std::string reason;
  if(reason1) { if(!reason.empty()) reason+=": "; reason += reason1; };
  if(reason2) { if(!reason.empty()) reason+=": "; reason += reason2; };
  if(reason3) { if(!reason.empty()) reason+=": "; reason += reason3; };
  if(!reason.empty()) soap.Fault()->Reason(0, reason.c_str());
  std::string xml; soap.GetXML(xml);
  PayloadRaw* payload = new PayloadRaw;
  payload->Insert(xml.c_str());
  outmsg.Payload(payload);
  return MCC_Status(STATUS_OK);
}

static MCC_Status make_soap_fault(Message& outmsg, bool senderfault, const char* reason1 = NULL, const char* reason2 = NULL, const char* reason3 = NULL) {
  PayloadSOAP* soap = new PayloadSOAP(NS(),true);
  soap->Fault()->Code(senderfault?SOAPFault::Sender:SOAPFault::Receiver);
  std::string reason;
  if(reason1) { if(!reason.empty()) reason+=": "; reason += reason1; };
  if(reason2) { if(!reason.empty()) reason+=": "; reason += reason2; };
  if(reason3) { if(!reason.empty()) reason+=": "; reason += reason3; };
  soap->Fault()->Reason(0, reason.c_str());
  outmsg.Payload(soap);
  return MCC_Status(STATUS_OK);
}

static MCC_Status make_soap_fault(Message& outmsg,Message& oldmsg,bool senderfault,const char* reason = NULL) {
  // Fetch additional info from HTTP and generic content
  std::string http_reason = oldmsg.Attributes()->get("HTTP:REASON");
  MCC_Status ret = make_soap_fault(outmsg, senderfault, reason,
                                                        (!http_reason.empty())?http_reason.c_str():NULL,
                                                        oldmsg.Payload()?ContentFromPayload(*oldmsg.Payload()):NULL);
  // Release original payload
  delete oldmsg.Payload();
  oldmsg.Payload(NULL);
  return ret;
}

MCC_Status MCC_SOAP_Service::process(Message& inmsg,Message& outmsg) {
  // Extracting payload
  MessagePayload* inpayload = inmsg.Payload();
  if(!inpayload) {
    logger.msg(WARNING, "empty input payload");
    return make_raw_fault(outmsg,"Missing incoming request");
  }
  // Check content type from HTTP if present
  std::string mime_type = inmsg.Attributes()->get("HTTP:Content-Type");
  if ((mime_type != "application/soap+xml") && 
      (mime_type != "text/xml") &&
      (mime_type != "application/xml") &&
      (mime_type != "")) {
    if (!_continueNonSoap) {
      logger.msg(WARNING, "MIME is not suitable for SOAP: %s",mime_type);
      return make_raw_fault(outmsg,"Request MIME is not SOAP");
    }
    MCCInterface* next = Next();
    if(!next) {
      logger.msg(WARNING, "empty next chain element");
      return make_raw_fault(outmsg,"Internal error");
    }
    return next->process(inmsg,outmsg); 
  }
  // Converting payload to SOAP
  PayloadSOAP nextpayload(*inpayload);
  if(!nextpayload) {
    if (!_continueNonSoap) {
      logger.msg(WARNING, "incoming message is not SOAP");
      return make_raw_fault(outmsg,"Incoming request is not SOAP");
    }
    MCCInterface* next = Next();
    if(!next) {
      logger.msg(WARNING, "empty next chain element");
      return make_raw_fault(outmsg,"Internal error");
    }
    return next->process(inmsg,outmsg); 
  }
  // Creating message to pass to next MCC and setting new payload.
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

  {
    MCC_Status sret = ProcessSecHandlers(nextinmsg,"incoming");
    if(!sret) {
      logger.msg(ERROR, "Security check failed in SOAP MCC for incoming message: %s",std::string(sret));
      return make_raw_fault(outmsg, "Security check failed for SOAP request", std::string(sret).c_str());
    };
  };
  // TODO: pass SOAP action from HTTP header to SOAP:ACTION attribute
  // Call next MCC 
  MCCInterface* next = Next();
  if(!next) {
    logger.msg(WARNING, "empty next chain element");
    return make_raw_fault(outmsg,"Internal error");
  }
  Message nextoutmsg = outmsg; nextoutmsg.Payload(NULL);
  MCC_Status ret = next->process(nextinmsg,nextoutmsg); 
  // Do checks and extract SOAP response
  if(!ret) {
    if(nextoutmsg.Payload()) delete nextoutmsg.Payload();
    logger.msg(WARNING, "next element of the chain returned error status: %s",(std::string)ret);
    if(ret.getKind() == UNKNOWN_SERVICE_ERROR) {
      return make_raw_fault(outmsg,"No requested service found");
    } else {
      return make_raw_fault(outmsg,"Internal error");
    }
  }
  if(!nextoutmsg.Payload()) {
    logger.msg(WARNING, "next element of the chain returned empty payload");
    return make_raw_fault(outmsg,"There is no response");
  }
  PayloadSOAP* retpayload = NULL;
  try {
    retpayload = dynamic_cast<PayloadSOAP*>(nextoutmsg.Payload());
  } catch(std::exception& e) { };
  if(!retpayload) {
    // There is a chance that next MCC decided to return pre-rendered SOAP
    // or otherwise valid non-SOAP response. For that case we simply pass 
    // it back to previous MCC and let it decide.
    logger.msg(INFO, "next element of the chain returned unknown payload - passing through");
    //Checking authentication and authorization; 
    {
      MCC_Status sret = ProcessSecHandlers(nextoutmsg,"outgoing");
      if(!sret) {
        logger.msg(ERROR, "Security check failed in SOAP MCC for outgoing message: %s",std::string(sret));
        delete nextoutmsg.Payload();
        return make_raw_fault(outmsg,"Security check failed for SOAP response", std::string(sret).c_str());
      };
    };
    outmsg = nextoutmsg;
    return MCC_Status(STATUS_OK);
  };
  if(!(*retpayload)) {
    delete retpayload;
    return make_raw_fault(outmsg,"There is no valid SOAP response");
  };
  //Checking authentication and authorization; 
  {
    MCC_Status sret = ProcessSecHandlers(nextoutmsg,"outgoing");
    if(!sret) {
      logger.msg(ERROR, "Security check failed in SOAP MCC for outgoing message: %s",std::string(sret));
      delete retpayload;
      return make_raw_fault(outmsg,"Security check failed for SOAP response", std::string(sret).c_str());
    };
  };
  // Convert to Raw - TODO: more efficient conversion
  PayloadRaw* outpayload = new PayloadRaw;
  std::string xml; retpayload->GetXML(xml);
  outpayload->Insert(xml.c_str());
  outmsg = nextoutmsg; outmsg.Payload(NULL);
  // Specifying attributes for binding to underlying protocols - HTTP so far
  std::string soap_action;
  bool soap_action_defined = nextoutmsg.Attributes()->count("SOAP:ACTION") > 0;
  if(soap_action_defined) {
    soap_action=nextoutmsg.Attributes()->get("SOAP:ACTION");
  } else {
    soap_action_defined=WSAHeader(*retpayload).hasAction();
    soap_action=WSAHeader(*retpayload).Action();
  };
  if(retpayload->Version() == SOAPEnvelope::Version_1_2) {
    // TODO: For SOAP 1.2 Content-Type is not sent in case of error - probably harmless
    std::string mime_type("application/soap+xml");
    if(soap_action_defined) mime_type+=" ;action=\""+soap_action+"\"";
    outmsg.Attributes()->set("HTTP:Content-Type",mime_type);
  } else {
    outmsg.Attributes()->set("HTTP:Content-Type","text/xml");
    if(soap_action_defined) outmsg.Attributes()->set("HTTP:SOAPAction",soap_action);
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
  delete retpayload;
  outmsg.Payload(outpayload);
  return MCC_Status(STATUS_OK);
}

MCC_Status MCC_SOAP_Client::process(Message& inmsg,Message& outmsg) {
  // Extracting payload
  if(!inmsg.Payload()) return make_soap_fault(outmsg,true,"No message to send");
  PayloadSOAP* inpayload = NULL;
  try {
    inpayload = dynamic_cast<PayloadSOAP*>(inmsg.Payload());
  } catch(std::exception& e) { };
  if(!inpayload) return make_soap_fault(outmsg,true,"No SOAP message to send");
  //Checking authentication and authorization;
  if(!ProcessSecHandlers(inmsg,"outgoing")) {
    logger.msg(ERROR, "Security check failed in SOAP MCC for outgoing message");
    return make_soap_fault(outmsg,true,"Security check failed for outgoing SOAP message");
  };
  // Converting payload to Raw
  PayloadRaw nextpayload;
  std::string xml; inpayload->GetXML(xml);
  nextpayload.Insert(xml.c_str());
  // Creating message to pass to next MCC and setting new payload.. 
  Message nextinmsg = inmsg;
  nextinmsg.Payload(&nextpayload);
  // Specifying attributes for binding to underlying protocols - HTTP so far
  std::string soap_action;
  bool soap_action_defined = inmsg.Attributes()->count("SOAP:ACTION") > 0;
  if(soap_action_defined) {
    soap_action=inmsg.Attributes()->get("SOAP:ACTION");
  } else {
    soap_action_defined=true; //WSAHeader(*inpayload).hasAction(); - SOAPAction must be always present
    soap_action=WSAHeader(*inpayload).Action();
  };
  if(inpayload->Version() == SOAPEnvelope::Version_1_2) {
    std::string mime_type("application/soap+xml");
    if(soap_action_defined) mime_type+=" ;action=\""+soap_action+"\"";
    nextinmsg.Attributes()->set("HTTP:Content-Type",mime_type);
  } else {
    nextinmsg.Attributes()->set("HTTP:Content-Type","text/xml");
    if(soap_action_defined) nextinmsg.Attributes()->set("HTTP:SOAPAction","\""+soap_action+"\"");
  };
  // Call next MCC 
  MCCInterface* next = Next();
  if(!next) return make_soap_fault(outmsg,true,"Internal chain failure: no next component");
  Message nextoutmsg = outmsg; nextoutmsg.Payload(NULL);
  MCC_Status ret = next->process(nextinmsg,nextoutmsg); 
  // Do checks and create SOAP response
  // TODO: pass SOAP action from HTTP header to SOAP:ACTION attribute
  if(!ret) {
    std::string errstr = "Failed to send SOAP message: "+(std::string)ret;
    return make_soap_fault(outmsg,nextoutmsg,false,errstr.c_str());
  };
  MessagePayload* retpayload = nextoutmsg.Payload();
  if(!retpayload) {
    return make_soap_fault(outmsg,nextoutmsg,false,"No response for SOAP message received");
  };
  // Try to interpret response payload as SOAP
  PayloadSOAP* outpayload  = new PayloadSOAP(*retpayload);
  if(!(*outpayload)) {
    delete outpayload;
    return make_soap_fault(outmsg,nextoutmsg,false,"Response is not valid SOAP");
  };
  outmsg = nextoutmsg;
  outmsg.Payload(outpayload);
  delete nextoutmsg.Payload(); nextoutmsg.Payload(NULL);
  //Checking authentication and authorization; 
  if(!ProcessSecHandlers(outmsg,"incoming")) {
    logger.msg(ERROR, "Security check failed in SOAP MCC for incoming message");
    delete outpayload; return make_soap_fault(outmsg,false,"Security check failed for incoming SOAP message");
  };
  return MCC_Status(STATUS_OK);
}

} // namespace ArcMCCSOAP

