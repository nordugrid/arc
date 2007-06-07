#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "Loader.h"
#include "MCCFactory.h"
#include "ServiceFactory.h"
#include "../../../libs/common/Logger.h"

namespace Arc {

Loader::Loader(Config *cfg)
{
    // Config empty_config;
    service_factory = new ServiceFactory(cfg);
    mcc_factory = new MCCFactory(cfg);
    //plexer_factory = new PlexerFactory(&empty_config);

    context_ = new ChainContext(*this);

    make_elements(cfg);
}

Loader::~Loader(void)
{
  Arc::Logger &l = Arc::Logger::rootLogger;

  // TODO: stop any processing on those MCCs or mark them for self-destruction
  // or break links first or use semaphors in MCC destructors
  // Unlink all objects
  l.msg(Arc::DEBUG, "befor loop");
  for(mcc_container_t::iterator mcc_i = mccs_.begin();mcc_i != mccs_.end();++mcc_i) {
      l.msg(Arc::DEBUG, "mcc unlink");
      MCC* mcc = mcc_i->second; 
      if(mcc) mcc->Unlink();
  };
  for(plexer_container_t::iterator plexer_i = plexers_.begin();plexer_i != plexers_.end();++plexer_i) {
      l.msg(Arc::DEBUG, "plexer unlink");
      Plexer* plexer = plexer_i->second; 
      if(plexer) plexer->Unlink();
  };
  // Destroy all objects
  for(mcc_container_t::iterator mcc_i = mccs_.begin();mcc_i != mccs_.end();mcc_i = mccs_.begin()) {
      l.msg(Arc::DEBUG, "mcc erase");
      MCC* mcc = mcc_i->second; 
      mccs_.erase(mcc_i);
      if(mcc) delete mcc;
  };
  for(service_container_t::iterator service_i = services_.begin();service_i != services_.end();service_i = services_.begin()) {
      l.msg(Arc::DEBUG, "service erase");
      Service* service = service_i->second;
      services_.erase(service_i);
      if(service) delete service;
  };
  for(plexer_container_t::iterator plexer_i = plexers_.begin();plexer_i != plexers_.end();plexer_i = plexers_.begin()) {
      l.msg(Arc::DEBUG, "plexer erase");
      Plexer* plexer = plexer_i->second; 
      plexers_.erase(plexer_i);
      if(plexer) delete plexer;
  };
  l.msg(Arc::DEBUG, "after loops");
  if(context_) delete context_;
  l.msg(Arc::DEBUG, "after delete context");
}


class mcc_connector_t {
    public:
        Loader::mcc_container_t::iterator mcc;
        std::string name;
        std::map<std::string, std::string> nexts;
        mcc_connector_t(Loader::mcc_container_t::iterator mcc_):mcc(mcc_) { };
};

std::ostream& operator<<(std::ostream& o,const mcc_connector_t& mcc) {
    o << mcc.name;
    o << "(" << mcc.mcc->first << ")";
    return o;
}

class plexer_connector_t {
    public:
        Loader::plexer_container_t::iterator plexer;
        std::string name;
        std::map<std::string, std::string> nexts;
        plexer_connector_t(Loader::plexer_container_t::iterator plexer_):plexer(plexer_) { };
};

std::ostream& operator<<(std::ostream& o,const plexer_connector_t& plexer) {
    o << plexer.name;
    o << "(" << plexer.plexer->first << ")";
    return o;
}

class mcc_connectors_t: public std::list<mcc_connector_t> { };
class plexer_connectors_t: public std::list<plexer_connector_t> { };

static XMLNode FindElementByID(XMLNode node,const std::string& id,const std::string& name) {
    for(int n = 0; ; ++n) {
        XMLNode cn = node.Child(n);
        if(!cn) break;
        Config cfg_(cn);

        if (MatchXMLName(cn, "ArcConfig")) {
            XMLNode result = FindElementByID(cn,id,name);
            if(result) return result;
            continue;
        }

        if (MatchXMLName(cn, "Chain")) {
            XMLNode result = FindElementByID(cn,id,name);
            if(result) return result;
            continue;
        }

        if (MatchXMLName(cn, name)) {
            if((std::string)(cn.Attribute("id")) == id) return cn;
        }
    }
    return XMLNode();
}


static AuthNHandler* MakeAuthNHandler(Config* cfg,ChainContext* ctx,Loader::authn_container_t& authns,AuthNHandlerFactory* authn_factory,XMLNode node) {
    if(!node) return NULL;
    XMLNode desc_node;
    std::string refid = node.Attribute("refid");
    if(refid.empty()) {
        desc_node = node;
        refid = (std::string)(node.Attribute("id"));
        if(refid.empty()) {
            char buf[256];
            snprintf(buf,sizeof(buf)-1,"__arc_authn_%u__",authns.size());
            buf[sizeof(buf)-1]=0;
            refid=buf;
        };
    } else {
        // Maybe it's already created
        Loader::authn_container_t::iterator auth = authns.find(refid);
        if(auth != authns.end()) {
            return auth->second;
        }
        // Look for it's configuration
        desc_node=FindElementByID(*cfg,refid,"Authentication");
    }
    if(!desc_node) {
        std::cerr << "Authentication handler has no configuration" << std::endl;
        return NULL;
    }
    std::string name = desc_node.Attribute("name");
    if(name.empty()) {
        std::cerr << "Authentication handler has no name attribute defined" << std::endl;
        return NULL;
    }
    // Create new handler
    Config cfg_(desc_node);
    AuthNHandler* handler=authn_factory->get_instance(name,&cfg_,ctx);
    if(handler) authns[refid]=handler;
    return handler;
}

static AuthZHandler* MakeAuthZHandler(Config* cfg,ChainContext* ctx,Loader::authz_container_t& authzs,AuthZHandlerFactory* authz_factory,XMLNode node) {
    if(!node) return NULL;
    XMLNode desc_node;
    std::string refid = node.Attribute("refid");
    if(refid.empty()) {
        desc_node = node;
        refid = (std::string)(node.Attribute("id"));
        if(refid.empty()) {
            char buf[256];
            snprintf(buf,sizeof(buf)-1,"__arc_authz_%u__",authzs.size());
            buf[sizeof(buf)-1]=0;
            refid=buf;
        };
    } else {
        // Maybe it's already created
        Loader::authz_container_t::iterator auth = authzs.find(refid);
        if(auth != authzs.end()) {
            return auth->second;
        }
        // Look for it's configuration
        desc_node=FindElementByID(*cfg,refid,"Authentication");
    }
    if(!desc_node) {
        std::cerr << "Authentication handler has no configuration" << std::endl;
        return NULL;
    }
    std::string name = desc_node.Attribute("name");
    if(name.empty()) {
        std::cerr << "Authentication handler has no name attribute defined" << std::endl;
        return NULL;
    }
    // Create new handler
    Config cfg_(desc_node);
    AuthZHandler* handler=authz_factory->get_instance(name,&cfg_,ctx);
    if(handler) authzs[refid]=handler;
    return handler;
}


void Loader::make_elements(Config *cfg,int level,mcc_connectors_t* mcc_connectors,plexer_connectors_t* plexer_connectors) {

    if(mcc_connectors == NULL) mcc_connectors = new mcc_connectors_t;
    if(plexer_connectors == NULL) plexer_connectors = new plexer_connectors_t;
    // 1st stage - creating all elements.
    // Configuration is parsed recursively - going deeper at ArcConfig and Chain elements
    for(int n = 0; ; ++n) {
        XMLNode cn = cfg->Child(n);
        if(!cn) break;
        Config cfg_(cn);

        if (MatchXMLName(cn, "ArcConfig")) {
            make_elements(&cfg_,level+1,mcc_connectors,plexer_connectors);
            continue;
        }

        if (MatchXMLName(cn, "ModuleManager")) {
            continue;
        }

        if (MatchXMLName(cn, "Chain")) {
            make_elements(&cfg_,level+1,mcc_connectors,plexer_connectors);
            continue;
        }

        if (MatchXMLName(cn, "Plugins")) {
            std::string name = cn["Name"];
            if(name.empty()) {
                std::cerr << "Plugins element has no Name defined" << std::endl;
                continue;
            };
            std::cout << "DEBUG: " << name << std::endl;
            service_factory->load_all_instances(name);
            mcc_factory->load_all_instances(name);
            continue;
        }

        if (MatchXMLName(cn, "Component")) {
            // Create new MCC
            std::string name = cn.Attribute("name");
            if(name.empty()) {
                std::cerr << "Component has no name attribute defined" << std::endl;
                continue;
            };
            std::string id = cn.Attribute("id");
            if(id.empty()) {
                std::cerr << "Component has no id attribute defined" << std::endl;
                continue;
            };
            MCC* mcc = mcc_factory->get_instance(name,&cfg_,context_);
            if(!mcc) {
                std::cerr << "Component " << name << "(" << id << ") could not be created" << std::endl;
                continue;
            };
            mccs_[id]=mcc;
            
            // Configure security plugins
            XMLNode an;
            an=cn["Authentication"];
            for(int n = 0;;++n) {
                XMLNode can = an[n];
                if(!can) break;
                AuthNHandler* handler = 
                     MakeAuthNHandler(cfg,context_,authns_,authn_factory,can);
                if(!handler) continue; // ????
                mcc->AuthN(handler,can.Attribute("event"));
            };
            an=cn["Authorization"];
            for(int n = 0;;++n) {
                XMLNode can = an[n];
                if(!can) break;
                AuthZHandler* handler = 
                     MakeAuthZHandler(cfg,context_,authzs_,authz_factory,can);
                if(!handler) continue; // ????
                mcc->AuthZ(handler,can.Attribute("event"));
            };

            // Add to chain list
            std::string entry = cn.Attribute("entry");
            if(!entry.empty()) mccs_exposed_[entry]=mcc;
            mcc_connector_t mcc_connector(mccs_.find(id));
            for(int nn = 0;;++nn) {
                XMLNode cnn = cn["next"][nn];
                if(!cnn) break;
                std::string nid = cnn.Attribute("id");
                if(nid.empty()) {
                    std::cerr << "Component's " << name << "(" << id << ") next has no id attribute defined" << std::endl;
                    continue;
                };
                std::string label = cnn;
                mcc_connector.nexts[nid]=label;
            };
            mcc_connector.name=name;
            mcc_connectors->push_back(mcc_connector);
            std::cout << "Loaded MCC " << name << "(" << id << ")" << std::endl;
            continue;
        }

        if (MatchXMLName(cn, "Plexer")) {
            std::string name = cn.Attribute("name");
            std::string id = cn.Attribute("id");
            if(id.empty()) id="plexer";
            Plexer* plexer = new Plexer(&cfg_);
            plexers_[id]=plexer;
            plexer_connector_t plexer_connector(plexers_.find(id));
            for(int nn = 0;;++nn) {
                XMLNode cnn = cn["next"][nn];
                if(!cnn) break;
                std::string nid = cnn.Attribute("id");
                if(nid.empty()) {
                    std::cerr << "Plexer's (" << id << ") next has no id attribute defined" << std::endl;
                    continue;
                };
                std::string label = cnn;
                plexer_connector.nexts[nid]=label;
            };
            plexer_connector.name=name;
            plexer_connectors->push_back(plexer_connector);
            std::cout << "Loaded Plexer " << name << "(" << id << ")" << std::endl;
            continue;
        }

        if (MatchXMLName(cn, "Service")) {
            std::string name = cn.Attribute("name");
            if(name.empty()) {
                std::cerr << "Service has no name attribute defined" << std::endl;
                continue;
            };
            std::string id = cn.Attribute("id");
            if(id.empty()) {
                std::cerr << "Service has no id attribute defined" << std::endl;
                continue;
            };
            Service* service = service_factory->get_instance(name,&cfg_,context_);
            if(!service) {
                std::cerr << "Service " << name << "(" << id << ") could not be created" << std::endl;
                continue;
            };
            services_[id]=service;
            std::cout << "Loaded Service " << name << "(" << id << ")" << std::endl;
            continue;
        }

        std::cerr << "Unknown element \"" << cn.Name() << "\" - ignoring." << std::endl;
    }

    if(level != 0) return;

    // 2nd stage - making links between elements.

    // Making links from MCCs
    for(mcc_connectors_t::iterator mcc = mcc_connectors->begin();
                                   mcc != mcc_connectors->end(); ++mcc) {
        for(std::map<std::string, std::string>::iterator next = mcc->nexts.begin();
                                next != mcc->nexts.end();next = mcc->nexts.begin()) {
            std::string id = next->first;
            std::string label = next->second;
            mcc_container_t::iterator mcc_l = mccs_.find(id);
            if(mcc_l != mccs_.end()) {
                // Make link MCC->MCC
                mcc->mcc->second->Next(mcc_l->second,label);
                std::cout << "Linking MCC " << *mcc << " to MCC (" << id << ") under " << label << std::endl;
                mcc->nexts.erase(next); continue; 
            };
            service_container_t::iterator service_l = services_.find(id);
            if(service_l != services_.end()) {
                // Make link MCC->Service
                mcc->mcc->second->Next(service_l->second,label);
                std::cout << "Linking MCC " << *mcc << " to Service (" << id << ") under " << label << std::endl;
                mcc->nexts.erase(next); continue; 
            };
            plexer_container_t::iterator plexer_l = plexers_.find(id);
            if(plexer_l != plexers_.end()) {
                // Make link MCC->Plexer
                mcc->mcc->second->Next(plexer_l->second,label);
                std::cout << "Linking MCC " << *mcc << " to Plexer (" << id << ") under " << label << std::endl;
                mcc->nexts.erase(next); continue; 
            };

            std::cerr << "MCC " << mcc->name << "(" << mcc->mcc->first << ") - next " <<
                         label << "(" << id << ") has no target" << std::endl;
            mcc->nexts.erase(next);
        }
    }
    // Making links from Plexers
    for(plexer_connectors_t::iterator plexer = plexer_connectors->begin();
                                      plexer != plexer_connectors->end(); ++plexer) {
        for(std::map<std::string, std::string>::iterator next = plexer->nexts.begin();
                               next!=plexer->nexts.end();next = plexer->nexts.begin()) {
            std::string id = next->first;
            std::string label = next->second;
            mcc_container_t::iterator mcc_l = mccs_.find(id);
            if(mcc_l != mccs_.end()) {
                // Make link Plexer->MCC
                plexer->plexer->second->Next(mcc_l->second,label);
                std::cout << "Linking Plexer " << *plexer << " to MCC (" << id << ") under " << label << std::endl;
                plexer->nexts.erase(next); continue;
            };
            service_container_t::iterator service_l = services_.find(id);
            if(service_l != services_.end()) {
                // Make link Plexer->Service
                plexer->plexer->second->Next(service_l->second,label);
                std::cout << "Linking Plexer " << *plexer << " to Service (" << id << ") under " << label << std::endl;
                plexer->nexts.erase(next); continue;
            };
            plexer_container_t::iterator plexer_l = plexers_.find(id);
            if(plexer_l != plexers_.end()) {
                // Make link Plexer->Plexer
                plexer->plexer->second->Next(plexer_l->second,label);
                std::cout << "Linking Plexer " << *plexer << " to Plexer (" << id << ") under " << label << std::endl;
                plexer->nexts.erase(next); continue;
            };

            std::cerr << "Plexer (" << plexer->plexer->first << ") - next " <<
                         label << "(" << id << ") has no target" << std::endl;
            plexer->nexts.erase(next);
        }
    }
    if(mcc_connectors) delete mcc_connectors;
    if(plexer_connectors) delete plexer_connectors;
}

MCC* Loader::operator[](const std::string& id) {
    mcc_container_t::iterator mcc = mccs_exposed_.find(id);
    if(mcc != mccs_exposed_.end()) return mcc->second;
    return NULL;
}


} // namespace Arc

