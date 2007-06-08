#ifndef __ARC_PAYLOADSOAP_H__
#define __ARC_PAYLOADSOAP_H__

#include "Message.h"
#include "SOAPMessage.h"

namespace Arc {

/** This class combines MessagePayload with SOAPMessage to make 
  it possible to pass SOAP messages through MCC chain */
class PayloadSOAP: public MessagePayload, public SOAPMessage {
 public:
  /** Constructor - creates new Message payload */
  PayloadSOAP(const Arc::XMLNode::NS& ns,bool fault = false);
  /** Constructor - creates Message payload from SOAP message. 
    Used SOAP message must exist as long as created object exists. */
  PayloadSOAP(const Arc::SOAPMessage& soap);
  /** Constructor - creates SOAP message from payload.
    PayloadRawInterface and derived classes are supported. */
  PayloadSOAP(const Arc::MessagePayload& source);
  virtual ~PayloadSOAP(void);
};

} // namespace Arc

#endif /* __ARC_PAYLOADSOAP_H__ */
