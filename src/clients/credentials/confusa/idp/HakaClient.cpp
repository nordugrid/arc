/*
 * HakaSAML2SSOHTTPClient.cpp
 *
 *  Created on: Apr 23, 2009
 *      Author: tzangerl
 */

#include "HakaClient.h"

namespace Arc {

	HakaClient::HakaClient(MCCConfig cfg, URL url, const std::string idp_name) : SAML2SSOHTTPClient(cfg, url, idp_name) {
		saml_post_response_ = "";
		saml_post_relaystate_ = "";
	}



	HakaClient::~HakaClient() {
	}


	MCC_Status HakaClient::processIdPLogin(const std::string username,
													   const std::string password) {
		const std::string origin = "HakaClient::processIdPLogin(const std::string username, const std::string password)";
		std::map<std::string, std::string> http_attributes;
		std::string cookie;
		std::string body_string;

		if ((*sso_pages_)["IdP"] == "") {
			return MCC_Status(GENERIC_ERROR, origin, "The IdP page is unknown!");
		}

		std::string actual_ip_login=ConfusaParserUtils::handle_redirect_step(cfg_, (*sso_pages_)["IdP"], &cookie, &http_attributes);
		logger.msg(VERBOSE, "Got over the actual ip login 2 to %s, cookie %s ", actual_ip_login, cookie);

		if (actual_ip_login.empty()) {
			return MCC_Status(PARSING_ERROR, origin, "Could not redirect from IdP with SAML2 token to IdP with internal Auth representation");
		}

		// set the idp's session cookies
		(*session_cookies_)["IdP"] = cookie;
		http_attributes["Cookie"] = cookie;

		ClientHTTP idp_login_page_client(cfg_, Arc::URL(actual_ip_login));
		PayloadRaw idp_login_page_request;
		PayloadRawInterface *idp_login_page_response = NULL;
		HTTPClientInfo idp_login_page_info;
		idp_login_page_client.process("GET", http_attributes, &idp_login_page_request, &idp_login_page_info, &idp_login_page_response);
		std::string idp_l_content = "";

		if (idp_login_page_response) {
			idp_l_content = idp_login_page_response->Content();
			delete idp_login_page_response;
		} else {
			return MCC_Status(PARSING_ERROR, origin, "Got no response from login page!");
		}

		body_string = ConfusaParserUtils::extract_body_information(idp_l_content);

		if (body_string.empty()) {
			return MCC_Status(PARSING_ERROR, origin, "Could not get the body information from the IdP's internal redirect!");
		}

		// post username and password to the page
		std::string post_params = "j_username=" + ConfusaParserUtils::urlencode(username) + "&j_password=" + ConfusaParserUtils::urlencode(password);

		ClientHTTP idp_login_post_client(cfg_, Arc::URL(actual_ip_login));
		PayloadRaw idp_login_post_body;
		PayloadRawInterface *idp_login_post_response = NULL;
		HTTPClientInfo idp_login_post_info;
		http_attributes["Content-Type"] = "application/x-www-form-urlencoded";
		idp_login_post_body.Insert(post_params.c_str(), 0, strlen(post_params.c_str()));
		idp_login_post_client.process("POST", http_attributes, &idp_login_post_body, &idp_login_post_info, &idp_login_post_response);

		std::string idp_login_post_content = "";

		if (idp_login_post_response) {
			idp_login_post_content = idp_login_post_response->Content();
			delete idp_login_post_response;
		} else {
			return MCC_Status(PARSING_ERROR, origin, "Got no response from Haka login page after posting username/password");
		}

		if (idp_login_post_content.empty()) {
			return MCC_Status(PARSING_ERROR, origin, "Could not get the id-login page from the IdP");
		}

		std::string html_body = ConfusaParserUtils::extract_body_information(idp_login_post_content);
		xmlDocPtr doc = ConfusaParserUtils::get_doc(html_body);
		logger.msg(VERBOSE, "Getting SAML response");
		saml_post_response_ = ConfusaParserUtils::evaluate_path(doc, "//input[@name='SAMLResponse']/@value");
		saml_post_relaystate_ = ConfusaParserUtils::evaluate_path(doc, "//input[@name='RelayState']/@value");
		std::string saml_post_action = ConfusaParserUtils::evaluate_path(doc, "//form/@action");
		ConfusaParserUtils::destroy_doc(doc);

		if (saml_post_response_.empty()) {
			return MCC_Status(PARSING_ERROR, origin, "Could not get the SAMLResponse from Haka's response page!");
		} else if (saml_post_relaystate_.empty()) {
			return MCC_Status(PARSING_ERROR, origin, "Could not get the RelayState from Haka's response page!");
		} else if (saml_post_action.empty()) {
			return MCC_Status(PARSING_ERROR, origin, "could not get the action from Haka's response page");
		}

		(*sso_pages_)["PostIdP"] = saml_post_action;
		return MCC_Status(STATUS_OK);
	}

	MCC_Status HakaClient::processIdP2Confusa() {
		const std::string origin = "SAML2SSOHTTPClient::processIdP2Confusa()";
		std::map<std::string, std::string> http_attributes;

		if ((*session_cookies_)["Confusa"] == "") {
			return MCC_Status(GENERIC_ERROR, origin, "Confusa's PHPSESSID Cookie is not present!");
		}

		std::string post_params = "SAMLResponse=" + ConfusaParserUtils::urlencode(saml_post_response_) + "&RelayState=" + ConfusaParserUtils::urlencode(saml_post_relaystate_);
		std::string confusa_url = (*sso_pages_)["PostIdP"];

		http_attributes["Cookie"] = (*session_cookies_)["Confusa"];
		ClientHTTP sp_asscom_client(cfg_, URL(confusa_url));
		PayloadRaw sp_asscom_request;
		PayloadRawInterface *sp_asscom_response = NULL;
		HTTPClientInfo sp_asscom_info;
		sp_asscom_request.Insert(post_params.c_str(), 0, strlen(post_params.c_str()));
		http_attributes["Content-Type"] = "application/x-www-form-urlencoded";
		sp_asscom_client.process("POST", http_attributes, &sp_asscom_request, &sp_asscom_info, &sp_asscom_response);

		// IdP SSOService to SP service page -- Confusa
		std::string confusa_service_page = sp_asscom_info.location;

		if (sp_asscom_response) {
			delete sp_asscom_response;
		}

		if (confusa_service_page.empty()) {
			return MCC_Status(PARSING_ERROR, origin, "Did not receive a Confusa service page redirect from the POST-authentication IdP!");
		} else {
			(*sso_pages_)["PostIdPConfusa"] = confusa_service_page;
			return MCC_Status(STATUS_OK);
		}
	}

	MCC_Status HakaClient::processConsent() {
		// no consent in Haka
		return MCC_Status(STATUS_OK);
	}
}
