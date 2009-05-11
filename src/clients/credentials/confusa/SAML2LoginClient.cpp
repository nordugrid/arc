// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/**
 * The original development of this class is based on ClientSAML2SSO, but because the modifications became too
 * large, it was moved to another class. I think it is now fair to say that it doesn't have too much in common with ClientSAML2SSO any more ;-)
 */

// This define is needed to have maximal values for types with fixed size
#define __STDC_LIMIT_MACROS
#include <stdlib.h>
#include <map>

#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/message/MCC.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <fstream>

#include "SAML2LoginClient.h"

namespace Arc {

  // SAML2LoginClient
  Logger SAML2LoginClient::logger(Logger::getRootLogger(), "SAML2LoginClient");

  SAML2LoginClient::SAML2LoginClient(const MCCConfig cfg,
                                     const URL url,
                                     const std::string idp_name) {
    server_loc_ = url;
    cfg_ = cfg;

    idp_name_ = idp_name;
    session_cookies_ = new std::map<std::string, std::string>();
    sso_pages_ = new std::map<std::string, std::string>();
    (*sso_pages_)["ConfusaStart"] = server_loc_.fullstr() + start_page;
  }


  SAML2LoginClient::~SAML2LoginClient() {
	  if (session_cookies_) {
		  delete session_cookies_;
	  }

	  if (sso_pages_) {
		  delete sso_pages_;
	  }

  }

  void SAML2LoginClient::printBrowsingHistory() {
 	  std::cout << "Your browsing history: " << std::endl;
 	  std::map<std::string,std::string>::iterator it = sso_pages_->begin();

 	  while (it != sso_pages_->end()) {
 		  std::cout << "============================================================" << std::endl;
 		  std::cout << (*it).first << ": " << (*it).second << std::endl;
 		  it++;
 	  }
  }


  //SAML2SSOHTTPClient
  SAML2SSOHTTPClient::SAML2SSOHTTPClient(const MCCConfig cfg, const URL url, const std::string idp_name) : SAML2LoginClient(cfg,url, idp_name) {
	  // need the idp_name Base64-encoded for the Web-SSO RelayState
	  idp_name_ = ConfusaParserUtils::urlencode(idp_name);
  }

  SAML2SSOHTTPClient::~SAML2SSOHTTPClient() {
  }


  MCC_Status SAML2SSOHTTPClient::processStart2WAYF() {
	    MCC_Status status;
	    const std::string origin = "SAML2SSOHTTPClient::processStart2WAYF()";

	    if ((*sso_pages_)["ConfusaStart"] == "") {
	    	return MCC_Status(GENERIC_ERROR, origin, "The start address is not defined");
	    }

	    logger.msg(VERBOSE, "ConfusaStart is %s", (*sso_pages_)["ConfusaStart"]);

	    std::string cookie;
	    std::map<std::string, std::string> http_attributes;
	    std::string authn_request_url=ConfusaParserUtils::handle_redirect_step(cfg_, (*sso_pages_)["ConfusaStart"], &cookie);
	    logger.msg(VERBOSE, "The received cookie was %s, url %s", cookie, authn_request_url);

	    if (authn_request_url.empty()) {
	    	return MCC_Status(PARSING_ERROR, origin, "Did not get authentication request URL in Confusa");
	    }

	    (*session_cookies_)["Confusa"] = cookie;
	    http_attributes["Cookie"] = (*session_cookies_)["Confusa"];
	    std::string current_sso_page=ConfusaParserUtils::handle_redirect_step(cfg_, authn_request_url, &cookie, &http_attributes);

	    if (current_sso_page.empty()) {
	    	return MCC_Status(PARSING_ERROR, origin, "Did not get WAYF URL from redirect in Confusa");
	    } else {
	    	(*sso_pages_)["WAYF"] = current_sso_page;
	    	return MCC_Status(STATUS_OK);
	    }
  }

