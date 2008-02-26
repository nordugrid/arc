#ifndef __ARC_MCCSOAP_H__
#define __ARC_MCCSOAP_H__

#include <arc/message/MCC.h>

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
 into PayloadRaw and passed to next MCC. Returned payload of 
 PayloadRawInterface type is treated as body part of returning
 PayloadHTTP. Generated HTTP response is sent though stream
 passed in input payload.
  During processing of request/input message following attributes
 are generated:
   HTTP:METHOD - HTTP method e.g. GET, PUT, POST, etc.
   HTTP:ENDPOINT - URL taken from HTTP request
   ENDPOINT - global attribute equal to HTTP:ENDPOINT
   HTTP:RANGESTART - start of requested byte range
   HTTP:RANGEEND - end of requested byte range (inclusive)
   HTTP:name - all 'name' attributes of HTTP header.
  Attributes of response message of HTTP:name type are
 translated into HTTP header with corresponding 'name's.
 */
class MCC_HTTP_Service: public MCC_HTTP {
    public:
        MCC_HTTP_Service(Arc::Config *cfg);
        virtual ~MCC_HTTP_Service(void);
        virtual MCC_Status process(Message&,Message&);
};

/** This class is a client part of HTTP MCC.
  It accepts PayloadRawInterface payload and uses it as body to
 generate HTTP request. Request is passed to next MCC as 
 PayloadRawInterface type of payload. Returned PayloadStreamInterface
 payload is parsed into HTTP response and it's body is passed back to
 calling MCC as PayloadRawInerface. 
  Attributes of request/input message of type HTTP:name are 
 translated into HTTP header with corresponding 'name's. Special
 attributes HTTP:METHOD and HTTP:ENDPOINT specify method and 
 URL in HTTP request. If not present meathod and URL are taken
 from configuration.
  In output/response message following attributes are present:
   HTTP:CODE - response code of HTTP
   HTTP:REASON - reason string of HTTP response
   HTTP:name - all 'name' attributes of HTTP header.
 */

class MCC_HTTP_Client: public MCC_HTTP {
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
