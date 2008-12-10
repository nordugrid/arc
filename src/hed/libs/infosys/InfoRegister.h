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

namespace Arc {

class InfoRegisterContainer;
class InfoRegistrar;

/// Registration to ISIS interface
/** This class represents service registering to Information Indexing Service.
  Ir does not perform registration itself. It only collects configuration information. */
class InfoRegister {
    friend class InfoRegisterContainer;
    friend class InfoRegistrar;
    private:
        // This class describes one ISIS endpoint
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
                url(url_), key(key_), cert(cert_),
                proxy(proxy_), cadir(cadir_)
                { };
                Peer(const Peer& peer):
                url(peer.url), key(peer.key), cert(peer.cert), 
                proxy(peer.proxy), cadir(peer.cadir)
                { };
        };
        // Registration period
        long int reg_period_;
        // List of ISISes associated with particular service
        std::list<Peer> peers_;
        // Associated service - it is used to fetch information document
        Arc::Service *service_;
        Arc::NS ns_;
//        Arc::Logger logger_;
    public:
        InfoRegister(Arc::XMLNode &node, Arc::Service *service_);
        ~InfoRegister();
        long int getPeriod(void) { return reg_period_; };
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


/// Registration process associated with particular ISIS
/** Currently this class is associated with particular InfoRegister.
  Later it will be associated with particular ISIS. */
class InfoRegistrar {
    friend class InfoRegisterContainer;
    private:
        InfoRegister* reg_;
        InfoRegistrar(InfoRegister* reg):reg_(reg) {};
        InfoRegistrar(const InfoRegistrar&) {};
    public:
        ~InfoRegistrar(void) {};
        long int getPeriod(void) { if(reg_) return reg_->reg_period_; return 0; };
        void registration(void);
};

/// Singleton class for storing configuration information
class InfoRegisterContainer {
    private:
        static InfoRegisterContainer instance_;
        std::list<InfoRegister*> regs_;
        std::list<InfoRegistrar*> regr_;
        InfoRegisterContainer(void) {};
        InfoRegisterContainer(const InfoRegisterContainer&) {};
    public:
        ~InfoRegisterContainer(void);
        static InfoRegisterContainer& Instance(void) { return instance_; };
        void Add(InfoRegister* reg);
        void Remove(InfoRegister* reg);
        //InfoRegister* Get(int num);
};

} // namespace Arc

#endif

