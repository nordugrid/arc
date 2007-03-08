#ifndef __ARC_XMLNODE_H__
#define __ARC_XMLNODE_H__

#include <string>
#include <list>
#include <map>

#include <libxml/tree.h>
#include <libxml/parser.h>

// Wrapper for libxml library Tree interface. This class wraps 
// XML Node, Document and Property/Attribute structures.
// It implements only small subset of XML capabilities, which 
// is probably enough for performing most of useful actions.
// This class also filters out (usually) useless textual nodes.
class XMLNode {
 friend bool MatchXMLName(const XMLNode& node1,const XMLNode& node2);
 friend bool MatchXMLName(const XMLNode& node,const char* name);
 public:
  // List of namespaces
  typedef std::map<std::string,std::string> NS;
 protected:
  xmlNodePtr node_;
  // Node is owned by class - hence freed in destructor.
  bool is_owner_;
  bool is_temporary_;
  XMLNode(xmlNodePtr node):is_owner_(false),node_(node),is_temporary_(false) { };
 public:
  // Constructor of invalid node - placeholder
  XMLNode(void):is_owner_(false),node_(NULL),is_temporary_(false) { };
  // Create reference to existing node. Ownership is not inherited.
  XMLNode(const XMLNode& node):is_owner_(false),node_(node.node_),is_temporary_(false) { };
  // Create XML document structure from textual representation of XML document
  XMLNode(const std::string& xml):is_owner_(false),node_(NULL),is_temporary_(false) {
    xmlDocPtr doc = xmlReadMemory(xml.c_str(),xml.length(),NULL,NULL,0);
    if(!doc) return;
    node_=(xmlNodePtr)doc;
    is_owner_=true;
    //XMLNode::NS ns;
    //ns["xsi"]="http://www.w3.org/2001/XMLSchema-instance";
    //ns["xsd"]="http://www.w3.org/2001/XMLSchema";
    //Namespaces(ns);
  };
  // Create empty XML document structure with specified namespaces
  XMLNode(const NS& ns):is_owner_(false),node_(NULL),is_temporary_(false) {
    node_=(xmlNodePtr)xmlNewDoc((const xmlChar*)"1.0");
    if(!node_) is_owner_=true;
    Namespaces(ns);
  };
  ~XMLNode(void) {
    if(is_owner_ && node_) {
      xmlFreeDoc((xmlDocPtr)node_);
    };
  };
  // If object represents XML node
  operator bool(void) const { return ((node_ != NULL) && (!is_temporary_)); };
  bool operator!(void) const { return ((node_ == NULL) || is_temporary_); };
  // n-th node in sequence of siblings of same name
  XMLNode operator[](int n) const;
  // n-th child of node
  XMLNode Child(int n = 0) const {
    if(!node_) return XMLNode();
    xmlNodePtr p = n<0?NULL:node_->children;
    for(;p;p=p->next) {
      if(p->type == XML_TEXT_NODE) continue;
      if((--n) < 0) break;
    };
    if(p) return XMLNode(p);
    // New invalid node
    return XMLNode(p);
  };
  // First child element of node with specified name. Name
  // may be "namespace_prefix:name" or simply "name".
  XMLNode operator[](const char* name) const;
  XMLNode operator[](const std::string& name) const {
    return operator[](name.c_str());
  };
  // Number of children nodes
  int Size(void) const {
    if(!node_) return 0;
    int n = 0;
    xmlNodePtr p = node_->children;
    for(;p;p=p->next) {
      if(p->type == XML_TEXT_NODE) continue;
      ++n;
    };
    return n;
  };
  // Name of node
  std::string Name(void) const { 
    const char* name = (node_)?((node_->name)?(char*)(node_->name):""):"";
    return std::string(name);
  };
  // Assign new name to node
  void Name(std::string name) {
    if(!node_) return;
    if(node_->name) xmlFree((void*)(node_->name));
    node_->name=xmlCharStrdup(name.c_str());
  };
  // Fill variable xml with XML (sub)tree represented by this node
  void GetXML(std::string& xml) const {
    xml.resize(0);
    if(!node_) return;
    if(node_->type == XML_DOCUMENT_NODE) {
      xmlDocPtr doc = (xmlDocPtr)node_;
      xmlChar* buf = NULL;
      int bufsize = 0;
      xmlDocDumpMemory(doc,&buf,&bufsize);
      if(buf) {
        xml=(char*)buf;
        xmlFree(buf);
      };
    } else {
      xmlBufferPtr buf = xmlBufferCreate();
      if(xmlNodeDump(buf,node_->doc,node_,0,0) > 0) {
        xml=(char*)(buf->content);
        xmlBufferFree(buf);
      };
    };
  };
  /*
  void SetXML(const std::string& xml) {
    xmlDocPtr doc = NULL;
    if(node_->type == XML_DOCUMENT_NODE) {
      doc=(xmlDocPtr)node_;
    } else {
      doc=xmlNewDoc(BAD_CAST "1.0");
      xmlDocSetRootElement(doc,node_);
    };
    xmlChar* buf = NULL;
    int bufsize = 0;
    xmlDocDumpMemory(doc,&buf,&bufsize);
    if(buf) {
      xml=(char*)buf;
      xmlFree(buf);
    };
  }
*/
  // Get textual content of node excluding content of children nodes
  operator std::string(void) const {
    std::string content;
    if(!node_) return content;
    for(xmlNodePtr p = node_->children;p;p=p->next) {
      if(p->type != XML_TEXT_NODE) return "";
      xmlChar* buf = xmlNodeGetContent(p);
      if(!buf) continue;
      content+=(char*)buf;
      xmlFree(buf);
    };
    return content;
  };
  // Set textual content of node. All existing children nodes are discarded.
  XMLNode& operator=(const std::string& content) {
    return operator=(content.c_str());
  };
  XMLNode& operator=(const char* content) {
    if(!node_) return *this;
    xmlNodeSetContent(node_,(xmlChar*)content);
    return *this;
  };
  // Make object refer to another node. Ownership is preserved.
  XMLNode& operator=(const XMLNode& node) {
    node_=node.node_;
    is_owner_=false;
    is_temporary_=node.is_temporary_;
  };
  // All attributes of node 
  std::list<XMLNode> Attributes(void);
  // Get n-th attribute of node. 
  XMLNode Attribute(int n = 0);
  // Create new attribute with specified name.
  XMLNode NewAttribute(const std::string& name);
  XMLNode NewAttribute(const char* name);
  // Get first attribute of node with specified by name
  XMLNode Attribute(const std::string& name); 
  // Number of attributes of node (not implemented)
  int AttributesSize(void) { return 0; };
  // Set/modify namespaces of XML document at point specified by node
  // Matching namespaces get new prefix. New namespaces are added.
  void Namespaces(const NS& namespaces);
  // Get prefix of namespace. Returns empty string if no such namespace.
  std::string NamespacePrefix(const char* urn);
  // Create new child node at specified position. Default is to put it 
  // at end of list
  XMLNode NewChild(const std::string& name,int n = -1) {
    return NewChild(name.c_str(),n);
  };
  XMLNode NewChild(const char* name,int n = -1);
  // Make a copy of supplied node and link it as child
  XMLNode NewChild(const XMLNode& node,int n = -1);
  void Destroy(void);
};

// Compare node names with namespace prefixes
bool MatchXMLName(xmlNodePtr node1,xmlNodePtr node2);
bool MatchXMLName(xmlNodePtr node,const char* name);
bool MatchXMLName(const XMLNode& node1,const XMLNode& node2);
bool MatchXMLName(const XMLNode& node,const char* name);

#endif
