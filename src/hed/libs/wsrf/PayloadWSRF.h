#ifndef __ARC_PAYLOADWSRF_H__
#define __ARC_PAYLOADWSRF_H__

#include "../libs/message/Message.h"
#include "WSRF.h"

namespace Arc {

/// This class combines MessagePayload with WSRF.
/** It's intention is to make it possible to pass WSRF messages
  through MCC chain as one more Payload type. */
class PayloadWSRF: public MessagePayload {
 protected:
  WSRF& wsrf_;
  bool owner_;
 public:
  /** Constructor - creates Message payload from SOAP message.
    Returns invalid WSRF if SOAP does not represent WS-ResourceProperties */
  PayloadWSRF(const SOAPEnvelope& soap);
  /** Constructor - creates Message payload with acquired WSRF message.
    WSRF message will be destroyed by destructor of this object. */
  PayloadWSRF(WSRF& wsrp);
  /** Constructor - creates WSRF message from payload.
    All classes derived from SOAPEnvelope are supported. */
  PayloadWSRF(const MessagePayload& source);
  virtual ~PayloadWSRF(void);
  operator WSRF&(void) { return wsrf_; };
  operator bool(void) { return (bool)wsrf_; };
};

} // namespace Arc

#endif /* __ARC_PAYLOADWSRF_H__ */
