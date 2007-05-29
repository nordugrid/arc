#ifndef __ARC_MCCTLS_H__
#define __ARC_MCCTLS_H__

#include "../../libs/message/MCC.h"

namespace Arc {

/** This two classed are MCCs implementing TLS functionality. Upon creation this 
object creats SSL_CTX object and configures SSL_CTX object with some environment
information about credential. 
Because we cannot know the "socket" when the creation of MCC_TLS_Service/MCC_TLS_Client 
object (not like MCC_TCP_Client, which can creat socket in the constructor method by using
information in configuration file), we can only creat "ssl" object which is binded to specified
"socket", when MCC_HTTP_Client calls the process() method of MCC_TLS_Client object, or MCC_TCP_Service
calls the process() method of MCC_TLS_Service object. The "ssl" object is embeded in a 
payload called PayloadTLSSocket.

The process() method of MCC_TLS_Service is passed payload implementing PayloadStreamInterface
(actually PayloadTCPSocket), and the method return empty payload in "outmsg", just as MCC_HTTP_Service does.
The ssl object is created and binded to socket object when constructing the PayloadTLSSocket in the process()
method. 

The process() method of MCC_TLS_Client is also passed payload impementing PayloadStreamInterface and return empty 
payload. 

So the MCC_TLS_Service and MCC_TLS_Client will only keep the imformation about SSL_CTX, nothing else.
It is the PayloadTLSSocket some keeps some information about ssl session. And the PayloadTLSSocket which implements
the PayloadStreamInterface will be used by PayloadHTTP.
*/

class MCC_TLS_Service: public MCC
{
    private:
        SSL_CTX* sslctx_;

    public:
        MCC_TLS_Service(Arc::Config *cfg);
        virtual ~MCC_TLS_Service(void);
        virtual MCC_Status process(Message&,Message&);
};

/** This class is MCC implementing TLS client.
 Unfortunately, the MCC_TLS_Client would be put behind MCC_TCP_Client, which looks different with server side(MCC_TLS_Server is put between 
MCC_HTTP_Server and MCC_TCP_Server).
the MCC_TLS_Client should get the socket fd and attache (I use "attache" :)) ssl to the fd, and the socket fd is created in MCC_TCP_client
and also used as s_.Put() in MCC_TCP_client to flush tcp request, the ssl attachement should be done before s_.Put() call. So I just put 
MCC_TLS_Client behind MCC_TCP_Client.
Also there PayloadTLSStream that implement PayloadStreamInterface, which is specified for TLS method, such like "SSL_read()" "SSL_write".
As Alexsandr's advice, we could replace TCP with TLS, it will be considered and done later.
*/
class MCC_TLS_Client: public MCC
{
    private:
        SSL_CTX* sslctx_; 
    public:
        MCC_TLS_Client(Arc::Config *cfg);
        virtual ~MCC_TLS_Client(void);
        virtual MCC_Status process(Message&,Message&);
};

} // namespace Arc

#endif /* __ARC_MCCTLS_H__ */
