#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <arc/loader/SecHandlerLoader.h>
#include <arc/loader/Loader.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/PayloadRaw.h>
#include <arc/URL.h>
#include <arc/DateTime.h>
#include <arc/GUID.h>
#include <arc/XMLNode.h>
#include <arc/xmlsec/XmlSecUtils.h>
#include <arc/xmlsec/saml_util.h>

#include "SAML2SSO_ServiceProviderSH.h"

static Arc::Logger logger(Arc::Logger::rootLogger, "SAMLSSO_ServiceProviderSH");

ArcSec::SecHandler* ArcSec::SAML2SSO_ServiceProviderSH::get_sechandler(Arc::Config *cfg, Arc::ChainContext* ctx) {
    return new ArcSec::SAML2SSO_ServiceProviderSH(cfg,ctx);
}

/*
sechandler_descriptors ARC_SECHANDLER_LOADER = {
    { "saml2ssoserviceprovider.handler", 0, &get_sechandler},
    { NULL, 0, NULL }
};
*/

namespace ArcSec {
using namespace Arc;

SAML2SSO_ServiceProviderSH::SAML2SSO_ServiceProviderSH(Config *cfg,ChainContext*):SecHandler(cfg), SP_service_loader(NULL){
  if(!init_xmlsec()) return;
#if 0
  //Initialize an embeded http service for accepting saml request from client side
  //Load service chain
  logger.msg(Arc::INFO, "Creating http service side chain");
  Arc::XMLNode service_doc("\
    <ArcConfig\
      xmlns=\"http://www.nordugrid.org/schemas/ArcConfig/2007\"\
      xmlns:tcp=\"http://www.nordugrid.org/schemas/ArcMCCTCP/2007\"\
      xmlns:tls=\"http://www.nordugrid.org/schemas/ArcMCCTLS/2007\">\
     <ModuleManager>\
        <Path>.libs/</Path>\
        <Path>../../hed/mcc/http/.libs/</Path>\
        <Path>../../hed/mcc/tls/.libs/</Path>\
        <Path>../../hed/mcc/tcp/.libs/</Path>\
        <Path>../../hed/pdc/saml2sso_serviceprovidersh/.libs/</Path>\
     </ModuleManager>\
     <Plugins><Name>mcctcp</Name></Plugins>\
     <Plugins><Name>mcctls</Name></Plugins>\
     <Plugins><Name>mcchttp</Name></Plugins>\
     <Plugins><Name>saml2sp</Name></Plugins>\
     <Chain>\
      <Component name='tcp.service' id='tcp'><next id='tls'/><tcp:Listen><tcp:Port>8443</tcp:Port></tcp:Listen></Component>\
      <Component name='tls.service' id='tls'><next id='http'/>\
        <tls:KeyPath>testkey-nopass.pem</tls:KeyPath>\
        <tls:CertificatePath>testcert.pem</tls:CertificatePath>\
        <tls:CACertificatesDir>./certificates</tls:CACertificatesDir>\
      </Component>\
      <Component name='http.service' id='http'>\
        <next id='samlsp'>POST</next>\
      </Component>\
      <Service name='saml.sp' id='samlsp'/>\
     </Chain>\
    </ArcConfig>");
  Arc::Config service_config(service_doc);

  if(!service_config) {
    logger.msg(Arc::ERROR, "Failed to load service configuration");
    return;
  };
  SP_service_loader = new Arc::Loader(&service_config);
  logger.msg(Arc::INFO, "Service side MCCs are loaded");
  logger.msg(Arc::INFO, "Service is waiting for requests");
#endif

}

SAML2SSO_ServiceProviderSH::~SAML2SSO_ServiceProviderSH() {
  final_xmlsec();
  if(SP_service_loader) delete SP_service_loader;
}

bool SAML2SSO_ServiceProviderSH::Handle(Arc::Message* msg){
  //TODO: Consume the saml assertion from client side (Push model): assertion inside soap message, 
  //assertion inside x509 certificate as exention;
  //Or contact the IdP and get back the saml assertion related to the client(Pull model)

  return true;
}

}


