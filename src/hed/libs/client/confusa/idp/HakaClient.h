/*
 * HakaSAML2SSOHTTPClient.h
 *
 *  Created on: Apr 23, 2009
 *      Author: tzangerl
 */

#ifndef HAKACLIENT_H_
#define HAKACLIENT_H_

#include "../SAML2LoginClient.h"

namespace Arc {

	class HakaClient : public SAML2SSOHTTPClient {
	public:
		HakaClient(MCCConfig cfg, URL url, const std::string idp_name);
//		HakaClient();
		virtual ~HakaClient();
	protected:
		MCC_Status processIdPLogin(const std::string username, const std::string password);
		MCC_Status processConsent();
		MCC_Status processIdP2Confusa();
	private:
		// these are needed to follow the HTTP POST profile followed by Haka
		std::string saml_post_response_;
		std::string saml_post_relaystate_;
	};
};

#endif /* HAKACLIENT_H_ */
