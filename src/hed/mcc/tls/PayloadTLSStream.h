#ifndef __ARC_PAYLOADTLSSTREAM_H__
#define __ARC_PAYLOADTLSSTREAM_H__

#include <unistd.h>
#include <string>

#include <arc/message/Message.h>
#include <arc/message/PayloadStream.h>
#include <arc/Logger.h>

#include <openssl/ssl.h>

namespace Arc {

/** Implemetation of PayloadStreamInterface for SSL handle. */
class PayloadTLSStream: public PayloadStreamInterface {
 protected:
  int timeout_;   /** Timeout for read/write operations */
  SSL* ssl_; 
  Logger& logger_;
public:
  /** Constructor. Attaches to already open handle.
    Handle is not managed by this class and must be closed by external code. */
  PayloadTLSStream(Logger& logger,SSL* ssl=NULL);
  PayloadTLSStream(PayloadTLSStream& stream);
  /** Destructor. */
  virtual ~PayloadTLSStream(void);
  
  void HandleError(int code = SSL_ERROR_NONE);
  static void HandleError(Logger& logger,int code = SSL_ERROR_NONE);
  static void ClearError(void);

  virtual bool Get(char* buf,int& size);
  virtual bool Get(std::string& buf);
  virtual std::string Get(void) { std::string buf; Get(buf); return buf; };
  virtual bool Put(const char* buf,int size);
  virtual bool Put(const std::string& buf) { return Put(buf.c_str(),buf.length()); };
  virtual bool Put(const char* buf) { return Put(buf,buf?strlen(buf):0); };
  virtual operator bool(void) { return (ssl_ != NULL); };
  virtual bool operator!(void) { return (ssl_ == NULL); };
  virtual int Timeout(void) const { return timeout_; };
  virtual void Timeout(int to) { timeout_=to; };
  virtual int Pos(void) const { return 0; };

  /**Get peer certificate from the established ssl.
    Obtained X509 object is owned by this instance and becomes invalid
    after destruction. Still obtained has to be freed at end of usage. */
  X509* GetPeerCert(void);
  /** Get chain of peer certificates from the established ssl.
    Obtained X509 object is owned by this instance and becomes invalid
    after destruction. */
  STACK_OF(X509)* GetPeerChain(void);
  /** Get local certificate from associated ssl.
    Obtained X509 object is owned by this instance and becomes invalid
    after destruction. */
  X509* GetCert(void);
};

}
#endif /* __ARC_PAYLOADTLSSTREAM_H__ */
