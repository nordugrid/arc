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

namespace Arc {

static Logger logger_(Logger::rootLogger, "InfoSys");

static void reg_thread(void *data) {
    InfoRegistrar *self = (InfoRegistrar *)data;
    { // Very important!!!: Delete this block imediately!!!
        // Sleep and exit if interrupted by request to exit
        unsigned int sleep_time = 15; //seconds
        logger_.msg(VERBOSE, "InfoRegistrar thread waiting %d seconds for the all Registers elements creation.", sleep_time);
        sleep(sleep_time);
    }
    self->registration();
}

// -------------------------------------------------------------------

InfoRegister::InfoRegister(XMLNode &cfg, Service *service):reg_period_(0),service_(service) {
    ns_["isis"] = ISIS_NAMESPACE;
    ns_["glue2"] = GLUE2_D42_NAMESPACE;
    ns_["register"] = REGISTRATION_NAMESPACE;

    // parse config
    std::string s_reg_period = (std::string)cfg["InfoRegister"]["Period"];
    if (!s_reg_period.empty()) {
        Period p(s_reg_period);
        reg_period_ = p.GetPeriod();
        if (reg_period_ < 120)
            reg_period_ = 120;
    } else {
        reg_period_ = -1;
    }
    std::string s_serviceid = (std::string)cfg["InfoRegister"]["ServiceID"];
    if (!s_serviceid.empty()) {
        serviceid = s_serviceid;
    }
    if ((bool)cfg["InfoRegister"]["Endpoint"])
        endpoint = (std::string)cfg["InfoRegister"]["Endpoint"];
    if ((bool)cfg["InfoRegister"]["Expiration"]) {
        expiration = (std::string)cfg["InfoRegister"]["Expiration"];
        Period p(expiration);
        if (p.GetPeriod() < 120) expiration = "PT2M";
    }

    //VERBOSE//
    std::string configuration_string;
    XMLNode temp;
    cfg.New(temp);
    temp.GetDoc(configuration_string, true);
    logger_.msg(VERBOSE, "InfoRegister created with config:\n%s", configuration_string);

    // Add service to registration list. Optionally only for
    // registration through specific registrants.
    std::list<std::string> ids;
    for(XMLNode r = cfg["InfoRegister"]["Registrar"];(bool)r;++r) {
      std::string id = (std::string) r["URL"];
      if(!id.empty()) ids.push_back(id);
      else logger_.msg(WARNING, "Discarding Registrar because the \"URL\" element is missing or empty.");
    };
    InfoRegisterContainer::Instance().addService(this,ids,cfg);
}

InfoRegister::~InfoRegister(void) {
    // This element is supposed to be destroyed with service.
    // Hence service should not be restered anymore.
    // TODO: initiate un-register of service
    InfoRegisterContainer::Instance().removeService(this);
}

// -------------------------------------------------------------------

InfoRegisterContainer* InfoRegisterContainer::instance_ = NULL;

InfoRegisterContainer& InfoRegisterContainer::Instance(void) {
  if(!instance_) instance_ = new InfoRegisterContainer;
  return *instance_;
}

InfoRegisterContainer::InfoRegisterContainer(void) {
}

InfoRegisterContainer::~InfoRegisterContainer(void) {
    Glib::Mutex::Lock lock(lock_);
    for(std::list<InfoRegistrar*>::iterator r = regr_.begin();
                              r != regr_.end();++r) {
        delete (*r);
    };
}

InfoRegistrar *InfoRegisterContainer::addRegistrar(XMLNode node) {
    //Glib::Mutex::Lock lock(lock_);
    InfoRegistrar* r = new InfoRegistrar(node);
    if(!(r)) {
         delete r;
         return NULL;
    }
    regr_.push_back(r);
    CreateThreadFunction(&reg_thread, r);
    return r;
}

void InfoRegisterContainer::addService(InfoRegister* reg,const std::list<std::string>& ids,XMLNode cfg) {
    // Add to registrars
    Glib::Mutex::Lock lock(lock_);
    for(std::list<std::string>::const_iterator i = ids.begin(); i != ids.end();++i) {
        bool id_found = false;
        for(std::list<InfoRegistrar*>::iterator r = regr_.begin(); r != regr_.end();++r) {
            if((*i).find((*r)->id()) != std::string::npos) {
                logger_.msg(VERBOSE, "InfoRegistrar id \"%s\" has been found.", (*i));
                (*r)->addService(reg, cfg);
                id_found = true;
            }
        }
        if (!id_found) {
            // id appears at first time - InfoRegistrar need to be created
            logger_.msg(VERBOSE, "InfoRegistrar id \"%s\" was not found. New registrar created", (*i));
            for(XMLNode node = cfg["InfoRegister"]["Registrar"];(bool)node;++node) {
                if ((*i) == (std::string)node["URL"]) {
                    InfoRegistrar *r = addRegistrar(node);
                    if (r != NULL) r->addService(reg, cfg);
                }
            }
        }
    }
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

InfoRegistrar::InfoRegistrar(XMLNode cfg):stretch_window("PT20S"), already_registered(false) {
    id_=(std::string)cfg["URL"];

    if ((bool)cfg["Retry"]) {
        if (!((std::string)cfg["Retry"]).empty()) {
            if(EOF == sscanf(((std::string)cfg["Retry"]).c_str(), "%d", &retry) || retry < 0)
            {
                logger_.msg(ERROR, "Configuration error. Retry: \"%s\" is not a valid value. Default value will be used.",(std::string)cfg["Retry"]);
                retry = 5;
            }
        } else retry = 5;
    } else retry = 5;

    logger_.msg(VERBOSE, "Retry: %d", retry);

    // Parsing security attributes
    key_ = (std::string)cfg["KeyPath"];
    cert_ = (std::string)cfg["CertificatePath"];
    proxy_ = (std::string)cfg["ProxyPath"];
    cadir_ = (std::string)cfg["CACertificatesDir"];
    cafile_ = (std::string)cfg["CACertificatePath"];

    logger_.msg(VERBOSE, "Key: %s, cert: %s", key_, cert_);

    if (id_.substr(0,7) == "EMIREG:") { 
        infosys_type = "EMIREG";
        path = "serviceadmin";
        id_ = id_.substr(7);
        if ( id_.at(id_.length()-1) != '/' ){
            path.insert(0,"/");            // now the new path is "/serviceadmin"
        }
    } else if (id_.substr(0,5) == "ISIS:") {
        infosys_type = "ISIS";
        id_ = id_.substr(5);
        cfg["URL"] = id_;
        initISIS(cfg);
    } else {
        // default behaviour
        infosys_type = "ISIS";
        initISIS(cfg);
    }

    time_t rawtime;
    time ( &rawtime );  //current time
    gmtime ( &rawtime );
    Time ctime(rawtime);
    creation_time = ctime;
}

bool InfoRegistrar::addService(InfoRegister* reg, XMLNode& cfg) {
    if ( bool(cfg["NoRegister"]) || !bool(cfg["InfoRegister"])){
       logger_.msg(VERBOSE, "The service won't be registered.");
       return true;
    }

    if (!(bool)cfg["InfoRegister"]["Period"] ) {
       logger_.msg(ERROR, "Configuration error. Missing mandatory \"Period\" element.");
       return false;
    }

    if (!(bool)cfg["InfoRegister"]["Endpoint"] ) {
       logger_.msg(ERROR, "Configuration error. Missing mandatory \"Endpoint\" element.");
       return false;
    }

    if (!(bool)cfg["InfoRegister"]["Expiration"] ) {
       logger_.msg(ERROR, "Configuration error. Missing mandatory \"Expiration\" element.");
       return false;
    }
    Glib::Mutex::Lock lock(lock_);
    for(std::list<Register_Info_Type>::iterator r = reg_.begin();
                                           r!=reg_.end();++r) {
        if(reg == r->p_register) {
            logger_.msg(VERBOSE, "Service was already registered to the InfoRegistrar connecting to infosys %s.", myISIS.url);
            return false;
        }
    }
    Register_Info_Type reg_info;
    reg_info.p_register = reg;

    std::string current_serviceid = reg->getServiceID();
    std::string current_expiration = reg->getExpiration();
    std::string current_endpoint = reg->getEndpoint();
    Period period(reg->getPeriod());
    for(XMLNode node = cfg["InfoRegister"]["Registrar"];(bool)node;++node) {
        if ( (std::string)node["URL"] == id_ ) {
            if (! ((std::string)node["Period"]).empty() ) {
                Period current_period((std::string)node["Period"]);
                period = current_period;
            }
            if (! ((std::string)node["ServiceID"]).empty() ) {
                current_serviceid = (std::string)node["ServiceID"];
            }
            if (! ((std::string)node["Endpoint"]).empty() ) {
                current_endpoint = (std::string)node["Endpoint"];
            }
            if (! ((std::string)node["Expiration"]).empty() ) {
                current_expiration = (std::string)node["Expiration"];
            }
       }
    }

    reg_info.period = period;
    reg_info.serviceid = current_serviceid;
    reg_info.expiration = current_expiration;
    reg_info.endpoint = current_endpoint;

    reg_info.next_registration = creation_time.GetTime();
    reg_.push_back(reg_info);
    logger_.msg(VERBOSE, "Service is successfully added to the InfoRegistrar connecting to infosys %s.", myISIS.url);
    return true;
}

bool InfoRegistrar::removeService(InfoRegister* reg) {
    Glib::Mutex::Lock lock(lock_);
    for(std::list<Register_Info_Type>::iterator r = reg_.begin();
                                           r!=reg_.end();++r) {
        if(reg == r->p_register) {
    	    if ( infosys_type == "ISIS" ) {
                sendDeleteToISIS(r);
            } else if ( infosys_type == "EMIREG" ) {
                sendDeleteToEMIREG(r);
            }
            return true;
        };
    };
    logger_.msg(VERBOSE, "Unregistred Service can not be removed.");
    return false;
}

void InfoRegistrar::sendDeleteToISIS(std::list<Register_Info_Type>::iterator r) {
    NS reg_ns;
    reg_ns["glue2"] = GLUE2_D42_NAMESPACE;
    reg_ns["isis"] = ISIS_NAMESPACE;

    time_t current_time;
    time ( &current_time );  //current time
    tm * ptm;
    ptm = gmtime ( &current_time );

    std::string mon_prefix = (ptm->tm_mon+1 < 10)?"0":"";
    std::string day_prefix = (ptm->tm_mday < 10)?"0":"";
    std::string hour_prefix = (ptm->tm_hour < 10)?"0":"";
    std::string min_prefix = (ptm->tm_min < 10)?"0":"";
    std::string sec_prefix = (ptm->tm_sec < 10)?"0":"";
    std::stringstream out;
    out << ptm->tm_year+1900<<"-"<<mon_prefix<<ptm->tm_mon+1<<"-"<<day_prefix<<ptm->tm_mday<<"T";
    out << hour_prefix<<ptm->tm_hour<<":"<<min_prefix<<ptm->tm_min<<":"<<sec_prefix<<ptm->tm_sec;
    out << "+0000";

    PayloadSOAP request(reg_ns);
    XMLNode op = request.NewChild("isis:RemoveRegistrations");
    op.NewChild("ServiceID") = r->serviceid_;
    op.NewChild("MessageGenerationTime") = out.str();

    // send
    PayloadSOAP *response;
    MCCConfig mcc_cfg;
    ISIS_description usedISIS = getISIS();
    if (!key_.empty())
        mcc_cfg.AddPrivateKey(key_);
    if (!cert_.empty())
        mcc_cfg.AddCertificate(cert_);
    if (!proxy_.empty())
        mcc_cfg.AddProxy(proxy_);
    if (!cadir_.empty())
        mcc_cfg.AddCADir(cadir_);
    if (!cafile_.empty())
        mcc_cfg.AddCAFile(cafile_);
    logger_.msg(VERBOSE, "Key: %s, Cert: %s, Proxy: %s, CADir: %s CAPath", key_, cert_, proxy_, cadir_, cafile_);

    int retry_ = retry;
    while ( retry_ >= 1 ){
        ClientSOAP cli(mcc_cfg,Arc::URL(usedISIS.url),60);
        MCC_Status status = cli.process(&request, &response);

        std::string response_string;
        (*response).GetDoc(response_string, true);
        logger_.msg(VERBOSE, "Response from the ISIS: %s", response_string);

        if ((!status.isOk()) ||
            (!response)) {
            logger_.msg(ERROR, "Failed to remove registration from %s ISIS )", usedISIS.url);
        } else {
            if(!(bool)(*response)["RemoveRegistrationResponseElement"])  {
                logger_.msg(VERBOSE, "Successful removed registration from ISIS (%s)", usedISIS.url);
                break;
            } else {
                int i=0;
                while ((bool)(*response)["RemoveRegistrationResponseElement"][i]) {
                     logger_.msg(VERBOSE, "Failed to remove registration from ISIS (%s) - %s",
                             usedISIS.url, std::string((*response)["RemoveRegistrationResponseElement"][i]["Fault"]));
                     i++;
                }
            }
         }
         retry_--;
         logger_.msg(VERBOSE, "Retry connecting to the ISIS (%s) %d. time(s).", usedISIS.url, retry-retry_);
    }

    if (retry_ == 0 )
        logger_.msg(VERBOSE, "ISIS (%s) is not available.", usedISIS.url);

    reg_.erase(r);

    logger_.msg(VERBOSE, "Service removed from InfoRegistrar connecting to infosys %s.", myISIS.url);
}

void InfoRegistrar::sendDeleteToEMIREG(std::list<Register_Info_Type>::iterator r) {
    // send
    PayloadRaw http_request;
    PayloadRawInterface *http_response = NULL;
    HTTPClientInfo http_info;
    std::multimap<std::string, std::string> http_attributes;
    http_attributes.insert( std::pair<std::string, std::string>("Accept","text/html, image/gif, image/jpeg, *; q=.2, */*; q=.2"));
    URL service_url(id_+path+"?Service_Endpoint_URL="+r->endpoint);

    MCCConfig mcc_cfg;
    if (!key_.empty())
        mcc_cfg.AddPrivateKey(key_);
    if (!cert_.empty())
        mcc_cfg.AddCertificate(cert_);
    if (!proxy_.empty())
        mcc_cfg.AddProxy(proxy_);
    if (!cadir_.empty())
        mcc_cfg.AddCADir(cadir_);
    if (!cafile_.empty())
        mcc_cfg.AddCAFile(cafile_);
    logger_.msg(VERBOSE, "Key: %s, Cert: %s, Proxy: %s, CADir: %s CAPath", key_, cert_, proxy_, cadir_, cafile_);

    int retry_ = retry;
    while ( retry_ >= 1 ){
        Arc::ClientHTTP cli(mcc_cfg, service_url);
        MCC_Status status = cli.process("DELETE", http_attributes, &http_request, &http_info, &http_response);

        if ((!status.isOk()) ||
            (!http_response)) {
            logger_.msg(ERROR, "Failed to remove registration from %s EMIRegistry )", id_);
        } else {
            if(http_info.code == 200)  {
                logger_.msg(VERBOSE, "Successful removed registration from EMIRegistry (%s)", id_);
                break;
            }
         }
         retry_--;
         logger_.msg(VERBOSE, "Retry connecting to the EMIRegistry (%s) %d. time(s).", id_, retry-retry_);
    }

    if (retry_ == 0 )
        logger_.msg(VERBOSE, "EMIRegistry (%s) is not available.", id_);

    reg_.erase(r);

    logger_.msg(VERBOSE, "Service removed from InfoRegistrar connecting to infosys %s.", id_);
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
    if ( infosys_type == "ISIS" ) {
        sendRegistrationToISIS();
    } else if ( infosys_type == "EMIREG" ) {
        sendRegistrationToEMIREG();
    }
}

void InfoRegistrar::sendRegistrationToISIS() {
    ISIS_description usedISIS;
    std::string isis_name;
    if(!myISISList_initialized) getISISList(myISIS);
    myISISList_initialized = true;
    while(reg_.size() > 0) {
        usedISIS = getISIS();
        isis_name = usedISIS.url;

        logger_.msg(VERBOSE, "Registration starts: %s",isis_name);
        logger_.msg(VERBOSE, "reg_.size(): %d",reg_.size());

        if(usedISIS.url.empty()) {
            logger_.msg(WARNING, "Registrant has no proper URL specified. Registration end.");
            return;
        }
        NS reg_ns;
        reg_ns["glue2"] = GLUE2_D42_NAMESPACE;
        reg_ns["isis"] = ISIS_NAMESPACE;

        // Registration algorithm is stupid and straightforward.
        // This part has to be redone to fit P2P network od ISISes

        time_t current_time;
        time ( &current_time );  //current time
        tm * ptm;
        ptm = gmtime ( &current_time );
        Time min_reg_time(-1);
        XMLNode send_doc(reg_ns, "");

        std::string mon_prefix = (ptm->tm_mon+1 < 10)?"0":"";
        std::string day_prefix = (ptm->tm_mday < 10)?"0":"";
        std::string hour_prefix = (ptm->tm_hour < 10)?"0":"";
        std::string min_prefix = (ptm->tm_min < 10)?"0":"";
        std::string sec_prefix = (ptm->tm_sec < 10)?"0":"";
        std::stringstream out;
        out << ptm->tm_year+1900<<"-"<<mon_prefix<<ptm->tm_mon+1<<"-"<<day_prefix<<ptm->tm_mday<<"T";
        out << hour_prefix<<ptm->tm_hour<<":"<<min_prefix<<ptm->tm_min<<":"<<sec_prefix<<ptm->tm_sec;
        out << "+0000";

        for(std::list<Register_Info_Type>::iterator r = reg_.begin();
                                               r!=reg_.end();++r) {
            if ( (r->next_registration).GetTime() <= current_time + stretch_window.GetPeriod() ){
                logger_.msg(VERBOSE,"Create RegEntry XML element");
                Time current(current_time);
                // set the next registration time
                r->next_registration = current + r->period;

                XMLNode services_doc(reg_ns,"RegEntry");
                if(!((r->p_register)->getService())) continue;
                (r->p_register)->getService()->RegistrationCollector(services_doc);

                // Fill attributes from InfoRegister configuration
                if (!((bool)services_doc["SrcAdv"]["EPR"]["Address"]) && !((r->endpoint).empty()) ) {
                    if (!(bool)services_doc["SrcAdv"]) services_doc.NewChild("SrcAdv");
                    if (!(bool)services_doc["SrcAdv"]["EPR"]) services_doc["SrcAdv"].NewChild("EPR");
                    if (!(bool)services_doc["SrcAdv"]["EPR"]["Address"]) services_doc["SrcAdv"]["EPR"].NewChild("Address");
                    services_doc["SrcAdv"]["EPR"]["Address"] = r->endpoint;
                }

                if (!((bool)services_doc["MetaSrcAdv"]["ServiceID"]) && !((r->serviceid).empty()) ) {
                    if (!(bool)services_doc["MetaSrcAdv"]) services_doc.NewChild("MetaSrcAdv");
                    if (!(bool)services_doc["MetaSrcAdv"]["ServiceID"]) services_doc["MetaSrcAdv"].NewChild("ServiceID");
                    services_doc["MetaSrcAdv"]["ServiceID"] = r->serviceid;
                }

                if (!((bool)services_doc["MetaSrcAdv"]["Expiration"]) && !((r->expiration).empty()) ) {
                    if (!(bool)services_doc["MetaSrcAdv"]) services_doc.NewChild("MetaSrcAdv");
                    if (!(bool)services_doc["MetaSrcAdv"]["Expiration"]) services_doc["MetaSrcAdv"].NewChild("Expiration");
                    services_doc["MetaSrcAdv"]["Expiration"] = r->expiration;
                }

                // Possible completion of the services_doc
                if (!((bool)services_doc["MetaSrcAdv"]["ServiceID"]) && ((bool)services_doc["SrcAdv"]["EPR"]["Address"])) {
                    services_doc["MetaSrcAdv"].NewChild("ServiceID") = (std::string) services_doc["SrcAdv"]["EPR"]["Address"];
                    logger_.msg(VERBOSE, "ServiceID attribute calculated from Endpoint Reference");
                }
                if (!(bool)services_doc["MetaSrcAdv"]["GenTime"]) {
                    services_doc["MetaSrcAdv"].NewChild("GenTime") = out.str();
                    logger_.msg(VERBOSE, "Generation Time attribute calculated from current time");
                }

                // Store the sent ServiceID for the clear shutdown RemoveRegistration operation, if necessary
                if ( (r->serviceid_).empty() &&
                     (bool)services_doc["MetaSrcAdv"]["ServiceID"]) {
                    r->serviceid_ = (std::string) services_doc["MetaSrcAdv"]["ServiceID"];
                    logger_.msg(VERBOSE,"ServiceID stored: %s", r->serviceid_);
                }

                // TODO check the received registration information
                bool valid_services_doc = true;
                if (!(services_doc.Name() == "RegEntry")) {
                    logger_.msg(ERROR,"Missing service document provided by the service %s", r->serviceid_);
                    valid_services_doc = false;
                }

                if (!((bool) services_doc["MetaSrcAdv"]) ||
                    !((bool) services_doc["MetaSrcAdv"]["Expiration"])) {
                    logger_.msg(ERROR,"Missing MetaServiceAdvertisment or Expiration values provided by the service %s",
                                r->serviceid_);
                    valid_services_doc = false;
                }

                if (!((bool) services_doc["SrcAdv"]) ||
                    !((bool) services_doc["SrcAdv"]["Type"])) {
                    logger_.msg(ERROR,"Missing Type value provided by the service %s",
                                r->serviceid_);
                    valid_services_doc = false;
                }

                if (!((bool) services_doc["SrcAdv"]) ||
                    !((bool) services_doc["SrcAdv"]["EPR"]) ||
                    !((bool) services_doc["SrcAdv"]["EPR"]["Address"])) {
                    logger_.msg(ERROR,"Missing Endpoint Reference value provided by the service %s",
                                r->serviceid_);
                    valid_services_doc = false;
                }

                if (valid_services_doc) send_doc.NewChild(services_doc);
            }
            // conditioned minimum search
            if ( min_reg_time.GetTime() == -1 ){
                min_reg_time = r->next_registration;
            }
            else if ( r->next_registration < min_reg_time ) {
                min_reg_time = r->next_registration;
            }
        }

        // prepare for sending to ISIS
        if ( min_reg_time.GetTime() != -1 ) {
            logger_.msg(VERBOSE, "Registering to %s ISIS", isis_name);
            PayloadSOAP request(reg_ns);
            XMLNode op = request.NewChild("isis:Register");
            XMLNode header = op.NewChild("isis:Header");
            header.NewChild("MessageGenerationTime") = out.str();

            // create body
            for(XMLNode node = send_doc["RegEntry"];(bool)node;++node) {
                op.NewChild(node);
            }

            // send
            PayloadSOAP *response;
            MCCConfig mcc_cfg;
            if (!key_.empty())
                mcc_cfg.AddPrivateKey(key_);
            if (!cert_.empty())
                mcc_cfg.AddCertificate(cert_);
            if (!proxy_.empty())
                mcc_cfg.AddProxy(proxy_);
            if (!cadir_.empty())
                mcc_cfg.AddCADir(cadir_);
            if (!cafile_.empty())
                mcc_cfg.AddCAFile(cafile_);
            logger_.msg(VERBOSE, "Key: %s, Cert: %s, Proxy: %s, CADir: %s, CAFile", key_, cert_, proxy_, cadir_, cafile_);

            {std::string services_document;
             op.GetDoc(services_document, true);
             logger_.msg(VERBOSE, "Sent RegEntries: %s", services_document);
            }
            //logger_.msg(VERBOSE, "Call the ISIS.process method.");

            int retry_ = retry;
            while ( retry_ >= 1 ) {
                ClientSOAP cli(mcc_cfg,Arc::URL(usedISIS.url),60);
                MCC_Status status = cli.process(&request, &response);

                // multiple tries
                if ((!status.isOk()) ||
                    (!response) ||
                    (!bool((*response)["RegisterResponse"]))) {
                    logger_.msg(ERROR, "Error during registration to %s ISIS", isis_name);
                } else {
                    XMLNode fault = (*response)["Fault"];

                    if(!fault)  {
                        std::string response_string;
                        (*response)["RegisterResponse"].GetDoc(response_string, true);
                        logger_.msg(VERBOSE, "Response from the ISIS: %s", response_string);

                        logger_.msg(VERBOSE, "Successful registration to ISIS (%s)", isis_name);
                        break;
                    } else {
                        logger_.msg(VERBOSE, "Failed to register to ISIS (%s) - %s", isis_name, std::string(fault["Description"]));
                    }
                }
                retry_--;
                logger_.msg(VERBOSE, "Retry connecting to the ISIS (%s) %d. time(s).", isis_name, retry-retry_);
            }
            if (response) delete response;

            if ( retry_ == 0 )
                removeISIS(usedISIS);
        } // end of the connection with the ISIS

        // Thread sleeping
        long int period_ = min_reg_time.GetTime() - current_time;

        logger_.msg(VERBOSE, "Registration ends: %s",isis_name);
        logger_.msg(VERBOSE, "Waiting period is %d second(s).",period_);
        // The next line is removed for infinite operation
        // if(period_ <= 0) break; // One time registration
        Glib::TimeVal etime;
        etime.assign_current_time();
        etime.add_milliseconds(period_*1000L);
        // Sleep and exit if interrupted by request to exit
        if(cond_exit_.timed_wait(lock_,etime)) break;
        //sleep(period_);
    }
    logger_.msg(VERBOSE, "Registration exit: %s",isis_name);
}

void InfoRegistrar::sendRegistrationToEMIREG() {
    while(reg_.size() > 0) {
        logger_.msg(VERBOSE, "Registration starts: %s",id_);
        logger_.msg(VERBOSE, "reg_.size(): %d",reg_.size());

        if(id_.empty()) {
            logger_.msg(WARNING, "Registrant has no proper URL specified. Registration end.");
            return;
        }

        time_t current_time;
        time ( &current_time );  //current time
        tm * ptm;
        ptm = gmtime ( &current_time );
        Time min_reg_time(-1);
        NS reg_ns;
        XMLNode send_doc(reg_ns, "");

        std::string mon_prefix = (ptm->tm_mon+1 < 10)?"0":"";
        std::string day_prefix = (ptm->tm_mday < 10)?"0":"";
        std::string hour_prefix = (ptm->tm_hour < 10)?"0":"";
        std::string min_prefix = (ptm->tm_min < 10)?"0":"";
        std::string sec_prefix = (ptm->tm_sec < 10)?"0":"";
        std::stringstream out;
        out << ptm->tm_year+1900<<"-"<<mon_prefix<<ptm->tm_mon+1<<"-"<<day_prefix<<ptm->tm_mday<<"T";
        out << hour_prefix<<ptm->tm_hour<<":"<<min_prefix<<ptm->tm_min<<":"<<sec_prefix<<ptm->tm_sec;
        out << "+0000";

        for(std::list<Register_Info_Type>::iterator r = reg_.begin();
                                               r!=reg_.end();++r) {
            if ( (r->next_registration).GetTime() <= current_time + stretch_window.GetPeriod() ){
                logger_.msg(VERBOSE,"Create RegEntry XML element");
                Time current(current_time);
                // set the next registration time
                r->next_registration = current + r->period;

               XMLNode services_doc(reg_ns,"RegEntry");
                if(!((r->p_register)->getService())) continue;
                (r->p_register)->getService()->RegistrationCollector(services_doc);

                // Fill attributes from InfoRegister configuration
                if (!((bool)services_doc["SrcAdv"]["EPR"]["Address"]) && !((r->endpoint).empty()) ) {
                    if (!(bool)services_doc["SrcAdv"]) services_doc.NewChild("SrcAdv");
                    if (!(bool)services_doc["SrcAdv"]["EPR"]) services_doc["SrcAdv"].NewChild("EPR");
                    if (!(bool)services_doc["SrcAdv"]["EPR"]["Address"]) services_doc["SrcAdv"]["EPR"].NewChild("Address");
                    services_doc["SrcAdv"]["EPR"]["Address"] = r->endpoint;
                }

                if (!((bool)services_doc["MetaSrcAdv"]["ServiceID"]) && !((r->serviceid).empty()) ) {
                    if (!(bool)services_doc["MetaSrcAdv"]) services_doc.NewChild("MetaSrcAdv");
                    if (!(bool)services_doc["MetaSrcAdv"]["ServiceID"]) services_doc["MetaSrcAdv"].NewChild("ServiceID");
                    services_doc["MetaSrcAdv"]["ServiceID"] = r->serviceid;
                }

                if (!((bool)services_doc["MetaSrcAdv"]["Expiration"]) && !((r->expiration).empty()) ) {
                    if (!(bool)services_doc["MetaSrcAdv"]) services_doc.NewChild("MetaSrcAdv");
                    if (!(bool)services_doc["MetaSrcAdv"]["Expiration"]) services_doc["MetaSrcAdv"].NewChild("Expiration");
                    services_doc["MetaSrcAdv"]["Expiration"] = r->expiration;
                }

                // Possible completion of the services_doc
                if (!((bool)services_doc["MetaSrcAdv"]["ServiceID"]) && ((bool)services_doc["SrcAdv"]["EPR"]["Address"])) {
                    services_doc["MetaSrcAdv"].NewChild("ServiceID") = (std::string) services_doc["SrcAdv"]["EPR"]["Address"];
                    logger_.msg(VERBOSE, "ServiceID attribute calculated from Endpoint Reference");
                }
                if (!(bool)services_doc["MetaSrcAdv"]["GenTime"]) {
                    services_doc["MetaSrcAdv"].NewChild("GenTime") = out.str();
                    logger_.msg(VERBOSE, "Generation Time attribute calculated from current time");
                }

                // Store the sent ServiceID for the clear shutdown RemoveRegistration operation, if necessary
                if ( (r->serviceid_).empty() &&
                     (bool)services_doc["MetaSrcAdv"]["ServiceID"]) {
                    r->serviceid_ = (std::string) services_doc["MetaSrcAdv"]["ServiceID"];
                    logger_.msg(VERBOSE,"ServiceID stored: %s", r->serviceid_);
                }

                // TODO check the received registration information
                bool valid_services_doc = true;
                if (!(services_doc.Name() == "RegEntry")) {
                    logger_.msg(ERROR,"Missing service document provided by the service %s", r->serviceid_);
                    valid_services_doc = false;
                }

                if (!((bool) services_doc["MetaSrcAdv"]) ||
                    !((bool) services_doc["MetaSrcAdv"]["Expiration"])) {
                    logger_.msg(ERROR,"Missing MetaServiceAdvertisment or Expiration values provided by the service %s",
                                r->serviceid_);
                    valid_services_doc = false;
                }

                if (!((bool) services_doc["SrcAdv"]) ||
                    !((bool) services_doc["SrcAdv"]["Type"])) {
                    logger_.msg(ERROR,"Missing Type value provided by the service %s",
                                r->serviceid_);
                    valid_services_doc = false;
                }

                if (!((bool) services_doc["SrcAdv"]) ||
                    !((bool) services_doc["SrcAdv"]["EPR"]) ||
                    !((bool) services_doc["SrcAdv"]["EPR"]["Address"])) {
                    logger_.msg(ERROR,"Missing Endpoint Reference value provided by the service %s",
                                r->serviceid_);
                    valid_services_doc = false;
                }

                if (valid_services_doc) send_doc.NewChild(services_doc);
            }
            // conditioned minimum search
            if ( min_reg_time.GetTime() == -1 ){
                min_reg_time = r->next_registration;
            }
            else if ( r->next_registration < min_reg_time ) {
                min_reg_time = r->next_registration;
            }
        }

        // prepare for sending to EMIREG
        if ( min_reg_time.GetTime() != -1 ) {
            logger_.msg(VERBOSE, "Registering to %s EMIRegistry", id_);

            // send
            PayloadRaw http_request;
            PayloadRawInterface *http_response = NULL;
            HTTPClientInfo http_info;
            std::multimap<std::string, std::string> http_attributes;

            //Add http attributes
            http_attributes.insert( std::pair<std::string, std::string>("Content-Type","application/json"));

            MCCConfig mcc_cfg;
            if (!key_.empty())
                mcc_cfg.AddPrivateKey(key_);
            if (!cert_.empty())
                mcc_cfg.AddCertificate(cert_);
            if (!proxy_.empty())
                mcc_cfg.AddProxy(proxy_);
            if (!cadir_.empty())
                mcc_cfg.AddCADir(cadir_);
            if (!cafile_.empty())
                mcc_cfg.AddCAFile(cafile_);
            logger_.msg(VERBOSE, "Key: %s, Cert: %s, Proxy: %s, CADir: %s, CAFile", key_, cert_, proxy_, cadir_, cafile_);

            // create body
            Arc::URL service_url(id_+path);
            Arc::ClientHTTP httpclient(mcc_cfg, service_url);

            std::string tmp_message;
            tmp_message += "[";
            bool first_item =true;
            for(XMLNode node = send_doc["RegEntry"];(bool)node;++node) {
                // Message generation from the XMLNode
                //tmp_message.clear();
                if (first_item){
		    first_item = false;
                } else {
                    tmp_message += ",";
		}
                tmp_message += "{";
		if ( ((std::string)node["SrcAdv"]["Type"]).find("org.nordugrid.execution") != std::string::npos){
                    tmp_message += "\"Service_Name\":\"ComputingService\",";
		} else if (((std::string)node["SrcAdv"]["Type"]).find("org.nordugrid.tests") != std::string::npos){
                    tmp_message += "\"Service_Name\":\"TestService\",";
		} else if (((std::string)node["SrcAdv"]["Type"]).find("org.nordugrid.storage") != std::string::npos){
                    tmp_message += "\"Service_Name\":\"StorageService\",";
		} else if (((std::string)node["SrcAdv"]["Type"]).find("org.nordugrid.security") != std::string::npos){
                    tmp_message += "\"Service_Name\":\"SecurityService\",";
		} else if (((std::string)node["SrcAdv"]["Type"]).find("org.nordugrid.infosys") != std::string::npos){
                    tmp_message += "\"Service_Name\":\"InfosysService\",";
		} else if (((std::string)node["SrcAdv"]["Type"]).find("org.nordugrid.accounting") != std::string::npos){
                    tmp_message += "\"Service_Name\":\"AccountingService\",";
		}
                tmp_message += "\"Service_Type\":\"";
		tmp_message += (std::string)node["SrcAdv"]["Type"];
		tmp_message += "\",";
                tmp_message += "\"Service_Endpoint_URL\":\"";
		tmp_message += (std::string)node["SrcAdv"]["EPR"]["Address"];
		tmp_message += "\"";

                tmp_message += "}";
            }
            tmp_message += "]";
	    const std::string &message( tmp_message );
            //header.NewChild("MessageGenerationTime") = out.str();

            { 
                    std::string doc;
		send_doc.GetXML(doc, true);
                    logger_.msg(VERBOSE, "Service_doc: %s", doc);
                    logger_.msg(VERBOSE, "Sent entry: %s", message);
            }
            //Add the message into the request
            http_request.Insert(message.c_str());

            int retry_ = retry;
            while ( retry_ >= 1 ) {
                MCC_Status status = httpclient.process( already_registered ? "PUT" : "POST", http_attributes, &http_request, &http_info, &http_response);

                // multiple tries
                std::string method = already_registered ? "updating" : "registration";
                if ( !status.isOk() ) {
                    logger_.msg(ERROR, "Error during %s to %s EMIRegistry", method, id_);
                } else {
                    if( http_info.code == 200 )  {
                        std::string response_string;
                         logger_.msg(VERBOSE, "Successful %s to EMIRegistry (%s)", method, id_);
                        break;
                    } else {
                        std::string method2 = already_registered ? "update" : "register";
                        logger_.msg(VERBOSE, "Failed to %s to EMIRegistry (%s) - %d", method2, id_, http_info.code );
                    }
                }
                retry_--;
                logger_.msg(VERBOSE, "Retry connecting to the EMIRegistry (%s) %d. time(s).", id_, retry-retry_);
            }
            if (http_response) delete http_response;
	}
	already_registered = true;
        // end of the connection with the EMIRegistry

        // Thread sleeping
        long int period_ = min_reg_time.GetTime() - current_time;

        logger_.msg(VERBOSE, "Registration ends: %s",id_);
        logger_.msg(VERBOSE, "Waiting period is %d second(s).",period_);
        // The next line is removed for infinite operation
        // if(period_ <= 0) break; // One time registration
        Glib::TimeVal etime;
        etime.assign_current_time();
        etime.add_milliseconds(period_*1000L);
        // Sleep and exit if interrupted by request to exit
        if(cond_exit_.timed_wait(lock_,etime)) break;
        //sleep(period_);
    }
    logger_.msg(VERBOSE, "Registration exit: %s",id_);
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

