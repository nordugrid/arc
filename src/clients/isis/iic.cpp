#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdexcept>

#include <arc/URL.h>
#include <arc/message/PayloadSOAP.h>
#include "iic.h"

namespace Arc 
{

IIClient::IIClient(std::string &url_str):logger(Arc::Logger::rootLogger, "IIClient")
{
    ns["isis"] = "http://www.nordugrid.org/schemas/isis/2007/06";
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
    cfg.AddPluginsPath("../../hed/mcc/tcp/.libs/");
    cfg.AddPluginsPath("../../hed/mcc/tls/.libs/");
    cfg.AddPluginsPath("../../hed/mcc/http/.libs/");
    cfg.AddPluginsPath("../../hed/mcc/soap/.libs/");
    cli = new Arc::ClientSOAP(cfg, url.Host(), url.Port(), tls, url.Path());
}

IIClient::~IIClient(void)
{
    delete cli;
}

Arc::MCC_Status IIClient::Register(Arc::XMLNode &req, Arc::XMLNode *resp)
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
    };
    if(!response) {
        std::cerr << "No response" << std::endl;
        return Arc::MCC_Status();
    };
#if 0
    std::string s;
    response->GetXML(s);
    std::cout << "Response:\n" << s << std::endl;
#endif
    if (resp != NULL) {
        *resp = response->Child(0);
    }
    return status;
}

Arc::MCC_Status IIClient::RemoveRegistrations(Arc::XMLNode &req, 
                                              Arc::XMLNode *resp)
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
    };
    if(!response) {
        std::cerr << "No response" << std::endl;
        return Arc::MCC_Status();
    };
#if 0
    std::string s;
    response->GetXML(s);
    std::cout << "Response:\n" << s << std::endl;
#endif
    if (resp != NULL) {
        *resp = response->Child(0);
    }
    return status;
}

Arc::MCC_Status IIClient::GetRegistrationStatuses(Arc::XMLNode &req, Arc::XMLNode *resp)
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
    };
    if(!response) {
        std::cerr << "No response" << std::endl;
        return Arc::MCC_Status();
    };
    
    if (resp != NULL) {
        *resp = response->Child(0);
    }
    
    return status;
}

Arc::MCC_Status IIClient::GetIISList(Arc::XMLNode &req, Arc::XMLNode *resp) 
{
    return MCC_Status();
}     

}; // namespace Arc 
