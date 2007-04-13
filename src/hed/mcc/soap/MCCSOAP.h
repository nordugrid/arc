#ifndef __ARC_MCCSOAP_H__
#define __ARC_MCCSOAP_H__

#include "../MCC.h"

namespace Arc {

class MCCSOAP: public MCC
{
    private:
        MCC* next_;
    public:
        MCCSOAP(Arc::Config *cfg);
        virtual ~MCCSOAP(void);
        virtual Message process(Message);
};

} // namespace Arc

#endif /* __ARC_MCCSOAP_H__ */
