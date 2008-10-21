#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "MCCMsgValidator.h"

#include <arc/message/PayloadRaw.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/XMLNode.h>
#include <arc/loader/MCCLoader.h>
#include <arc/ws-addressing/WSA.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xmlschemas.h>
#include <libxml/xmlstring.h>

Arc::Logger Arc::MCC_MsgValidator::logger(Arc::MCC::logger,"MsgValidator");


Arc::MCC_MsgValidator::MCC_MsgValidator(Arc::Config *cfg) : MCC(cfg) {
    // Collect services to be validated
    for(int i = 0;;++i) {
        XMLNode n = (*cfg)["ValidatedService"][i];
        if(!n) break;
        std::string servicepath = n["ServicePath"];
        if(servicepath.empty()) {
            //missing path
            logger.msg(Arc::WARNING, "Skipping service: no ServicePath found!");
            continue;
        };
        std::string schemapath = n["SchemaPath"];
        if(schemapath.empty()) {
            //missing path
            logger.msg(Arc::WARNING, "Skipping service: no SchemaPath found!");
            continue;
        };

        // register schema path with service
        schemas[servicepath] = schemapath;
    }
}

static Arc::MCC* get_mcc_service(Arc::Config *cfg,Arc::ChainContext*) {
    return new Arc::MCC_MsgValidator_Service(cfg);
}

static Arc::MCC* get_mcc_client(Arc::Config *cfg,Arc::ChainContext*) {
    return new Arc::MCC_MsgValidator_Client(cfg);
}

mcc_descriptors ARC_MCC_LOADER = {
    { "msg.validator.service", 0, &get_mcc_service },
    { "msg.validator.client", 0, &get_mcc_client },
    { NULL, 0, NULL }
};

