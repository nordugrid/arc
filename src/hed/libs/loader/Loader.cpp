#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "Loader.h"

namespace Arc {

mcc_conf_container_t Loader::get_message_chain_components(void)
{
    // Collect the names of MCCs
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
            // XXX: it is a little dirty
            ret.insert(make_pair(name, new Arc::Config(cn)));
        }
    }
    return ret;
}

Loader::Loader(Arc::Config *cfg)
{
    config = cfg;
}

Loader::~Loader(void)
{
}

void Loader::bootstrap(void)
{
    mcc_conf_container_t mcc_conf = get_message_chain_components();
    /* Create Factories */
    for (mcc_conf_container_t::iterator it = mcc_conf.begin(); 
         it != mcc_conf.end(); it++) {
         Arc::MCCFactory *mcc_factory = new Arc::MCCFactory(it->first, config);
         mccs.insert(make_pair(it->first, mcc_factory));
    }
    
    /* Load mccs and call request function of them */
    for (mcc_container_t::iterator it = mccs.begin();
         it != mccs.end(); it++) {
        std::cout << "Bootstrap: " << it->first << std::endl;
        // Get new instance of MCC object from the factory 
        MCC *mcc = it->second->get_instance(mcc_conf[it->first]);
        printf("MCC: %p\n");
        mcc->request();
    }

    /* for all MessageChainComponent in Config create MCCFactory and  
     * add to mccs map */

    /* for all MessageChain in Config create MessageChain 
     * create nexts maps for MCC initialization */
    /* Formulate chain from service side to terminator MCC */
}

}
