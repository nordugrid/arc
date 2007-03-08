#include "../SOAPMessage.h"

class WSRP {
 protected:
  SOAPMessage& soap_;
  bool allocated_;
  bool valid_;
  void set_namespaces(void);
 public:
  // Prepare for creation of new WSRP request/response
  WSRP(void):soap_(*(new SOAPMessage(XMLNode::NS()))),allocated_(true) {
    set_namespaces();
    valid_=true;
  };
  // Acquire presented SOAP tree as one of WS-RP requests/responses.
  // Actual check for validity of structure is done by derived class.
  WSRP(SOAPMessage& soap):soap_(soap),allocated_(false) {
    valid_=(bool)soap;
    set_namespaces();
  };
  ~WSRP(void) { if(allocated_) delete &soap_; };
  // Check if instance is valid
  operator bool(void) { return valid_; };
  // Direct access to underlying SOAP element
  SOAPMessage& SOAP(void) { return soap_; };
};

class WSRPGetResourcePropertyDocumentRequest: public WSRP {
 public:
  WSRPGetResourcePropertyDocumentRequest(SOAPMessage& soap);
  WSRPGetResourcePropertyDocumentRequest(void);
  ~WSRPGetResourcePropertyDocumentRequest(void);
};

class WSRPGetResourcePropertyDocumentResponse: public WSRP {
 public:
  WSRPGetResourcePropertyDocumentResponse(SOAPMessage& soap);
  WSRPGetResourcePropertyDocumentResponse(const XMLNode& prop_doc = XMLNode());
  ~WSRPGetResourcePropertyDocumentResponse(void);
};

class WSRPGetResourcePropertyRequest: public WSRP {
};

class WSRPGetResourcePropertyResponse: public WSRP {
};

class WSRPGetMultipleResourcePropertiesRequest: public WSRP {
};

class WSRPGetMultipleResourcePropertiesResponse: public WSRP {
};

class WSRPPutResourcePropertyDocumentRequest: public WSRP {
};

class WSRPPutResourcePropertyDocumentResponse: public WSRP {
};

class WSRPSetResourcePropertiesRequest: public WSRP {
};

class WSRPSetResourcePropertiesResponse: public WSRP {
};

class WSRPInsertResourcePropertiesRequest: public WSRP {
};

class WSRPInsertResourcePropertiesResponse: public WSRP {
};

class WSRPUpdateResourcePropertiesRequest: public WSRP {
};

class WSRPUpdateResourcePropertiesResponse: public WSRP {
};

class WSRPDeleteResourcePropertiesRequest: public WSRP {
};

class WSRPDeleteResourcePropertiesResponse: public WSRP {
};

class WSRPQueryResourcePropertiesRequest: public WSRP {
};

class WSRPQueryResourcePropertiesResponse: public WSRP {
};


