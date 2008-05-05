#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <signal.h>

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/loader/Loader.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/arclib/Credential.h>

#include <lasso/lasso.h>
#include <lasso/saml-2.0/assertion_query.h>
#include <lasso/xml/saml-2.0/saml2_attribute.h>
#include <lasso/xml/saml-2.0/saml2_attribute_value.h>
#include <lasso/xml/saml-2.0/samlp2_attribute_query.h>
#include <lasso/xml/saml-2.0/saml2_assertion.h>
#include <lasso/xml/saml-2.0/saml2_audience_restriction.h>
#include <lasso/xml/saml-2.0/samlp2_response.h>
#include <lasso/xml/saml-2.0/saml2_attribute_statement.h>
#include <lasso/xml/xml.h>


///A example about how to compose a SAML <AttributeQuery> and call the Service_AA service, by
///using lasso library to compose <AttributeQuery> and process the <Response>. A example with 
/// the same functioanlity but by using opensaml library will be available later.
int main(void) {
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  signal(SIGPIPE,SIG_IGN);
  Arc::Logger logger(Arc::Logger::rootLogger, "SAMLTest");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);

  // Load service chain
  logger.msg(Arc::INFO, "Creating service side chain");
  Arc::Config service_config("service.xml");
  if(!service_config) {
    logger.msg(Arc::ERROR, "Failed to load service configuration");
    return -1;
  };
  Arc::Loader service_loader(&service_config);
  logger.msg(Arc::INFO, "Service side MCCs are loaded");
  logger.msg(Arc::INFO, "Creating client side chain");

  // Create client chain
  Arc::Config client_config("client.xml");
  if(!client_config) {
    logger.msg(Arc::ERROR, "Failed to load client configuration");
    return -1;
  };
  Arc::Loader client_loader(&client_config);
  logger.msg(Arc::INFO, "Client side MCCs are loaded");
  Arc::MCC* client_entry = client_loader["soap"];
  if(!client_entry) {
    logger.msg(Arc::ERROR, "Client chain does not have entry point");
    return -1;
  };
  
  // -------------------------------------------------------
  //    Compose request and send to aa service
  // -------------------------------------------------------
  //Compose request
  LassoAssertionQuery *assertion_query;
  assertion_query = NULL;
  lasso_init();
  LassoServer *server;
  server = lasso_server_new("./sp_saml_metadata.xml", "./sp_private-key-raw.pem", NULL, "./sp_public-key.pem");
  lasso_server_add_provider(server, LASSO_PROVIDER_ROLE_ATTRIBUTEAUTHORITY, "./aa_saml_metadata.xml", "./aa_public-key.pem", "./ca.pem");
  assertion_query_ = lasso_assertion_query_new (server);
  if(assertion_query_ == NULL) { logger.msg(Arc::DEBUG, "lasso_assertion_query_new() failed"); return -1; }

  //The LocalNameIdentifier could be parsed from the local credential
  std::string identity_sp("<Identity xmlns=\"http://www.entrouvert.org/namespaces/lasso/0.0\" Version=\"2\">\
    <Federation xmlns:saml=\"urn:oasis:names:tc:SAML:2.0:assertion\"\
       RemoteProviderID=\"https://idp.com/SAML\"\
       FederationDumpVersion=\"2\">\
      <LocalNameIdentifier>\
        <saml:NameID Format=\"urn:oasis:names:tc:SAML:1.1:nameid-format:X509SubjectName\"></saml:NameID>\
      </LocalNameIdentifier>\
    </Federation>\
   </Identity>");
  std::string cert("./sp_public-key.pem");
  std::string key("./sp_private-key-raw.pem");
  std::string cafile("./ca.pem");
  ArcLib::Credential cred(cert, key, "", cafile);
  std::string local_dn = cred.GetDN();

  Arc::XMLNode id_node(identity_sp);
  id_node["Federation"]["LocalIdentifier"]["saml:NameID"] = local_dn;
  //The "RemoteProviderID" could be also set here
  id_node.GetXML(identity_sp);

  lasso_profile_set_identity_from_dump(LASSO_PROFILE(assertion_query), identity_sp.c_str());

  std::string remote_provide_id = (std::string)(id_node["Federation"].Attribute("RemoteProviderID"));
  int rc = lasso_assertion_query_init_request(assertion_query, remote_provide_id.c_str(), LASSO_HTTP_METHOD_SOAP, LASSO_ASSERTION_QUERY_REQUEST_TYPE_ATTRIBUTE);
  if(rc != 0) { logger.msg(Arc::ERROR, "lasso_assertion_query_init_request failed"); return -1; }

  //Add one or more <Attribute>s into AttributeQuery here, which means the Requestor would
  //get these <Attribute>s from AA
  LassoSaml2Attribute *attribute;
  attribute = LASSO_SAML2_ATTRIBUTE(lasso_saml2_attribute_new());
  attribute->Name = g_strdup("urn:oid:1.3.6.1.4.1.5923.1.1.1.6");
  attribute->NameFormat = g_strdup("urn:oasis:names:tc:SAML:2.0:attrname-format:uri");
  attribute->FriendlyName = g_strdup("eduPersonPrincipalName");
  //Since here the request only would get the attributevalue from the AA,
  //we don't specify the AttributeValue inside the Attribute.

  LassoSamlp2AttributeQuery *attribute_query;
  attribute_query = LASSO_SAMLP2_ATTRIBUTE_QUERY(LASSO_PROFILE(assertion_query)->request);
  attribute_query->Attribute = g_list_append(attribute_query>Attribute, attribute);

  //Build the soap message
  rc = lasso_assertion_query_build_request_msg(assertion_query);
  if(rc != 0) { logger.msg(Arc::ERROR, "lasso_assertion_query_build_request_msg failed"); return -1; }

  std::string assertionRequestBody(LASSO_PROFILE(assertion_query)->msg_body);
  if(assertionRequestBody.empty()) { logger.msg(Arc::ERROR,"assertionRequestBody shouldn't be NULL"); return -1; }
  std::cout<<"Request: ---assertionRequestBody: "<<assertionRequestBody<<std::endl;

  //Conver the lasso-generated soap message into Arc's PayloasSOAP. It could be not efficient here
  Arc::XMLNode request_soap(assertionRequestBody);
  Arc::XMLNode request_saml_attributequery = request_soap["Envelop"]["Body"].Child(0);
  Arc::SOAPEnvelope envelope(request_saml_attributequery);

  Arc::PayloadSOAP *payload = new Arc::PayloadSOAP(envelope);
 
  // Send request
  Arc::MessageContext context;
  Arc::Message reqmsg;
  Arc::Message repmsg;
  Arc::MessageAttributes attributes_in;
  Arc::MessageAttributes attributes_out;
  reqmsg.Payload(payload);
  reqmsg.Attributes(&attributes_in);
  reqmsg.Context(&context);
  repmsg.Attributes(&attributes_out);
  repmsg.Context(&context);

  Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
  if(!status) {
    logger.msg(Arc::ERROR, "Request failed");
    return -1;
  };
  logger.msg(Arc::INFO, "Request succeed!!!");
  if(repmsg.Payload() == NULL) {
    logger.msg(Arc::ERROR, "There is no response");
    return -1;
  };
  Arc::PayloadSOAP* resp = NULL;
  try {
   resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
  } catch(std::exception&) { };
  if(resp == NULL) {
    logger.msg(Arc::ERROR, "Response is not SOAP");
    delete repmsg.Payload();
    return -1;
  };
  
  std::string str;
  resp->GetXML(str);
  logger.msg(Arc::INFO, "Response: %s", str);
  
  delete repmsg.Payload();

  return 0;
}