  MCC_Status SAML2SSOHTTPClient::processWAYF2IdPLogin() {
	  const std::string origin = "SAML2SSOHTTPClient::processWAYF2IdPLogin()";
	  std::string cookie;

	  if ((*sso_pages_)["WAYF"] == "") {
		  return MCC_Status(GENERIC_ERROR, origin, "The WAYF page is unknown!");
	  }

	  if ((*session_cookies_)["Confusa"] == "") {
		  return MCC_Status(GENERIC_ERROR, origin, "Confusa's PHPSESSID Cookie is not present!");
	  }

	  URL wayf_url((*sso_pages_)["WAYF"]);
	  ClientHTTP wayf_client(cfg_, wayf_url);
	  PayloadRaw wayf_request;
	  PayloadRawInterface *wayf_response = NULL;
	  HTTPClientInfo wayf_info;
	  std::map<std::string, std::string> http_attributes;
	  http_attributes["Cookie"] = (*session_cookies_)["Confusa"];

	  wayf_client.process("GET", http_attributes, &wayf_request, &wayf_info, &wayf_response);

	  std::string wayf_html = "";

	  if (wayf_response) {
		  wayf_html = wayf_response->Content();
		  delete wayf_response;
	  } else {
		  return MCC_Status(PARSING_ERROR, origin, "Could not get a valid response from the WAYF site");
	  }

	  std::string body_string = ConfusaParserUtils::extract_body_information(wayf_html);

	  if (body_string.empty()) {
		  return MCC_Status(PARSING_ERROR, origin, "Could not get the WAYF body from the WAYF service!");
	  }

	  xmlDocPtr doc = ConfusaParserUtils::get_doc(body_string);
	  std::string action = ConfusaParserUtils::evaluate_path(doc, "//form/@action");
	  std::string returnName = ConfusaParserUtils::evaluate_path(doc, "//input[@name='return']/@value");
	  std::string entityID = ConfusaParserUtils::evaluate_path(doc, "//input[@name='entityID']/@value");
	  ConfusaParserUtils::destroy_doc(doc);

	  if (action.empty()) {
		  return MCC_Status(PARSING_ERROR, origin, "Could not parse the action from the WAYF service!");
	  } else if (returnName.empty()) {
		  return MCC_Status(PARSING_ERROR, origin, "Could not parse the return name from the WAYF service!");
	  } else if (entityID.empty()) {
		  return MCC_Status(PARSING_ERROR, origin, "Could not parse the entity ID from the WAYF service!");
	  }

	  // TODO prio=high add more providers
	  std::string identity_redirect = action + "?entityID=" + ConfusaParserUtils::urlencode(entityID) + "&return=" + ConfusaParserUtils::urlencode(returnName) + "&returnIDParam=idpentityid" + "&idp_" + idp_name_ + "=Select";

	  // WAYF back to simplesamlphp_initSSO -- Confusa
	  std::string post_wayf_redirect = ConfusaParserUtils::handle_redirect_step(cfg_, identity_redirect,&cookie,&http_attributes);

	  if (post_wayf_redirect.empty()) {
		  return MCC_Status(PARSING_ERROR, origin, "Could not get the redirect from WAYF back to initSSO!");
	  }

	  // simplesamlphp_initSSO to actual identity provdider with SAML2 token -- IdP
	  std::string actual_ip_login = ConfusaParserUtils::handle_redirect_step(cfg_, post_wayf_redirect, &cookie);
	  logger.msg(VERBOSE, "Actual ip login: %s", actual_ip_login);

	  if (actual_ip_login.empty()) {
		  return MCC_Status(PARSING_ERROR, origin, "Could not get the SAML2 redirect from Confusa initSSO to IdP!");
	  } else {
		  // need to set session cookie
		  //(*session_cookies_)["IdP"] = cookie;
		  //logger.msg(VERBOSE, "IdP cookie is the following: %s", cookie);
		  (*sso_pages_)["IdP"] = actual_ip_login;
		  return MCC_Status(STATUS_OK);
	  }

  }


