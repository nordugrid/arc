#include <iostream>

#include <stdlib.h>
#include <string>

#include <lasso/lasso.h>
#include <lasso/saml-2.0/assertion_query.h>

#include <lasso/xml/saml-2.0/saml2_attribute.h>
#include <lasso/xml/saml-2.0/saml2_attribute_value.h>
#include <lasso/xml/saml-2.0/samlp2_attribute_query.h>
#include <lasso/xml/saml-2.0/saml2_assertion.h>
#include <lasso/xml/saml-2.0/saml2_audience_restriction.h>
#include <lasso/xml/saml-2.0/samlp2_response.h>
#include <lasso/xml/saml-2.0/saml2_attribute_statement.h>
#include <lasso/xml/misc_text_node.h>
#include <lasso/xml/xml.h>

static char*
generateIdentityProviderContextDump()
{
  LassoServer *serverContext;
	
  serverContext = lasso_server_new(
                   TESTSDATADIR "/idp1-la/saml_metadata.xml",
                   "./key.pem",
                   NULL, /* Secret key to unlock private key */
                   "./cert.pem");
  lasso_server_add_provider(
                   serverContext,
                   LASSO_PROVIDER_ROLE_SP,
                   TESTSDATADIR "/sp1-la/saml_metadata.xml",
                   "./cert.pem",
                   "./ca.pem");
  return lasso_server_dump(serverContext);
}

static char*
generateServiceProviderContextDump()
{
  LassoServer *serverContext;
	
  serverContext = lasso_server_new(
                   TESTSDATADIR "/sp1-la/saml_metadata.xml",
                   "./key.pem",
                   NULL, /* Secret key to unlock private key */
                   "./cert.pem");
  lasso_server_add_provider(
                   serverContext,
                   LASSO_PROVIDER_ROLE_ATTRIBUTEAUTHORITY,
                   TESTSDATADIR "/idp1-la/saml_metadata.xml",
                   "./cert.pem",
                   "./ca.pem");
  return lasso_server_dump(serverContext);
}

void
build_random_sequence(char *buffer, unsigned int size)
{
	char *t;
	unsigned int rnd, i;

	t = buffer;
	while (t-buffer < (int)size) {
		rnd = g_random_int();
		for (i=0; i<sizeof(int); i++) {
			*(t++) = '0' + ((rnd>>i*4)&0xf);
			if (*(t-1) > '9') *(t-1) += 7;
		}
	}
}

char*
build_unique_id(unsigned int size)
{
	char *result;

	g_assert(size >= 32);

	result = (char*)g_malloc(size+2); /* trailing \0 and leading _ */
	result[0] = '_';
	build_random_sequence(result+1, size);
	result[size+1] = 0;
	return result;
}


char*
get_current_time()
{
	time_t now;
	struct tm *tm;
	char *ret;

	ret = (char*)g_malloc(21);
	now = time(NULL);
	tm = gmtime(&now);
	strftime(ret, 21, "%Y-%m-%dT%H:%M:%SZ", tm);

	return ret;
}


