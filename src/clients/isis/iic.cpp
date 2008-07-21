#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdexcept>

#include <arc/URL.h>
#include <arc/infosys/InfoRegister.h>
#include <arc/message/PayloadSOAP.h>
#include "iic.h"

namespace Arc 
{

IIClient::IIClient(std::string &url_str):logger(Arc::Logger::rootLogger, "IIClient")
{
    ns["isis"] = ISIS_NAMESPACE;
    ns["glue2"] = GLUE2_D42_NAMESPACE;

    Arc::URL url(url_str);
    bool tls;
    if(url.Protocol() == "http") { 
        tls=false; 
    } else if(url.Protocol() == "https") {
        tls=true; 
    }
    else {
        throw(std::invalid_argument(std::string("URL contains unsupported protocol")));
    }
    cli = new Arc::ClientSOAP(cfg, url.Host(), url.Port(), tls, url.Path());
}

IIClient::~IIClient(void)
{
    delete cli;
}

Arc::MCC_Status IIClient::Query(Arc::XMLNode &req, Arc::XMLNode *resp)
{
    Arc::PayloadSOAP request(ns);
    request.NewChild(req);
    Arc::PayloadSOAP* response;

    Arc::MCC_Status status = cli->process(&request, &response);
    if(!status) {
        std::cerr << "Request failed" << std::endl;
        if(response) {
            std::string str;
            response->GetXML(str);
            std::cout << str << std::endl;
            delete response;
        }
        return status;  
    }
    if(!response) {
        std::cerr << "No response" << std::endl;
        return Arc::MCC_Status();
    }

    *resp = response->Child(0);
    
    return status;
}

} // namespace Arc 
