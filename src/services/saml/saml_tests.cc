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
                   TESTSDATADIR "/idp1-la/metadata.xml",
                   TESTSDATADIR "/idp1-la/private-key-raw.pem",
                   NULL, /* Secret key to unlock private key */
                   TESTSDATADIR "/idp1-la/certificate.pem");
  lasso_server_add_provider(
                   serverContext,
                   LASSO_PROVIDER_ROLE_SP,
                   TESTSDATADIR "/sp1-la/metadata.xml",
                   TESTSDATADIR "/sp1-la/public-key.pem",
                   TESTSDATADIR "/ca1-la/certificate.pem");
  return lasso_server_dump(serverContext);
}

static char*
generateServiceProviderContextDump()
{
  LassoServer *serverContext;
	
  serverContext = lasso_server_new(
                   TESTSDATADIR "/sp1-la/metadata.xml",
                   TESTSDATADIR "/sp1-la/private-key-raw.pem",
                   NULL, /* Secret key to unlock private key */
                   TESTSDATADIR "/sp1-la/certificate.pem");
  lasso_server_add_provider(
                   serverContext,
                   LASSO_PROVIDER_ROLE_IDP,
                   TESTSDATADIR "/idp1-la/metadata.xml",
                   TESTSDATADIR "/idp1-la/public-key.pem",
                   TESTSDATADIR "/ca1-la/certificate.pem");
  return lasso_server_dump(serverContext);
}

