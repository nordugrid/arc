#ifndef __ARC_LOADER_H__
#define __ARC_LOADER_H__

#include <string>
#include <map>
#include "common/ArcConfig.h"
#include "MCCFactory.h"
/* #include "MessageChain.h" */

namespace Arc {

typedef std::map<std::string, Arc::MCCFactory *> mcc_container_t;
typedef std::map<std::string, Arc::Config *> mcc_conf_container_t; 

class Loader 
{
    private:
        Arc::Config *config;
        mcc_container_t mccs;
        // std::map<std::string, Arc::ServiceFactory &> services;
        // std::list<Arc::MessageChain &> chains;
        mcc_conf_container_t get_message_chain_components(void);
    public:
        Loader(Arc::Config *cfg);
        ~Loader(void);
        void bootstrap(void);
};

}
#endif /* __ARC_LOADER_H__ */
