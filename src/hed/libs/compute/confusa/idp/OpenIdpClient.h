#ifndef __ARC_OPENIDPCLIENT_H__
#define __ARC_OPENIDPCLIENT_H__

#include "../SAML2LoginClient.h"

namespace Arc {

	class OpenIdpClient : public SAML2SSOHTTPClient {
	public:
		OpenIdpClient(const MCCConfig cfg, const URL url, std::list<std::string> idp_stack);
		virtual ~OpenIdpClient();
	protected:
		MCC_Status processIdPLogin(const std::string username, const std::string password);
		MCC_Status processConsent();
		MCC_Status processIdP2Confusa();
	private:
		/* helper variable to store the body of the post-idp if needed, because it is a bad idea
		 * to make two requests to the same site
		 */
		std::string post_idp_page_;
	};
};

#endif /* __ARC_OPENIDPCLIENT_H__ */