int main(void) {

  lasso_init();

  char *serviceProviderContextDump, *identityProviderContextDump;
  LassoServer *spContext, *idpContext;
  LassoLogin *spLoginContext, *idpLoginContext;

  LassoAssertionQuery *assertionqry, *assertionqry1;

  int rc;
  char *assertionRequestUrl, *assertionRequestQuery;

  char *responseUrl, *responseQuery;
  char *idpIdentityContextDump, *idpSessionContextDump;
  char *serviceProviderId, *soapRequestMsg, *soapResponseMsg;
  char *spIdentityContextDump;
  char *spSessionDump;
  int requestType;


  // Construct a SAML assertion requester/queryer
  serviceProviderContextDump = generateServiceProviderContextDump();
  std::cout<<"SP context: "<<serviceProviderContextDump<<std::endl;
  spContext = lasso_server_new_from_dump(serviceProviderContextDump);

  // Requester: Generate an assertion request
  assertionqry = lasso_assertion_query_new(spContext);
  if(assertionqry == NULL) { std::cout<<"lasso_assertion_query_new() failed"<<std::endl; goto exit; }

  rc = lasso_assertion_query_init_request(assertionqry, "https://idp1/metadata", LASSO_HTTP_METHOD_REDIRECT, LASSO_ASSERTION_QUERY_REQUEST_TYPE_ATTRIBUTE);
  if(rc != 0) { std::cout<<"lasso_assertion_query_init_request failed"<< rc <<std::endl; goto exit; }


  rc = lasso_assertion_query_build_request_msg(assertionqry);
  if(rc != 0) { std::cout<<"lasso_assertion_query_build_request_msg failed"<<std::endl; goto exit; }


  assertionRequestUrl = LASSO_PROFILE(assertionqry)->msg_url;
  if(assertionRequestUrl == NULL) { std::cout<<"assertionRequestUrl shouldn't be NULL"<<std::endl; goto exit; }
  std::cout<<"Request: ---assertionRequestUrl: "<<assertionRequestUrl<<std::endl;

  assertionRequestQuery = strchr(assertionRequestUrl, '?')+1;
  if(strlen(assertionRequestQuery) == 0) { std::cout<<"assertionRequestQuery shouldn't be empty string"<<std::endl; goto exit; }
  std::cout<<"Request:---assertionRequestQuery: "<<assertionRequestQuery<<std::endl;


  //Construct a Attribute Authority
  identityProviderContextDump = generateIdentityProviderContextDump();
  std::cout<<"IdP context: "<<identityProviderContextDump<<std::endl;
  idpContext = lasso_server_new_from_dump(identityProviderContextDump);


  //AA: Process the request which is generated above (from Request)
  assertionqry1 = lasso_assertion_query_new(idpContext);
  if(assertionqry1 == NULL) { std::cout<<"lasso_assertion_query_new() failed"<<std::endl; goto exit; }

  rc = lasso_assertion_query_process_request_msg(assertionqry1, assertionRequestQuery);
  if(rc != 0) { std::cout<<"lasso_assertion_query_process_request_msg failed"<<std::endl; goto exit; }

  rc = lasso_assertion_query_validate_request(assertionqry1);

  // AA: Generate a response 
  rc = lasso_assertion_query_build_response_msg(assertionqry1);
  if(rc != 0 ) { std::cout<<"lasso_assertion_query_build_response_msg failed"<<std::endl; goto exit; }



  /* Service provider assertion consumer */
//  lasso_server_destroy(spContext);
//  lasso_login_destroy(spLoginContext);

  //SP: Construct a SOAP request message
//  spContext = lasso_server_new_from_dump(serviceProviderContextDump);
//  spLoginContext = lasso_login_new(spContext);
//  rc = lasso_login_init_request(spLoginContext, responseQuery, LASSO_HTTP_METHOD_REDIRECT);
//  if(rc != 0) { std::cout<<"lasso_login_init_request failed"<<std::endl; goto exit; }

//  rc = lasso_login_build_request_msg(spLoginContext);
//  if(rc != 0) { std::cout<<"lasso_login_build_request_msg failed"<<std::endl; goto exit; }

//  soapRequestMsg = LASSO_PROFILE(spLoginContext)->msg_body;
//  if(soapRequestMsg == NULL) { std::cout<<"soapRequestMsg must not be NULL"<<std::endl; goto exit; }
//  std::cout<<"SP---soapRequestMsg: "<<soapRequestMsg<<std::endl;

  /* Identity provider SOAP endpoint */
//  lasso_server_destroy(idpContext);
//  lasso_login_destroy(idpLoginContext);
//  requestType = lasso_profile_get_request_type_from_soap_msg(soapRequestMsg);
//  if(requestType != LASSO_REQUEST_TYPE_LOGIN) { std::cout<<"requestType should be LASSO_REQUEST_TYPE_LOGIN"<<std::endl; goto exit; }

  //IdP: Process the SOAP request message
//  idpContext = lasso_server_new_from_dump(identityProviderContextDump);
//  idpLoginContext = lasso_login_new(idpContext);
//  rc = lasso_login_process_request_msg(idpLoginContext, soapRequestMsg);
//  if(rc != 0) { std::cout<<"lasso_login_process_request_msg failed"<<std::endl; goto exit; }

  //IdP: Generate a SOAP response message
//  rc = lasso_profile_set_session_from_dump(LASSO_PROFILE(idpLoginContext), idpSessionContextDump);
//  if(rc != 0) { std::cout<<"lasso_login_set_assertion_from_dump failed"<<std::endl; goto exit; }

//  rc = lasso_login_build_response_msg(idpLoginContext, serviceProviderId);
//  if(rc != 0) { std::cout<<"lasso_login_build_response_msg failed"<<std::endl; goto exit; }

//  soapResponseMsg =  LASSO_PROFILE(idpLoginContext)->msg_body;
//  if(soapResponseMsg == NULL) { std::cout<<"soapResponseMsg must not be NULL"<<std::endl; goto exit; }
//  std::cout<<"IdP---soapResponseMsg: "<<soapResponseMsg<<std::endl;

  /* Service provider assertion consumer (step 2: process SOAP response) */
  //SP: Process the SOAP response message
//  rc = lasso_login_process_response_msg(spLoginContext, soapResponseMsg);
//  if(rc != 0) { std::cout<<"lasso_login_process_response_msg"<<std::endl; goto exit; }

//  rc = lasso_login_accept_sso(spLoginContext);
//  if(rc != 0) { std::cout<<"lasso_login_accept_sso failed"<<std::endl; goto exit; }
//  if(LASSO_PROFILE(spLoginContext)->identity == NULL) { std::cout<<"spLoginContext has no identity"<<std::endl; goto exit; }

//  spIdentityContextDump = lasso_identity_dump(LASSO_PROFILE(spLoginContext)->identity);
//  if(spIdentityContextDump == NULL) { std::cout<<"lasso_identity_dump failed"<<std::endl; goto exit; }
//  spSessionDump = lasso_session_dump(LASSO_PROFILE(spLoginContext)->session);

//  std::cout<<"SP---SP Session dump: "<<spSessionDump<<std::endl;

exit:
      return 0;
//        if(serviceProviderId != NULL) g_free(serviceProviderId);
//	if(serviceProviderContextDump != NULL) g_free(serviceProviderContextDump);
//	if(identityProviderContextDump != NULL) g_free(identityProviderContextDump);
//	if(spContext != NULL) lasso_server_destroy(spContext);
//	if(idpContext != NULL) lasso_server_destroy(idpContext);
}

