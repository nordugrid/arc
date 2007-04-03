#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "Loader.h"
#include "Plexer.h"

namespace Arc {


Loader::Loader(Arc::Config *cfg)
{
    config = cfg;
}

Loader::~Loader(void)
{
}

config_container_t Loader::get_mcc_confs(void)
{
    XMLNode comm = (*config)["ArcConfig"]["Communication"];
    std::map<std::string, Arc::Config *> ret;
    std::string name;
    for (int n = 0; n < comm.Size(); n++) {
        XMLNode cn = comm.Child(n);
        if (MatchXMLName(cn, "MessageChainComponent")) { 
            for (int i = 0; i < cn.AttributesSize(); i++) {
                XMLNode an = cn.Attribute(i);
                if (MatchXMLName(an, "name")) {
                    name = (std::string)cn.Attribute(i);
                }
            }
            ret.insert(make_pair(name, new Arc::Config(cn)));
        }
    }
    return ret;
}

config_container_t Loader::get_service_confs()
{
    config_container_t ret;
    XMLNode s = (*config)["ArcConfig"]["Services"];
    std::string name;
    
    for (int n = 0; n < s.Size(); n++) {
        XMLNode sn = s.Child(n);
        if (MatchXMLName(sn, "Service")) {
            for (int i = 0; i < sn.AttributesSize(); i++) {
                XMLNode an = sn.Attribute(i);
                if (MatchXMLName(an, "name")) {
                    name = (std::string)sn.Attribute(i);
                }
            }
            ret.insert(make_pair(name, new Arc::Config(sn)));
        }
    }

    return ret;
}


config_container_t Loader::get_message_chain_confs(void)
{
    config_container_t ret;
    XMLNode comm = (*config)["ArcConfig"]["Communication"];
    std::string name;

    for (int n = 0; n < comm.Size(); n++) {
        XMLNode cn = comm.Child(n);
        if (MatchXMLName(cn, "MessageChain")) {
            for (int i = 0; i < cn.AttributesSize(); i++) {
                XMLNode an = cn.Attribute(i);
                if (MatchXMLName(an, "name")) {
                    name = (std::string)cn.Attribute(i);
                }
            }
            ret.insert(make_pair(name, new Arc::Config(cn)));
        }
    }
    return ret;
}

void Loader::bootstrap(void)
{
    config_container_t::iterator it;
    /* Create MCC Factories */
    config_container_t mcc_conf = get_mcc_confs();
    for (it = mcc_conf.begin(); 
         it != mcc_conf.end(); it++) {
         Arc::MCCFactory *mcc_factory = new Arc::MCCFactory(it->first, config);
         mccs.insert(make_pair(it->first, mcc_factory));
    }
    
    /* Create services factories */
    config_container_t services_conf = get_service_confs();
    for (it = services_conf.begin(); it != services_conf.end(); it++) {
         Arc::ServiceFactory *service_factory = new Arc::ServiceFactory(it->first, config);
         services.insert(make_pair(it->first, service_factory));
    }
    


    /* Get Message chains */
    // config_container_t mcs_conf = get_message_chain_confs();
    /* Load mccs and call process function of them */
    for (mcc_container_t::iterator it = mccs.begin();
         it != mccs.end(); it++) {
        std::cout << "Bootstrap: " << it->first << std::endl;
        // Get new instance of MCC object from the factory 
        MCC *mcc = it->second->get_instance(mcc_conf[it->first]);
        printf("MCC: %p\n");
        mcc->process(Message());
    }

    /* for all MessageChainComponent in Config create MCCFactory and  
     * add to mccs map */

    /* for all MessageChain in Config create MessageChain 
     * create nexts maps for MCC initialization */
    /* Formulate chain from service side to terminator MCC */
}

}
