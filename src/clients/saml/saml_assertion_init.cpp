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
#include <arc/StringConv.h>
#include <arc/message/MCC.h>
#include <arc/client/ClientInterface.h>
#include <arc/credential/Credential.h>

#include <arc/ArcLocation.h>
#include <arc/OptionParser.h>

#include <arc/xmlsec/XmlSecUtils.h>
#include <arc/xmlsec/XMLSecNode.h>

#define SAML_NAMESPACE "urn:oasis:names:tc:SAML:2.0:assertion"
#define SAMLP_NAMESPACE "urn:oasis:names:tc:SAML:2.0:protocol"

#define XENC_NAMESPACE   "http://www.w3.org/2001/04/xmlenc#"
#define DSIG_NAMESPACE   "http://www.w3.org/2000/09/xmldsig#"

int main(int argc, char* argv[]){

    setlocale(LC_ALL, "");

    Arc::Logger logger(Arc::Logger::getRootLogger(), "voms_assertion_init");
    Arc::LogStream logcerr(std::cerr);
    logcerr.setFormat(Arc::ShortFormat);
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
                      istring("FATAL, ERROR, WARNING, INFO, VERBOSE or DEBUG"),
                      istring("debuglevel"), debug);

    bool version = false;
    options.AddOption('v', "version", istring("print version information"),
                      version);

    std::list<std::string> params = options.Parse(argc, argv);

    if (version) {
        std::cout << Arc::IString("%s version %s", "saml_assertion_init", VERSION)
                  << std::endl;
        return 0;
    }

    if (!debug.empty())
        Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

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
      std::string local_dn = Arc::convert_to_rdn(local_dn_str);

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
      issuer.NewAttribute("Format") = std::string("urn:oasis:names:tc:SAML:1.1:nameid-format:x509SubjectName");

      //<saml:Subject/>
      Arc::XMLNode subject = attr_query.NewChild("saml:Subject");
      Arc::XMLNode name_id = subject.NewChild("saml:NameID");
      name_id.NewAttribute("Format")=std::string("urn:oasis:names:tc:SAML:1.1:nameid-format:x509SubjectName");
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
        logger.msg(Arc::ERROR, "SOAP Request failed: No response");
        throw std::runtime_error("SOAP Request failed: No response");
      }
      if (!status) {
        logger.msg(Arc::ERROR, "SOAP Request failed: Error");
        throw std::runtime_error("SOAP Request failed: Error");
      }

      Arc::XMLNode saml_response = (*response).Body().Child(0);
      if(!saml_response) {
        logger.msg(Arc::ERROR, "No <saml:Response/> in SOAP response");
        throw std::runtime_error("No <saml:Response/> in SOAP response");
      }

      Arc::XMLNode saml_assertion = saml_response["Assertion"];
      if(!saml_assertion) {
        logger.msg(Arc::ERROR, "No <saml:Assertion/> in SAML response");
        throw std::runtime_error("No <saml:Assertion/> in SAML response");
      }

      std::string str;
      saml_assertion.GetXML(str);
      std::cout<<"SAML assertion: "<<str<<std::endl;

      //Verify the assertion
      std::string assertion_idname = "ID";
      Arc::XMLSecNode assertion_secnode(saml_assertion);
      if(assertion_secnode.VerifyNode(assertion_idname, cafile, cadir,false)) {
        logger.msg(Arc::INFO, "Succeeded to verify the signature under <saml:Assertion/>");
      }
      else {
        logger.msg(Arc::ERROR, "Failed to verify the signature under <saml:Assertion/>");
        throw std::runtime_error("Failed to verify the signature under <saml:Assertion/>");
      }
    } catch (std::exception& e){
      Arc::final_xmlsec();
      // Notify the user about the failure
      std::cerr << "ERROR: " << e.what() << std::endl;
      return EXIT_FAILURE;
    }

    Arc::final_xmlsec();
    return EXIT_SUCCESS;
}
