#ifndef __ARC_MESSAGE_H__
#define __ARC_MESSAGE_H__

#include <stdlib.h>

#include "MessageAttributes.h"
#include "MessageAuth.h"

namespace Arc {

/// Base class for content of message passed through chain.
/** It's not intended to be used directly. Instead functional 
  classes must be derived from it. */
class MessagePayload {
 public:
  virtual ~MessagePayload(void) { };
};

/// Top class for elements contained in message context.
/** Objects of classes inherited with this one may be stored in
  MessageContext container. */
class MessageContextElement {
 public:
  MessageContextElement(void) { };
  virtual ~MessageContextElement(void) { };
};

/// Handler for context of message context.
/** This class is a container for objects derived from MessageContextElement.
 It gets associated with Message object usually by first MCC in a chain and 
 is kept as long as connection persists. */
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

/// Object being passed through chain of MCCs. 
/** An instance of this class refers to objects with main content (MessagePayload), 
 authentication/authorization information (MessageAuth) and common purpose attributes
 (MessageAttributes). Message class does not manage pointers to objects and their content.
 It only serves for grouping those objects.
  Message objects are supposed to be processed by MCCs and Services implementing 
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
     process() method must not rely on that object to stay intact.

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
    persistency of connection/session. It must be created and destroyed by
    that element. This object must survive during whole connectivity session -
    whatever that means. This is a place for MCCs and services to store information
    related to connection. All the other objects are only guaranteed to stay
    during single request. */
  MessageContext* context_;
 public:
  /** Dummy constructor */
  Message(void):payload_(NULL),auth_(NULL),attributes_(NULL),context_(NULL) { };
  /** Copy constructor. Ensures shallow copy. */
  Message(Message& msg):payload_(msg.payload_),auth_(msg.auth_),attributes_(msg.attributes_),context_(msg.context_) { };
  /** Copy constructor. Used by language bindigs */
  Message(long msg_ptr_addr);
  /** Destructor does not affect refered objects */
  ~Message(void) { };
  /** Assignment. Ensures shallow copy. */
  Message& operator=(Message& msg) {
    payload_=msg.payload_;
    if(msg.auth_) auth_=msg.auth_;
    if(msg.attributes_) attributes_=msg.attributes_;
    if(msg.context_) context_=msg.context_;
    return *this;
  };
  /** Returns pointer to current payload or NULL if no payload assigned. */
  MessagePayload* Payload(void) { return payload_; };
  /** Replaces payload with new one. Returns the old one. */
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
  /** Returns a pointer to the current authentication/authorization object 
    or NULL if no object has been assigned. */
  MessageAuth* Auth(void) { return auth_; };
  void Auth(MessageAuth* auth) {
    auth_ = auth;
  };
  /** Returns a pointer to the current context object or NULL if no object has 
    been assigned. Last case can happen only if first MCC in a chain is connectionless
    like one implementing UDP protocol. */
  MessageContext* Context(void) { return context_; };
  /** Assigns message context object */
  void Context(MessageContext* context) {
    context_=context;
  };
};

} // namespace Arc

#endif /* __ARC_MESSAGE_H__ */

