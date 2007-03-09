#include <vector>

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

// ============================================================
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
  void Document(const XMLNode& prop_doc);
  XMLNode Document(void);
};

// ============================================================
class WSRPGetResourcePropertyRequest: public WSRP {
 public:
  WSRPGetResourcePropertyRequest(SOAPMessage& soap);
  WSRPGetResourcePropertyRequest(const std::string& name);
  ~WSRPGetResourcePropertyRequest(void);
  std::string Name(void);
  void Name(const std::string& name);
};

class WSRPGetResourcePropertyResponse: public WSRP {
 public:
  WSRPGetResourcePropertyResponse(SOAPMessage& soap);
  WSRPGetResourcePropertyResponse(void);
  //WSRPGetResourcePropertyResponse(const std::list<XMLNode>& properties);
  ~WSRPGetResourcePropertyResponse(void);
  int Size(void);
  void Property(const XMLNode& prop,int pos = -1);
  XMLNode Property(int pos);
  XMLNode Properties(void);
};

// Fault InvalidResourcePropertyQNameFault

// ============================================================
class WSRPGetMultipleResourcePropertiesRequest: public WSRP {
 public:
  WSRPGetMultipleResourcePropertiesRequest(SOAPMessage& soap);
  WSRPGetMultipleResourcePropertiesRequest(void);
  WSRPGetMultipleResourcePropertiesRequest(const std::vector<std::string>& names);
  ~WSRPGetMultipleResourcePropertiesRequest(void);
  std::vector<std::string> Names(void);
  void Names(const std::vector<std::string>& names);
};

class WSRPGetMultipleResourcePropertiesResponse: public WSRP {
 public:
  WSRPGetMultipleResourcePropertiesResponse(SOAPMessage& soap);
  WSRPGetMultipleResourcePropertiesResponse(void);
  //WSRPGetMultipleResourcePropertiesResponse(const std::list<XMLNode>& properties);
  ~WSRPGetMultipleResourcePropertiesResponse(void);
  int Size(void);
  void Property(const XMLNode& prop,int pos = -1);
  XMLNode Property(int pos);
  XMLNode Properties(void);
};

// ============================================================
class WSRPPutResourcePropertyDocumentRequest: public WSRP {
 public:
  WSRPPutResourcePropertyDocumentRequest(SOAPMessage& soap);
  WSRPPutResourcePropertyDocumentRequest(const XMLNode& prop_doc = XMLNode());
  ~WSRPPutResourcePropertyDocumentRequest(void);
  void Document(const XMLNode& prop_doc);
  XMLNode Document(void);
};

class WSRPPutResourcePropertyDocumentResponse: public WSRP {
 public:
  WSRPPutResourcePropertyDocumentResponse(SOAPMessage& soap);
  WSRPPutResourcePropertyDocumentResponse(const XMLNode& prop_doc = XMLNode());
  ~WSRPPutResourcePropertyDocumentResponse(void);
  void Document(const XMLNode& prop_doc);
  XMLNode Document(void);
};

// ResourcePropertyChangeFailure
// UnableToPutResourcePropertyDocumentFault

// ============================================================

class WSRPModifyResourceProperties {
 protected:
  XMLNode element_;
 public:
  // Create new node in XML tree or acquire element from XML tree
  WSRPModifyResourceProperties(XMLNode& node,bool create,const std::string& name = "");
  WSRPModifyResourceProperties(void) { };
  virtual ~WSRPModifyResourceProperties(void);
  //operator XMLNode(void) { return element_; };
  operator bool(void) { return (bool)element_; };
  bool operator!(void) { return !element_; };
};

class WSRPInsertResourceProperties: public WSRPModifyResourceProperties {
 public:
  WSRPInsertResourceProperties(XMLNode node,bool create):WSRPModifyResourceProperties(node,create,"wsrf-rp:Insert") { };
  WSRPInsertResourceProperties(void) { };
  virtual ~WSRPInsertResourceProperties(void);
  XMLNode Properties(void) { return element_; };
};

