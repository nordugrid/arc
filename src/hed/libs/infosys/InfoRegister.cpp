#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/URL.h>
#include <arc/message/PayloadSOAP.h>
#include "InfoRegister.h"
#ifdef WIN32
#define NOGDI
#include <objbase.h>
#endif

#include <unistd.h>

namespace Arc
{

InfoRegister::InfoRegister(const std::string &sid, long int period, Config &cfg):logger(Logger::rootLogger, "InfoRegister")
{
    ns["isis"] = "http://www.nordugrid.org/schemas/isis/2007/06";
    service_id = sid;
    reg_period = period;
    cfg["EPR"].New(service_epr);
    service_epr.Namespaces(ns);
    service_epr.Name("isis:EPR");
    // XXX how to get plugin paths from cfg?
    mcc_cfg.AddPluginsPath("../../hed/mcc/tcp/.libs/");
    mcc_cfg.AddPluginsPath("../../hed/mcc/tls/.libs/");
    mcc_cfg.AddPluginsPath("../../hed/mcc/soap/.libs/");
    mcc_cfg.AddPluginsPath("../../hed/mcc/http/.libs/");
}

void InfoRegister::AddUrl(const std::string &url)
{
    urls.push_back(url);
}

void InfoRegister::registration_forever(void)
{
    for (;;) {
        registration();
#ifndef WIN32
        sleep(reg_period);
#else
	Sleep(reg_period*1000);
#endif
    }
}

void InfoRegister::registration(void)
{
    std::list<std::string>::iterator it;

    for (it = urls.begin(); it != urls.end(); it++) {
        URL u(*it);
        const char *isis_name = (*it).c_str();
        bool tls;
        if (u.Protocol() == "http") {
            tls = false;
        } else if (u.Protocol() == "https") {
            tls = true;
        } else {
            logger.msg(WARNING, "unsupported protocol: %s", isis_name);
            continue;
        }
        cli = new ClientSOAP(mcc_cfg, u.Host(), u.Port(), tls, u.Path());
        logger.msg(DEBUG, "Registering to %s ISIS", isis_name);
        PayloadSOAP request(ns);
        XMLNode op = request.NewChild("isis:Register");
        op.NewChild("isis:Header").NewChild("isis:RequesterID") = service_id;
        XMLNode re = op.NewChild("isis:RegEntry");
        re.NewChild("isis:ID") = service_id;
        XMLNode sa = re.NewChild("isis:SrcAdv");
        XMLNode msa = re.NewChild("isis:MetaSrcAdv");
        sa.NewChild(service_epr);
        // TODO: Fill other required elements
        msa.NewChild("isis:Expiration")="3600";
        msa.NewChild("isis:GenTime")="";
        msa.NewChild("isis:Source")="";
        msa.NewChild("isis:Status")="";
        PayloadSOAP *response;
        MCC_Status status = cli->process(&request, &response);
        if ((!status) || (!response)) {
            logger.msg(ERROR, "Error during registration to %s ISIS", isis_name);
         } else {
            XMLNode fault = (*response)["Fault"];
            if(!fault)  {
                logger.msg(DEBUG, "Successful registration to %s ISIS", isis_name); 
            } else {
                logger.msg(DEBUG, "Failed to register to %s ISIS - %s", isis_name, std::string(fault["Description"])); 
            }
        }
        
        delete cli;
    }
}

} // namespace