  MCC_Status SAML2SSOHTTPClient::processLogin(const std::string username, const std::string password) {

    // -------------------------------------------
    // User-Agent: Send an empty http request to SP, the saml2sso process
    // share the same tcp/tls connection (on the service side, SP service and
    // the functional/real service are at the same service chain) with the
    // main client chain. And because of this connection sharing, if the
    // saml2sso process (interaction between user-agent/SP service/extenal IdP
    // service, see SAML2 SSO profile) is succeeded we can suppose that the
    // later real client/real service interaction is authorized.
    // User-Agent then get back <AuthnRequest/>, and send it response to IdP
    //
    // SP Service: a service based on http service on the service side, which is
    // specifically in charge of the funtionality of Service Provider of SAML2
    // SSO profile.
    // User-Agent: Since the SAML2 SSO profile is web-browser based, so here we
    // implement the code which is with the same functionality as browser's user agent.
    // -------------------------------------------
    //
	MCC_Status status;

	status = processStart2WAYF();

	if (status.getKind() != STATUS_OK) {
		logger.msg(ERROR, "Getting to the WAYF page failed!\nCheck the SLCS URL and if the SLCS CA-certificate is included (CACertificatePath option)");
		return status;
	}

	logger.msg(DEBUG, "Successfully redirected to the WAYF page!");

	status = processWAYF2IdPLogin();

	if (status.getKind() != STATUS_OK) {
		logger.msg(ERROR, "Getting from WAYF to the IdP page failed!");
		return status;
	}

	logger.msg(DEBUG, "Successfully redirected from the WAYF page to the IdP login!");

	status = processIdPLogin(username, password);

	if (status.getKind() != STATUS_OK) {
		std::string msg = "Logging in to the IdP failed: Wrong username/password?";
		logger.msg(ERROR, msg);
		return status;
	}

	status = processConsent();

	if (status.getKind() != STATUS_OK) {
		logger.msg(ERROR, "Getting the user consent for SSO failed!");
		return status;
	}

	logger.msg(DEBUG, "Successfully logged in to the IdP!");

	status = processIdP2Confusa();

	if (status.getKind() != STATUS_OK) {
		logger.msg(ERROR, "Directing back from the IdP to Confusa failed!");
		return status;
	}

	logger.msg(DEBUG, "Successfully redirected back from the IdP to Confusa!");

	return status;

  }

  MCC_Status SAML2SSOHTTPClient::parseDN(std::string* dn) {
	  std::string origin = "SAML2SSOHTTPClient::parseDN(std::string&)";

	  std::string about_loc = this->server_loc_.fullstr() + about_page + "?text=yes";

	  ClientHTTP confusa_dn_client(cfg_, URL(about_loc));
	  PayloadRaw confusa_dn_request;
	  PayloadRawInterface *confusa_dn_response = NULL;
	  HTTPClientInfo confusa_dn_info;

	  std::map<std::string, std::string> http_attributes;
	  http_attributes["Cookie"] = (*session_cookies_)["Confusa"];
	  MCC_Status stat = confusa_dn_client.process("GET", http_attributes, &confusa_dn_request, &confusa_dn_info, &confusa_dn_response);

	  std::string about_page = "";

	  if (confusa_dn_response) {
		  about_page = confusa_dn_response->Content();
		  delete confusa_dn_response;
	  } else {
		  return MCC_Status(PARSING_ERROR, origin, "Did not get a response from the about_you page to parse the DN!");
	  }

	  std::string body_string = ConfusaParserUtils::extract_body_information(about_page);

	  xmlDocPtr doc = ConfusaParserUtils::get_doc(body_string);
	  *dn = ConfusaParserUtils::evaluate_path(doc, "//div[@id='dn-section']");
	  ConfusaParserUtils::destroy_doc(doc);

	  logger.msg(INFO, "The retrieved dn is %s", *dn);
	  return stat;

  }

