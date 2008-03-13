#ifndef __ARC_INFOSYS_REGISTER_H__
#define __ARC_INFOSYS_REGISTER_H__

#include <list>
#include <string>
#include <arc/ArcConfig.h>
#include <arc/XMLNode.h>
#include <arc/misc/ClientInterface.h>
#include <arc/Logger.h>

namespace Arc {

class Register
{
    private:
        std::list<std::string> urls;
        Arc::NS ns;
        Arc::MCCConfig mcc_cfg;
        Arc::ClientSOAP *cli;
        Arc::Logger logger;
        std::string service_id;

    public:
        Register(std::string &service_id, Arc::Config &cfg);
        ~Register(void);
        void AddUrl(std::string &url);
        void registration(void);

}; // class Register

} // namespace

#endif
