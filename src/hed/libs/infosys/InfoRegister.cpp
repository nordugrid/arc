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

namespace Arc
{

static void reg_thread(void *data)
{
    Arc::InfoRegister *self = (Arc::InfoRegister *)data;
    for (;;) {
        sleep(self->getPeriod());
        self->registration();
    }
}

InfoRegister::InfoRegister(Arc::XMLNode &cfg, Arc::Service *service):logger_(Logger::rootLogger, "InfoRegister"),reg_period_(0)
{
    service_ = service;
    ns_["isis"] = ISIS_NAMESPACE;
    ns_["glue2"] = GLUE2_D42_NAMESPACE;
    ns_["register"] = REGISTRATION_NAMESPACE;

    // parse config tree
    std::string s_reg_period = (std::string)cfg["Period"];
    if (!s_reg_period.empty()) { 
        reg_period_ = strtol(s_reg_period.c_str(), NULL, 10);
    }
    Arc::XMLNode peers = cfg["Peers"];
    for(;(bool)peers;++peers) {
        Arc::XMLNode n;
        std::string key = peers["KeyPath"];
        std::string cert = peers["CertificatePath"];
        std::string proxy = peers["ProxyPath"];
        std::string cadir = peers["CACertificatesDir"];
        for (int i = 0; (n = peers["URL"][i]) != false; i++) {
            std::string url_str = (std::string)n;
            Peer peer(Arc::URL(url_str),key,cert,proxy,cadir);
            if(!peer.url) {
                logger_.msg(Arc::ERROR, "Can't recognize URL: %s",url_str);
            } else {
                peers_.push_back(peer);
            }
        }
    }
    
    // Start Registration thread
    if (reg_period_ > 0) {
        Arc::CreateThreadFunction(&reg_thread, this);
    }
}

InfoRegister::~InfoRegister(void)
{
    // Service should not be deleted here!
    // Nope
    // TODO: stop registration threads created by this object
}

void InfoRegister::registration(void)
{
    logger_.msg(Arc::DEBUG, "Registration Start");
    if (service_ == NULL) {
        logger_.msg(Arc::ERROR, "Invalid service");
        return;
    }
    Arc::NS reg_ns;
    reg_ns["glue2"] = ns_["glue2"];
    reg_ns["isis"] = ns_["isis"];
    Arc::XMLNode reg_doc(reg_ns, "isis:Advertisements");
    Arc::XMLNode services_doc(reg_ns, "Services");
    service_->RegistrationCollector(services_doc);
    // Look for service registration information
    // TODO: use more efficient way to find <Service> entries than XPath. 
    std::list<Arc::XMLNode> services = services_doc.XPathLookup("//*[normalize-space(@BaseType)='Service']", reg_ns);
    if (services.size() == 0) {
        logger_.msg(Arc::WARNING, "Service provided no registration entries");
        return;
    }
    // create advertisement
    std::list<Arc::XMLNode>::iterator sit;
    for (sit = services.begin(); sit != services.end(); sit++) {
        // filter may come here
        Arc::XMLNode ad = reg_doc.NewChild("isis:Advertisement");
        ad.NewChild((*sit));
        // XXX metadata
    }
    
    // send to all peers
    std::list<Peer>::iterator it;
    for (it = peers_.begin(); it != peers_.end(); it++) {
        std::string isis_name = it->url.fullstr();
        bool tls;
        if (it->url.Protocol() == "http") {
            tls = false;
        } else if (it->url.Protocol() == "https") {
            tls = true;
        } else {
            logger_.msg(Arc::WARNING, "unsupported protocol: %s", isis_name);
            continue;
        }
        logger_.msg(DEBUG, "Registering to %s ISIS", isis_name);
        Arc::PayloadSOAP request(ns_);
        Arc::XMLNode op = request.NewChild("isis:Register");
        
        // create header
        op.NewChild("isis:Header").NewChild("isis:SourcePath").NewChild("isis:ID") = service_->getID();
        
        // create body
        op.NewChild(reg_doc);   
        
        // send
        Arc::PayloadSOAP *response;
        mcc_cfg_.AddPrivateKey(it->key);
        mcc_cfg_.AddCertificate(it->cert);
        mcc_cfg_.AddProxy(it->proxy);
        mcc_cfg_.AddCADir(it->cadir);
        Arc::ClientSOAP cli(mcc_cfg_, it->url.Host(), it->url.Port(), tls, it->url.Path());
        Arc::MCC_Status status = cli.process(&request, &response);
        if ((!status) || (!response)) {
            logger_.msg(ERROR, "Error during registration to %s ISIS", isis_name);
            continue;
        } 
        Arc::XMLNode fault = (*response)["Fault"];
        if(!fault)  {
            logger_.msg(DEBUG, "Successful registration to ISIS (%s)", isis_name); 
        } else {
            logger_.msg(DEBUG, "Failed to register to ISIS (%s) - %s", isis_name, std::string(fault["Description"])); 
        }
    }
}

InfoRegisters::InfoRegisters(Arc::XMLNode &cfg, Arc::Service *service) {
    if(!service) return;
    NS ns;
    ns["iregc"]=REGISTRATION_CONFIG_NAMESPACE;
    cfg.Namespaces(ns);
    for(XMLNode node = cfg["iregc:InfoRegister"];(bool)node;++node) {
        registers_.push_back(new InfoRegister(node,service));
    }
}

InfoRegisters::~InfoRegisters(void) {
    for(std::list<Arc::InfoRegister*>::iterator i = registers_.begin();i!=registers_.end();++i) {
        if(*i) delete (*i);
    }
}

} // namespace


