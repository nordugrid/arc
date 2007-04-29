#ifndef __ARC_WSRP_H__
#define __ARC_WSRP_H__

#include <vector>
#include "../libs/message/SOAPMessage.h"
#include "WSRFBaseFault.h"

namespace Arc {

/** Base class for all WS-ResourceProperties structures.
  Inheriting classes implement specific WS-ResourceProperties messages and
  their properties/elements. Refer to WS-ResourceProperties specifications
  for things specific to every message. */
class WSRP: public WSRF {
 protected:
  /** set WS-ResourceProperties namespaces and default prefixes in SOAP message */
  void set_namespaces(void);
 public:
  /** Constructor - prepares object for creation of new WSRP request/response/fault */
  WSRP(bool fault = false,const std::string& action = "");
  /** Constructor - creates object out of supplied SOAP tree.
    It does not check if 'soap' represents valid WS-ResourceProperties structure.
    Actual check for validity of structure has to be done by derived class. */
  WSRP(SOAPMessage& soap,const std::string& action = "");
  ~WSRP(void) { };
};


// ==================== Faults ================================

/** Base class for all WS-ResourceProperties faults */
class WSRPBaseFault: public WSRFBaseFault {
 public:
  /** Constructor - creates object out of supplied SOAP tree. */
  WSRPBaseFault(SOAPMessage& soap);
  /** Constructor - creates new WSRP fault */
  WSRPBaseFault(const std::string& type);
  virtual ~WSRPBaseFault(void);
};

class WSRPInvalidResourcePropertyQNameFault: public WSRPBaseFault {
 public:
   WSRPInvalidResourcePropertyQNameFault(SOAPMessage& soap):WSRPBaseFault(soap) { };
   WSRPInvalidResourcePropertyQNameFault(void):WSRPBaseFault("wsrf-rp:InvalidResourcePropertyQNameFault") { };
   virtual ~WSRPInvalidResourcePropertyQNameFault(void) { };
};

/** Base class for WS-ResourceProperties faults which contain ResourcePropertyChangeFailure */
class WSRPResourcePropertyChangeFailure: public WSRPBaseFault {
 public:
  /** Constructor - creates object out of supplied SOAP tree. */
   WSRPResourcePropertyChangeFailure(SOAPMessage& soap):WSRPBaseFault(soap) { };
  /** Constructor - creates new WSRP fault */
   WSRPResourcePropertyChangeFailure(const std::string& type):WSRPBaseFault(type) { };
   virtual ~WSRPResourcePropertyChangeFailure(void) { };
   XMLNode CurrentProperties(bool create = false);
   XMLNode RequestedProperties(bool create = false);
};

class WSRPUnableToPutResourcePropertyDocumentFault: public WSRPResourcePropertyChangeFailure {
 public:
   WSRPUnableToPutResourcePropertyDocumentFault(SOAPMessage& soap):WSRPResourcePropertyChangeFailure(soap) { };
   WSRPUnableToPutResourcePropertyDocumentFault(void):WSRPResourcePropertyChangeFailure("wsrf-rp:UnableToPutResourcePropertyDocumentFault") { };
   virtual ~WSRPUnableToPutResourcePropertyDocumentFault(void) { };
};

class WSRPInvalidModificationFault: public WSRPResourcePropertyChangeFailure {
 public:
   WSRPInvalidModificationFault(SOAPMessage& soap):WSRPResourcePropertyChangeFailure(soap) { };
   WSRPInvalidModificationFault(void):WSRPResourcePropertyChangeFailure("wsrf-rp:InvalidModificationFault") { };
   virtual ~WSRPInvalidModificationFault(void) { };
};

class WSRPUnableToModifyResourcePropertyFault: public WSRPResourcePropertyChangeFailure {
 public:
   WSRPUnableToModifyResourcePropertyFault(SOAPMessage& soap):WSRPResourcePropertyChangeFailure(soap) { };
   WSRPUnableToModifyResourcePropertyFault(void):WSRPResourcePropertyChangeFailure("wsrf-rp:UnableToModifyResourcePropertyFault") { };
   virtual ~WSRPUnableToModifyResourcePropertyFault(void) { };
};

class WSRPSetResourcePropertyRequestFailedFault: public WSRPResourcePropertyChangeFailure {
 public:
   WSRPSetResourcePropertyRequestFailedFault(SOAPMessage& soap):WSRPResourcePropertyChangeFailure(soap) { };
   WSRPSetResourcePropertyRequestFailedFault(void):WSRPResourcePropertyChangeFailure("wsrf-rp:SetResourcePropertyRequestFailedFault") { };
   virtual ~WSRPSetResourcePropertyRequestFailedFault(void) { };
};

class WSRPInsertResourcePropertiesRequestFailedFault: public WSRPResourcePropertyChangeFailure {
 public:
   WSRPInsertResourcePropertiesRequestFailedFault(SOAPMessage& soap):WSRPResourcePropertyChangeFailure(soap) { };
   WSRPInsertResourcePropertiesRequestFailedFault(void):WSRPResourcePropertyChangeFailure("wsrf-rp:InsertResourcePropertiesRequestFailedFault") { };
   virtual ~WSRPInsertResourcePropertiesRequestFailedFault(void) { };
};

class WSRPUpdateResourcePropertiesRequestFailedFault: public WSRPResourcePropertyChangeFailure {
 public:
   WSRPUpdateResourcePropertiesRequestFailedFault(SOAPMessage& soap):WSRPResourcePropertyChangeFailure(soap) { };
   WSRPUpdateResourcePropertiesRequestFailedFault(void):WSRPResourcePropertyChangeFailure("wsrf-rp:UpdateResourcePropertiesRequestFailedFault") { };
   virtual ~WSRPUpdateResourcePropertiesRequestFailedFault(void) { };
};

class WSRPDeleteResourcePropertiesRequestFailedFault: public WSRPResourcePropertyChangeFailure {
 public:
   WSRPDeleteResourcePropertiesRequestFailedFault(SOAPMessage& soap):WSRPResourcePropertyChangeFailure(soap) { };
   WSRPDeleteResourcePropertiesRequestFailedFault(void):WSRPResourcePropertyChangeFailure("wsrf-rp:DeleteResourcePropertiesRequestFailedFault") { };
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
 public:
  WSRPInsertResourcePropertiesRequest(SOAPMessage& soap);
  WSRPInsertResourcePropertiesRequest(void);
  ~WSRPInsertResourcePropertiesRequest(void);
  WSRPInsertResourceProperties Property(void);
};

class WSRPInsertResourcePropertiesResponse: public WSRP {
 public:
  WSRPInsertResourcePropertiesResponse(SOAPMessage& soap);
  WSRPInsertResourcePropertiesResponse(void);
  ~WSRPInsertResourcePropertiesResponse(void);
};

// ============================================================
class WSRPUpdateResourcePropertiesRequest: public WSRP {
 public:
  WSRPUpdateResourcePropertiesRequest(SOAPMessage& soap);
  WSRPUpdateResourcePropertiesRequest(void);
  ~WSRPUpdateResourcePropertiesRequest(void);
  WSRPUpdateResourceProperties Property(void);
};

class WSRPUpdateResourcePropertiesResponse: public WSRP {
 public:
  WSRPUpdateResourcePropertiesResponse(SOAPMessage& soap);
  WSRPUpdateResourcePropertiesResponse(void);
  ~WSRPUpdateResourcePropertiesResponse(void);
};

// ============================================================
class WSRPDeleteResourcePropertiesRequest: public WSRP {
 public:
  WSRPDeleteResourcePropertiesRequest(SOAPMessage& soap);
  WSRPDeleteResourcePropertiesRequest(const std::string& name);
  WSRPDeleteResourcePropertiesRequest(void);
  ~WSRPDeleteResourcePropertiesRequest(void);
  std::string Name(void);
  void Name(const std::string& name);
};

class WSRPDeleteResourcePropertiesResponse: public WSRP {
 public:
  WSRPDeleteResourcePropertiesResponse(SOAPMessage& soap);
  WSRPDeleteResourcePropertiesResponse(void);
  ~WSRPDeleteResourcePropertiesResponse(void);
};

// ============================================================
class WSRPQueryResourcePropertiesRequest: public WSRP {
 public:
  WSRPQueryResourcePropertiesRequest(SOAPMessage& soap);
  WSRPQueryResourcePropertiesRequest(const std::string& dialect);
  WSRPQueryResourcePropertiesRequest(void);
  ~WSRPQueryResourcePropertiesRequest(void);
  std::string Dialect(void);
  void Dialect(const std::string& dialect);
  XMLNode Query(void);
};

class WSRPQueryResourcePropertiesResponse: public WSRP {
 public:
  WSRPQueryResourcePropertiesResponse(SOAPMessage& soap);
  WSRPQueryResourcePropertiesResponse(void);
  ~WSRPQueryResourcePropertiesResponse(void);
  XMLNode Properties(void);
};

// UnknownQueryExpressionDialectFaultType
// InvalidQueryExpressionFault
// QueryEvaluationErrorFault
//


// ============================================================

WSRF& CreateWSRP(SOAPMessage& soap);


} // namespace Arc

#endif /* _ARC_WSRP_H__ */

