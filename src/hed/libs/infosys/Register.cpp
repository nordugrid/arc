#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/URL.h>
#include <arc/message/PayloadSOAP.h>
#include "Register.h"

namespace Arc
{

Register::Register(std::string &service_id, Arc::Config&):logger(Arc::Logger::rootLogger, "Register")
{
    ns["isis"] = "http://www.nordugrid.org/schemas/isis/2007/06";
    service_id = service_id;
    // XXX how to get plugin paths from cfg?
    mcc_cfg.AddPluginsPath("../../hed/mcc/tcp/.libs/");
}

void Register::AddUrl(std::string &url)
{
    urls.push_back(url);
}

void Register::registration(void)
{
    std::list<std::string>::iterator it;

    for (it = urls.begin(); it != urls.end(); it++) {
        Arc::URL u(*it);
        const char *isis_name = (*it).c_str();
        bool tls;
        if (u.Protocol() == "http") {
            tls = true;
        } else if (u.Protocol() == "https") {
            tls = false;
        } else {
            logger.msg(Arc::WARNING, "invalid protocol: %s", isis_name);
            continue;
        }
        cli = new Arc::ClientSOAP(mcc_cfg, u.Host(), u.Port(), tls, u.Path());
        logger.msg(Arc::DEBUG, "Start registartion to %s ISIS", isis_name);
        Arc::PayloadSOAP request(ns);
        // XXX read data from InfoCache
        // request.NewChild()
        Arc::PayloadSOAP *response;
        Arc::MCC_Status status = cli->process(&request, &response);
        if (!status) {
            if (response) {
                // XXX process response
                logger.msg(Arc::DEBUG, "Succesful registartion to %s ISIS", isis_name); 
            } else {
                logger.msg(Arc::ERROR, "Error during registarton to %s ISIS", isis_name);
            }
        } else {
            logger.msg(Arc::ERROR, "Error during registarton to %s ISIS", isis_name);
        }
        
        delete cli;
    }
}

} // namespace
