#ifndef __ARC_OAUTHCONSUMER_H__
#define __ARC_OAUTHCONSUMER_H__

#include "SAML2LoginClient.h"

extern "C" {
	#include <oauth.h>
}

namespace Arc {

	static const std::string REQUEST_TOKEN_URL = "/simplesaml/module.php/oauth/requestToken.php";
	static const std::string ACCESS_TOKEN_URL = "/simplesaml/module.php/oauth/accessToken.php";
	static const std::string AUTHORIZE_URL = "/simplesaml/module.php/oauth/authorize.php";
	static const std::string USER_INFO = "/simplesaml/module.php/oauth/getUserInfo.php";
	static const std::string ABOUT_YOU = "/slcs/about_you.php?text=yes";

	/**
	 * The OAuth functionality depends on the availability of the liboauth C-bindings library
	 */
	class OAuthConsumer : public SAML2LoginClient {

	public:
		/**
		 * Construct an OAuth consumer with url as service provider.
		 * idp_name is currently ignored, since the idp to which the SAML2 redirect will take place is presently
		 * a hardcoded value on the SAML2 SP side.
		 * This is expected to change in the future.
		 */
		OAuthConsumer(const MCCConfig cfg, const URL url, std::list<std::string> idp_stack);
		virtual ~OAuthConsumer();
		std::string getIdPName();
		/**
		 * Unsupported placeholder function until Confusa supports OAuth.
		 */
		MCC_Status parseDN(std::string* dn);
		/**
		 * Unsupported placeholder function until Confusa supports OAuth.
		 */
		MCC_Status approveCSR(const std::string approve_page);
		/**
		 * Unsupported placeholder function until Confusa supports OAuth.
		 */
		MCC_Status pushCSR(const std::string b64_pub_key, const std::string pub_key_hash, std::string *approve_page);
		/**
		 * Unsupported placeholder function until Confusa supports OAuth.
		 */
		MCC_Status storeCert(const std::string cert_path, const std::string auth_token, const std::string b64_dn);
	protected:
		/**
		 * Main function performing all the OAuth login steps.
		 * Username and password will be ignored.
		 */
		MCC_Status processLogin(const std::string username="", const std::string password="");
	private:
		/**
		 * Get an OAuth request token for the given key and secret. Currently, OAuth on simplesamlphp supports
		 * only "key" and "secret" as values for this
		 */
		MCC_Status processOAuthRequest();
		/**
		 * Authorize the OAuth request made in step 1 of OAuth authentication.
		 * Will display an URL to a website, which the user has to visit.
		 */
		MCC_Status processOAuthAuthorization();
		/**
		 * If authorization for the request has been obtained in step 2 of OAuth authentication,
		 * this procedure contacts the OAuth SP to obtain an access token
		 */
		MCC_Status processOAuthAccessToken();

		/**
		 * All the tokens obtained during OAuth authentication
		 */
		std::map<std::string, std::string> *tokens_;
	};
};

#endif /* __ARC_OAUTHCONSUMER_H__ */
