/*
 * OpenIdpClient.cpp
 *
 *  Created on: Apr 23, 2009
 *      Author: tzangerl
 */

#include "OpenIdpClient.h"

namespace Arc {

	OpenIdpClient::OpenIdpClient(const MCCConfig cfg, const URL url, const std::string idp_name) : SAML2SSOHTTPClient(cfg, url, idp_name) {
	}

	OpenIdpClient::~OpenIdpClient() {
	}


	MCC_Status OpenIdpClient::processIdPLogin(const std::string username, const std::string password) {
		  const std::string origin = "SAML2SSOHTTPClient::processIdPLogin(const std::string username, const std::string password)";
		  std::map<std::string, std::string> http_attributes;
		  std::string cookie;
		  std::string body_string;

		  if ((*sso_pages_)["IdP"] == "") {
			  return MCC_Status(GENERIC_ERROR, origin, "The IdP page is unknown!");
		  }

		  // IdP with SAML2 token to IdP with internal auth representation -- IdP
		  std::string actual_ip_login2=ConfusaParserUtils::handle_redirect_step(cfg_, (*sso_pages_)["IdP"], &cookie);

		  (*session_cookies_)["IdP"] = cookie;
		  http_attributes["Cookie"] = cookie;

		  if (actual_ip_login2.empty()) {
			  return MCC_Status(PARSING_ERROR, origin, "Could not redirect from IdP with SAML2 token to IdP with internal Auth representation");
		  }

		  ClientHTTP idp_login_page_client(cfg_, Arc::URL(actual_ip_login2));
		  PayloadRaw idp_login_page_request;
		  PayloadRawInterface *idp_login_page_response = NULL;
		  HTTPClientInfo idp_login_page_info;
		  idp_login_page_client.process("GET", http_attributes, &idp_login_page_request, &idp_login_page_info, &idp_login_page_response);
		  std::string idp_l_content = "";

		  if (idp_login_page_response) {
			  idp_l_content = idp_login_page_response->Content();
			  delete idp_login_page_response;
		  } else {
			  return MCC_Status(GENERIC_ERROR, origin, "Did not receive a response from the OpenIdP login page");
		  }

		  body_string = ConfusaParserUtils::extract_body_information(idp_l_content);

		  if (body_string.empty()) {
			  return MCC_Status(PARSING_ERROR, origin, "Could not get the body information from the IdP's internal redirect!");
		  }

		  xmlDocPtr doc = ConfusaParserUtils::get_doc(body_string);
		  std::string auth_state = ConfusaParserUtils::evaluate_path(doc, "//input[@name='AuthState']/@value");
		  std::string relay_state = ConfusaParserUtils::evaluate_path(doc, "//input[@name='RelayState']/@value");
		  std::string action = ConfusaParserUtils::evaluate_path(doc, "//form/@action");
		  ConfusaParserUtils::destroy_doc(doc);

		  if (auth_state.empty()) {
			  return MCC_Status(PARSING_ERROR, origin, "Could not get the AuthState from the IdP!");
		  } else if (action.empty()) {
			  return MCC_Status(PARSING_ERROR, origin, "Could not get the action from the IdP!");
		  }

		  std::string::size_type query_part_pos = actual_ip_login2.find("?");
		  std::string host_part = actual_ip_login2.substr(0,query_part_pos);

		  std::string post_params = "username=" + ConfusaParserUtils::urlencode(username) + "&password=" + ConfusaParserUtils::urlencode(password) + "&wp-submit=Login+%C2%BB&RelayState=" + relay_state + "&AuthState=" + auth_state;
		  action = host_part + action + post_params;

		  // IdP loginuserpass POST to IdP SSOService -- IdP
		  ClientHTTP id_login_post_client(cfg_, URL(action));
		  PayloadRaw id_login_post_request;
		  PayloadRawInterface *id_login_post_response = NULL;
		  HTTPClientInfo id_login_post_info;
		  id_login_post_client.process("POST", http_attributes, &id_login_post_request, &id_login_post_info, &id_login_post_response);

		  if (id_login_post_response) {
			  delete id_login_post_response;
		  }

		  std::string post_2_ssoservice_redirect = id_login_post_info.location;
		  logger.msg(DEBUG, "post_2_ssoservice_redirect url is %s", post_2_ssoservice_redirect);

		  if(post_2_ssoservice_redirect.empty()) {
			  return MCC_Status(PARSING_ERROR, origin, "Could not get the id-login post location from the IdP");
		  } else {
			std::string consent_page = ConfusaParserUtils::handle_redirect_step(cfg_, post_2_ssoservice_redirect, &cookie, &http_attributes);

			// consent?
			if (consent_page != "") {
				 (*sso_pages_)["Consent"] = consent_page;
			} else {
				 (*sso_pages_)["PostIdP"] = post_2_ssoservice_redirect;
			}

			  return MCC_Status(STATUS_OK);
		  }
	}


