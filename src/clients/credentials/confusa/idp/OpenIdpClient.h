/*
 * OpenIdpClient.h
 *
 *  Created on: Apr 23, 2009
 *      Author: tzangerl
 */

#ifndef OPENIDPCLIENT_H_
#define OPENIDPCLIENT_H_

#include "../SAML2LoginClient.h"

namespace Arc {

	class OpenIdpClient : public SAML2SSOHTTPClient {
	public:
		OpenIdpClient(const MCCConfig cfg, const URL url, const std::string idp_name);
		virtual ~OpenIdpClient();
	protected:
		MCC_Status processIdPLogin(const std::string username, const std::string password);
		MCC_Status processConsent();
		MCC_Status processIdP2Confusa();
	};
};

#endif /* OPENIDPCLIENT_H_ */
