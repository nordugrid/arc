#ifndef __ARC_MCCTCP_H__
#define __ARC_MCCTCP_H__

#include <arc/message/MCC.h>
#include <arc/message/PayloadStream.h>
#include "PayloadTCPSocket.h"

namespace Arc {

  //! A base class for TCP client and service MCCs.
  /*! This is a base class for TCP client and service MCCs. It
    provides some common functionality for them, i.e. so far only a
    logger.
   */
  class MCC_TCP : public MCC {
  public:
    MCC_TCP(Config *cfg);
  protected:
    static Logger logger;
    friend class PayloadTCPSocket;
  };

/** This class is MCC implementing TCP server.
  Upon creation this object binds to specified TCP ports and listens
 for incoming TCP connections on dedicated thread. Each connection is
 accepted and dedicated thread is created. Then that thread is used
 to call process() method of next MCC in chain. That method is passed
 payload implementing PayloadStreamInterface. On response payload
 with PayloadRawInterface is expected. Alternatively called MCC
 may use provided PayloadStreamInterface to send it's response back
 directly.
  During processing of request this MCC generates following attributes:
   TCP:HOST - IP address of interface to which local TCP socket is bound
   TCP:PORT - port number to which local TCP socket is bound
   TCP:REMOTEHOST - IP address from which connection is accepted
   TCP:REMOTEPORT - TCP port from which connection is accepted
   TCP:ENDPOINT - URL-like representation of remote connection - ://HOST:PORT
   ENDPOINT - global attribute equal to TCP:ENDPOINT
*/
class MCC_TCP_Service: public MCC_TCP
{
    friend class mcc_tcp_exec_t;
    private:
        class mcc_tcp_exec_t {
            public:
                MCC_TCP_Service* obj;
                int handle;
                /* pthread_t thread; */
                int id;
                bool no_delay;
                int timeout;
                mcc_tcp_exec_t(MCC_TCP_Service* o,int h,int t, bool nd = false);
                operator bool(void) { return (handle != -1); };
        };
        class mcc_tcp_handle_t {
            public:
                int handle;
                bool no_delay;
                int timeout;
                mcc_tcp_handle_t(int h, int t, bool nd = false):handle(h),no_delay(nd),timeout(t) { };
                operator int(void) { return handle; };
        };
        bool valid_;
        std::list<mcc_tcp_handle_t> handles_; /** listening sockets */
        std::list<mcc_tcp_exec_t> executers_; /** active connections and associated threads */
        int max_executers_;
        bool max_executers_drop_;
        /* pthread_t listen_th_; ** thread listening for incoming connections */
        Glib::Mutex lock_; /** lock for safe operations in internal lists */
        Glib::Cond cond_;
        static void listener(void *); /** executing function for listening thread */
        static void executer(void *); /** executing function for connection thread */
    public:
        MCC_TCP_Service(Config *cfg);
        virtual ~MCC_TCP_Service(void);
        virtual MCC_Status process(Message&,Message&);
        operator bool(void) { return valid_; };
        bool operator!(void) { return !valid_; };
};

/** This class is MCC implementing TCP client.
  Upon creation it connects to specified TCP post at specified host.
 process() method accepts PayloadRawInterface type of payload. Content
 of payload is sent over TCP socket. It returns PayloadStreamInterface
 payload for previous MCC to read response.
*/
class MCC_TCP_Client: public MCC_TCP
{
    private:
        /** Socket object connected to remote site.
          It contains NULL if connectino failed. */
        PayloadTCPSocket* s_;
    public:
        MCC_TCP_Client(Config *cfg);
        virtual ~MCC_TCP_Client(void);
        virtual MCC_Status process(Message&,Message&);
        operator bool(void) { return (s_ != NULL); };
        bool operator!(void) { return (s_ == NULL); };
};

} // namespace Arc

#endif /* __ARC_MCCTCP_H__ */
