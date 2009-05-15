// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SAML2LOGINCLIENT_H__
#define __ARC_SAML2LOGINCLIENT_H__

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
  const std::string initSSO_loc = "saml2/sp/initSSO.php";

 /*
  * ABC to all different kinds of login clients
  */
 class SAML2LoginClient {
	 public:
		/**
		 * list with the idp for nested wayf
		 * For example, Confusa can use betawayf.wayf.dk as an identity provider, which is itself only a wayf
		 * and shares the metadata with concrete service providers or even further nested wayfs.
		 * Since due to mutual authentication with metadata, we are obliged to follow the SSO redirects from
		 * WAYF to WAYF, the WAYFs are stored in a list.
		 */
		SAML2LoginClient(const MCCConfig cfg, const URL url, std::list<std::string> idp_stack);
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

		  /**
		  * find the location of the simplesamlphp installation on the SP side
		  * Will be stored in (*sso_pages)[SimpleSAML]
		  */
		 MCC_Status findSimpleSAMLInstallation();
	 protected:
		 std::map<std::string, std::string> *session_cookies_;
		 std::map<std::string, std::string> *sso_pages_;
		 MCCConfig cfg_;
		 static Logger logger;
		 URL server_loc_;
		 std::list<std::string> idp_stack_;

 };

 /*
  *  this class is purely virtual
  *  plugins implementing actual IdP login can be found in folder idp
  */
 class SAML2SSOHTTPClient : public SAML2LoginClient {
  public:
    SAML2SSOHTTPClient(const MCCConfig cfg, const URL url, std::list<std::string> idp_stack);
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
 	 * If the IdP has a consent module and the user has not saved her consent, this method will ask
 	 * the user for consent to transmission of her data to Confusa
 	 */
 	virtual MCC_Status processConsent() = 0;
 	/**
 	 * Redirects the user back from identity provider to the Confusa SP
 	 */
 	virtual MCC_Status processIdP2Confusa() = 0;

  private:
	/**
	 * Proceed from Confusa to the IdP login site
	 */
	MCC_Status processStart2Login();
  };
} // namespace Arc

#endif // __ARC_SAML2LOGINCLIENT_H__
