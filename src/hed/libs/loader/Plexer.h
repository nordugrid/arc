#ifndef __ARC_PLEXER_H__
#define __ARC_PLEXER_H__

#include <map>
#include <string>

#include "common/ArcConfig.h"
#include "../message/MCC.h"

namespace Arc {

/** This class routes Messages depending on their destination. */
class Plexer: public MCC {
    private:
        /** Routing table - list of endpoints and associated chains */
        std::map<std::string, MCC *> service_end_points;
    public:
        /** Constructor - it will take configuration tree and generate 
          routing table of it */
        Plexer(Arc::Config *cfg);
        ~Plexer(void);
        /** Entry point to Plexer.
           This method is called by last MCC in chain ending at Plexer. */
        void process(void);
};

} // namespace Arc

#endif /* __ARC_PLEXER_H__ */
