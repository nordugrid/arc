#ifndef __ARC_MESSAGE_H__
#define __ARC_MESSAGE_H__

#include <stdlib.h>

// Undefined (yet) classes
class MessageAuth;
class MessageAttr;

// Base class for content of message paased through chain. It's not 
// intended to be used directly. Instead functional classes must be 
// derived from it.
class MessagePayload {
 public:
  MessagePayload(void) { };
  virtual ~MessagePayload(void) { };
};

// Message. It is going to contain main content (payload), 
// authentication/authorization information, attributes, etc.
class Message {
 private:
  MessagePayload& payload_;
  MessageAuth& auth_;
  MessageAttr& attr_;
 public:
  Message(MessagePayload& payload):payload_(payload),auth_(*(MessageAuth*)NULL),attr_(*(MessageAttr*)NULL) { };
  ~Message(void);
  // Get current payload
  MessagePayload& Payload(void) { return payload_; };
  // Update payload
  MessagePayload& Payload(MessagePayload& new_payload) {
    MessagePayload& p = payload_;
    payload_=new_payload;
    return p;
  };
};

#endif /* __ARC_MESSAGE_H__ */

