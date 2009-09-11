#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdexcept>

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <glibmm/stringutils.h>
#include <glibmm/fileutils.h>
#include <glibmm.h>

#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/User.h>
#include <arc/Utils.h>
#include <arc/UserConfig.h>
#include <arc/message/MCC.h>
#include <arc/client/ClientInterface.h>
#include <arc/ArcLocation.h>
#include <arc/OptionParser.h>
#include <arc/GUID.h>
#include <arc/credential/Credential.h>

//#include <arc/xmlsec/XmlSecUtils.h>
//#include <arc/xmlsec/XMLSecNode.h>

#define SAML_NAMESPACE "urn:oasis:names:tc:SAML:2.0:assertion"
#define SAMLP_NAMESPACE "urn:oasis:names:tc:SAML:2.0:protocol"
#define XACML_SAMLP_NAMESPACE "urn:oasis:xacml:2.0:saml:protocol:schema:os"

#define XENC_NAMESPACE   "http://www.w3.org/2001/04/xmlenc#"
#define DSIG_NAMESPACE   "http://www.w3.org/2000/09/xmldsig#"

int main(int argc, char* argv[]){

    setlocale(LC_ALL, "");

    Arc::LogStream logcerr(std::cerr);
    Arc::Logger::getRootLogger().addDestination(logcerr);
    Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

    Arc::ArcLocation::Init(argv[0]);

    Arc::OptionParser options(istring("service_url request_file"));

    std::string service_url;
    options.AddOption('s', "service",
                      istring("url of the policy decision service"),
                      istring("string"), service_url);

    std::string request_path;
    options.AddOption('r', "request",
                      istring("path to request file"),
                      istring("path"), request_path);

    bool use_saml = false;
    options.AddOption('p', "saml", istring("use SAML 2.0 profile of XACML v2.0 to contact the service"), use_saml);

    std::string config_path;
    options.AddOption('c', "config",
                      istring("path to config file"),
                      istring("path"), config_path);

    int timeout = -1;
    options.AddOption('t', "timeout", istring("timeout in seconds (default " + Arc::tostring(Arc::UserConfig::DEFAULT_TIMEOUT) + ")"),
                      istring("seconds"), timeout);
                      
    std::string debug;
    options.AddOption('d', "debug",
                      istring("FATAL, ERROR, WARNING, INFO, DEBUG or VERBOSE"),
                      istring("debuglevel"), debug);

    bool version = false;
    options.AddOption('v', "version", istring("print version information"),
                      version);

    std::list<std::string> params = options.Parse(argc, argv);

    Arc::UserConfig usercfg(config_path, true);
    if (!usercfg) {
      std::cerr<<"Failed configuration initialization"<<std::endl;
      return EXIT_FAILURE;
    }
  
    if (timeout > 0) {
      usercfg.SetTimeOut((unsigned int)timeout);
    }
  
    if (!debug.empty())
      Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));
    else if (debug.empty() && usercfg.ConfTree()["Debug"]) {
      debug = (std::string)usercfg.ConfTree()["Debug"];
      Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));
    }

    if (version) {
        std::cout << Arc::IString("%s version %s", "arcdecision", VERSION)
                  << std::endl;
        return 0;
    }

    try{
      std::string key_path = (std::string)usercfg.ConfTree()["KeyPath"];
      std::string cert_path = (std::string)usercfg.ConfTree()["CertificatePath"];
      std::string proxy_path = (std::string)usercfg.ConfTree()["ProxyPath"];
      std::string ca_dir = (std::string)usercfg.ConfTree()["CACertificatesDir"];
      std::string ca_file = (std::string)usercfg.ConfTree()["CACertificatePath"];
      //Create a SOAP client
      Arc::URL url(service_url);
      if(!url) throw std::invalid_argument("Can't parse specified URL");
      Arc::MCCConfig cfg;
      if(!proxy_path.empty()) cfg.AddProxy(proxy_path);
      if(!cert_path.empty()) cfg.AddCertificate(cert_path);
      if(!key_path.empty()) cfg.AddPrivateKey(key_path);
      if(!ca_dir.empty()) cfg.AddCADir(ca_dir);
      if(!ca_file.empty()) cfg.AddCAFile(ca_file);

      Arc::ClientSOAP client(cfg,url,Arc::stringtoi(usercfg.ConfTree()["TimeOut"]));

      // Read the request from file to string
      std::ifstream requestfile(request_path.c_str());
      std::string request_str;
      if (!requestfile)
        throw std::invalid_argument("Could not open " + request_path);
      getline(requestfile, request_str, '\0');
      Arc::XMLNode request_xml(request_str); 

      if(use_saml) {
        Arc::NS ns;
        ns["saml"] = SAML_NAMESPACE;
        ns["samlp"] = SAMLP_NAMESPACE;
        ns["xacml-samlp"] = XACML_SAMLP_NAMESPACE; 
        Arc::XMLNode authz_query(ns, "xacml-samlp:XACMLAuthzDecisionQuery");
        std::string query_id = Arc::UUID();
        authz_query.NewAttribute("ID") = query_id;
        Arc::Time t;
        std::string current_time = t.str(Arc::UTCTime);
        authz_query.NewAttribute("IssueInstant") = current_time;
        authz_query.NewAttribute("Version") = std::string("2.0");

        Arc::Credential cred(cert_path.empty() ? proxy_path : cert_path, 
           cert_path.empty() ? proxy_path : key_path, ca_dir, ca_file);
        std::string local_dn_str = cred.GetDN();
        std::string local_dn = Arc::convert_to_rdn(local_dn_str);
        std::string issuer_name = local_dn;
        authz_query.NewChild("saml:Issuer") = issuer_name;

        authz_query.NewAttribute("InputContextOnly") = std::string("false");
        authz_query.NewAttribute("ReturnContext") = std::string("true");

        authz_query.NewChild(request_xml);

        Arc::NS req_ns;
        Arc::SOAPEnvelope req_env(req_ns);
        req_env.NewChild(authz_query);
        Arc::PayloadSOAP req(req_env);

        Arc::PayloadSOAP *resp = NULL;
        Arc::MCC_Status status = client.process(&req,&resp);
        if(!status) {
          std::cerr<<"Policy Decision Service invokation failed"<<std::endl;
          throw std::runtime_error("Policy Decision Service invokation failed");
        }
        if(resp == NULL) {
          std::cerr<<"There was no SOAP response"<<std::endl;
          throw std::runtime_error("There was no SOAP response");
        }
        std::string str;     
        resp->GetXML(str);
        std::cout<<"Response: "<<str<<std::endl;

        std::string authz_res = (std::string)((*resp)["samlp:Response"]["saml:Assertion"]["xacml-saml:XACMLAuthzDecisionStatement"]["xacml-context:Response"]["xacml-context:Result"]["xacml-context:Decision"]);

        if(resp) delete resp;

        if(authz_res == "Permit") { std::cout<<"Authorized from remote pdp service"<<std::endl; }
        else { std::cout<<"Unauthorized from remote pdp service"<<std::endl; }

      } else {
        //Invoke the remote pdp service
        Arc::NS req_ns;
        req_ns["pdp"] = "http://www.nordugrid.org/schemas/pdp";
        Arc::PayloadSOAP req(req_ns);
        Arc::XMLNode reqnode = req.NewChild("pdp:GetPolicyDecisionRequest");
        reqnode.NewChild(request_xml);

        std::string str;
        req.GetDoc(str);
        std::cout<<"SOAP message:"<<str<<std::endl;

        Arc::PayloadSOAP* resp = NULL;
        Arc::MCC_Status status = client.process(&req,&resp);
        if(!status) {
          std::cerr<<"Policy Decision Service invokation failed"<<std::endl;
          throw std::runtime_error("Policy Decision Service invokation failed");
        }

        if(resp == NULL) {
          std::cerr<<"There was no SOAP response"<<std::endl;
          throw std::runtime_error("There was no SOAP response");
        }

        resp->GetXML(str);
        std::cout<<"Response: "<<str<<std::endl;

        std::string authz_res = (std::string)((*resp)["pdp:GetPolicyDecisionResponse"]["response:Response"]["response:AuthZResult"]);

        if(resp) delete resp;

        if(authz_res == "PERMIT") { std::cout<<"Authorized from remote pdp service"<<std::endl; }
        else { std::cout<<"Unauthorized from remote pdp service"<<std::endl; }
      }      
      return EXIT_SUCCESS;
    } catch (std::exception& e){
      // Notify the user about the failure
      std::cerr << "ERROR: " << e.what() << std::endl;
      return EXIT_FAILURE;
    }
}
