#ifndef __ARC_MESSAGE_H__
#define __ARC_MESSAGE_H__

#include <stdlib.h>

namespace Arc {

/** Class MessageAuth will contain authencity information, 
  authorization tokens and decisions. */
class MessageAuth;

/** Class MessageAttr will contain all attributes which 
  MCC developers find useful to expose. */
class MessageAttr;

/** Base class for content of message passed through chain. It's not 
  intended to be used directly. Instead functional classes must be 
  derived from it. */
class MessagePayload {
 public:
  MessagePayload(void) { };
  virtual ~MessagePayload(void) { };
};

/** Message is passed through chain of MCCs. 
  It is going to contain main content (payload), authentication/authorization 
  information, attributes, etc. */
class Message {
 private:
  MessagePayload* payload_; /** Main content of message */
  MessageAuth& auth_; /** Auth. related information */
  MessageAttr& attr_; /** Various useful attributes */
 public:
  Message(void):payload_(NULL),auth_(NULL),attr_(NULL) { };
  ~Message(void);
  /** Returns pointer to current payload or NULL if no payload assigned. */
  MessagePayload* Payload(void) { return payload_; };
  /** Replace payload with new one */
  MessagePayload* Payload(MessagePayload* new_payload) {
    MessagePayload* p = payload_;
    payload_=new_payload;
    return p;
  };
};

} // namespace Arc

#endif /* __ARC_MESSAGE_H__ */

