#ifndef __ARC_PAYLOADSOAP_H__
#define __ARC_PAYLOADSOAP_H__

#include "Message.h"
#include "SOAPMessage.h"

// Combining MessagePayload with SOAPMessage
class PayloadSOAP: public MessagePayload, public SOAPMessage {
 public:
  // Create payload from SOAP message
  PayloadSOAP(const SOAPMessage& soap);
  // Create SOAP message from payload (PayloadRaw and derived classes
  // are supported).
  PayloadSOAP(const MessagePayload& source);
  virtual ~PayloadSOAP(void);
};

#endif /* __ARC_PAYLOADSOAP_H__ */
