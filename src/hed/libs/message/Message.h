#ifndef __ARC_MESSAGE_H__
#define __ARC_MESSAGE_H__

#include <stdlib.h>

#include "MessageAttributes.h"

namespace Arc {

/** Class MessageAuth will contain authencity information, 
  authorization tokens and decisions. */
class MessageAuth;

/** Base class for content of message passed through chain. It's not 
  intended to be used directly. Instead functional classes must be 
  derived from it. */
class MessagePayload {
 public:
  virtual ~MessagePayload(void) { };
};

/** Just a top class for elements contained in context - needed
  for destruction to work. */
class MessageContextElement {
 public:
  MessageContextElement(void) { };
  virtual ~MessageContextElement(void) { };
};

/** Handler for context of message associated to persistent 
  conenction. */
class MessageContext {
 private:
  std::map<std::string,MessageContextElement*> elements_;
 public:
  MessageContext(void);
  ~MessageContext(void);
  /** Provided element is taken over by this class. It is
    remembered by it and destroyed when this class is destroyed. */
  void Add(const std::string& name,MessageContextElement* element);
  MessageContextElement* operator[](const std::string& id);
};

/** Message is passed through chain of MCCs. 
  It refers to objects with main content (payload), authentication/authorization 
 information and common purpose attributes. Message class does not manage pointers
 to objects and theur content. it only serves for grouping those objects.
  Message objects are supposed to be processed by objects' implementing 
 MCCInterface method process(). All objects constituting content of Message object 
 are subject to following policies:

  1. All objects created inside call to process() method using new command must 
     be explicitely destroyed within same call using delete command  with 
     following exceptions.
   a) Objects which are assigned to 'response' Message.
   b) Objects whose management is completely acquired by objects assigned to 
      'response' Message.

  2. All objects not created inside call to process() method are not explicitely
     destroyed within that call with following exception.
   a) Objects which are part of 'response' Method returned from call to next's 
      process() method. Unless those objects are passed further to calling
      process(), of course.

  3. It is not allowed to make 'response' point to same objects as 'request' does
     on entry to process() method. That is needed to avoid double destruction of
     same object. (Note: if in a future such need arises it may be solved by storing
     additional flags in Message object).

  4. It is allowed to change content of pointers of 'request' Message. Calling 
     process() method mus tnot rely on that object to stay intact.

  5. Called process() method should either fill 'response' Message with pointers to 
     valid objects or to keep them intact. This makes it possible for calling 
     process() to preload 'response' with valid error message.
*/
class Message {
 private:
  MessagePayload* payload_; /** Main content of message */
  MessageAuth* auth_; /** Authentication and authorization related information */
  MessageAttributes* attributes_; /** Various useful attributes */
  /** This element is maintained by MCC/element which handles/knows
    persistency of connection. It must be created and destroyed by
    that element. */
  MessageContext* context_;
 public:
  /** Dummy constructor */
  Message(void):payload_(NULL),auth_(NULL),attributes_(NULL) { };
  /** Copy constructor. Ensures shallow copy. */
  Message(Message& msg):payload_(msg.payload_),auth_(msg.auth_),attributes_(msg.attributes_) { };
  /** Destructor does not affect refered objects */
  ~Message(void) { };
  /** Assignment. Ensures shallow copy. */
  Message& operator=(Message& msg) { payload_=msg.payload_; auth_=msg.auth_, attributes_=msg.attributes_; };
  /** Returns pointer to current payload or NULL if no payload assigned. */
  MessagePayload* Payload(void) { return payload_; };
  /** Replace payload with new one */
  MessagePayload* Payload(MessagePayload* new_payload) {
    MessagePayload* p = payload_;
    payload_=new_payload;
    return p;
  };
  /** Returns a pointer to the current attributes object or NULL if no
      attributes object has been assigned. */
  MessageAttributes* Attributes(void) { return attributes_; };
  void Attributes(MessageAttributes* attributes) {
    attributes_=attributes;
  };
  MessageAuth* Auth(void) { return auth_; };
  void Auth(MessageAuth* auth) {
    auth_ = auth;
  };
  MessageContext* Context(void) { return context_; };
  void Context(MessageContext* context) {
    context_=context;
  };
};

} // namespace Arc

#endif /* __ARC_MESSAGE_H__ */

