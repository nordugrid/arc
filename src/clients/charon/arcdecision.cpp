#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdexcept>

#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/URL.h>
#include <arc/message/MCC.h>
#include <arc/client/ClientInterface.h>

#include <iostream>
#include <fstream>
#include <cstdlib>

#include <arc/ArcLocation.h>
#include <arc/OptionParser.h>

int main(int argc, char* argv[]){

    setlocale(LC_ALL, "");

    Arc::LogStream logcerr(std::cerr);
    Arc::Logger::getRootLogger().addDestination(logcerr);
    Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

    Arc::ArcLocation::Init(argv[0]);

    Arc::OptionParser options(istring("service_url request_file"));

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
        std::cout << Arc::IString("%s version %s", "arcdecision", VERSION)
                  << std::endl;
        return 0;
    }

    try{
        if (params.size()!=2) {
            throw std::invalid_argument("Wrong number of arguments!");
        }

        //Create a SOAP client
        std::list<std::string>::iterator it = params.begin();
        Arc::URL url(*it++);
        std::string requestfilestr = *it++;
        if(!url)
            throw std::invalid_argument("Can't parse specified URL");
        Arc::MCCConfig cfg;
        if(config_path != "") cfg.GetOverlay(config_path);

        Arc::ClientSOAP client(cfg,url.Host(),url.Port(),url.Protocol() == "https",url.Path());

        // Read the request from file to string
        std::ifstream requestfile(requestfilestr.c_str());
        std::string request_str;
        if (!requestfile)
            throw std::invalid_argument("Could not open " + requestfilestr);
        getline(requestfile, request_str, '\0');
        Arc::XMLNode request_xml(request_str); 

        //Invoke the remote pdp service
        Arc::NS req_ns;
        req_ns["ra"] = "http://www.nordugrid.org/schemas/request-arc";
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
        
        return EXIT_SUCCESS;
    } catch (std::exception& e){
        // Notify the user about the failure
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
