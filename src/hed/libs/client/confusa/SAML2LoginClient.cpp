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
                                     std::list<std::string> idp_stack) {
    server_loc_ = url;
    cfg_ = cfg;

    idp_stack_ = idp_stack;
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
  SAML2SSOHTTPClient::SAML2SSOHTTPClient(const MCCConfig cfg, const URL url, std::list<std::string> idp_stack) : SAML2LoginClient(cfg,url, idp_stack) {
	  logger.msg(DEBUG, "Called SAML2SSOHTTPClient constructor");

	  std::list<std::string> idp_stack2;

	  // need the idp_name Base64-encoded for the Web-SSO RelayState
	  for (std::list<std::string>::iterator it = idp_stack.begin(); it != idp_stack.end();) {
		  std::string idp_entry = (*it);
		  it = idp_stack.erase(it);
		  idp_entry = ConfusaParserUtils::urlencode(idp_entry);
		  idp_stack2.push_back(idp_entry);;
	  }

	  idp_stack_ = idp_stack2;

  }

  SAML2SSOHTTPClient::~SAML2SSOHTTPClient() {
  }

  MCC_Status SAML2SSOHTTPClient::processStart2Login() {
	  MCC_Status status;
	  const std::string origin = "SAML2SSOHTTPClient::processStart2Login()";
	  std::string cookie = "";

	  // here the redirect back to Confusa will take place
	  std::string relaystate = ConfusaParserUtils::urlencode((*sso_pages_)["ConfusaStart"]);
	  std::string idpentityid = idp_stack_.front();

	  // for the first redirect, we assume that there is only one idp in the stack
	  std::multimap<std::string, std::string> http_attributes;
	  // start with confusa cookie
	  http_attributes.insert(std::pair<std::string, std::string>("Cookie",(*session_cookies_)["Confusa"]));

	  logger.msg(DEBUG, "Relaystate %s", relaystate);

	  std::string initSSO_url = (*sso_pages_)["SimpleSAML"] + "/" + initSSO_loc + "?RelayState=" + relaystate + "&idpentityid=" + idpentityid;

	  logger.msg(DEBUG, "Performing SSO with %s ", initSSO_url);
	  std::string actual_ip_login = ConfusaParserUtils::handle_redirect_step(cfg_,initSSO_url, &cookie, &http_attributes);

	  if (actual_ip_login.empty()) {
		  return MCC_Status(PARSING_ERROR, origin, "Could not get the SAML2 redirect to initSSO at " + initSSO_url);
	  }

	  // we are now either on the login site or at a nested wayf
	  // if we are at a wayf site, perform the respective SAML redirects until the final login form has been reached
	  // NOTE: this should work if the WAYF is simplesamlphp based
	  for (std::list<std::string>::iterator it = ++(idp_stack_.begin()); it != idp_stack_.end(); it++) {
		  // set the new cookie
		  if (!cookie.empty()) {
			  http_attributes.insert(std::pair<std::string,std::string>("Cookie",cookie));
		  }

		  std::string internal_request_site = ConfusaParserUtils::handle_redirect_step(cfg_,actual_ip_login, &cookie, &http_attributes);

		  if (internal_request_site.empty()) {
			  return MCC_Status(PARSING_ERROR, origin, "Could not redirect SAML2 request on WAYF site to internal request representation at " + actual_ip_login);
		  }

		  // the internal request site is already the WAYF's SAML2 SP
		  URL internal_request_url(internal_request_site);
		  std::string simplesamlbase = internal_request_url.Protocol() + "://" + internal_request_url.Host() + "/" + internal_request_url.FullPath();
		  std::string::size_type param_pos = internal_request_site.find('?');

		  if (param_pos == std::string::npos) {
			  return MCC_Status(PARSING_ERROR, origin, "Parameters reflecting relaystate and AuthId could not be found in the initSSO URL!");
		  }

		  std::string relayparams = internal_request_site.substr(param_pos, internal_request_site.size()-param_pos);
		  initSSO_url = simplesamlbase + relayparams + "&idpentityid=" + *it;

		  logger.msg(DEBUG, "Performing SSO with %s ", initSSO_url);
		  actual_ip_login = ConfusaParserUtils::handle_redirect_step(cfg_,initSSO_url, &cookie, &http_attributes);

		  if (actual_ip_login.empty()) {
			  return MCC_Status(PARSING_ERROR, origin, "Could not get the nested SAML2 redirect from WAYF initSSO to the IdP");
		  }
	  }

	  (*sso_pages_)["IdP"] = actual_ip_login;
	  logger.msg(DEBUG, "The idp login is %s", actual_ip_login);
	  return MCC_Status(STATUS_OK);

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

	status = findSimpleSAMLInstallation();

	if (status.getKind() != STATUS_OK) {
		logger.msg(ERROR, "Retrieving the remote SimpleSAMLphp installation failed!");
		return status;
	}

	status = processStart2Login();

	if (status.getKind() != STATUS_OK) {
		logger.msg(ERROR, "Getting from Confusa to the IdP page failed!");
		return status;
	}

	logger.msg(DEBUG, "Successfully redirected from Confusa to the IdP login!");

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

	  std::multimap<std::string, std::string> http_attributes;
	  http_attributes.insert(std::pair<std::string,std::string>("Cookie",(*session_cookies_)["Confusa"]));
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

	  std::multimap<std::string, std::string> http_attributes;
	  http_attributes.insert(std::pair<std::string,std::string>("Cookie",(*session_cookies_)["Confusa"]));
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

	  std::multimap<std::string, std::string> http_attributes;
	  http_attributes.insert(std::pair<std::string,std::string>("Cookie",(*session_cookies_)["Confusa"]));

	  logger.msg(VERBOSE, "The cookie sent with approve is %s", http_attributes.find("Cookie")->second);

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

	  std::multimap<std::string, std::string> http_attributes;
	  http_attributes.insert(std::pair<std::string,std::string>("Cookie",(*session_cookies_)["Confusa"]));
	  MCC_Status stat = confusa_push_client.process("GET", http_attributes, &confusa_push_request, &confusa_push_info, &confusa_push_response);

	  *approve_page = this->server_loc_.fullstr() + "/index.php?auth_token=" + auth_token;

	  if (confusa_push_response) {
		  delete confusa_push_response;
	  }

	  return stat;
  };

  MCC_Status SAML2LoginClient::findSimpleSAMLInstallation() {
	  const std::string origin = "SAML2LoginClient::findSimpleSAMLLocation()";
	  std::string cookie;

	  if((*sso_pages_)["ConfusaStart"].empty()) {
		  return MCC_Status(GENERIC_ERROR, origin, "Confusa start url is undefined!");
	  }

	  std::string simplesaml_location = ConfusaParserUtils::handle_redirect_step(cfg_, (*sso_pages_)["ConfusaStart"], &cookie);

	  if (simplesaml_location.empty()) {
		  return MCC_Status(PARSING_ERROR, origin, "Could not determine the location of the SimpleSAML installation!");
	  }

	  // let it really point to the simpleSAML location and not to the initSSO script
	  std::string::size_type saml2_pos = simplesaml_location.find(initSSO_loc);
	  simplesaml_location.erase(saml2_pos, (simplesaml_location.size()-saml2_pos));

	  (*sso_pages_)["SimpleSAML"] = simplesaml_location;
	  (*session_cookies_)["Confusa"] = cookie;
	  return MCC_Status(STATUS_OK);
  }


}; // namespace Arc
