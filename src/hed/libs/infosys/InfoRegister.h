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

/// Registration to ISIS interface
/** This class represents service registering to Information Indexing Service.
  It does not perform registration itself. It only collects configuration information. */
class InfoRegister {
    friend class InfoRegisterContainer;
    private:
        // Registration period
        long int reg_period_;
        // Associated service - it is used to fetch information document
        Service *service_;
        NS ns_;
    public:
        InfoRegister(XMLNode &node, Service *service_);
        ~InfoRegister();
        operator bool(void) { return service_; };
        bool operator!(void) { return !service_; };
        long int getPeriod(void) const { return reg_period_; };
        Service* getService(void) { return service_; };
};

/// Handling multiple registrations to ISISes
class InfoRegisters {
    private:
        std::list<InfoRegister*> registers_;
    public:
        /// Constructor creates InfoRegister objects according to configuration
        /** Inside cfg elements isis:InfoRegister are found and for each
           corresponding InfoRegister object is created. Those objects 
           are destroyed in destructor of this class. */
        InfoRegisters(XMLNode &cfg, Service *service_);
        ~InfoRegisters(void);
};


/// Registration process associated with particular ISIS
class InfoRegistrar {
    friend class InfoRegisterContainer;
    private:
        InfoRegistrar(XMLNode cfg);
        // Configuration parameters
        URL url_;
        std::string key_;
        std::string cert_;
        std::string proxy_;
        std::string cadir_;
        long int period_;
        std::string id_;
        // Associated services
        std::list<InfoRegister*> reg_;
        // Mutex protecting reg_ list
        Glib::Mutex lock_;
        // Condition signaled when thread has to exit
        Glib::Cond cond_exit_;
        // Condition signaled when thread exited
        Glib::Cond cond_exited_;
    public:
        ~InfoRegistrar(void);
        operator bool(void) { return (bool)url_; };
        bool operator!(void) { return !url_; };
        void registration(void);
        bool addService(InfoRegister*);
        bool removeService(InfoRegister*);
        const std::string& id(void) { return id_; };
};

/// Singleton class for scanning configuration and storing
/// refernces to registration elements.
// TODO: Make it per chain
class InfoRegisterContainer {
    private:
        static InfoRegisterContainer instance_;
        //std::list<InfoRegister*> regs_;
        std::list<InfoRegistrar*> regr_;
        bool regr_done_;
        // Mutex protecting regr_ list
        Glib::Mutex lock_;
        InfoRegisterContainer(void);
        InfoRegisterContainer(const InfoRegisterContainer&) {};
    public:
        ~InfoRegisterContainer(void);
        static InfoRegisterContainer& Instance(void) { return instance_; };
        void addRegistrars(XMLNode doc);
        void addService(InfoRegister* reg,const std::list<std::string>& ids,XMLNode cfg = XMLNode());
        void removeService(InfoRegister* reg);
};

} // namespace Arc

#endif

