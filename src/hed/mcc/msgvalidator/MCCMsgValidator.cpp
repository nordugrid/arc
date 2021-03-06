#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "MCCMsgValidator.h"

#include <arc/message/PayloadRaw.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/XMLNode.h>
#include <arc/loader/Plugin.h>
#include <arc/ws-addressing/WSA.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xmlschemas.h>
//#include <libxml/xmlstring.h>

Arc::Logger ArcMCCMsgValidator::MCC_MsgValidator::logger(Arc::Logger::getRootLogger(), "MCC.MsgValidator");


ArcMCCMsgValidator::MCC_MsgValidator::MCC_MsgValidator(Arc::Config *cfg,PluginArgument* parg) : Arc::MCC(cfg,parg) {
    // Collect services to be validated
    for(int i = 0;;++i) {
        Arc::XMLNode n = (*cfg)["ValidatedService"][i];
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

static Arc::Plugin* get_mcc_service(Arc::PluginArgument* arg) {
    Arc::MCCPluginArgument* mccarg =
            arg?dynamic_cast<Arc::MCCPluginArgument*>(arg):NULL;
    if(!mccarg) return NULL;
    return new ArcMCCMsgValidator::MCC_MsgValidator_Service((Arc::Config*)(*mccarg),mccarg);
}

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
    { "msg.validator.service", "HED:MCC", NULL, 0, &get_mcc_service },
    { "msg.validator.client", "HED:MCC", NULL, 0, &get_mcc_service },
    { NULL, NULL, NULL, 0, NULL }
};

namespace ArcMCCMsgValidator {

using namespace Arc;

MCC_MsgValidator_Service::MCC_MsgValidator_Service(Config *cfg,PluginArgument* parg):MCC_MsgValidator(cfg,parg) {
}

MCC_MsgValidator_Service::~MCC_MsgValidator_Service(void) {
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
  return MCC_Status(GENERIC_ERROR);
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
    logger.msg(ERROR, "Could not create PayloadSOAP!");
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
    if(nextoutmsg.Payload()) delete nextoutmsg.Payload();
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
  return MCC_Status(STATUS_OK);
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

} // namespace ArcMCCMsgValidator