namespace Arc {

MCC_MsgValidator_Service::MCC_MsgValidator_Service(Arc::Config *cfg):MCC_MsgValidator(cfg) {
}

MCC_MsgValidator_Service::~MCC_MsgValidator_Service(void) {
}

MCC_MsgValidator_Client::MCC_MsgValidator_Client(Arc::Config *cfg):MCC_MsgValidator(cfg) {
}

MCC_MsgValidator_Client::~MCC_MsgValidator_Client(void) {
}

std::string MCC_MsgValidator::getSchemaPath(std::string servicePath) {
    // Look for servicePath in the map.
    // Using a const_iterator since we are not going to change the values.
    for(std::map<std::string,std::string>::const_iterator iter = schemas.begin(); iter != schemas.end(); ++iter)
    {
        if(iter->first == servicePath) {
            // found servicePath; returning schemaPath
            return iter->second;
        }
    }
    // nothing found; returning empty path
    return "";
}

bool MCC_MsgValidator::validateMessage(Message& msg, std::string schemaPath){
    // create parser ctxt for schema accessible on schemaPath
    xmlSchemaParserCtxtPtr schemaParserP = xmlSchemaNewParserCtxt(schemaPath.c_str());

    if(!schemaParserP) {
        // could not create context
        logger.msg(ERROR, "Parser Context creation failed!");
        return false;
    }

    // parse schema
    xmlSchemaPtr schemaP = xmlSchemaParse(schemaParserP);

    if(!schemaP) {
        // could not parse schema
        logger.msg(ERROR, "Cannot parse schema!");
        // have to free parser ctxt
        xmlSchemaFreeParserCtxt(schemaParserP);
        return false;
    }

    // we do not need schemaParserP any more, so it can be freed
    xmlSchemaFreeParserCtxt(schemaParserP);

    // Extracting payload
    MessagePayload* payload = msg.Payload();
    if(!payload) {
        logger.msg(ERROR, "Empty payload!");
        return false;
    }

    // Converting payload to SOAP
    PayloadSOAP* plsp = NULL; 
    plsp = dynamic_cast<PayloadSOAP*>(payload);
    if(!plsp) {
        // cast failed
        logger.msg(ERROR, "Could not convert payload!");
        return false;
    }
    PayloadSOAP soapPL(*plsp);

    if(!soapPL) {
        logger.msg(ERROR, "Could not create PayloadSOAP!");
        return false;
    }

    std::string arcPSstr;
    // get SOAP payload as string
    soapPL.GetXML(arcPSstr);

    // parse string into libxml2 xmlDoc
    xmlDocPtr lxdocP = xmlParseDoc(xmlCharStrdup(arcPSstr.c_str()));

    // create XPath context; later, we will have to free it!
    xmlXPathContextPtr xpCtxtP = xmlXPathNewContext(lxdocP);

    // content is the first child _element_ of SOAP Body
    std::string exprstr = "//*[local-name()='Body' and namespace-uri()='http://schemas.xmlsoap.org/soap/envelope/'][1]/*[1]";

    // result is a xmlXPathObjectPtr; later, we will have to free it!
    xmlXPathObjectPtr xpObP = xmlXPathEval(xmlCharStrdup(exprstr.c_str()),xpCtxtP);

    // xnsP is the set of result nodes
    xmlNodeSetPtr xnsP = xpObP->nodesetval;

    // the set has only one member (content) - see above (exprstr)
    xmlNodePtr content = xnsP->nodeTab[0];
    
    // create a new xmlDoc for content
    xmlDocPtr doc = xmlNewDoc(xmlCharStrdup("1.0"));

    // create schema validation context
    xmlSchemaValidCtxtPtr validity_ctxt_ptr = xmlSchemaNewValidCtxt(schemaP);

    // copy & add content to doc as a child
    xmlNodePtr tmpNode = xmlDocCopyNode(content, doc, 1);
    xmlAddChild((xmlNodePtr)doc, tmpNode);

    // validate against schema
    bool result = (xmlSchemaValidateDoc(validity_ctxt_ptr, doc) == 0);

    // free resources and return result
    xmlSchemaFreeValidCtxt(validity_ctxt_ptr);

    xmlSchemaFree(schemaP);

    xmlFreeDoc(doc);

    xmlFreeDoc(lxdocP);

    xmlXPathFreeContext(xpCtxtP);

    xmlXPathFreeObject(xpObP);

    return result;
}

static MCC_Status make_raw_fault(Message& outmsg,const char* = NULL) 
{
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

MCC_Status MCC_MsgValidator_Service::process(Message& inmsg,Message& outmsg) {
  // Extracting payload
  MessagePayload* inpayload = inmsg.Payload();
  if(!inpayload) {
    logger.msg(WARNING, "Empty input payload!");
    return make_raw_fault(outmsg);
  }

  // Converting payload to SOAP
  PayloadSOAP* plsp = NULL;
  plsp = dynamic_cast<PayloadSOAP*>(inpayload);
  if(!plsp) {
      // cast failed
      logger.msg(ERROR, "Could not convert incoming payload!");
      return make_raw_fault(outmsg);
  }

  PayloadSOAP nextpayload(*plsp);
  
  if(!nextpayload) {
    logger.msg(ERROR, "Could not create payloadSOAP!");
    return make_raw_fault(outmsg);
  }

  // Creating message to pass to next MCC and setting new payload.. 
  // Using separate message. But could also use same inmsg.
  // Just trying to keep it intact as much as possible.
  Message nextinmsg = inmsg;
  nextinmsg.Payload(&nextpayload);
  
  std::string endpoint_attr = inmsg.Attributes()->get("ENDPOINT");

  // extract service path
  std::string servicePath = getPath(endpoint_attr);

  // check config fog corresponding service
  std::string schemaPath = getSchemaPath(servicePath);

  if("" == schemaPath) {
      // missing schema
      logger.msg(WARNING, "Missing schema! Skipping validation...");
  } else {
      // try to validate message against service schema
      if(!validateMessage(nextinmsg,schemaPath)) {
          // message validation failed for some reason
          logger.msg(ERROR, "Could not validate message!");
          return make_raw_fault(outmsg);
      }
  }

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

  // replace old payload with retpayload
  // then delete old payload
  delete outmsg.Payload(retpayload);
  return MCC_Status(Arc::STATUS_OK);
}

std::string MCC_MsgValidator_Service::getPath(std::string url){
    std::string::size_type ds, ps;
    ds=url.find("//");
    if (ds==std::string::npos)
        ps=url.find("/");
    else
        ps=url.find("/", ds+2);
    if (ps==std::string::npos)
        return "";
    else
        return url.substr(ps);
}

MCC_Status MCC_MsgValidator_Client::process(Message& inmsg,Message& outmsg) {
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

  return MCC_Status(Arc::STATUS_OK);
}

} // namespace Arc