int main(void) {

  lasso_init();

  char *serviceProviderContextDump, *identityProviderContextDump;
  
  LassoServer *spContext, *idpContext;
  LassoAssertionQuery *assertionqry, *assertionqry1;
  int rc;
  char *assertionRequestBody = NULL;
  char *assertionResponseBody = NULL;

  LassoSaml2Attribute *attr;  
  LassoSaml2AttributeValue *attrval;
  LassoSamlp2AttributeQuery *attribute_query;
  LassoSamlp2Response *attribute_response;
  LassoSaml2Assertion *assertion;
  LassoSaml2AudienceRestriction *audience_restriction;
  LassoSaml2NameID *name_id = NULL;
  LassoSaml2AttributeStatement *attribute_statement;
  LassoProvider *provider = NULL;
  LassoSaml2EncryptedElement *encrypted_element = NULL;
  LassoFederation *federation = NULL;
  LassoMiscTextNode *text_node = NULL;

  std::string identity_sp("<Identity xmlns=\"http://www.entrouvert.org/namespaces/lasso/0.0\" Version=\"2\">\
    <Federation xmlns:saml=\"urn:oasis:names:tc:SAML:2.0:assertion\"\
       RemoteProviderID=\"https://idp.com/SAML\"\
       FederationDumpVersion=\"2\">\
      <LocalNameIdentifier>\
        <saml:NameID Format=\"urn:oasis:names:tc:SAML:1.1:nameid-format:X509SubjectName\">/O=Grid/OU=UIO/CN=test</saml:NameID>\
      </LocalNameIdentifier>\
    </Federation>\
   </Identity>");

//The RemoteNameIdentifier could be parsed from the remote credential during transport level authentication 
  std::string identity_idp("<Identity xmlns=\"http://www.entrouvert.org/namespaces/lasso/0.0\" Version=\"2\">\
    <Federation xmlns:saml=\"urn:oasis:names:tc:SAML:2.0:assertion\"\
       RemoteProviderID=\"https://sp.com/SAML\"\
       FederationDumpVersion=\"2\">\
      <RemoteNameIdentifier>\
        <saml:NameID Format=\"urn:oasis:names:tc:SAML:1.1:nameid-format:X509SubjectName\">/O=Grid/OU=UIO/CN=test</saml:NameID>\
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

  //Add one or more <Attribute>s into AttributeQuery here, which means the Requestor would
  //get these <Attribute>s from AA
  attr = LASSO_SAML2_ATTRIBUTE(lasso_saml2_attribute_new());
  attr->Name = g_strdup("urn:oid:1.3.6.1.4.1.5923.1.1.1.6");
  attr->NameFormat = g_strdup("urn:oasis:names:tc:SAML:2.0:attrname-format:uri");
  attr->FriendlyName = g_strdup("eduPersonPrincipalName");
  //Since here the request only would get the attributevalue from the AA,
  //we don't specify the AttributeValue inside the Attribute.
  //attrval= LASSO_SAML2_ATTRIBUTE_VALUE(lasso_saml2_attribute_value_new());
  attribute_query = LASSO_SAMLP2_ATTRIBUTE_QUERY(LASSO_PROFILE(assertionqry)->request);
  attribute_query->Attribute = g_list_append(attribute_query->Attribute, attr);

  //Build the soap message
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
  //AA will try to query local attribute database, get the attribute result
  //Then, insert <Attribute> into response
  attr = LASSO_SAML2_ATTRIBUTE(lasso_saml2_attribute_new());
  attr->Name = g_strdup("urn:oid:1.3.6.1.4.1.5923.1.1.1.6");
  attr->NameFormat = g_strdup("urn:oasis:names:tc:SAML:2.0:attrname-format:uri");
  attr->FriendlyName = g_strdup("eduPersonPrincipalName");
  attrval= LASSO_SAML2_ATTRIBUTE_VALUE(lasso_saml2_attribute_value_new());

  text_node = LASSO_MISC_TEXT_NODE(lasso_misc_text_node_new_with_string(g_strdup("RoleA")));
  text_node->text_child = TRUE;
  attrval->any = g_list_prepend(NULL, text_node);
  //Add one or more <AttributeValue> into <Attribute>
  attr->AttributeValue = g_list_append(attr->AttributeValue, attrval);

  assertion = LASSO_SAML2_ASSERTION(lasso_saml2_assertion_new());
  assertion->ID = build_unique_id(32);
  assertion->Version = g_strdup("2.0");
  assertion->IssueInstant = get_current_time();
  assertion->Issuer = LASSO_SAML2_NAME_ID(lasso_saml2_name_id_new_with_string(
			  LASSO_PROVIDER(LASSO_PROFILE(assertionqry1)->server)->ProviderID));

  assertion->Conditions = LASSO_SAML2_CONDITIONS(lasso_saml2_conditions_new());
  audience_restriction = LASSO_SAML2_AUDIENCE_RESTRICTION(
			  lasso_saml2_audience_restriction_new());
  audience_restriction->Audience = g_strdup(LASSO_PROFILE(assertionqry1)->remote_providerID);
  assertion->Conditions->AudienceRestriction = g_list_append(NULL, audience_restriction);

  assertion->Subject = LASSO_SAML2_SUBJECT(lasso_saml2_subject_new());

  federation = LASSO_FEDERATION(g_hash_table_lookup(LASSO_PROFILE(assertionqry1)->identity->federations,
			  LASSO_PROFILE(assertionqry1)->remote_providerID));
  if (federation == NULL) { std::cout<<"can't find federation for identity"<<std::endl; goto exit;}
  if (federation->remote_nameIdentifier) {
	  assertion->Subject->NameID = LASSO_SAML2_NAME_ID(g_object_ref(federation->remote_nameIdentifier));
  } else { assertion->Subject->NameID = LASSO_SAML2_NAME_ID(g_object_ref(federation->local_nameIdentifier)); }

  attribute_statement = LASSO_SAML2_ATTRIBUTE_STATEMENT(lasso_saml2_attribute_statement_new());
  //Add one or more <Attribute> into <Assertion>
  attribute_statement->Attribute = g_list_append(attribute_statement->Attribute, attr);
  
  assertion->AttributeStatement= g_list_append(assertion->AttributeStatement, attribute_statement);
  
  /* Save signing material in assertion private datas to be able to sign later */
  if (LASSO_PROFILE(assertionqry1)->server->certificate) {
	  assertion->sign_type = LASSO_SIGNATURE_TYPE_WITHX509;
  } else {
	  assertion->sign_type = LASSO_SIGNATURE_TYPE_SIMPLE;
  }
  assertion->sign_method = LASSO_PROFILE(assertionqry1)->server->signature_method;
  assertion->private_key_file = g_strdup(LASSO_PROFILE(assertionqry1)->server->private_key);
  assertion->certificate_file = g_strdup(LASSO_PROFILE(assertionqry1)->server->certificate);

  attribute_response = LASSO_SAMLP2_RESPONSE(LASSO_PROFILE(assertionqry1)->response);
  attribute_response->Assertion= g_list_append(attribute_response->Assertion, assertion);

  
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

