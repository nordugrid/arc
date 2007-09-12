#ifndef __ARC_PAYLOADTLSSTREAM_H__
#define __ARC_PAYLOADTLSSTREAM_H__

#include <unistd.h>
#include <string>

#include <arc/message/Message.h>
#include <arc/message/PayloadStream.h>

#include <openssl/ssl.h>

namespace Arc {

/** Implemetation of PayloadStreamInterface for "ssl" handle. */
class PayloadTLSStream: public PayloadStreamInterface {
 protected:
  int timeout_;   /** Timeout for read/write operations */
  SSL* ssl_; 
public:
  /** Constructor. Attaches to already open handle.
    Handle is not managed by this class and must be closed by external code. */
  PayloadTLSStream(SSL* ssl=NULL);  /***************/
  /** Destructor. */
  virtual ~PayloadTLSStream(void) { };
  
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

  /**Getting peer certificate from the established ssl*/
  X509* GetPeercert(void);
};

}
#endif /* __ARC_PAYLOADTLSSTREAM_H__ */
