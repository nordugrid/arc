#ifndef __ARC_LOADER_H__
#define __ARC_LOADER_H__

#include <string>
#include <map>
#include "common/ArcConfig.h"
#include "MCC.h"
#include "MCCFactory.h"
#include "Service.h"
#include "ServiceFactory.h"
#include "Plexer.h"

namespace Arc {




class Loader 
{
    public:
        typedef std::map<std::string, MCC *>     mcc_container_t;
        typedef std::map<std::string, Service *> service_container_t;
        typedef std::map<std::string, Plexer *>  plexer_container_t;

    private:
        ServiceFactory *service_factory;
        MCCFactory *mcc_factory;
        //PlexerFactory *plexer_factory;

        mcc_container_t     mccs_;
        service_container_t services_;
        plexer_container_t  plexers_;

        void make_elements(Config *cfg);

    public:
        Loader(Config *cfg);
        ~Loader(void);
};

}
#endif /* __ARC_LOADER_H__ */
