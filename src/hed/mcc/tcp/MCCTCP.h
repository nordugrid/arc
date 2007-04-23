#ifndef __ARC_MCCTCP_H__
#define __ARC_MCCTCP_H__

#include "../MCC.h"
#include "../../PayloadStream.h"
#include "PayloadTCPSocket.h"

namespace Arc {

class MCC_TCP_Service: public MCC
{
    friend class mcc_tcp_exec_t;
    private:
        class mcc_tcp_exec_t {
            public:
                MCC_TCP_Service* obj;
                int handle;
                pthread_t thread;
                mcc_tcp_exec_t(MCC_TCP_Service* o,int h);
                operator bool(void) { return (handle != -1); };
        };
        std::list<int> handles_;
        std::list<mcc_tcp_exec_t> executers_;
        pthread_t listen_th_;
        pthread_mutex_t lock_;
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
