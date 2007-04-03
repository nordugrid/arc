#ifndef __ARC_PLEXER_H__
#define __ARC_PLEXER_H__

#include <map>
#include <string>

#include "common/ArcConfig.h"
#include "MCC.h"

namespace Arc {

class Plexer: public MCC {
    private:
        std::map<std::string, MCC *> service_end_points;
    public:
        Plexer(Arc::Config *cfg);
        ~Plexer(void);
        void process(void);
};

} // namespace Arc

#endif /* __ARC_PLEXER_H__ */
