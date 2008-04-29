#include <iostream>

#include <stdlib.h>
#include <string>

#include <lasso/lasso.h>
#include <lasso/saml-2.0/assertion_query.h>

static char*
generateIdentityProviderContextDump()
{
  LassoServer *serverContext;
	
  serverContext = lasso_server_new(
                   TESTSDATADIR "/idp1-la/saml_metadata.xml",
                   TESTSDATADIR "/idp1-la/private-key-raw.pem",
                   NULL, /* Secret key to unlock private key */
                   TESTSDATADIR "/idp1-la/certificate.pem");
  lasso_server_add_provider(
                   serverContext,
                   LASSO_PROVIDER_ROLE_SP,
                   TESTSDATADIR "/sp1-la/saml_metadata.xml",
                   TESTSDATADIR "/sp1-la/public-key.pem",
                   TESTSDATADIR "/ca1-la/certificate.pem");
  return lasso_server_dump(serverContext);
}

static char*
generateServiceProviderContextDump()
{
  LassoServer *serverContext;
	
  serverContext = lasso_server_new(
                   TESTSDATADIR "/sp1-la/saml_metadata.xml",
                   TESTSDATADIR "/sp1-la/private-key-raw.pem",
                   NULL, /* Secret key to unlock private key */
                   TESTSDATADIR "/sp1-la/certificate.pem");
  lasso_server_add_provider(
                   serverContext,
                   LASSO_PROVIDER_ROLE_ATTRIBUTEAUTHORITY,
                   TESTSDATADIR "/idp1-la/saml_metadata.xml",
                   TESTSDATADIR "/idp1-la/public-key.pem",
                   TESTSDATADIR "/ca1-la/certificate.pem");
  return lasso_server_dump(serverContext);
}

int main(void) {

  lasso_init();

  char *serviceProviderContextDump, *identityProviderContextDump;
  LassoServer *spContext, *idpContext;
  LassoAssertionQuery *assertionqry, *assertionqry1;
  int rc;
  char *assertionRequestBody = NULL;
  char *assertionResponseBody = NULL;

  std::string identity_sp("<Identity xmlns=\"http://www.entrouvert.org/namespaces/lasso/0.0\" Version=\"2\">\
    <Federation xmlns:saml=\"urn:oasis:names:tc:SAML:2.0:assertion\"\
       RemoteProviderID=\"https://idp.com/SAML\"\
       FederationDumpVersion=\"2\">\
      <RemoteNameIdentifier>\
        <saml:NameID Format=\"urn:oasis:names:tc:SAML:2.0:nameid-format:persistent\"\
         NameQualifier=\"http://idp.localhost\"\
         SPNameQualifier=\"http://sp.localhost/\">abcdefg</saml:NameID>\
      </RemoteNameIdentifier>\
    </Federation>\
   </Identity>");

  std::string identity_idp("<Identity xmlns=\"http://www.entrouvert.org/namespaces/lasso/0.0\" Version=\"2\">\
    <Federation xmlns:saml=\"urn:oasis:names:tc:SAML:2.0:assertion\"\
       RemoteProviderID=\"https://sp.com/SAML\"\
       FederationDumpVersion=\"2\">\
      <RemoteNameIdentifier>\
        <saml:NameID Format=\"urn:oasis:names:tc:SAML:2.0:nameid-format:persistent\"\
         NameQualifier=\"http://idp.localhost\"\
         SPNameQualifier=\"http://sp.localhost/\">abcdefg</saml:NameID>\
      </RemoteNameIdentifier>\
    </Federation>\
   </Identity>");

  // Construct a SAML assertion requester/queryer
  serviceProviderContextDump = generateServiceProviderContextDump();
  std::cout<<"Requester/SP context: "<<serviceProviderContextDump<<std::endl;
  spContext = lasso_server_new_from_dump(serviceProviderContextDump);

  // Requester: Generate an assertion request
  assertionqry = lasso_assertion_query_new(spContext);
  if(assertionqry == NULL) { std::cout<<"lasso_assertion_query_new() failed"<<std::endl; goto exit; }

  lasso_profile_set_identity_from_dump(LASSO_PROFILE(assertionqry), identity_sp.c_str());

  rc = lasso_assertion_query_init_request(assertionqry, "https://idp.com/SAML", LASSO_HTTP_METHOD_SOAP, LASSO_ASSERTION_QUERY_REQUEST_TYPE_ATTRIBUTE);
  if(rc != 0) { std::cout<<"lasso_assertion_query_init_request failed"<< rc <<std::endl; goto exit; }

  rc = lasso_assertion_query_build_request_msg(assertionqry);
  if(rc != 0) { std::cout<<"lasso_assertion_query_build_request_msg failed"<< rc <<std::endl; goto exit; }

  assertionRequestBody = LASSO_PROFILE(assertionqry)->msg_body;
  if(assertionRequestBody == NULL) { std::cout<<"assertionRequestBody shouldn't be NULL"<<std::endl; goto exit; }
  std::cout<<"Request: ---assertionRequestBody: "<<assertionRequestBody<<std::endl;

  //Construct a Attribute Authority
  identityProviderContextDump = generateIdentityProviderContextDump();
  std::cout<<"AA context: "<<identityProviderContextDump<<std::endl;
  idpContext = lasso_server_new_from_dump(identityProviderContextDump);

  //AA: Process the request which is generated above (from Request)
  assertionqry1 = lasso_assertion_query_new(idpContext);
  if(assertionqry1 == NULL) { std::cout<<"lasso_assertion_query_new() failed"<<std::endl; goto exit; }

  lasso_profile_set_identity_from_dump(LASSO_PROFILE(assertionqry1), identity_idp.c_str());

  rc = lasso_assertion_query_process_request_msg(assertionqry1, assertionRequestBody);
  if(rc != 0) { std::cout<<"lasso_assertion_query_process_request_msg failed"<<std::endl; goto exit; }

  rc = lasso_assertion_query_validate_request(assertionqry1);

  // AA: Generate a response 
  // TODO: Insert attribute value into response
  rc = lasso_assertion_query_build_response_msg(assertionqry1);
  if(rc != 0 ) { std::cout<<"lasso_assertion_query_build_response_msg failed"<<std::endl; goto exit; }

  assertionResponseBody = LASSO_PROFILE(assertionqry1)->msg_body;
  if(assertionResponseBody == NULL) { std::cout<<"assertionResponseBody shouldn't be NULL"<<std::endl; goto exit; }
  std::cout<<"Response: ---assertionResponseBody: "<<assertionResponseBody<<std::endl;

  //Requester: Consume the response from AA, by using the original AssertionQuery Profile
  rc = lasso_assertion_query_process_response_msg(assertionqry, assertionResponseBody);  
  if(rc != 0) { std::cout<<"lasso_assertion_query_process_response_msg failed"<<std::endl; goto exit; }


exit:
	if(serviceProviderContextDump != NULL) g_free(serviceProviderContextDump);
	if(identityProviderContextDump != NULL) g_free(identityProviderContextDump);
	if(spContext != NULL) lasso_server_destroy(spContext);
	if(idpContext != NULL) lasso_server_destroy(idpContext);
}