	MCC_Status OpenIdpClient::processConsent() {
		  const std::string origin = "SAML2SSOHTTPClient::processConsent()";
		  std::map<std::string, std::string> http_attributes;
		  std::list<std::string> *attrname = new std::list<std::string>();
		  std::list<std::string> *attrvalue = new std::list<std::string>();

		  if ((*sso_pages_)["Consent"] == "") {
			  return MCC_Status(STATUS_OK);	// no consent necessary
		  }

		  logger.msg(DEBUG, "SAML2SSOHTTPClient::processConsent()");

		  if ((*session_cookies_)["IdP"] == "") {
			  return MCC_Status(GENERIC_ERROR, origin, "IdP's PHPSESSID Cookie not present");
		  }

		  http_attributes["Cookie"] = (*session_cookies_)["IdP"];
		  ClientHTTP consent_client(cfg_, URL((*sso_pages_)["Consent"]));
		  PayloadRaw consent_request;
		  PayloadRawInterface *consent_response = NULL;
		  HTTPClientInfo consent_info;
		  consent_client.process("GET", http_attributes, &consent_request, &consent_info, &consent_response);

		  std::string consent_response_str = "";

		  if (consent_response) {
			  consent_response_str = consent_response->Content();
			  delete consent_response;
		  } else {
			  return MCC_Status(PARSING_ERROR, origin, "Could not get the consent page's body information!");
		  }

		  std::string consent_content = ConfusaParserUtils::extract_body_information(consent_response_str);
		  xmlDocPtr doc = ConfusaParserUtils::get_doc(consent_content);
		  ConfusaParserUtils::evaluate_path(doc, "//td[@class='attrname']", attrname);
		  ConfusaParserUtils::evaluate_path(doc, "//td[@class='attrvalue']", attrvalue);
		  std::string state_id = ConfusaParserUtils::evaluate_path(doc, "//input[@name='StateId']/@value");
		  std::string confirm_param = ConfusaParserUtils::evaluate_path(doc, "//input[@type='submit' and @name='yes' or @name='confirm']/@value");
		  std::string yes_action = ConfusaParserUtils::evaluate_path(doc, "//input[@name='yes']/../@action");
		  ConfusaParserUtils::destroy_doc(doc);

		  if (state_id.empty()) {
			  return MCC_Status(PARSING_ERROR, origin, "Could not get a state_id from the consent page!");
		  } else if (confirm_param.empty()) {
			  return MCC_Status(PARSING_ERROR, origin, "Could not get a confirm parameter from the consent page!");
		  } else if (yes_action.empty()) {
			  return MCC_Status(PARSING_ERROR, origin, "Could not get a confirmation action from the consent page!");
		  }

		  std::string user_conf;
		  std::cout << std::endl << "Your identity provider will send the following information to the SLCS service:" << std::endl;
		  std::cout << "==============================================================================" << std::endl;

		  std::list<std::string>::iterator it;
		  while(!(attrname->empty() || attrvalue->empty())) {
			  std::cout << attrname->front() << ": " << attrvalue->front() << std::endl;
			  attrname->pop_front();
			  attrvalue->pop_front();
		  }
		  std::cout << "==============================================================================" << std::endl;

		  if (attrname) {
			  delete attrname;
		  }

		  if (attrvalue) {
			  delete attrvalue;
		  }

		  std::cout << std::endl;
		  std::cout << "Do you consent to the release of that information? (y/n) ";
		  std::cin >> user_conf;

		  if (user_conf == "y") {
			  std::string confirm_site = yes_action + "?StateId=" + ConfusaParserUtils::urlencode(state_id) + "&yes=" + ConfusaParserUtils::urlencode(confirm_param);
			  logger.msg(DEBUG, "Trying to open confirm site %s", confirm_site);

			  std::string cookie;
			  std::string post_sso_idp_page = ConfusaParserUtils::handle_redirect_step(cfg_, confirm_site, &cookie, &http_attributes);

			  if (post_sso_idp_page == "") {
				  return MCC_Status(PARSING_ERROR, origin, "Forwarding positive user consent did not work!");
			  } else {
				  (*sso_pages_)["PostIdP"] = post_sso_idp_page;
				  return MCC_Status(STATUS_OK);
			  }
		  } else {
			  return MCC_Status(GENERIC_ERROR, origin, "No user consent to SAML2 attribute exchange. Aborting.");
		  }
	}

