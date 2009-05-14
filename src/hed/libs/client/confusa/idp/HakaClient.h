#ifndef __ARC_HAKACLIENT_H__
#define __ARC_HAKACLIENT_H__

#include "../SAML2LoginClient.h"

namespace Arc {

	class HakaClient : public SAML2SSOHTTPClient {
	public:
		HakaClient(MCCConfig cfg, URL url, std::list<std::string> idp_stack);
		virtual ~HakaClient();
	protected:
		MCC_Status processIdPLogin(const std::string username, const std::string password);
		MCC_Status processConsent();
		MCC_Status processIdP2Confusa();
	private:
		// these are needed to follow the HTTP POST profile followed by Haka
		std::string saml_post_response_;
		std::string saml_post_relaystate_;
		// post parameters needed for consent
		std::string principal_;
		std::string providerid_;
		std::string attributes_;
	};
};

#endif /* __ARC_HAKACLIENT_H__ */
