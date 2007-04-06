#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "Loader.h"
#include "MCCFactory.h"
#include "ServiceFactory.h"

namespace Arc {


Loader::Loader(Config *cfg)
{
    Config empty_config;
    service_factory = new ServiceFactory(&empty_config);
    mcc_factory = new MCCFactory(&empty_config);
    //plexer_factory = new PlexerFactory(&empty_config);

    make_elements(cfg);
}

Loader::~Loader(void)
{



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

void Loader::make_elements(Config *cfg) {

    std::list<mcc_connector_t> mcc_connectors;
    std::list<plexer_connector_t> plexer_connectors;

    // 1st stage - creating all elements.
    // Configuration is parsed recursively - going deeper at ArcConfig and Chain elements
    for(int n = 0; ; ++n) {
        XMLNode cn = cfg->Child(n);
        if(!cn) break;
        Config cfg_(cn);

        if (MatchXMLName(cn, "ArcConfig")) {
            make_elements(&cfg_);
            continue;
        }

        if (MatchXMLName(cn, "Chain")) {
            make_elements(&cfg_);
            continue;
        }

        if (MatchXMLName(cn, "Component")) {
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
            MCC* mcc = mcc_factory->get_instance(name,&cfg_);
            if(!mcc) {
                std::cerr << "Component " << name << "(" << id << ") could not be created" << std::endl;
                continue;
            };
            mccs_[id]=mcc;
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
            mcc_connectors.push_back(mcc_connector);
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
            plexer_connectors.push_back(plexer_connector);
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
            Service* service = service_factory->get_instance(name,&cfg_);
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

    // 2nd stage - making links between elements.

    // Making links form MCCs
    for(std::list<mcc_connector_t>::iterator mcc = mcc_connectors.begin();
                                              mcc != mcc_connectors.end(); ++mcc) {
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
    // Making links form Plexers
    for(std::list<plexer_connector_t>::iterator plexer = plexer_connectors.begin();
                                              plexer != plexer_connectors.end(); ++plexer) {
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
}

} // namespace Arc