	MCC_Status OpenIdpClient::processIdP2Confusa() {
		  const std::string origin = "SAML2SSOHTTPClient::processIdP2Confusa()";
		  std::map<std::string, std::string> http_attributes;

		  if ((*sso_pages_)["PostIdP"] == "") {
			  return MCC_Status(GENERIC_ERROR, origin, "The post-idp page is unknown!");
		  }

		  if ((*session_cookies_)["Confusa"] == "") {
			  return MCC_Status(GENERIC_ERROR, origin, "Confusa's PHPSESSID Cookie is not present!");
		  } else if ((*session_cookies_)["IdP"] == "") {
			  return MCC_Status(GENERIC_ERROR, origin, "IdP's PHPSESSID Cookie is not present!");
		  }

		  http_attributes["Cookie"] = (*session_cookies_)["IdP"];
		  // IdP SSOService to SP AssertionConsumerService URL retrieval, no automatic redirect :( -- IdP
		  // normally here should also be the user consent question
		  ClientHTTP id_login_ssoservice_client(cfg_, URL((*sso_pages_)["PostIdP"]));
		  PayloadRaw id_login_ssoservice_request;
		  PayloadRawInterface *id_login_ssoservice_response = NULL;
		  HTTPClientInfo id_login_ssoservice_info;
		  id_login_ssoservice_client.process("GET", http_attributes, &id_login_ssoservice_request, &id_login_ssoservice_info, &id_login_ssoservice_response);

		  std::string idp_ssoservice_content = "";

		  if (id_login_ssoservice_response) {
			  idp_ssoservice_content = id_login_ssoservice_response->Content();
			  delete id_login_ssoservice_response;
		  } else {
			  return MCC_Status(PARSING_ERROR, origin, "Did not get a response from the IdP to AssertionConsumer redirect page!");
		  }

		  std::string idp_s_content = ConfusaParserUtils::extract_body_information(idp_ssoservice_content);

		  if (idp_s_content.empty()) {
			  return MCC_Status(PARSING_ERROR, origin, "Could not get the post-IdP page's body information!");
		  }

		  xmlDocPtr doc = ConfusaParserUtils::get_doc(idp_s_content);
		  std::string idp_s_action = ConfusaParserUtils::evaluate_path(doc, "//form/@action");
		  std::string idp_s_samlresponse = ConfusaParserUtils::evaluate_path(doc, "//form/input[@name='SAMLResponse']/@value");
		  std::string idp_s_relaystate = ConfusaParserUtils::evaluate_path(doc, "//form/input[@name='RelayState']/@value");
	 	  ConfusaParserUtils::destroy_doc(doc);

		  if (idp_s_action.empty()) {
			  return MCC_Status(PARSING_ERROR, origin, "Could not get the action from the consent body!");
		  } else if (idp_s_samlresponse.empty()) {
			  return MCC_Status(PARSING_ERROR, origin, "Could not get the SAMLResponse from the consent body!");
		  } else if (idp_s_relaystate.empty()) {
			  return MCC_Status(PARSING_ERROR, origin, "Could not get the RelayState from the consent body!");
		  }

		  // now redirect from IdP SSOService to SP AssertionConsumerService
		  std::string post_params = "SAMLResponse=" + ConfusaParserUtils::urlencode(idp_s_samlresponse) + "&RelayState=" + ConfusaParserUtils::urlencode(idp_s_relaystate);

		  logger.msg(DEBUG, "The post IdP-authentication action is %s", idp_s_action);

		  // IdP loginuserpass POST to SP AssertionConsumerervice -- Confusa
		  http_attributes["Cookie"] = (*session_cookies_)["Confusa"];
		  ClientHTTP sp_assertionconsumer_post_client(cfg_, URL(idp_s_action));
		  PayloadRaw sp_ass_cons_body;
		  PayloadRawInterface *sp_assertionconsumer_post_response = NULL;
		  HTTPClientInfo sp_assertionconsumer_post_info;
		  http_attributes["Content-Type"] = "application/x-www-form-urlencoded";
		  sp_ass_cons_body.Insert(post_params.c_str(), 0, strlen(post_params.c_str()));
		  sp_assertionconsumer_post_client.process("POST", http_attributes, &sp_ass_cons_body, &sp_assertionconsumer_post_info, &sp_assertionconsumer_post_response);

		  if (sp_assertionconsumer_post_response) {
			  delete sp_assertionconsumer_post_response;
		  }

		  // IdP SSOService to SP service page -- Confusa
		  std::string confusa_service_page = sp_assertionconsumer_post_info.location;

		  if (confusa_service_page.empty()) {
			  return MCC_Status(PARSING_ERROR, origin, "Did not receive a Confusa service page redirect from the POST-authentication IdP!");
		  }

		  ClientHTTP confusa_service_client(cfg_, URL(confusa_service_page));
		  PayloadRaw confusa_service_request;
		  PayloadRawInterface *confusa_service_response = NULL;
		  HTTPClientInfo confusa_service_info;

		  confusa_service_client.process("GET", http_attributes, &confusa_service_request, &confusa_service_info, &confusa_service_response);
		  std::string response = "";

		  if (confusa_service_response) {
			  response = confusa_service_response->Content();
			  delete confusa_service_response;
		  } else {
			  return MCC_Status(GENERIC_ERROR, origin, "The redirect from the IdP to Confusa does not correspond to a valid page!");
		  }


		  (*sso_pages_)["PostIdPConfusa"] = confusa_service_page;
		  return MCC_Status(STATUS_OK);

	}
};
