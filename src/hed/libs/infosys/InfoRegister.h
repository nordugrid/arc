#ifndef __ARC_INFOSYS_REGISTER_H__
#define __ARC_INFOSYS_REGISTER_H__

#include <list>
#include <string>
#include <arc/ArcConfig.h>
#include <arc/XMLNode.h>
#include <arc/message/MCC.h>
#include <arc/message/Service.h>
#include <arc/Logger.h>
#include <arc/URL.h>

#define GLUE2_D42_NAMESPACE "http://schemas.ogf.org/glue/2008/05/spec_2.0_d42_r1"
#define REGISTRATION_NAMESPACE "http://www.nordugrid.org/schemas/registartion/2008/08"
#define ISIS_NAMESPACE "http://www.nordugrid.org/schemas/isis/2008/08"
#define REGISTRATION_CONFIG_NAMESPACE "http://www.nordugrid.org/schemas/InfoRegisterConfig/2008"

namespace Arc
{

/// Registration to ISIS interface
/** This class provides an interface for service to register
  itself in Information Indexing Service. */
class InfoRegister
{
    private:
        class Peer {
            public:  
                Arc::URL url;
                std::string key;
                std::string cert;
                std::string proxy;
                std::string cadir;
                Peer(const Arc::URL& url_,
                     const std::string& key_,
                     const std::string& cert_,
                     const std::string& proxy_,
                     const std::string& cadir_):
                url(url_), key(key_), cert(cert_), proxy(proxy_), cadir(cadir_)
                { };
        };
        std::list<Peer> peers_;
        Arc::NS ns_;
        Arc::MCCConfig mcc_cfg_;
        Arc::Logger logger_;
        Arc::Service *service_;
        long int reg_period_;
    public:
        InfoRegister(Arc::XMLNode &node, Arc::Service *service_);
        ~InfoRegister();
        long int getPeriod(void) { return reg_period_; };
        void registration(void);
};

/// Handling multiple registrations to ISISes
class InfoRegisters
{
    private:
        std::list<Arc::InfoRegister*> registers_;
    public:
        /// Constructor creates InfoRegister objects according to configuration
        /** Inside cfg elements isis:InfoRegister are found and for each
           corresponding InfoRegister object is created. Those objects 
           are destroyed in destructor of this class. */
        InfoRegisters(Arc::XMLNode &cfg, Arc::Service *service_);
        ~InfoRegisters(void);
};

}
#endif
