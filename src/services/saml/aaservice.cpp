#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <sys/types.h>
#include <pwd.h>

#include <arc/loader/Loader.h>
#include <arc/loader/ClassLoader.h>
#include <arc/loader/ServiceLoader.h>
#include <arc/loader/Plexer.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/Thread.h>
#include <arc/DateTime.h>
#include <arc/GUID.h>

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

#include "aaservice.h"

namespace ArcSec {

static Arc::LogStream logcerr(std::cerr);

static Arc::Service* get_service(Arc::Config *cfg,Arc::ChainContext*) {
    return new Service_AA(cfg);
}

Arc::MCC_Status Service_AA::make_soap_fault(Arc::Message& outmsg) {
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
  Arc::SOAPFault* fault = outpayload?outpayload->Fault():NULL;
  if(fault) {
    fault->Code(Arc::SOAPFault::Sender);
    fault->Reason("Failed processing request");
  };
  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status Service_AA::process(Arc::Message& inmsg,Arc::Message& outmsg) {

  //The following xml structure is according to the definition in lasso.
  //The RemoteProviderID here is meaningless. The useful information is <RemoteNameIdentifier>, which
  //is got from transport level authentication
  std::string identity_aa("<Identity xmlns=\"http://www.entrouvert.org/namespaces/lasso/0.0\" Version=\"2\">\
    <Federation xmlns:saml=\"urn:oasis:names:tc:SAML:2.0:assertion\"\
       RemoteProviderID=\"https://sp.com/SAML\"\
       FederationDumpVersion=\"2\">\
      <RemoteNameIdentifier>\
        <saml:NameID Format=\"urn:oasis:names:tc:SAML:1.1:nameid-format:X509SubjectName\"></saml:NameID>\
      </RemoteNameIdentifier>\
    </Federation>\
   </Identity>");
  Arc::XMLNode id_node(identity_aa);

  //Set the <saml:NameID> inside <RemoteNameIdentifier> by using the DN of peer certificate, 
  //which is the authentication result during tls;
  //<saml:NameID> could also be set by using the authentication result in message level
  //authentication, such as saml token profile in WS-Security.
  //Note: since the <saml:NameID> is obtained from transport level authentication, the request 
  //(with the peer certificate) is exactly the principal inside the <saml:NameID>, which means
  //the request can not be like ServiceProvider in WebSSO scenario which gets the <AuthnQuery>
  //result from IdP, and uses the principal (not the principal of ServiceProvider) in <AuthnQuery> 
  //result to initiate a <AttributeQuery> to AA.
  //Here the scenario is "SAML Attribute Self-Query Deployment Profile for X.509 Subjects" inside 
  //document "SAML V2.0 Deployment Profiles for X.509 Subjects"

  std::string peer_dn = inmsg.Attributes()->get("TLS:PEERDN");
  id_node["Federation"]["RemoteNameIdentifier"]["saml:NameID"] = peer_dn;
  id_node.GetXML(identity_aa);

  lasso_profile_set_identity_from_dump(LASSO_PROFILE(assertion_query_), identity_aa.c_str());

  // Extracting payload
  Arc::PayloadSOAP* inpayload = NULL;
  try {
    inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
  } catch(std::exception& e) { };
  if(!inpayload) {
    logger.msg(Arc::ERROR, "input is not SOAP");
    return make_soap_fault(outmsg);
  }

  std::string assertionRequestBody;
  inpayload->GetXML(assertionRequestBody);
  int rc = lasso_assertion_query_process_request_msg(assertion_query_, (gchar*)(assertionRequestBody.c_str()));
  if(rc != 0) { logger.msg(Arc::ERROR, "lasso_assertion_query_process_request_msg failed"); return Arc::MCC_Status();}

  //Compare the <saml:NameID> inside the <AttributeQuery> message with the <saml:NameID>
  //which has been got from the former authentication
  //More complicated processing should be considered, according to 3.3.4 in SAML core specification
  std::string name_id(LASSO_SAML2_NAME_ID(LASSO_PROFILE(assertion_query_)->nameIdentifier)->content);
  if(name_id == peer_dn) {
    logger.msg(Arc::INFO, "The NameID inside request is the same as the NameID from the tls authentication: %s", peer_dn.c_str());
  }
  else {
    logger.msg(Arc::INFO, "The NameID inside request is: %s; not the same as the NameID from the tls authentication: %s",\
      name_id.c_str(), peer_dn.c_str());
    return Arc::MCC_Status();
  }

  //Get the <Attribute>s from <AttributeQuery> message, which is required by request; 
  //AA will only return those <Attribute> which is required by request
  LassoSamlp2AttributeQuery *attribute_query;
  attribute_query = LASSO_SAMLP2_ATTRIBUTE_QUERY(LASSO_PROFILE(assertion_query_)->request);
  GList *attrs = (GList*)(attribute_query->Attribute);

  rc = lasso_assertion_query_validate_request(assertion_query_);

  //AA: Generate a response 
  //AA will try to query local attribute database, intersect the attribute result from
  //database and the attribute requirement from the request
  //Then, insert those <Attribute> into response

  //TODO: access the local attribute database, probabaly by using the <NameID> as searching key
  LassoSaml2Attribute *attribute;
  std::list<std::string> attribute_name_list;
  int length = g_list_length(attrs);
  for(int i=0; i<length; i++) {
    attribute = (LassoSaml2Attribute*)g_list_nth_data(attrs, i);
    if(attribute->Name != NULL) { 
      std::string attribute_name(attribute->Name);
      attribute_name_list.push_back(attribute_name); 
    }
    else {
      logger.msg(Arc::ERROR, "There should be Name attribute in request's <Attribute> node");
      return Arc::MCC_Status();
    }
  }
  //TODO: Compare the attribute name from database result and the attribute_name_list,
  //Only use the intersect as the response
  
  //The following is just one <Attribute> result for test. The real <Attribute> 
  //should be compose according to the database searching result
  attribute = LASSO_SAML2_ATTRIBUTE(lasso_saml2_attribute_new());
  attribute->Name = g_strdup("urn:oid:1.3.6.1.4.1.5923.1.1.1.6");
  attribute->NameFormat = g_strdup("urn:oasis:names:tc:SAML:2.0:attrname-format:uri");
  attribute->FriendlyName = g_strdup("eduPersonPrincipalName");
  LassoSaml2AttributeValue *attrval;
  attrval= LASSO_SAML2_ATTRIBUTE_VALUE(lasso_saml2_attribute_value_new());
  //attrval->any = g_list_prepend(NULL, g_strdup("RoleA"));
  //Add one or more <AttributeValue> into <Attribute>
  attribute->AttributeValue = g_list_append(attribute->AttributeValue, attrval);

  //Compose <Assertion>
  LassoSaml2Assertion *assertion;
  LassoSaml2AudienceRestriction *audience_restriction;
  
  assertion = LASSO_SAML2_ASSERTION(lasso_saml2_assertion_new());
  std::string id = Arc::UUID();
  assertion->ID = g_strdup(id.c_str());
  assertion->Version = g_strdup("2.0");
  Arc::Time t;
  std::string current_time = t.str(Arc::UTCTime); 
  assertion->IssueInstant = g_strdup(current_time.c_str());
  assertion->Issuer = LASSO_SAML2_NAME_ID(lasso_saml2_name_id_new_with_string(
                          LASSO_PROVIDER(LASSO_PROFILE(assertion_query_)->server)->ProviderID));

  assertion->Conditions = LASSO_SAML2_CONDITIONS(lasso_saml2_conditions_new());
  audience_restriction = LASSO_SAML2_AUDIENCE_RESTRICTION(
                          lasso_saml2_audience_restriction_new());
  audience_restriction->Audience = g_strdup(LASSO_PROFILE(assertion_query_)->remote_providerID);
  assertion->Conditions->AudienceRestriction = g_list_append(NULL, audience_restriction);

  assertion->Subject = LASSO_SAML2_SUBJECT(lasso_saml2_subject_new());

  //Get the NameID, and put it into <Assertion>
  LassoFederation *federation = NULL;
  federation = LASSO_FEDERATION(g_hash_table_lookup(LASSO_PROFILE(assertion_query_)->identity->federations,
                          LASSO_PROFILE(assertion_query_)->remote_providerID));
  if (federation == NULL) { logger.msg(Arc::ERROR, "Can't find federation for identity"); return Arc::MCC_Status();}
  if (federation->remote_nameIdentifier) {
    assertion->Subject->NameID = LASSO_SAML2_NAME_ID(g_object_ref(federation->remote_nameIdentifier));
  }
  else {
    logger.msg(Arc::ERROR, "Can not find NameID");
  }

  LassoSaml2AttributeStatement *attribute_statement;
  attribute_statement = LASSO_SAML2_ATTRIBUTE_STATEMENT(lasso_saml2_attribute_statement_new());
  //Add one or more <Attribute> into <Assertion>
  attribute_statement->Attribute = g_list_append(attribute_statement->Attribute, attribute);

  assertion->AttributeStatement= g_list_append(assertion->AttributeStatement, attribute_statement);


   /* Save signing material in assertion private datas to be able to sign later */
  if (LASSO_PROFILE(assertion_query_)->server->certificate) {
          assertion->sign_type = LASSO_SIGNATURE_TYPE_WITHX509;
  } else {
          assertion->sign_type = LASSO_SIGNATURE_TYPE_SIMPLE;
  }
  assertion->sign_method = LASSO_PROFILE(assertion_query_)->server->signature_method;
  assertion->private_key_file = g_strdup(LASSO_PROFILE(assertion_query_)->server->private_key);
  assertion->certificate_file = g_strdup(LASSO_PROFILE(assertion_query_)->server->certificate);
  
  LassoSamlp2Response *attribute_response;
  attribute_response = LASSO_SAMLP2_RESPONSE(LASSO_PROFILE(assertion_query_)->response);
  attribute_response->Assertion= g_list_append(attribute_response->Assertion, assertion);

  //Build the response soap message
  rc = lasso_assertion_query_build_response_msg(assertion_query_);
  if(rc != 0 ) { logger.msg(Arc::ERROR, "lasso_assertion_query_build_response_msg failed"); return Arc::MCC_Status(); }
  
  std::string assertionResponseBody(LASSO_PROFILE(assertion_query_)->msg_body);
  if(assertionResponseBody.empty()) { logger.msg(Arc::ERROR, "assertionResponseBody shouldn't be NULL"); return Arc::MCC_Status(); }
  
  //Conver the lasso-generated soap message into Arc's PayloasSOAP.
  Arc::SOAPEnvelope envelope(assertionResponseBody);

  Arc::PayloadSOAP *outpayload = new Arc::PayloadSOAP(envelope);

  std::string tmp;
  outpayload->GetXML(tmp);
  std::cout<<"Output payload: ----"<<tmp<<std::endl;

  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Service_AA::Service_AA(Arc::Config *cfg):Service(cfg), logger_(Arc::Logger::rootLogger, "AA_Service") {
  logger_.addDestination(logcerr);
  assertion_query_ = NULL;
  lasso_init();
  LassoServer *server;
  server = lasso_server_new("./aa_saml_metadata.xml", "./key.pem", NULL, "./cert.pem"); 
  lasso_server_add_provider(server, LASSO_PROVIDER_ROLE_SP, "./sp_saml_metadata.xml", "./cert.pem", "./ca.pem");
  assertion_query_ = lasso_assertion_query_new (server);
  if(assertion_query_ == NULL) logger.msg(Arc::DEBUG, "lasso_assertion_query_new() failed"); 
}

Service_AA::~Service_AA(void) {
  if(assertion_query_ != NULL)
    lasso_assertion_query_destroy(assertion_query_);
  assertion_query_ = NULL;
}

} // namespace ArcSec

service_descriptors ARC_SERVICE_LOADER = {
    { "aa.service", 0, &ArcSec::get_service },
    { NULL, 0, NULL }
};