  MCC_Status SAML2SSOHTTPClient::storeCert(const std::string cert_path,
										   const std::string auth_token,
										   const std::string b64_dn) {

	  std::string origin = "SAML2SSOHTTPClient::storeCert(const std::string, const std::string, const std::string)";

	  // get the cert from Confusa
	  std::string download_loc = this->server_loc_.fullstr() + down_page;

	  // TODO prio=medium again, configurable params
	  std::string params = "?auth_key=" + auth_token + "&common_name=" + b64_dn;

	  std::string endpoint = download_loc + params;
	  logger.msg(INFO, "The location to which the GET is performed is %s", endpoint);

	  ClientHTTP confusa_get_client(cfg_, URL(endpoint));
	  PayloadRaw confusa_get_request;
	  PayloadRawInterface *confusa_get_response = NULL;
	  HTTPClientInfo confusa_get_info;

	  std::map<std::string, std::string> http_attributes;
	  http_attributes["Cookie"] = (*session_cookies_)["Confusa"];
	  MCC_Status stat = confusa_get_client.process("GET", http_attributes, &confusa_get_request, &confusa_get_info, &confusa_get_response);

	  std::string content = "";

	  if (confusa_get_response) {
		  content = confusa_get_response->Content();
		  delete confusa_get_response;
	  } else {
		  return MCC_Status(PARSING_ERROR, origin, "Could not get the certificate from Confusa!");
	  }


	  std::fstream fop(cert_path.c_str(), std::ios::out);
	  fop << content;
	  fop.close();

	  return stat;
  }

  // TODO: Why isn't approve_page a constant variable?
  MCC_Status SAML2SSOHTTPClient::approveCSR(const std::string approve_page) {

	  logger.msg(INFO, "Approving CSR on Confusa's approve page %s", approve_page);
	  ClientHTTP confusa_approve_client(cfg_, URL(approve_page));
	  PayloadRaw confusa_approve_request;
	  PayloadRawInterface *confusa_approve_response = NULL;
	  HTTPClientInfo confusa_approve_info;

	  std::map<std::string, std::string> http_attributes;
	  http_attributes["Cookie"] = (*session_cookies_)["Confusa"];

	  logger.msg(VERBOSE, "The cookie sent with approve is %s", http_attributes["Cookie"]);

	  MCC_Status stat = confusa_approve_client.process("GET", http_attributes, &confusa_approve_request, &confusa_approve_info, &confusa_approve_response);

	  if (confusa_approve_response) {
		  delete confusa_approve_response;
	  }

	  return stat;
  }


  MCC_Status SAML2SSOHTTPClient::pushCSR(const std::string b64_cert_req,
										 const std::string auth_token,
										 std::string *approve_page) {

	  std::string upload_loc = this->server_loc_.fullstr() + up_page;
	  logger.msg(INFO, "The server location is %s ", this->server_loc_.fullstr());

	  // TODO prio=medium the parameters are configurable in confusa, so is the auth_token length
	  std::string params = "?auth_key=" + auth_token + "&remote_csr=" + b64_cert_req;

	  std::string endpoint = upload_loc + params;

	  logger.msg(INFO, "The location to which the GET is performed is %s", endpoint);

	  ClientHTTP confusa_push_client(cfg_, URL(endpoint));
	  PayloadRaw confusa_push_request;
	  PayloadRawInterface *confusa_push_response = NULL;
	  HTTPClientInfo confusa_push_info;

	  std::map<std::string, std::string> http_attributes;
	  http_attributes["Cookie"] = (*session_cookies_)["Confusa"];
	  MCC_Status stat = confusa_push_client.process("GET", http_attributes, &confusa_push_request, &confusa_push_info, &confusa_push_response);

	  *approve_page = this->server_loc_.fullstr() + "/index.php?auth_token=" + auth_token;

	  if (confusa_push_response) {
		  delete confusa_push_response;
	  }

	  return stat;
  };

}; // namespace Arc
