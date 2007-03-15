#ifndef __ARC_WSRF_H__
#define __ARC_WSRF_H__

#include <vector>
#include "../SOAPMessage.h"

namespace Arc {

class WSRP {
 protected:
  SOAPMessage& soap_;
  bool allocated_;
  bool valid_;
  void set_namespaces(void);
 public:
  // Prepare for creation of new WSRP request/response
  WSRP(bool fault = false,const std::string& action = "");
  // Acquire presented SOAP tree as one of WS-RP requests/responses.
  // Actual check for validity of structure is done by derived class.
  WSRP(SOAPMessage& soap,const std::string& action = "");
  ~WSRP(void) { if(allocated_) delete &soap_; };
  // Check if instance is valid
  operator bool(void) { return valid_; };
  // Direct access to underlying SOAP element
  SOAPMessage& SOAP(void) { return soap_; };
};


// ==================== Faults ================================

class WSRPBaseFault: public WSRP {
 public:
  WSRPBaseFault(SOAPMessage& soap);
  WSRPBaseFault(void);
  virtual ~WSRPBaseFault(void);
};

class WSRPInvalidResourcePropertyQNameFault: public WSRPBaseFault {
 public:
   WSRPInvalidResourcePropertyQNameFault(SOAPMessage& soap):WSRPBaseFault(soap) { };
   WSRPInvalidResourcePropertyQNameFault(void) { };
   virtual ~WSRPInvalidResourcePropertyQNameFault(void) { };
};

class WSRPResourcePropertyChangeFailure: public WSRPBaseFault {
 public:
   WSRPResourcePropertyChangeFailure(SOAPMessage& soap):WSRPBaseFault(soap) { };
   WSRPResourcePropertyChangeFailure(void) { };
   virtual ~WSRPResourcePropertyChangeFailure(void) { };
   XMLNode CurrentProperties(bool create = false);
   XMLNode RequestedProperties(bool create = false);
};

class WSRPUnableToPutResourcePropertyDocumentFault: public WSRPResourcePropertyChangeFailure {
 public:
   WSRPUnableToPutResourcePropertyDocumentFault(SOAPMessage& soap):WSRPResourcePropertyChangeFailure(soap) { };
   WSRPUnableToPutResourcePropertyDocumentFault(void) { };
   virtual ~WSRPUnableToPutResourcePropertyDocumentFault(void) { };
};

class WSRPInvalidModificationFault: public WSRPResourcePropertyChangeFailure {
 public:
   WSRPInvalidModificationFault(SOAPMessage& soap):WSRPResourcePropertyChangeFailure(soap) { };
   WSRPInvalidModificationFault(void) { };
   virtual ~WSRPInvalidModificationFault(void) { };
};

class WSRPUnableToModifyResourcePropertyFault: public WSRPResourcePropertyChangeFailure {
 public:
   WSRPUnableToModifyResourcePropertyFault(SOAPMessage& soap):WSRPResourcePropertyChangeFailure(soap) { };
   WSRPUnableToModifyResourcePropertyFault(void) { };
   virtual ~WSRPUnableToModifyResourcePropertyFault(void) { };
};

class WSRPSetResourcePropertyRequestFailedFault: public WSRPResourcePropertyChangeFailure {
 public:
   WSRPSetResourcePropertyRequestFailedFault(SOAPMessage& soap):WSRPResourcePropertyChangeFailure(soap) { };
   WSRPSetResourcePropertyRequestFailedFault(void) { };
   virtual ~WSRPSetResourcePropertyRequestFailedFault(void) { };
};

class WSRPInsertResourcePropertiesRequestFailedFault: public WSRPResourcePropertyChangeFailure {
 public:
   WSRPInsertResourcePropertiesRequestFailedFault(SOAPMessage& soap):WSRPResourcePropertyChangeFailure(soap) { };
   WSRPInsertResourcePropertiesRequestFailedFault(void) { };
   virtual ~WSRPInsertResourcePropertiesRequestFailedFault(void) { };
};

class WSRPUpdateResourcePropertiesRequestFailedFault: public WSRPResourcePropertyChangeFailure {
 public:
   WSRPUpdateResourcePropertiesRequestFailedFault(SOAPMessage& soap):WSRPResourcePropertyChangeFailure(soap) { };
   WSRPUpdateResourcePropertiesRequestFailedFault(void) { };
   virtual ~WSRPUpdateResourcePropertiesRequestFailedFault(void) { };
};

class WSRPDeleteResourcePropertiesRequestFailedFault: public WSRPResourcePropertyChangeFailure {
 public:
   WSRPDeleteResourcePropertiesRequestFailedFault(SOAPMessage& soap):WSRPResourcePropertyChangeFailure(soap) { };
   WSRPDeleteResourcePropertiesRequestFailedFault(void) { };
   virtual ~WSRPDeleteResourcePropertiesRequestFailedFault(void) { };
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
  WSRPQueryResourcePropertiesRequest(SOAPMessage& soap);
  WSRPQueryResourcePropertiesRequest(const std::string& dialect);
  WSRPQueryResourcePropertiesRequest(void);
  ~WSRPQueryResourcePropertiesRequest(void);
  std::string Dialect(void);
  void Dialect(const std::string& dialect);
  XMLNode Query(void);
};

class WSRPQueryResourcePropertiesResponse: public WSRP {
  WSRPQueryResourcePropertiesResponse(SOAPMessage& soap);
  WSRPQueryResourcePropertiesResponse(void);
  ~WSRPQueryResourcePropertiesResponse(void);
  XMLNode Properties(void);
};

// UnknownQueryExpressionDialectFaultType
// InvalidQueryExpressionFault
// QueryEvaluationErrorFault
//
} // namespace Arc

#endif /* _ARC_WSRF_H__ */

