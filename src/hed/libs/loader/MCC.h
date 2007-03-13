#ifndef __ARC_MCC_H__
#define __ARC_MCC_H__

#include "common/ArcConfig.h"

namespace Arc {

class MCC
{
    public:
        MCC(Arc::Config *cfg) { };
        virtual ~MCC(void) { };
        virtual void request(void) { };
};

} // namespace Arc

typedef struct {
    int version;
    Arc::MCC *(*get_instance)(Arc::Config *cfg);
} mcc_descriptor;

#endif /* __ARC_MCC_H__ */