class WSRPUpdateResourceProperties: public WSRPModifyResourceProperties {
 public:
  WSRPUpdateResourceProperties(XMLNode node,bool create):WSRPModifyResourceProperties(node,create,"wsrf-rp:Update") { };
  WSRPUpdateResourceProperties(void) { };
  virtual ~WSRPUpdateResourceProperties(void);
  XMLNode Properties(void) { return element_; };
};

class WSRPDeleteResourceProperties: public WSRPModifyResourceProperties {
 public:
  WSRPDeleteResourceProperties(XMLNode node,bool create):WSRPModifyResourceProperties(node,create,"wsrf-rp:Delete") { };
  WSRPDeleteResourceProperties(void) { };
  virtual ~WSRPDeleteResourceProperties(void);
  std::string Property(void);  
  void Property(const std::string& name); 
};

// ============================================================
class WSRPSetResourcePropertiesRequest: public WSRP {
 public:
  WSRPSetResourcePropertiesRequest(SOAPMessage& soap);
  WSRPSetResourcePropertiesRequest(void);
  ~WSRPSetResourcePropertiesRequest(void);
  XMLNode Properties(void);
};

class WSRPSetResourcePropertiesResponse: public WSRP {
 public:
  WSRPSetResourcePropertiesResponse(SOAPMessage& soap);
  WSRPSetResourcePropertiesResponse(void);
  ~WSRPSetResourcePropertiesResponse(void);
};

// InvalidModificationFault
// UnableToModifyResourcePropertyFault
// SetResourcePropertyRequestFailedFault
// InsertResourcePropertiesRequestFailedFault
// UpdateResourcePropertiesRequestFailedFault
// DeleteResourcePropertiesRequestFailedFault


// ============================================================
class WSRPInsertResourcePropertiesRequest: public WSRP {
  WSRPInsertResourcePropertiesRequest(SOAPMessage& soap);
  WSRPInsertResourcePropertiesRequest(void);
  ~WSRPInsertResourcePropertiesRequest(void);
  WSRPInsertResourceProperties Property(void);
};

class WSRPInsertResourcePropertiesResponse: public WSRP {
  WSRPInsertResourcePropertiesResponse(SOAPMessage& soap);
  WSRPInsertResourcePropertiesResponse(void);
  ~WSRPInsertResourcePropertiesResponse(void);
};

// ============================================================
class WSRPUpdateResourcePropertiesRequest: public WSRP {
  WSRPUpdateResourcePropertiesRequest(SOAPMessage& soap);
  WSRPUpdateResourcePropertiesRequest(void);
  ~WSRPUpdateResourcePropertiesRequest(void);
  WSRPUpdateResourceProperties Property(void);
};

class WSRPUpdateResourcePropertiesResponse: public WSRP {
  WSRPUpdateResourcePropertiesResponse(SOAPMessage& soap);
  WSRPUpdateResourcePropertiesResponse(void);
  ~WSRPUpdateResourcePropertiesResponse(void);
};

// ============================================================
class WSRPDeleteResourcePropertiesRequest: public WSRP {
  WSRPDeleteResourcePropertiesRequest(SOAPMessage& soap);
  WSRPDeleteResourcePropertiesRequest(const std::string& name);
  WSRPDeleteResourcePropertiesRequest(void);
  ~WSRPDeleteResourcePropertiesRequest(void);
  std::string Name(void);
  void Name(const std::string& name);
};

class WSRPDeleteResourcePropertiesResponse: public WSRP {
  WSRPDeleteResourcePropertiesResponse(SOAPMessage& soap);
  WSRPDeleteResourcePropertiesResponse(void);
  ~WSRPDeleteResourcePropertiesResponse(void);
};

// ============================================================
class WSRPQueryResourcePropertiesRequest: public WSRP {
};

class WSRPQueryResourcePropertiesResponse: public WSRP {
};


