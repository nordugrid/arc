#ifndef __ARC_DATA_H__
#define __ARC_DATA_H__

#include <stdlib.h>

// Undefined (yet) classes
class DataAuth;
class DataAttr;

// Base class for content of message paased through chain. It's not 
// intended to be used directly. Instead functional classes must be 
// derived from it.
class DataPayload {
 public:
  DataPayload(void) { };
  virtual ~DataPayload(void) { };
};

// Message. It is going to contain main content (payload), 
// authentication/authorization information, attributes, etc.
class Data {
 private:
  DataPayload& payload_;
  DataAuth& auth_;
  DataAttr& attr_;
 public:
  Data(DataPayload& payload):payload_(payload),auth_(*(DataAuth*)NULL),attr_(*(DataAttr*)NULL) { };
  ~Data(void);
  // Get current payload
  DataPayload& Payload(void) { return payload_; };
  // Update payload
  DataPayload& Payload(DataPayload& new_payload) {
   DataPayload& p = payload_;
   payload_=new_payload;
   return p;
  };
};

#endif

