#include "OAuthConsumer.h"

namespace Arc {

	/**
	 * The passed idp will currently be ignored, since simplesamlphp will redirect to an idp hardcoded
	 * in the simplesamlphp configuration.
	 * In the future this is expected to change, therefore the class has a member representing the idp.
	 */
	OAuthConsumer::OAuthConsumer(const MCCConfig cfg, const URL url, std::list<std::string> idp_stack) : SAML2LoginClient(cfg, url, idp_stack)  {
		tokens_ = new std::map<std::string, std::string>();
		(*tokens_)["consumer_key"] = "key";
		(*tokens_)["consumer_secret"] = "secret";
	}

	OAuthConsumer::~OAuthConsumer() {
		if (tokens_) {
			delete tokens_;
		}
	}

	std::string OAuthConsumer::getIdPName() {
		if ((*sso_pages_)["IdP"].empty()) {
			return "Generic OAuth consumer";
		} else {
			return "Generic OAuth consumer on " + (*sso_pages_)["IdP"];
		}
	}

	// TODO: error handling, parameters, configurability
	MCC_Status OAuthConsumer::processLogin(const std::string username, const std::string password) {
		const std::string origin = "OAuthConsumer::processIdPLogin()";

		MCC_Status status;

		status = findSimpleSAMLInstallation();

		if (status.getKind() != STATUS_OK) {
			return status;
		}

		status = processOAuthRequest();

		if (status.getKind() != STATUS_OK) {
			return status;
		}

		status = processOAuthAuthorization();

		if (status.getKind() != STATUS_OK) {
			return status;
		}


		status = processOAuthAccessToken();

		if (status.getKind() != STATUS_OK) {
			return status;
		}

		std::string oauth_access_token = (*tokens_)["oauth_access_token"];
		std::string oauth_access_secret = (*tokens_)["oauth_access_secret"];
		std::string consumer_key = (*tokens_)["consumer_key"];
		std::string consumer_secret = (*tokens_)["consumer_secret"];

		logger.msg(DEBUG, "Contacting the OAuth user info endpoint");

		URL sp_url((*sso_pages_)["SimpleSAML"] + USER_INFO);
		std::string about_url = sp_url.fullstr();

		char *req_url = oauth_sign_url(about_url.c_str(), NULL, OA_HMAC, consumer_key.c_str(), consumer_secret.c_str(), oauth_access_token.c_str(), oauth_access_secret.c_str());
		ClientHTTP about_client(cfg_,URL(req_url));
		PayloadRaw about_request;
		PayloadRawInterface *about_response = NULL;
		HTTPClientInfo about_client_info;
		about_client.RelativeURI(true);
		about_client.process("GET", &about_request, &about_client_info, &about_response);

		std::string about_response_str = "";

		if (req_url) {
			free(req_url);
		}

		if (about_response) {
			about_response_str = about_response->Content();
			delete about_response;
		} else {
			return MCC_Status(GENERIC_ERROR, origin, "Did not get user information from the OAuth User-Info service");
		}

		logger.msg(INFO, "The OAuth service provider knows the following about you: %s", about_response_str);


		return MCC_Status(STATUS_OK);
	}

	MCC_Status OAuthConsumer::processOAuthRequest() {
		const std::string origin = "OAuthConsumer::processOAuthRequest()";

		if ((*sso_pages_)["SimpleSAML"] == "") {
			return MCC_Status(GENERIC_ERROR, origin, "Start address is not defined");
		}

		// Currently OAuth on the simplesamlphp-side only supports key/secret
		const std::string consumer_key = (*tokens_)["consumer_key"];
		// TODO autogenerate that, e.g. using a random number generator or something similar
		const std::string consumer_secret = (*tokens_)["consumer_secret"];

		// we need to pass that through an URL to have the port in it, so checking the SHA1 signature verify on the OAuth SP will not fail
		URL request_url((*sso_pages_)["SimpleSAML"] + REQUEST_TOKEN_URL);
		std::string request_url_str = request_url.fullstr();
		logger.msg(DEBUG, "The request_url is %s", request_url_str);

		// produce an OAuth request token
		char *req_url = oauth_sign_url(request_url_str.c_str(), NULL, OA_HMAC, consumer_key.c_str(), consumer_secret.c_str(), NULL, NULL);
		logger.msg(DEBUG, "Sending oauth request to signed url %s", req_url);

		if (!req_url) {
			return MCC_Status(GENERIC_ERROR, origin, "Could not sign the request url " + request_url);
		}

		std::string str_req_url = req_url;

		ClientHTTP oauth_request_client(cfg_, URL(str_req_url));
		PayloadRaw oauth_request_request;
		PayloadRawInterface *oauth_request_post_response = NULL;
		HTTPClientInfo oauth_request_client_info;
		oauth_request_client.RelativeURI(true);
		oauth_request_client.process("GET", &oauth_request_request, &oauth_request_client_info, &oauth_request_post_response);

		std::string oauth_response = "";

		if (req_url) {
			// c-string as produced by "extern C" library. Free, not delete.
			free(req_url);
		}

		if (oauth_request_post_response) {
			oauth_response = oauth_request_post_response->Content();
			delete oauth_request_post_response;
		} else {
			return MCC_Status(PARSING_ERROR, origin, "Did not get a response from the OAuth SP request page");
		}

		std::map<std::string, std::string> oauth_params = ConfusaParserUtils::get_url_params(oauth_response);

		std::string oauth_request_token = oauth_params["oauth_token"];
		std::string oauth_request_secret = oauth_params["oauth_token_secret"];

		if (oauth_request_token.empty() || oauth_request_secret.empty()) {
			return MCC_Status(PARSING_ERROR, origin, "OAuth request response is not wellformed. OAuth request token: " + oauth_request_token
					+ " OAuth request secret: " + oauth_request_secret);
		}

		(*tokens_)["oauth_request_token"] = oauth_request_token;
		(*tokens_)["oauth_request_secret"] = oauth_request_secret;

		return MCC_Status(STATUS_OK);
	}

