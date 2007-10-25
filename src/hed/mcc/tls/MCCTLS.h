#ifndef __ARC_MCCTLS_H__
#define __ARC_MCCTLS_H__

#include <arc/message/MCC.h>

namespace Arc {

  //! A base class for SOAP client and service MCCs.
  /*! This is a base class for SOAP client and service MCCs. It
    provides some common functionality for them, i.e. so far only a
    logger.
   */
  class MCC_TLS : public MCC {
  public:
    MCC_TLS(Arc::Config *cfg);
  protected:
    //bool tls_random_seed(std::string filename, long n);
    bool tls_load_certificate(SSL_CTX* sslctx,
			      const std::string& cert_file,
			      const std::string& key_file,
			      const std::string& password,
			      const std::string& random_file);
    bool do_ssl_init(void);
    static bool ssl_initialized_;
    static Glib::Mutex lock_;
    static Glib::Mutex* ssl_locks_;
    static Logger logger;
    static void ssl_locking_cb(int mode, int n, const char *file, int line);
    static unsigned long ssl_id_cb(void);
    //static void* ssl_idptr_cb(void);
  };

/** This MCC implements TLS server side functionality. Upon creation this 
  object creats SSL_CTX object and configures SSL_CTX object with some environment
  information about credential. 
  Because we cannot know the "socket" when the creation of MCC_TLS_Service/MCC_TLS_Client 
  object (not like MCC_TCP_Client, which can creat socket in the constructor method 
  by using information in configuration file), we can only creat "ssl" object which is 
  binded to specified "socket", when MCC_HTTP_Client calls the process() method of 
  MCC_TLS_Client object, or MCC_TCP_Service calls the process() method of MCC_TLS_Service 
  object. The "ssl" object is embeded in a payload called PayloadTLSSocket.

  The process() method of MCC_TLS_Service is passed payload implementing 
  PayloadStreamInterface and the method returns empty PayloadRaw payload in "outmsg". 
  The ssl object is created and bound to Stream payload when constructing the PayloadTLSSocket 
  in the process() method. 

  During processing of message this MCC generates attribute TLS:PEERDN which contains
  Distinguished Name of remoote peer.
*/

class MCC_TLS_Service: public MCC_TLS
{
    private:
        SSL_CTX* sslctx_;
    public:
        MCC_TLS_Service(Arc::Config *cfg);
        virtual ~MCC_TLS_Service(void);
        virtual MCC_Status process(Message&,Message&);
};

class PayloadTLSMCC;

/** This class is MCC implementing TLS client.
*/
class MCC_TLS_Client: public MCC_TLS
{
    private:
        SSL_CTX* sslctx_; 
        PayloadTLSMCC* stream_;
    public:
        MCC_TLS_Client(Arc::Config *cfg);
        virtual ~MCC_TLS_Client(void);
        virtual MCC_Status process(Message&,Message&);
        virtual void Next(MCCInterface* next,const std::string& label = "");
};

} // namespace Arc

#endif /* __ARC_MCCTLS_H__ */
