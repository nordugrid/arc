#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <stdexcept>

#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/URL.h>
#include <arc/GUID.h>
#include <arc/message/MCC.h>
#include <arc/client/ClientInterface.h>
#include <arc/credential/Credential.h>

#include <arc/ArcLocation.h>
#include <arc/OptionParser.h>

#include "../../hed/libs/xmlsec/XmlSecUtils.h"
#include "../../hed/libs/xmlsec/XMLSecNode.h"

#define SAML_NAMESPACE "urn:oasis:names:tc:SAML:2.0:assertion"
#define SAMLP_NAMESPACE "urn:oasis:names:tc:SAML:2.0:protocol"

#define XENC_NAMESPACE   "http://www.w3.org/2001/04/xmlenc#"
#define DSIG_NAMESPACE   "http://www.w3.org/2000/09/xmldsig#"

int main(int argc, char* argv[]){

    setlocale(LC_ALL, "");

    Arc::Logger logger(Arc::Logger::getRootLogger(), "voms_assertion_init");
    Arc::LogStream logcerr(std::cerr);
    Arc::Logger::getRootLogger().addDestination(logcerr);
    Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

    Arc::ArcLocation::Init(argv[0]);

    Arc::OptionParser options(istring("service_url"));

    std::string config_path;
    options.AddOption('c', "config",
                      istring("path to config file"),
                      istring("path"), config_path);

    std::string debug;
    options.AddOption('d', "debug",
                      istring("FATAL, ERROR, WARNING, INFO, DEBUG or VERBOSE"),
                      istring("debuglevel"), debug);

    bool version = false;
    options.AddOption('v', "version", istring("print version information"),
                      version);

    std::list<std::string> params = options.Parse(argc, argv);

    if (!debug.empty())
        Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

    if (version) {
        std::cout << Arc::IString("%s version %s", "voms_assertion_init", VERSION)
                  << std::endl;
        return 0;
    }

    try{
        if (params.size()!=1) {
            throw std::invalid_argument("Wrong number of arguments!");
        }

        //Create a SOAP client
        std::list<std::string>::iterator it = params.begin();
        Arc::URL url(*it++);
        if(!url)
            throw std::invalid_argument("Can't parse specified URL");
        Arc::MCCConfig cfg;
        if(config_path != "") cfg.GetOverlay(config_path);

        Arc::ClientSOAP client(cfg,url);

        //Compose the soap which include <samlp:AttributeQuery/>

        //Use the credential which is configured in MCCConfig for the 
        //credential setup in <samlp:AttributeQuery> 
        std::string cert = (std::string)(cfg.overlay["Chain"]["Component"]["CertificatePath"]);
        std::string key = (std::string)(cfg.overlay["Chain"]["Component"]["KeyPath"]);
        std::string cafile = (std::string)(cfg.overlay["Chain"]["Component"]["CACertificatePath"]);
        std::string cadir = (std::string)(cfg.overlay["Chain"]["Component"]["CACertificatesDir"]);

        Arc::Credential cred(cert, key, cadir, cafile);
        std::string local_dn_str = cred.GetDN();
        std::string local_dn;
        size_t pos1 = std::string::npos;
        size_t pos2;
        do {
          std::string str;
          pos2 = local_dn_str.find_last_of("/", pos1);
          if(pos2 != std::string::npos && pos1 == std::string::npos) {
            str = local_dn_str.substr(pos2+1);
            local_dn.append(str);
            pos1 = pos2-1;
          }
          else if (pos2 != std::string::npos && pos1 != std::string::npos) {
            str = local_dn_str.substr(pos2+1, pos1-pos2);
            local_dn.append(str);
            pos1 = pos2-1;
          }
          if(pos2 != (std::string::npos+1)) local_dn.append(",");
        }while(pos2 != std::string::npos && pos2 != (std::string::npos+1));
       
        //Compose <samlp:AttributeQuery/> 
        Arc::NS ns;
        ns["saml"] = SAML_NAMESPACE;
        ns["samlp"] = SAMLP_NAMESPACE;

        Arc::XMLNode attr_query(ns, "samlp:AttributeQuery");
        std::string query_id = Arc::UUID();
        attr_query.NewAttribute("ID") = query_id;
        Arc::Time t;
        std::string current_time = t.str(Arc::UTCTime);
        attr_query.NewAttribute("IssueInstant") = current_time;
        attr_query.NewAttribute("Version") = std::string("2.0");

        //<saml:Issuer/>
        std::string issuer_name = local_dn;
        Arc::XMLNode issuer = attr_query.NewChild("saml:Issuer");
        issuer = issuer_name;
        issuer.NewAttribute("Format") = std::string("urn:oasis:names:tc:SAML:1.1:nameid-format:X509SubjectName");

        //<saml:Subject/>
        Arc::XMLNode subject = attr_query.NewChild("saml:Subject");
        Arc::XMLNode name_id = subject.NewChild("saml:NameID");
        name_id.NewAttribute("Format")=std::string("urn:oasis:names:tc:SAML:1.1:nameid-format:X509SubjectName");
        name_id = local_dn;
 
        //Add one or more <Attribute>s into AttributeQuery here, which means the 
        //Requestor would get these <Attribute>s from AA <saml:Attribute/>
        //TODO

        //Initialize lib xmlsec
        Arc::init_xmlsec();

        //Put the attribute query into soap message
        Arc::NS soap_ns;
        Arc::SOAPEnvelope envelope(soap_ns);
        envelope.NewChild(attr_query);
        Arc::PayloadSOAP request(envelope);

        //Send the soap message to the other end
        Arc::PayloadSOAP *response = NULL;
        Arc::MCC_Status status = client.process(&request,&response);
        if (!response) {
          logger.msg(Arc::ERROR, "Request failed: No response");
          throw std::runtime_error("Request failed: No response");
        }
        std::string str;
        response->GetXML(str);
        std::cout<<"Response: "<<str<<std::endl;
        if (!status) {
          logger.msg(Arc::ERROR, "Request failed: Error");
          throw std::runtime_error("Request failed: Error");
        }

        //Verify the assertion, TODO
        //The saml response from voms saml service is messed up and can not be verified currently,
        //still waiting for the support of voms people.
        
        return EXIT_SUCCESS;
    } catch (std::exception& e){
        Arc::final_xmlsec();
        // Notify the user about the failure
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
