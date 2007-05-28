#ifndef __ARC_MCCTCP_H__
#define __ARC_MCCTCP_H__

#include "../../libs/message/MCC.h"
#include "../../libs/message/PayloadStream.h"
#include "PayloadTCPSocket.h"

namespace Arc {

/** This class is MCC implementing TCP server.
  Upon creation this object binds to specified TCP ports and listens
 for incoming TCP connections on dedicated thread. Each connection is 
 accepted and dedicated thread is created. Then that thread is used 
 to call process() method of next MCC in chain. That method is passed
 payload implementing PayloadStreamInterface. On response payload
 with PayloadRawInterface is expected. Alternatively called MCC 
 may use provided PayloadStreamInterface to send it's response back 
 directly.
*/
class MCC_TCP_Service: public MCC
{
    friend class mcc_tcp_exec_t;
    private:
        class mcc_tcp_exec_t {
            public:
                MCC_TCP_Service* obj;
                int handle;
                /* pthread_t thread; */
                int id;
                mcc_tcp_exec_t(MCC_TCP_Service* o,int h);
                operator bool(void) { return (handle != -1); };
        };
        std::list<int> handles_; /** listening sockets */
        std::list<mcc_tcp_exec_t> executers_; /** active connections and associated threads */
        /* pthread_t listen_th_; ** thread listening for incoming connections */
        Glib::Mutex lock_; /** lock for safe operations in internal lists */
        static void listener(void *); /** executing function for listening thread */
        static void executer(void *); /** executing function for connection thread */
    public:
        MCC_TCP_Service(Arc::Config *cfg);
        virtual ~MCC_TCP_Service(void);
        virtual MCC_Status process(Message&,Message&);
};

/** This class is MCC implementing TCP client.
  Upon creation it connects to specified TCP post at specified host. 
 process() method ccepts PayloadRawInterface type of payload. Specified
 payload is sent over TCP socket. It returns PayloadStreamInterface 
 payload for previous MCC to read response.
*/
class MCC_TCP_Client: public MCC
{
    private:
        /** Socket object connected to remote site. 
          It contains NULL if connectino failed. */
        PayloadTCPSocket* s_; 
    public:
        MCC_TCP_Client(Arc::Config *cfg);
        virtual ~MCC_TCP_Client(void);
        virtual MCC_Status process(Message&,Message&);
};

} // namespace Arc

#endif /* __ARC_MCCTCP_H__ */
