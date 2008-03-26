#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/URL.h>
#include <arc/message/PayloadSOAP.h>
#include "Register.h"

#include <unistd.h>

namespace Arc
{

Register::Register(const std::string &sid, long int period, Arc::Config &cfg):logger(Arc::Logger::rootLogger, "Register")
{
    ns["isis"] = "http://www.nordugrid.org/schemas/isis/2007/06";
    service_id = sid;
    reg_period = period;
    // XXX how to get plugin paths from cfg?
    mcc_cfg.AddPluginsPath("../../hed/mcc/tcp/.libs/");
    mcc_cfg.AddPluginsPath("../../hed/mcc/tls/.libs/");
    mcc_cfg.AddPluginsPath("../../hed/mcc/soap/.libs/");
    mcc_cfg.AddPluginsPath("../../hed/mcc/http/.libs/");
}

void Register::AddUrl(std::string &url)
{
    urls.push_back(url);
}

void Register::registration_forever(void)
{
    for (;;) {
        registration();
        sleep(reg_period);
    }
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
        logger.msg(Arc::DEBUG, "Start registration to %s ISIS", isis_name);
        Arc::PayloadSOAP request(ns);
        request.NewChild("Header").NewChild("RequesterID") = service_id;
        Arc::XMLNode re = request.NewChild("RegEntry");
        re.NewChild("ID") = service_id;
        Arc::XMLNode sa = re.NewChild("SrvAdv");
        // sa.NewChild("Type") = 
        // XXX read data from InfoCache
        // request.NewChild()
        Arc::PayloadSOAP *response;
        Arc::MCC_Status status = cli->process(&request, &response);
        if (!status) {
            if (response) {
                // XXX process response
                logger.msg(Arc::DEBUG, "Succesful registration to %s ISIS", isis_name); 
            } else {
                logger.msg(Arc::ERROR, "Error during registration to %s ISIS", isis_name);
            }
        } else {
            logger.msg(Arc::ERROR, "Error during registration to %s ISIS", isis_name);
        }
        
        delete cli;
    }
}

} // namespace
