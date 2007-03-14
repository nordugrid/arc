#ifndef __ARC_LOADER_H__
#define __ARC_LOADER_H__

#include <string>
#include <map>
#include "common/ArcConfig.h"
#include "MCCFactory.h"
#include "ServiceFactory.h"

namespace Arc {

typedef std::map<std::string, Arc::Config *> config_container_t; 
typedef std::map<std::string, Arc::MCCFactory *> mcc_container_t;
typedef std::map<std::string, Arc::ServiceFactory *> service_container_t;
        // std::list<Arc::MessageChain &> chains;

class Loader 
{
    private:
        Arc::Config *config;
        mcc_container_t mccs;
        service_container_t services;
        config_container_t get_mcc_confs(void);
        config_container_t get_service_confs(void);
        config_container_t get_message_chain_confs(void);

    public:
        Loader(Arc::Config *cfg);
        ~Loader(void);
        void bootstrap(void);
};

}
#endif /* __ARC_LOADER_H__ */
