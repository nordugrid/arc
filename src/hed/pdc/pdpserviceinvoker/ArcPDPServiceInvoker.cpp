#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdexcept>

#include <arc/XMLNode.h>
#include <arc/Thread.h>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/message/MCC.h>
//#include <arc/security/ArcPDP/Response.h>
//#include <arc/security/ArcPDP/attr/AttributeValue.h>

#include "ArcPDPServiceInvoker.h"

Arc::Logger ArcSec::ArcPDPServiceInvoker::logger(ArcSec::PDP::logger,"ArcPDPServiceInvoker");

using namespace Arc;

namespace ArcSec {
PDP* ArcPDPServiceInvoker::get_pdpservice_invoker(Config *cfg,ChainContext*) {
    return new ArcPDPServiceInvoker(cfg);
}

ArcPDPServiceInvoker::ArcPDPServiceInvoker(Config* cfg):PDP(cfg), client(NULL) {
  XMLNode filter = (*cfg)["Filter"];
  if((bool)filter) {
    XMLNode select_attr = filter["Select"];
    XMLNode reject_attr = filter["Reject"];
    for(;(bool)select_attr;++select_attr) select_attrs.push_back((std::string)select_attr);
    for(;(bool)reject_attr;++reject_attr) reject_attrs.push_back((std::string)reject_attr);
  };

  //Create a SOAP client
  logger.msg(Arc::INFO, "Creating a pdpservice client");

  std::string url_str;
  url_str = (std::string)((*cfg)["ServiceEndpoint"]);
  Arc::URL url(url_str);

  Arc::MCCConfig mcc_cfg;
  mcc_cfg.AddPrivateKey((std::string)((*cfg)["KeyPath"]));
  mcc_cfg.AddCertificate((std::string)((*cfg)["CertificatePath"]));
  mcc_cfg.AddProxy((std::string)((*cfg)["ProxyPath"]));
  mcc_cfg.AddCAFile((std::string)((*cfg)["CACertificateFile"]));
  mcc_cfg.AddCADir((std::string)((*cfg)["CACertificatesDir"]));

  client = new Arc::ClientSOAP(mcc_cfg,url.Host(),url.Port(),url.Protocol() == "https",url.Path());
}

bool ArcPDPServiceInvoker::isPermitted(Message *msg){
  //Compose the request
  MessageAuth* mauth = msg->Auth()->Filter(select_attrs,reject_attrs);
  MessageAuth* cauth = msg->AuthContext()->Filter(select_attrs,reject_attrs);
  if((!mauth) && (!cauth)) {
    logger.msg(ERROR,"Missing security object in message");
    return false;
  };
  NS ns;
  XMLNode requestxml(ns,"");
  if(mauth) {
    if(!mauth->Export(SecAttr::ARCAuth,requestxml)) {
      delete mauth;
      logger.msg(ERROR,"Failed to convert security information to ARC request");
      return false;
    };
    delete mauth;
  };
  if(cauth) {
    if(!cauth->Export(SecAttr::ARCAuth,requestxml)) {
      delete mauth;
      logger.msg(ERROR,"Failed to convert security information to ARC request");
      return false;
    };
    delete cauth;
  };
  {
    std::string s;
    requestxml.GetXML(s);
    logger.msg(VERBOSE,"ARC Auth. request: %s",s);
  };
  if(requestxml.Size() <= 0) {
    logger.msg(ERROR,"No requested security information was collected");
    return false;
  };

  //Invoke the remote pdp service

  Arc::NS req_ns;
  req_ns["ra"] = "http://www.nordugrid.org/schemas/request-arc";
  req_ns["pdp"] = "http://www.nordugrid.org/schemas/pdp";
  Arc::PayloadSOAP req(req_ns);
  Arc::XMLNode reqnode = req.NewChild("pdp:GetPolicyDecisionRequest");
  reqnode.NewChild(requestxml);

  Arc::PayloadSOAP* resp = NULL;
  if(client) {
    Arc::MCC_Status status = client->process(&req,&resp);
    if(!status) {
      logger.msg(Arc::ERROR, "Policy Decision Service invokation failed");
      throw std::runtime_error("Policy Decision Service invokation failed");
    }
    if(resp == NULL) {
      logger.msg(Arc::ERROR,"There was no SOAP response");
      throw std::runtime_error("There was no SOAP response");
    }
  }

  std::string str;
  resp->GetXML(str);
  logger.msg(Arc::INFO, "Response: %s", str);

  if(resp) delete resp;

  //TODO

  return true;
}

ArcPDPServiceInvoker::~ArcPDPServiceInvoker(){
}

} // namespace ArcSec

