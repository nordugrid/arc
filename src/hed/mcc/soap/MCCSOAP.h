#ifndef __ARC_MCCSOAP_H__
#define __ARC_MCCSOAP_H__

#include "../../libs/message/MCC.h"

namespace Arc {

class MCC_SOAP_Service: public MCC
{
    public:
        MCC_SOAP_Service(Arc::Config *cfg);
        virtual ~MCC_SOAP_Service(void);
        virtual MCC_Status process(Message&,Message&);
};

class MCC_SOAP_Client: public MCC
{
    public:
        MCC_SOAP_Client(Arc::Config *cfg);
        virtual ~MCC_SOAP_Client(void);
        virtual MCC_Status process(Message&,Message&);
};

} // namespace Arc

#endif /* __ARC_MCCSOAP_H__ */
