#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>

#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/client/ClientInterface.h>
#include "InfoRegister.h"
#ifdef WIN32
#include <arc/win32.h>
#endif

static Arc::Logger logger_(Arc::Logger::rootLogger, "InfoRegister");

namespace Arc {

static void reg_thread(void *data) {
    InfoRegistrar *self = (InfoRegistrar *)data;
    for (;;) {
        sleep(self->getPeriod());
        self->registration();
    }
}

InfoRegister::InfoRegister(XMLNode &cfg, Service *service):reg_period_(0) {
    service_ = service;
    ns_["isis"] = ISIS_NAMESPACE;
    ns_["glue2"] = GLUE2_D42_NAMESPACE;
    ns_["register"] = REGISTRATION_NAMESPACE;

    // parse config tree
    std::string s_reg_period = (std::string)cfg["Period"];
    if (!s_reg_period.empty()) { 
        reg_period_ = strtol(s_reg_period.c_str(), NULL, 10);
    }
    XMLNode peers = cfg["Peers"];
    for(;(bool)peers;++peers) {
        XMLNode n;
        std::string key = peers["KeyPath"];
        std::string cert = peers["CertificatePath"];
        std::string proxy = peers["ProxyPath"];
        std::string cadir = peers["CACertificatesDir"];
        for (int i = 0; (bool)(n = peers["URL"][i]); i++) {
            std::string url_str = (std::string)n;
            Peer peer(URL(url_str),key,cert,proxy,cadir);
            if(!peer.url) {
                logger_.msg(ERROR, "Can't recognize URL: %s",url_str);
            } else {
                peers_.push_back(peer);
            }
        }
    }
    InfoRegisterContainer::Instance().Add(this);
}

InfoRegister::~InfoRegister(void) {
    // Service should not be deleted here!
    // Nope
    // TODO: stop registration threads created by this object
}
    
void InfoRegistrar::registration(void) {
    logger_.msg(DEBUG, "Registration Start");
    if (reg_ == NULL) {
        logger_.msg(ERROR, "Invalid registration configuration");
        return;
    }
    if (reg_->service_ == NULL) {
        logger_.msg(ERROR, "Invalid service");
        return;
    }
    NS reg_ns;
    reg_ns["glue2"] = GLUE2_D42_NAMESPACE;
    reg_ns["isis"] = ISIS_NAMESPACE;
    XMLNode reg_doc(reg_ns, "isis:Advertisements");
    XMLNode services_doc(reg_ns, "Services");
    reg_->service_->RegistrationCollector(services_doc);
    // Look for service registration information
    // TODO: use more efficient way to find <Service> entries than XPath. 
    std::list<XMLNode> services = services_doc.XPathLookup("//*[normalize-space(@BaseType)='Service']", reg_ns);
    if (services.size() == 0) {
        logger_.msg(WARNING, "Service provided no registration entries");
        return;
    }
    // create advertisement
    std::list<XMLNode>::iterator sit;
    for (sit = services.begin(); sit != services.end(); sit++) {
        // filter may come here
        XMLNode ad = reg_doc.NewChild("isis:Advertisement");
        ad.NewChild((*sit));
        // XXX metadata
    }
    
    // send to all peers
    std::list<InfoRegister::Peer>::iterator it;
    for (it = reg_->peers_.begin(); it != reg_->peers_.end(); it++) {
        std::string isis_name = it->url.fullstr();
        logger_.msg(DEBUG, "Registering to %s ISIS", isis_name);
        PayloadSOAP request(reg_ns);
        XMLNode op = request.NewChild("isis:Register");
        
        // create header
        op.NewChild("isis:Header").NewChild("isis:SourcePath").NewChild("isis:ID") = reg_->service_->getID();
        
        // create body
        op.NewChild(reg_doc);   
        
        // send
        PayloadSOAP *response;
        MCCConfig mcc_cfg;
        mcc_cfg.AddPrivateKey(it->key);
        mcc_cfg.AddCertificate(it->cert);
        mcc_cfg.AddProxy(it->proxy);
        mcc_cfg.AddCADir(it->cadir);
        ClientSOAP cli(mcc_cfg, it->url);
        MCC_Status status = cli.process(&request, &response);
        if ((!status) || (!response)) {
            logger_.msg(ERROR, "Error during registration to %s ISIS", isis_name);
            continue;
        } 
        XMLNode fault = (*response)["Fault"];
        if(!fault)  {
            logger_.msg(DEBUG, "Successful registration to ISIS (%s)", isis_name); 
        } else {
            logger_.msg(DEBUG, "Failed to register to ISIS (%s) - %s", isis_name, std::string(fault["Description"])); 
        }
    }
}

InfoRegisters::InfoRegisters(XMLNode &cfg, Service *service) {
    if(!service) return;
    NS ns;
    ns["iregc"]=REGISTRATION_CONFIG_NAMESPACE;
    cfg.Namespaces(ns);
    for(XMLNode node = cfg["iregc:InfoRegister"];(bool)node;++node) {
        registers_.push_back(new InfoRegister(node,service));
    }
}

InfoRegisters::~InfoRegisters(void) {
    for(std::list<InfoRegister*>::iterator i = registers_.begin();i!=registers_.end();++i) {
        if(*i) delete (*i);
    }
}


InfoRegisterContainer InfoRegisterContainer::instance_;

InfoRegisterContainer::~InfoRegisterContainer(void) {
}

void InfoRegisterContainer::Add(InfoRegister* reg) {
  // Add element to list
  regs_.push_back(reg);
  // Create registrar
  InfoRegistrar* r = new InfoRegistrar(reg);
  regr_.push_back(r);
  // Start registration thread
  if (reg->reg_period_ > 0) {
     CreateThreadFunction(&reg_thread,r);
  }
}

void InfoRegisterContainer::Remove(InfoRegister* reg) {
  // Not implemented yet till relationship between elements is not clear
}

} // namespace Arc


