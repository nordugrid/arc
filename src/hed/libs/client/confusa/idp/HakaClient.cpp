#include "HakaClient.h"

namespace Arc {

	HakaClient::HakaClient(MCCConfig cfg, URL url, std::list<std::string> idp_stack) : SAML2SSOHTTPClient(cfg, url, idp_stack) {
		saml_post_response_ = "";
		saml_post_relaystate_ = "";
	}

	HakaClient::~HakaClient() {
	}


	MCC_Status HakaClient::processIdPLogin(const std::string username,
													   const std::string password) {
		const std::string origin = "HakaClient::processIdPLogin(const std::string, const std::string)";
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
		ConfusaParserUtils::add_cookie(&(*session_cookies_)["IdP"], cookie);
		ConfusaParserUtils::add_cookie(&(*session_cookies_)["IdP"], "_idp_session=MTMwLjIzNy4yMjEuMTE2%7COTQ2YmNhYjk0NzIxZWY1Y2M5NjI2NjIxMzBkZjdiYzYxZWY5NzQ0NmZmZDlhNjJhOWIxM2RlMWNjMmY4ZWU3YQ%3D%3D%7C7tqOZ2x83TUmF6cdMhUislQb%2Bp4%3D");
		http_attributes["Cookie"] = (*session_cookies_)["IdP"];

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

		logger.msg(VERBOSE, "Cookie received from login page %s", idp_login_page_info.cookie);
		ConfusaParserUtils::add_cookie(&((*session_cookies_)["IdP"]), idp_login_page_info.cookie);
		http_attributes["Cookie"] = (*session_cookies_)["IdP"];

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

		logger.msg(DEBUG, "The idp_login_post_info cookie is %s, while the sent cookie was %s", idp_login_post_info.cookie, (*session_cookies_)["IdP"]);
		ConfusaParserUtils::add_cookie(&((*session_cookies_)["IdP"]),idp_login_post_info.cookie);
		http_attributes["Cookie"] = (*session_cookies_)["IdP"];


		std::string html_body = ConfusaParserUtils::extract_body_information(idp_login_post_content);
		xmlDocPtr doc = ConfusaParserUtils::get_doc(html_body);
		logger.msg(VERBOSE, "Getting SAML response");
		std::string saml_post_action = ConfusaParserUtils::evaluate_path(doc, "//form/@action");

		bool approve = !(saml_post_action.find("uApprove") == std::string::npos);

		if (approve) {
			principal_ = ConfusaParserUtils::evaluate_path(doc, "//input[@name='principal']/@value");
			providerid_ = ConfusaParserUtils::evaluate_path(doc, "//input[@name='providerid']/@value");
			attributes_ = ConfusaParserUtils::evaluate_path(doc, "//input[@name='attributes']/@value");
		} else {
			saml_post_response_ = ConfusaParserUtils::evaluate_path(doc, "//input[@name='SAMLResponse']/@value");
			saml_post_relaystate_ = ConfusaParserUtils::evaluate_path(doc, "//input[@name='RelayState']/@value");
		}

		ConfusaParserUtils::destroy_doc(doc);

		if (!approve && saml_post_response_.empty()) {
			return MCC_Status(PARSING_ERROR, origin, "Could not get the SAMLResponse from Haka's response page!");
		} else if (!approve && saml_post_relaystate_.empty()) {
			return MCC_Status(PARSING_ERROR, origin, "Could not get the RelayState from Haka's response page!");
		} else if (!approve && saml_post_action.empty()) {
			return MCC_Status(PARSING_ERROR, origin, "could not get the action from Haka's response page");
		} else if (approve && principal_.empty()) {
			return MCC_Status(PARSING_ERROR, origin, "Could not get the principal from Haka's response page");
		} else if (approve && providerid_.empty()) {
			return MCC_Status(PARSING_ERROR, origin, "Could not get the provider ID from Haka's response page");
		} else if (approve && attributes_.empty()) {
			return MCC_Status(PARSING_ERROR, origin, "Could not get the attributes from Haka's response page");
		}

		// check if Haka asks for user consent
		if (approve) {
			(*sso_pages_)["Consent"] = saml_post_action;
		} else {
			(*sso_pages_)["PostIdP"] = saml_post_action;
		}

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

	/** TODO: ugly moloch
	 * from looking at the current situation it will not be possible to handle Haka consent until multiple cookies in the response
	 * header are supported by ClientHTTP
	 */
	MCC_Status HakaClient::processConsent() {
		logger.msg(DEBUG, "Called HakaClient::processConsent()");
		const std::string origin = "HakaClient::processConsent()";
		std::map<std::string, std::string> http_attributes;

		logger.msg(DEBUG, "Checking if consent is necessary");
		// no consent necessary
		if ((*sso_pages_)["Consent"] == "") {
			return MCC_Status(STATUS_OK);
		}

		if ((*session_cookies_)["IdP"] == "") {
			return MCC_Status(GENERIC_ERROR, origin, "Could not find Haka's browser cookie!");
		}

		logger.msg(DEBUG, "User consent to attribute transfer is necessary");

		std::string post_params = "principal=" + ConfusaParserUtils::urlencode(principal_) + "&providerid=" + ConfusaParserUtils::urlencode(providerid_) + "&attributes=" + ConfusaParserUtils::urlencode(attributes_);

		std::string cookie;
		http_attributes["Cookie"] = (*session_cookies_)["IdP"];
		URL consent_url((*sso_pages_)["Consent"]);
		ClientHTTP consent_client(cfg_, consent_url);
		PayloadRaw consent_request;
		PayloadRawInterface *consent_response = NULL;
		HTTPClientInfo consent_info;
		consent_request.Insert(post_params.c_str(), 0, strlen(post_params.c_str()));
		http_attributes["Content-Type"] = "application/x-www-form-urlencoded";
		consent_client.process("POST", http_attributes, &consent_request, &consent_info, &consent_response);
		// the consent module will set a cookie valid for the consent session
		(*session_cookies_)["Consent"] = consent_info.cookie;
		http_attributes["Cookie"] = (*session_cookies_)["Consent"];
		std::string consent_response_str = "";

		if (consent_response) {
			consent_response_str = consent_response->Content();
			delete consent_response;
		} else {
			return MCC_Status(PARSING_ERROR, origin, "Could not get the consent page's body information!");
		}


		std::string user_conf;
		std::cout << std::endl << "Your identity provider will send the following information to the SLCS service:" << std::endl;
		std::cout << "==============================================================================" << std::endl;

		// this page has too many HTML errors and is unparsable by libxml2
		std::string::size_type last_attr_pos = consent_response_str.find("attr-name");
		while (last_attr_pos != std::string::npos && last_attr_pos < consent_response_str.size()) {

			std::string::size_type attr_name_content_pos = consent_response_str.find(">", last_attr_pos) + 1;

			if (attr_name_content_pos == std::string::npos) {
				break;
			}

			std::string::size_type attr_name_content_end = consent_response_str.find("<", attr_name_content_pos);

			if (attr_name_content_end == std::string::npos) {
				break;
			}

			std::string attr_name_str = consent_response_str.substr(attr_name_content_pos, (attr_name_content_end-attr_name_content_pos));
			std::cout << attr_name_str + ": ";

			std::string::size_type attr_value_pos = consent_response_str.find("attr-value", attr_name_content_end);

			if (attr_value_pos == std::string::npos) {
				break;
			}

			std::string::size_type attr_value_content_pos = consent_response_str.find(">", attr_value_pos) + 1;

			if (attr_value_content_pos == std::string::npos) {
				break;
			}

			std::string::size_type attr_value_content_end = consent_response_str.find("<", attr_value_content_pos);

			if (attr_value_content_end == std::string::npos) {
				break;
			}

			std::string attr_value_str = consent_response_str.substr(attr_value_content_pos, (attr_value_content_end-attr_value_content_pos));
			std::cout << attr_value_str << std::endl;

			last_attr_pos = consent_response_str.find("attr-name", attr_value_content_end);
		}

		std::cout << "==============================================================================" << std::endl;

		std::cout << std::endl;
		std::cout << "Do you consent to the release of that information? (y/n) ";
		std::cin >> user_conf;

		if (user_conf == "y") {
			std::stringstream portstring;
			portstring << consent_url.Port();
			std::string consent_confirm_port = portstring.str();

			std::string consent_confirm_url = consent_url.Protocol() + "://" + consent_url.Host() + ":" + consent_confirm_port + "/" + consent_url.Path() + "?attributes-confirm=Confirm";
			std::string consent_confirm_redir = ConfusaParserUtils::handle_redirect_step(cfg_, consent_confirm_url, &cookie, &http_attributes);
			logger.msg(DEBUG, "Consent confirm redir URL is %s, cookies %s", consent_confirm_redir, (*session_cookies_)["IdP"]);
			http_attributes["Cookie"] = (*session_cookies_)["IdP"];

			ClientHTTP post_consent_client(cfg_, URL(consent_confirm_redir));
			PayloadRaw post_consent_request;
			HTTPClientInfo post_consent_info;
			PayloadRawInterface *post_consent_response = NULL;

			post_consent_client.process("GET", &post_consent_request, &post_consent_info, &post_consent_response);
			std::string post_consent_response_str = "";

			if (post_consent_response) {
				post_consent_response_str = post_consent_response->Content();
				delete post_consent_response;
			} else {
				return MCC_Status(PARSING_ERROR, origin, "Can not get SAML response and relay state after consent!");
			}

			std::string body_string = ConfusaParserUtils::extract_body_information(post_consent_response_str);

			xmlDocPtr doc = ConfusaParserUtils::get_doc(body_string);
			saml_post_response_ = ConfusaParserUtils::evaluate_path(doc, "//input[@name='SAMLResponse']/@value");
			saml_post_relaystate_ = ConfusaParserUtils::evaluate_path(doc, "//input[@name='RelayState']/@value");
			ConfusaParserUtils::destroy_doc(doc);

			if (saml_post_response_.empty()) {
				return MCC_Status(PARSING_ERROR, origin, "Could not extract SAMLResponse!");
			} else if (saml_post_relaystate_.empty()) {
				return MCC_Status(PARSING_ERROR, origin, "Could not extract RelayState");
			}

		} else {
			return MCC_Status(GENERIC_ERROR, origin, "No user consent to attribute release. Aborting");
		}


		return MCC_Status(STATUS_OK);
	}
}
