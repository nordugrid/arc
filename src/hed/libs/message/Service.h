#ifndef __ARC_SERVICE_H__
#define __ARC_SERVICE_H__

#include "common/ArcConfig.h"
#include "../message/MCC.h"

namespace Arc {

/** Service - last plugin in a Message Chain.
  This is virtual class which defines interface (in a future also 
 common functionality) for every Service plugin. Interface is made of 
 method process() which is called by Plexer class.
 */
class Service: public MCCInterface
{
    public:
        /** Example contructor - Server takes at least it's configuration subtree */
        Service(Arc::Config *cfg) { };
        virtual ~Service(void) { };
};

} // namespace Arc

#endif /* __ARC_SERVICE_H__ */
