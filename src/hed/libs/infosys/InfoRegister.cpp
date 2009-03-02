#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>

#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/client/ClientInterface.h>
#include <arc/StringConv.h>
#include "InfoRegister.h"
#ifdef WIN32
#include <arc/win32.h>
#endif

static Arc::Logger logger_(Arc::Logger::rootLogger, "InfoRegister");

namespace Arc {

static void reg_thread(void *data) {
    InfoRegistrar *self = (InfoRegistrar *)data;
    self->registration();
}

// -------------------------------------------------------------------

InfoRegister::InfoRegister(XMLNode &cfg, Service *service):reg_period_(0),service_(service) {
    ns_["isis"] = ISIS_NAMESPACE;
    ns_["glue2"] = GLUE2_D42_NAMESPACE;
    ns_["register"] = REGISTRATION_NAMESPACE;

    // parse config
    std::string s_reg_period = (std::string)cfg["Period"];
    if (!s_reg_period.empty()) { 
        reg_period_ = strtol(s_reg_period.c_str(), NULL, 10);
    } else {
        reg_period_ = -1;
    }
    // Add service to registration list. Optionally only for 
    // registration through specific registrants.
    std::list<std::string> ids;
    for(XMLNode r = cfg["Registrant"];(bool)r;++r) {
      std::string id = r;
      if(!id.empty()) ids.push_back(id);
    };
    InfoRegisterContainer::Instance().addService(this,ids,cfg);
    std::cout << "InfoRegister - ready" << std::endl;
}

InfoRegister::~InfoRegister(void) {
    // This element is supposed to be destroyed with service.
    // Hence service should not be restered anymore.
    // TODO: initiate un-register of service
    InfoRegisterContainer::Instance().removeService(this);
}
    
// -------------------------------------------------------------------

InfoRegisterContainer InfoRegisterContainer::instance_;

InfoRegisterContainer::InfoRegisterContainer(void):regr_done_(false) {
}

InfoRegisterContainer::~InfoRegisterContainer(void) {
    Glib::Mutex::Lock lock(lock_);
    for(std::list<InfoRegistrar*>::iterator r = regr_.begin();
                              r != regr_.end();++r) {
        delete (*r);
    };
}

void InfoRegisterContainer::addRegistrars(XMLNode doc) {
    Glib::Mutex::Lock lock(lock_);
    for(XMLNode node = doc["InfoRegistrar"];(bool)node;++node) {
        InfoRegistrar* r = new InfoRegistrar(node);
        if(!r) continue;
        if(!(*r)) { delete r; continue; };
        regr_.push_back(r);
    };
    // Start registration threads
    for(std::list<InfoRegistrar*>::iterator r = regr_.begin();
                              r != regr_.end();++r) {
       CreateThreadFunction(&reg_thread,*r);
    };
    regr_done_=true;
}

void InfoRegisterContainer::addService(InfoRegister* reg,const std::list<std::string>& ids,XMLNode cfg) {
    // Add element to list
    //regs_.push_back(reg);
    // If not yet done create registrars
    if(!regr_done_) {
        if((bool)cfg) {
            addRegistrars(cfg.GetRoot());
        } else {
            // Bad situation
        };
    };

    // Add to registrars
    Glib::Mutex::Lock lock(lock_);
    for(std::list<InfoRegistrar*>::iterator r = regr_.begin();
                              r != regr_.end();++r) {
        if(ids.size() > 0) {
            for(std::list<std::string>::const_iterator i = ids.begin();
                                            i != ids.end();++i) {
                if((*i) == (*r)->id()) (*r)->addService(reg);
            };
        } else {
            (*r)->addService(reg);
        };
    };
}

void InfoRegisterContainer::removeService(InfoRegister* reg) {
    // If this method is called that means service is most probably
    // being deleted.
    Glib::Mutex::Lock lock(lock_);
    for(std::list<InfoRegistrar*>::iterator r = regr_.begin();
                              r != regr_.end();++r) {
            (*r)->removeService(reg);
    };
}

// -------------------------------------------------------------------

InfoRegistrar::InfoRegistrar(XMLNode cfg) {
    id_=(std::string)cfg.Attribute("id");
    if(!stringto((std::string)cfg["Period"],period_)) {
      period_=0; // Wrong number
      logger_.msg(ERROR, "Can't recognize period: %s",(std::string)cfg["Period"]);
    };
    key_   = (std::string)cfg["KeyPath"];
    cert_  = (std::string)cfg["CertificatePath"];
    proxy_ = (std::string)cfg["ProxyPath"];
    cadir_ = (std::string)cfg["CACertificatesDir"];
    url_ = URL((std::string)cfg["URL"]);
    if(!url_) {
      logger_.msg(ERROR, "Can't recognize URL: %s",(std::string)cfg["URL"]);
    };
}
  
bool InfoRegistrar::addService(InfoRegister* reg) {
    Glib::Mutex::Lock lock(lock_);
    for(std::list<InfoRegister*>::iterator r = reg_.begin();
                                           r!=reg_.end();++r) {
        if(reg == *r) return false;
    };
    reg_.push_back(reg);
    return true;
}

bool InfoRegistrar::removeService(InfoRegister* reg) {
    Glib::Mutex::Lock lock(lock_);
    for(std::list<InfoRegister*>::iterator r = reg_.begin();
                                           r!=reg_.end();++r) {
        if(reg == *r) {
            reg_.erase(r);
            return true;
        };
    };
    return false;
}

InfoRegistrar::~InfoRegistrar(void) {
    // Registering thread must be stopped before destructor succeeds
    Glib::Mutex::Lock lock(lock_);
    cond_exit_.signal();
    cond_exited_.wait(lock_);
}

class CondExit {
    private:
        Glib::Cond& cond_;
    public:
        CondExit(Glib::Cond& cond):cond_(cond) { };
        ~CondExit(void) { cond_.signal(); };
};

void InfoRegistrar::registration(void) {
    Glib::Mutex::Lock lock(lock_);
    CondExit cond(cond_exited_);
    std::string isis_name = url_.fullstr();
    while(true) {
        logger_.msg(DEBUG, "Registration starts: %s",isis_name);
        if(!url_) {
            logger_.msg(WARNING, "Registrant has no proper URL specified");
            return;
        }
        NS reg_ns;
        reg_ns["glue2"] = GLUE2_D42_NAMESPACE;
        reg_ns["isis"] = ISIS_NAMESPACE;

        // Registration algorithm is stupid and straightforward.
        // This part has to be redone to fit P2P network od ISISes

        for(std::list<InfoRegister*>::iterator r = reg_.begin();
                                               r!=reg_.end();++r) {
            XMLNode reg_doc(reg_ns, "isis:Advertisements");
            XMLNode services_doc(reg_ns, "Services");
            if(!((*r)->getService())) continue;
            (*r)->getService()->RegistrationCollector(services_doc);
            // Look for service registration information
            // TODO: use more efficient way to find <Service> entries than XPath. 
            std::list<XMLNode> services = services_doc.XPathLookup("//*[normalize-space(@BaseType)='Service']", reg_ns);
            if (services.size() == 0) {
                logger_.msg(WARNING, "Service provided no registration entries");
                continue;
            }
            // create advertisement
            std::list<XMLNode>::iterator sit;
            for (sit = services.begin(); sit != services.end(); sit++) {
                // filter may come here
                XMLNode ad = reg_doc.NewChild("isis:Advertisement");
                ad.NewChild((*sit));
                // XXX metadata
            }

            // prepare for sending to ISIS
            logger_.msg(DEBUG, "Registering to %s ISIS", isis_name);
            PayloadSOAP request(reg_ns);
            XMLNode op = request.NewChild("isis:Register");

            // create header
            op.NewChild("isis:Header").NewChild("isis:SourcePath").NewChild("isis:ID") = (*r)->getService()->getID();
        
            // create body
            op.NewChild(reg_doc);   
        
            // send
            PayloadSOAP *response;
            MCCConfig mcc_cfg;
            mcc_cfg.AddPrivateKey(key_);
            mcc_cfg.AddCertificate(cert_);
            mcc_cfg.AddProxy(proxy_);
            mcc_cfg.AddCADir(cadir_);
            ClientSOAP cli(mcc_cfg,url_);
            MCC_Status status = cli.process(&request, &response);
            if ((!status) || 
                (!response) || 
                (response->Name() != "RegisterResponse")) {
                logger_.msg(ERROR, "Error during registration to %s ISIS", isis_name);
            } else {
                XMLNode fault = (*response)["Fault"];
                if(!fault)  {
                    logger_.msg(DEBUG, "Successful registration to ISIS (%s)", isis_name); 
                } else {
                    logger_.msg(DEBUG, "Failed to register to ISIS (%s) - %s", isis_name, std::string(fault["Description"])); 
                }
            }
        }
        logger_.msg(DEBUG, "Registration ends: %s",isis_name);
        if(period_ <= 0) break; // One time registration
        Glib::TimeVal etime;
        etime.assign_current_time();
        etime.add_milliseconds(period_*1000L);
        // Sleep and exit if interrupted by request to exit
        if(cond_exit_.timed_wait(lock_,etime)) break; 
        //sleep(period_);
    }
    logger_.msg(DEBUG, "Registration exit: %s",isis_name);
}

// -------------------------------------------------------------------

InfoRegisters::InfoRegisters(XMLNode &cfg, Service *service) {
    if(!service) return;
    NS ns;
    ns["iregc"]=REGISTRATION_CONFIG_NAMESPACE;
    cfg.Namespaces(ns);
    for(XMLNode node = cfg["iregc:InfoRegistration"];(bool)node;++node) {
        registers_.push_back(new InfoRegister(node,service));
    }
}

InfoRegisters::~InfoRegisters(void) {
    for(std::list<InfoRegister*>::iterator i = registers_.begin();i!=registers_.end();++i) {
        if(*i) delete (*i);
    }
}

// -------------------------------------------------------------------


} // namespace Arc


