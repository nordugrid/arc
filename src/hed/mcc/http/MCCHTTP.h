#ifndef __ARC_MCCSOAP_H__
#define __ARC_MCCSOAP_H__

#include "../MCC.h"

namespace Arc {

class MCC_HTTP_Service: public MCC
{
    public:
        MCC_HTTP_Service(Arc::Config *cfg);
        virtual ~MCC_HTTP_Service(void);
        virtual MCC_Status process(Message&,Message&);
};

class MCC_HTTP_Client: public MCC
{
    public:
        MCC_HTTP_Client(Arc::Config *cfg);
        virtual ~MCC_HTTP_Client(void);
        virtual MCC_Status process(Message&,Message&);
};

} // namespace Arc

#endif /* __ARC_MCCSOAP_H__ */
