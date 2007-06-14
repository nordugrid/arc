#ifndef __ARC_SOAPMESSAGE_H__
#define __ARC_SOAPMESSAGE_H__

#include <stdlib.h>

#include "PayloadSOAP.h"
#include "Message.h"

namespace Arc {

/** Message is passed through chain of MCCs works like the Message but use only 
 * SOAP message
 */

class SOAPMessage {
 private:
  Arc::PayloadSOAP* payload_; /** Main content of message */
  Arc::MessageAuth* auth_; /** Authentication and authorization related information */
  Arc::MessageAttributes* attributes_; /** Various useful attributes */
  /** This element is maintained by MCC/element which handles/knows
    persistency of connection. It must be created and destroyed by
    that element. */
  Arc::MessageContext* context_;
 public:
  /** Dummy constructor */
  SOAPMessage(void):payload_(NULL),auth_(NULL),attributes_(NULL) { };
  /** Copy constructor. Used by language bindigs */
  SOAPMessage(long msg_ptr_addr);
  /** Copy constructor. Ensures shallow copy. */
  SOAPMessage(SOAPMessage& msg):payload_(msg.payload_),auth_(msg.auth_),attributes_(msg.attributes_) { };
  /** Copy constructor. Ensures shallow copy. */
  SOAPMessage(Arc::Message& msg);
  /** Destructor does not affect refered objects */
  ~SOAPMessage(void) { };
  /** Assignment. Ensures shallow copy. */
  SOAPMessage& operator=(SOAPMessage& msg) { payload_=msg.payload_; auth_=msg.auth_, attributes_=msg.attributes_; };
  /** Returns pointer to current payload or NULL if no payload assigned. */
  Arc::PayloadSOAP* Payload(void) { return payload_; };
  /** Replace payload with new one */
  Arc::PayloadSOAP* Payload(Arc::PayloadSOAP* new_payload) {
    PayloadSOAP* p = payload_;
    payload_=new_payload;
    return p;
  };
  /** Returns a pointer to the current attributes object or NULL if no
      attributes object has been assigned. */
  Arc::MessageAttributes* Attributes(void) { return attributes_; };
  void Attributes(Arc::MessageAttributes* attributes) {
    attributes_=attributes;
  };

  Arc::MessageAuth *Auth(void) { return auth_; };
  void Auth(Arc::MessageAuth *auth) {
    auth_ = auth;
  };
  Arc::MessageContext* Context(void) { return context_; };
  void Context(Arc::MessageContext* context) {
    context_=context;
  };
};

} // namespace Arc

#endif /* __ARC_SOAPMESSAGE_H__ */