	MCC_Status OAuthConsumer::processOAuthAuthorization() {
		const std::string origin = "OAuthConsumer::processOAuthAuthorization()";

		URL sp_url((*sso_pages_)["SimpleSAML"] + AUTHORIZE_URL);

		std::string oauth_request_token = (*tokens_)["oauth_request_token"];
		std::string oauth_request_secret = (*tokens_)["oauth_request_secret"];
		std::string consumer_key = (*tokens_)["consumer_key"];
		std::string consumer_secret = (*tokens_)["consumer_secret"];

		std::string authorize_url = sp_url.fullstr();

		// is the oauth_token the first parameter or not?
		if (authorize_url.find("?") == std::string::npos) {
			authorize_url = authorize_url + "?oauth_token=";
		} else {
			authorize_url = authorize_url + "&oauth_token=";
		}

		authorize_url = authorize_url + (*tokens_)["oauth_request_token"];
		//authorize_url = authorize_url + "&RelayState=" + ParserUtils::urlencode("https://confusatest.pdc.kth.se/simplesaml/module.php/oauth/authorize.php");
		std::cout << "Please login at the following url " << authorize_url << std::endl;


		// just waiting for the user to press enter
		std::cin.clear();
		std::cout << "Press enter to continue\n";
		std::cin.ignore(1,0);

		return MCC_Status(STATUS_OK);
	}

	MCC_Status OAuthConsumer::processOAuthAccessToken() {
		std::string origin = "OAuthConsumer::processOAuthAccessToken()";

		std::string oauth_request_token = (*tokens_)["oauth_request_token"];
		std::string oauth_request_secret = (*tokens_)["oauth_request_secret"];
		std::string consumer_key = (*tokens_)["consumer_key"];
		std::string consumer_secret = (*tokens_)["consumer_secret"];

		URL sp_url((*sso_pages_)["SimpleSAML"] + ACCESS_TOKEN_URL);
		std::string access_token_url = sp_url.fullstr();

		char *req_url = oauth_sign_url(access_token_url.c_str(), NULL, OA_HMAC, consumer_key.c_str(), consumer_secret.c_str(), oauth_request_token.c_str(), oauth_request_secret.c_str());
		std::string str_req_url = req_url;

		ClientHTTP oauth_access_client(cfg_, URL(str_req_url));
		PayloadRaw oauth_access_request;
		PayloadRawInterface *oauth_access_post_response = NULL;
		HTTPClientInfo oauth_access_client_info;
		oauth_access_client.RelativeURI(true);
		oauth_access_client.process("GET", &oauth_access_request, &oauth_access_client_info, &oauth_access_post_response);

		std::string oauth_access_response = "";

		if (req_url) {
			// again, C-free due to "extern C" declaration
			free(req_url);
		}

		if (oauth_access_post_response) {
			oauth_access_response = oauth_access_post_response->Content();
			delete oauth_access_post_response;
		} else {
			return MCC_Status(PARSING_ERROR, origin, "Invalid response from access token endpoint!");
		}

		std::map<std::string, std::string> oauth_params = ConfusaParserUtils::get_url_params(oauth_access_response);
		std::string oauth_access_token = oauth_params["oauth_token"];
		std::string oauth_access_secret = oauth_params["oauth_token_secret"];

		if (oauth_access_token.empty() || oauth_access_secret.empty()) {
			return MCC_Status(PARSING_ERROR, origin, "OAuth access token response is not wellformed. OAuth access token: " + oauth_access_token
					+ " OAuth access secret: " + oauth_access_secret);
		}

		(*tokens_)["oauth_access_token"] = oauth_access_token;
		(*tokens_)["oauth_access_secret"] = oauth_access_secret;

		return MCC_Status(STATUS_OK);
	}

	MCC_Status OAuthConsumer::parseDN(std::string* dn) {
		std::string origin = "OAuthConsumer::parseDN(string *dn)";
		return MCC_Status(GENERIC_ERROR, origin, "OAuth authentication currently not supported in Confusa");
	}

	MCC_Status OAuthConsumer::approveCSR(const std::string approve_page) {
		std::string origin = "OAuthConsumer::approveCSR(const string)";
		return MCC_Status(GENERIC_ERROR, origin, "OAuth authentication currently not supported in Confusa");
	}

	MCC_Status OAuthConsumer::pushCSR(const std::string b64_pub_key, const std::string pub_key_hash, std::string *approve_page) {
		std::string origin = "OAuthConsumer::pushCSR(const string, const string, string *";
		return MCC_Status(GENERIC_ERROR, origin, "OAuth authentication currently not supported in Confusa");
	}

	MCC_Status OAuthConsumer::storeCert(const std::string cert_path, const std::string auth_token, const std::string b64_dn) {
		std::string origin = "OAuthConsumer::storeCert(const string, const string, const string)";
		return MCC_Status(GENERIC_ERROR, origin, "OAuth authentication currently not supported in Confusa");
	}
};
