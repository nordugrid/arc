#include <iostream>

#include <stdlib.h>
#include <string>

#include <lasso/lasso.h>


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
  LassoLibAuthnRequest *request;
  int rc;
  char *relayState;
  char *authnRequestUrl, *authnRequestQuery;
  char *responseUrl, *responseQuery;
  char *idpIdentityContextDump, *idpSessionContextDump;
  char *serviceProviderId, *soapRequestMsg, *soapResponseMsg;
  char *spIdentityContextDump;
  char *spSessionDump;
  int requestType;

  // Construct a Service Provider
  serviceProviderContextDump = generateServiceProviderContextDump();
  std::cout<<"SP context: "<<serviceProviderContextDump<<std::endl;
  spContext = lasso_server_new_from_dump(serviceProviderContextDump);

  // SP: Generate an authentication request based on the Service Provider
  spLoginContext = lasso_login_new(spContext);
  if(spLoginContext == NULL) { std::cout<<"lasso_login_new() failed"<<std::endl; goto exit; }

  rc = lasso_login_init_authn_request(spLoginContext, "https://idp1/metadata", LASSO_HTTP_METHOD_REDIRECT);
  if(rc != 0) { std::cout<<"lasso_login_init_authn_request failed"<<std::endl; goto exit; }

  request = LASSO_LIB_AUTHN_REQUEST(LASSO_PROFILE(spLoginContext)->request);
  if(!LASSO_IS_LIB_AUTHN_REQUEST(request)) { std::cout<<"request should be authn_request"<<std::endl; goto exit; }
  request->IsPassive = 0;
  request->NameIDPolicy = g_strdup(LASSO_LIB_NAMEID_POLICY_TYPE_FEDERATED);
  request->consent = g_strdup(LASSO_LIB_CONSENT_OBTAINED);
  relayState = "fake";
  request->RelayState = g_strdup(relayState);

  rc = lasso_login_build_authn_request_msg(spLoginContext);
  if(rc != 0) { std::cout<<"lasso_login_init_authn_request_msg failed"<<std::endl; goto exit; }

  authnRequestUrl = LASSO_PROFILE(spLoginContext)->msg_url;
  if(authnRequestUrl == NULL) { std::cout<<"authnRequestUrl shouldn't be NULL"<<std::endl; goto exit; }
  std::cout<<"SP---authnRequestUrl: "<<authnRequestUrl<<std::endl;

  authnRequestQuery = strchr(authnRequestUrl, '?')+1;
  if(strlen(authnRequestQuery) == 0) { std::cout<<"authnRequestRequest shouldn't be empty string"<<std::endl; goto exit; }
  std::cout<<"SP---authnRequestQuery: "<<authnRequestQuery<<std::endl;

  /* Identity provider singleSignOn, for a user having no federation. */

  //Construct a Identity Provider
  identityProviderContextDump = generateIdentityProviderContextDump();
  std::cout<<"IdP context: "<<identityProviderContextDump<<std::endl;
  idpContext = lasso_server_new_from_dump(identityProviderContextDump);

  //IdP: Process the request which is generated above (from Service Provider)
  idpLoginContext = lasso_login_new(idpContext);
  if(idpLoginContext == NULL) { std::cout<<"lasso_login_new() failed"<<std::endl; goto exit; }

  rc = lasso_login_process_authn_request_msg(idpLoginContext, authnRequestQuery);
  if(rc != 0) { std::cout<<"lasso_login_process_authn_request_msg failed"<<std::endl; goto exit; }
  if(!lasso_login_must_authenticate(idpLoginContext)) { std::cout<<"lasso_login_must_authenticate() should be TRUE"<<std::endl; goto exit; }
  if(idpLoginContext->protocolProfile != LASSO_LOGIN_PROTOCOL_PROFILE_BRWS_ART) { 
    std::cout<<"protocoleProfile should be ProfileBrwsArt"<<std::endl; goto exit; 
  }
  if(lasso_login_must_ask_for_consent(idpLoginContext)) { 
    std::cout<<"lasso_login_must_ask_for_consent() should be FALSE"<<std::endl;  goto exit; 
  }

  rc = lasso_login_validate_request_msg(idpLoginContext, 1, /* authentication_result */ 0 /* is_consent_obtained */);

  // IdP: Generate a response 
  rc = lasso_login_build_assertion(idpLoginContext,
          LASSO_SAML_AUTHENTICATION_METHOD_PASSWORD,
          "FIXME: authenticationInstant",
	  "FIXME: reauthenticateOnOrAfter",
	  "FIXME: notBefore",
	  "FIXME: notOnOrAfter");
  rc = lasso_login_build_artifact_msg(idpLoginContext, LASSO_HTTP_METHOD_REDIRECT);
  if(rc != 0) { std::cout<<"lasso_login_build_artifact_msg failed"<<std::endl; goto exit; }

  idpIdentityContextDump = lasso_identity_dump(LASSO_PROFILE(idpLoginContext)->identity);
  if(idpIdentityContextDump == NULL) { std::cout<<"lasso_identity_dump shouldn't return NULL"<<std::endl; goto exit; }

  idpSessionContextDump = lasso_session_dump(LASSO_PROFILE(idpLoginContext)->session);
  if(idpSessionContextDump == NULL) { std::cout<<"lasso_session_dump shouldn't return NULL"<<std::endl; goto exit; }

  responseUrl = LASSO_PROFILE(idpLoginContext)->msg_url;
  if(responseUrl == NULL) { std::cout<<"responseUrl shouldn't be NULL"<<std::endl; goto exit; }
  std::cout<<"IdP---responseUrl: "<<responseUrl<<std::endl;

  responseQuery = strchr(responseUrl, '?')+1;
  if(strlen(responseQuery) == 0) { std::cout<<"responseQuery shouldn't be an empty string"<<std::endl; goto exit; }
  std::cout<<"IdP---responseQuery: "<<responseQuery<<std::endl;

  serviceProviderId = g_strdup(LASSO_PROFILE(idpLoginContext)->remote_providerID);
  if(serviceProviderId == NULL) { std::cout<<"lasso_profile_get_remote_providerID shouldn't return NULL"<<std::endl; goto exit; }


  /* Service provider assertion consumer */
  lasso_server_destroy(spContext);
  lasso_login_destroy(spLoginContext);

  //SP: Construct a SOAP request message
  spContext = lasso_server_new_from_dump(serviceProviderContextDump);
  spLoginContext = lasso_login_new(spContext);
  rc = lasso_login_init_request(spLoginContext, responseQuery, LASSO_HTTP_METHOD_REDIRECT);
  if(rc != 0) { std::cout<<"lasso_login_init_request failed"<<std::endl; goto exit; }

  rc = lasso_login_build_request_msg(spLoginContext);
  if(rc != 0) { std::cout<<"lasso_login_build_request_msg failed"<<std::endl; goto exit; }

  soapRequestMsg = LASSO_PROFILE(spLoginContext)->msg_body;
  if(soapRequestMsg == NULL) { std::cout<<"soapRequestMsg must not be NULL"<<std::endl; goto exit; }
  std::cout<<"SP---soapRequestMsg: "<<soapRequestMsg<<std::endl;

  /* Identity provider SOAP endpoint */
  lasso_server_destroy(idpContext);
  lasso_login_destroy(idpLoginContext);
  requestType = lasso_profile_get_request_type_from_soap_msg(soapRequestMsg);
  if(requestType != LASSO_REQUEST_TYPE_LOGIN) { std::cout<<"requestType should be LASSO_REQUEST_TYPE_LOGIN"<<std::endl; goto exit; }

  //IdP: Process the SOAP request message
  idpContext = lasso_server_new_from_dump(identityProviderContextDump);
  idpLoginContext = lasso_login_new(idpContext);
  rc = lasso_login_process_request_msg(idpLoginContext, soapRequestMsg);
  if(rc != 0) { std::cout<<"lasso_login_process_request_msg failed"<<std::endl; goto exit; }

  //IdP: Generate a SOAP response message
  rc = lasso_profile_set_session_from_dump(LASSO_PROFILE(idpLoginContext), idpSessionContextDump);
  if(rc != 0) { std::cout<<"lasso_login_set_assertion_from_dump failed"<<std::endl; goto exit; }

  rc = lasso_login_build_response_msg(idpLoginContext, serviceProviderId);
  if(rc != 0) { std::cout<<"lasso_login_build_response_msg failed"<<std::endl; goto exit; }

  soapResponseMsg =  LASSO_PROFILE(idpLoginContext)->msg_body;
  if(soapResponseMsg == NULL) { std::cout<<"soapResponseMsg must not be NULL"<<std::endl; goto exit; }
  std::cout<<"IdP---soapResponseMsg: "<<soapResponseMsg<<std::endl;

  /* Service provider assertion consumer (step 2: process SOAP response) */
  //SP: Process the SOAP response message
  rc = lasso_login_process_response_msg(spLoginContext, soapResponseMsg);
  if(rc != 0) { std::cout<<"lasso_login_process_response_msg"<<std::endl; goto exit; }

  rc = lasso_login_accept_sso(spLoginContext);
  if(rc != 0) { std::cout<<"lasso_login_accept_sso failed"<<std::endl; goto exit; }
  if(LASSO_PROFILE(spLoginContext)->identity == NULL) { std::cout<<"spLoginContext has no identity"<<std::endl; goto exit; }

  spIdentityContextDump = lasso_identity_dump(LASSO_PROFILE(spLoginContext)->identity);
  if(spIdentityContextDump == NULL) { std::cout<<"lasso_identity_dump failed"<<std::endl; goto exit; }
  spSessionDump = lasso_session_dump(LASSO_PROFILE(spLoginContext)->session);

  std::cout<<"SP---SP Session dump: "<<spSessionDump<<std::endl;

exit:
        if(serviceProviderId != NULL) g_free(serviceProviderId);
	if(serviceProviderContextDump != NULL) g_free(serviceProviderContextDump);
	if(identityProviderContextDump != NULL) g_free(identityProviderContextDump);
	if(spContext != NULL) lasso_server_destroy(spContext);
	if(idpContext != NULL) lasso_server_destroy(idpContext);
}

