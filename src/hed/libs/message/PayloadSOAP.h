#ifndef __ARC_PAYLOADSOAP_H__
#define __ARC_PAYLOADSOAP_H__

#include "Message.h"
#include "SOAPEnvelop.h"

namespace Arc {

/** This class combines MessagePayload with SOAPEnvelop to make 
  it possible to pass SOAP messages through MCC chain */
class PayloadSOAP: public Arc::SOAPEnvelop, public Arc::MessagePayload {
 public:
  /** Constructor - creates new Message payload */
  PayloadSOAP(const Arc::NS& ns,bool fault = false);
  /** Constructor - creates Message payload from SOAP message. 
    Used SOAP message must exist as long as created object exists. */
  PayloadSOAP(const Arc::SOAPEnvelop& soap);
  /** Constructor - creates SOAP message from payload.
    PayloadRawInterface and derived classes are supported. */
  PayloadSOAP(const Arc::MessagePayload& source);
  virtual ~PayloadSOAP(void);
};

} // namespace Arc

#endif /* __ARC_PAYLOADSOAP_H__ */
