#ifndef __ARC_PAYLOADSOAP_H__
#define __ARC_PAYLOADSOAP_H__

#include "Message.h"
#include "SOAPMessage.h"

namespace Arc {

/** This class combines MessagePayload with WSRP to make 
  it possible to pass WS-ResourceProperties messages through MCC chain */
class PayloadWSRP: public MessagePayload, public WSRP {
 public:
  /** Constructor - creates Message payload from SOAP message
    Returns invalid WSRP if SOAP does not represent WS-ResourceProperties */
  PayloadWSRP(const SOAPMessage& soap);
  /** Constructor - creates Message payload from WSRP message
  PayloadWSRP(const WSRP& wsrp);
  /** Constructor - creates WSRP message from payload.
    All classes derived from SOAPMessage are supported. */
  PayloadSOAP(const MessagePayload& source);
  virtual ~PayloadSOAP(void);
};

} // namespace Arc

#endif /* __ARC_PAYLOADSOAP_H__ */
