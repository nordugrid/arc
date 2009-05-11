// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SAML2SSOHTTPCLIENT_H__
#define __ARC_SAML2SSOHTTPCLIENT_H__

#include <string>
#include <list>

#include <inttypes.h>

#include <arc/URL.h>
#include <arc/message/MCC_Status.h>
#include <arc/message/PayloadRaw.h>
#include <arc/client/ClientInterface.h>
#include "ConfusaParserUtils.h"

namespace Arc {

  // TODO: This is somewhat Confusa-configuration dependent!
  const std::string up_page = "/key_upload.php";
  const std::string down_page = "/key_download.php";
  const std::string about_page = "/about_you.php";
  const std::string start_page = "/index.php?start_login=yes";

 /*
  * ABC to all different kinds of login clients
  */
 class SAML2LoginClient {
	 public:
		SAML2LoginClient(const MCCConfig cfg, const URL url, const std::string idp_name);
		virtual ~SAML2LoginClient();
		/**
		 * Base interface for all login procedures
		 */
		virtual MCC_Status processLogin(const std::string username="", const std::string password="") = 0;

		virtual MCC_Status parseDN(std::string* dn) = 0;
		virtual MCC_Status approveCSR(const std::string approve_page) = 0;
		virtual MCC_Status pushCSR(const std::string b64_pub_key, const std::string pub_key_hash, std::string *approve_page) = 0;
		virtual MCC_Status storeCert(const std::string cert_path, const std::string auth_token, const std::string b64_dn) = 0;
		void printBrowsingHistory();
	 protected:
		 std::map<std::string, std::string> *session_cookies_;
		 std::map<std::string, std::string> *sso_pages_;
		 MCCConfig cfg_;
		 static Logger logger;
		 URL server_loc_;
		 std::string idp_name_;

 };

 /*
  *  this class is purely virtual
  *  plugins implementing actual IdP login can be found in folder idp
  */
 class SAML2SSOHTTPClient : public SAML2LoginClient {
  public:
    SAML2SSOHTTPClient(const MCCConfig cfg, const URL url, const std::string idp_name);
    virtual ~SAML2SSOHTTPClient();
    /**
     * Models complete SAML2 WebSSO authN flow with start -> WAYF -> Login -> (consent) -> start
     */
    MCC_Status processLogin(const std::string username, const std::string password);
    /**
     * Parse the used DN from the Confusa about_you page
     */
    MCC_Status parseDN(std::string* dn);
    /**
     * Simulate click on the approve cert signing request link
     */
    MCC_Status approveCSR(const std::string approve_page);
    /**
     * Send the cert signing request to Confusa for signing
     */
    MCC_Status pushCSR(const std::string b64_pub_key, const std::string pub_key_hash, std::string *approve_page);
    /**
     * Download the signed certificate from Confusa and store it locally
     */
    MCC_Status storeCert(const std::string cert_path, const std::string auth_token, const std::string b64_dn);

  protected:
	/**
	 * Actual identity provider parsers for next three methods implemented in subdirectory idp/
	 *
	 * Parse identity provider login page and submit username and password in the previsioned way
	 */
 	virtual MCC_Status processIdPLogin(const std::string username, const std::string password) = 0;
 	/**
 	 * If the IdP has a consent module and the user has not saved here consent, this method will ask
 	 * the user for consent to transmission of her data to Confusa
 	 */
 	virtual MCC_Status processConsent() = 0;
 	/**
 	 * Redirects the user back from identity provider to the Confusa SP
 	 */
 	virtual MCC_Status processIdP2Confusa() = 0;

  private:
	/**
	 * Redirect the user to the WAYF (where are you from) service.
	 */
	MCC_Status processStart2WAYF();
	/**
	 * Use the configured identity provider to make a choice for the user on the WAYF page
	 * Redirect the user to the actual IdP from the WAYF page.
	 */
	MCC_Status processWAYF2IdPLogin();
  };
} // namespace Arc

#endif // __ARC_SAML2SSOHTTPCLIENT_H__
