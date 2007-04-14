#ifndef __ARC_MCCTCP_H__
#define __ARC_MCCTCP_H__

#include "../MCC.h"
#include "../../PayloadStream.h"
#include "../../PayloadTCPSocket.h"

namespace Arc {

class MCC_TCP_Service: public MCC
{
    private:
        std::list<int> handles_;
        static void* listener(void *);
        static void* executer(void *);
    public:
        MCC_TCP_Service(Arc::Config *cfg);
        virtual ~MCC_TCP_Service(void);
        virtual MCC_Status process(Message&,Message&);
};

class MCC_TCP_Client: public MCC
{
    private:
        PayloadTCPSocket* s_;
    public:
        MCC_TCP_Client(Arc::Config *cfg);
        virtual ~MCC_TCP_Client(void);
        virtual MCC_Status process(Message&,Message&);
};

} // namespace Arc

#endif /* __ARC_MCCTCP_H__ */
