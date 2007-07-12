#ifndef __ARC_MCCSOAP_H__
#define __ARC_MCCSOAP_H__

#include "message/MCC.h"

namespace Arc {

  //! A base class for HTTP client and service MCCs.
  /*! This is a base class for HTTP client and service MCCs. It
    provides some common functionality for them, i.e. so far only a
    logger.
   */
  class MCC_HTTP : public MCC {
  public:
    MCC_HTTP(Arc::Config *cfg);
  protected:
    static Arc::Logger logger;
  };

/** This class implements MCC to processes HTTP request.
  On input payload with PayloadStreamInterface is expected. 
  HTTP message is read from stream ans it's body is converted 
 into PayloadRaw and passed next MCC. Returned payload of 
 PayloadRawInterface type is treated as body part of returning
 PayloadHTTP. Generated HTTP response is sent though stream
 passed in input payload. */
class MCC_HTTP_Service: public MCC_HTTP
{
    public:
        MCC_HTTP_Service(Arc::Config *cfg);
        virtual ~MCC_HTTP_Service(void);
        virtual MCC_Status process(Message&,Message&);
};

/** This class is a client part of HTTP MCC.
  It accepts PayloadRawInterface payload and uses it as body to
 generate HTTP request. Request is passed to next MCC as 
 PayloadRawInterface type of payload. Returned PayloadStreamInterface
 payload is parsed into HTTP respinse and it's body is passed back to
 calling MCC. */ 
class MCC_HTTP_Client: public MCC_HTTP
{
    protected:
        std::string method_;
        std::string endpoint_;
    public:
        MCC_HTTP_Client(Arc::Config *cfg);
        virtual ~MCC_HTTP_Client(void);
        virtual MCC_Status process(Message&,Message&);
};

} // namespace Arc

#endif /* __ARC_MCCSOAP_H__ */
