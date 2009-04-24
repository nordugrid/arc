#ifndef __ARC_SOAPMESSAGE_H__
#define __ARC_SOAPMESSAGE_H__

#include <stdlib.h>

#include "SOAPEnvelope.h"
#include "Message.h"

namespace Arc {

/// Message restricted to SOAP payload
/** This is a special Message intended to be used in language bindings for
  programming languages which are not flexible enough to support all kinds of Payloads.
  It is passed through chain of MCCs and works like the Message but can carry only
  SOAP content. */
class SOAPMessage {
 private:
  SOAPEnvelope* payload_; /** Main content of message */
  MessageAuth* auth_; /** Authentication and authorization related information */
  MessageAttributes* attributes_; /** Various useful attributes */
  /** This element is maintained by MCC/element which handles/knows
    persistency of connection. It must be created and destroyed by
    that element. */
  MessageContext* context_;
  /** No copying is allowed */
  SOAPMessage(SOAPMessage& msg);
  /** No assignment is allowed. */
  SOAPMessage& operator=(SOAPMessage& msg);
 public:
  /** Dummy constructor */
  SOAPMessage(void):payload_(NULL),auth_(NULL),attributes_(NULL) { };
  /** Copy constructor. Used by language bindigs */
  SOAPMessage(long msg_ptr_addr);
  /** Copy constructor. Ensures shallow copy. */
  SOAPMessage(Message& msg);
  /** Destructor does not affect refered objects */
  ~SOAPMessage(void);
  /** Returns pointer to current payload or NULL if no payload assigned. */
  SOAPEnvelope* Payload(void);
  /** Replace payload with a COPY of new one */
  void Payload(SOAPEnvelope* new_payload);
  /** Returns a pointer to the current attributes object or NULL if no
      attributes object has been assigned. */
  MessageAttributes* Attributes(void) { return attributes_; };
  void Attributes(MessageAttributes* attributes) {
    attributes_=attributes;
  };

  MessageAuth *Auth(void) { return auth_; };
  void Auth(MessageAuth *auth) {
    auth_ = auth;
  };
  MessageContext* Context(void) { return context_; };
  void Context(MessageContext* context) {
    context_=context;
  };
};

} // namespace Arc

#endif /* __ARC_SOAPMESSAGE_H__ */

