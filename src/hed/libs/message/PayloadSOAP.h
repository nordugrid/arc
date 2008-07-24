#ifndef __ARC_PAYLOADSOAP_H__
#define __ARC_PAYLOADSOAP_H__

#include "Message.h"
#include "SOAPEnvelope.h"

namespace Arc {

/// Payload of Message with SOAP content.
/** This class combines MessagePayload with SOAPEnvelope to make 
  it possible to pass SOAP messages through MCC chain. */
class PayloadSOAP: public Arc::SOAPEnvelope, public Arc::MessagePayload {
 public:
  /** Constructor - creates new Message payload */
  PayloadSOAP(const Arc::NS& ns,bool fault = false);
  /** Constructor - creates Message payload from SOAP document. 
    Provided SOAP document is copied to new object. */
  PayloadSOAP(const Arc::SOAPEnvelope& soap);
  /** Constructor - creates SOAP message from payload.
    PayloadRawInterface and derived classes are supported. */
  PayloadSOAP(const Arc::MessagePayload& source);
  virtual ~PayloadSOAP(void);
};

} // namespace Arc

#endif /* __ARC_PAYLOADSOAP_H__ */
