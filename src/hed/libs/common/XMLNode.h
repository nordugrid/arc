#ifndef __ARC_XMLNODE_H__
#define __ARC_XMLNODE_H__

#include <iostream>
#include <string>
#include <list>
#include <map>

#include <libxml/xmlmemory.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>


namespace Arc {

typedef std::map<std::string,std::string> NS;

/// Wrapper for LibXML library Tree interface. 
/** This class wraps XML Node, Document and Property/Attribute structures.
  Each instance serves as pointer to actual LibXML element and provides convenient
  (for chosen purpose) methods for manipulating it.
  This class has no special ties to LibXML library and may be easily rewritten for any XML 
  parser which provides interface similar to LibXML Tree.
  It implements only small subset of XML capabilities, which is probably enough for performing 
  most of useful actions. This class also filters out (usually) useless textual nodes which 
  are often used to make XML documents human-readable. */
  
class XMLNode {
 friend bool MatchXMLName(const XMLNode& node1,const XMLNode& node2);
 friend bool MatchXMLName(const XMLNode& node,const char* name);
 friend bool MatchXMLNamespace(const XMLNode& node1,const XMLNode& node2);
 friend bool MatchXMLNamespace(const XMLNode& node,const char* uri);

 protected:
  xmlNodePtr node_;
  /**  If true node is owned by this instance - hence released in destructor. 
    Normally that may be true only for top level node of XML document.
  */
  bool is_owner_;
  /** This variable is for future */
  bool is_temporary_;
  /** Private constructor for inherited classes
    Creates instance and links to existing LibXML structure. Acquired structure is
    not owned by class instance. If there is need to completely pass control of 
    LibXML document to then instance's is_owner_ variable has to be set to true.
  */
  XMLNode(xmlNodePtr node):node_(node),is_owner_(false),is_temporary_(false) {};
 public:
  /** Constructor of invalid node
    Created instance does not point to XML element. All methods are still allowed 
    for such instance but produce no results. */
  XMLNode(void):node_(NULL),is_owner_(false),is_temporary_(false) {};
  /** Copies existing instance.
    Underlying XML element is NOT copied. Ownership is NOT inherited. */
  XMLNode(const XMLNode& node):node_(node.node_),is_owner_(false),is_temporary_(false) {};
  /** Creates XML document structure from textual representation of XML document.
    Created structure is pointed and owned by constructed instance */
  XMLNode(const std::string& xml);
  /** Same as previous */
  XMLNode(const char* xml,int len = -1);
  /** Creates empty XML document structure with specified namespaces.
    Created XML contains only root element named 'name'.
    Created structure is pointed and owned by constructed instance */
  XMLNode(const Arc::NS& ns,const char* name);
  /** Destructor
    Also destroys underlying XML document if owned by this instance */
  ~XMLNode(void);
  /** Creates a copy of XML (sub)tree.
     If object does not represent whole document - top level document
    is created. 'new_node' becomes a pointer owning new XML document. */
  void New(XMLNode& new_node) const;
  /** Returns true if instance points to XML element - valid instance */
  operator bool(void) const { return ((node_ != NULL) && (!is_temporary_)); };
  /** Returns true if instance does not point to XML element - invalid instance */
  bool operator!(void) const { return ((node_ == NULL) || is_temporary_); };
  /** Returns XMLNode instance representing n-th child of XML element.
    If such does not exist invalid XMLNode instance is returned */
  XMLNode Child(int n = 0) const {
    if(!node_) return XMLNode();
    if(node_->type != XML_ELEMENT_NODE) return XMLNode();
    xmlNodePtr p = n<0?NULL:node_->children;
    for(;p;p=p->next) {
      if(p->type == XML_TEXT_NODE) continue;
      if((--n) < 0) break;
    };
    return XMLNode(p);
  };
  /** Returns XMLNode instance representing first child element with specified name. 
    Name may be "namespace_prefix:name" or simply "name". In last case namespace is ignored.
    If such node does not exist invalid XMLNode instance is returned */
  XMLNode operator[](const char* name) const;
  /** Similar to previous method */
  XMLNode operator[](const std::string& name) const { return operator[](name.c_str()); };
  /** Returns XMLNode instance representing n-th node in sequence of siblings of same name.
    It's main purpose is to be used to retrieve element in array of children of same name
    like  node["name"][5] */
  XMLNode operator[](int n) const;
  /** Returns number of children nodes */
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
  /** Same as operator[] **/
  //XMLNode Get(const std::string& name) const {
  //  return operator[](name.c_str());
  //};

  /** Returns name of XML node */
  std::string Name(void) const { 
    const char* name = (node_)?((node_->name)?(char*)(node_->name):""):"";
    return std::string(name);
  };
  /** Returns namespace prefix of XML node */
  std::string Prefix(void) const;
  /** Returns prefix:name of XML node */
  std::string FullName(void) const {
    return Prefix()+":"+Name();
  };
  /** Assign new name to XML node */
  void Name(const char* name);
  /** Fills argument with this instance XML subtree textual representation */
  void GetXML(std::string& xml) const;
  /** Fills argument with whole XML document textual representation */
  void GetDoc(std::string& xml) const;
  /** Returns textual content of node excluding content of children nodes */
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
  /** Sets textual content of node. All existing children nodes are discarded. */
  XMLNode& operator=(const char* content) {
    if(!node_) return *this;
    xmlNodeSetContent(node_,(xmlChar*)content);
    return *this;
  };
  /** Sets textual content of node. All existing children nodes are discarded. */
  XMLNode& operator=(const std::string& content) {
    return operator=(content.c_str());
  };
    
  /** Common set method **/
  //void Set(const std::string& content) {
  //  if (!node_) return;
  //  xmlNodeSetContent(node_, (xmlChar*)content.c_str());
  //}

  /** Make instance refer to another XML node. Ownership is not inherited. */
  XMLNode& operator=(const XMLNode& node);
  /** Returns list of all attributes of node */
  // std::list<XMLNode> Attributes(void);
  /** Returns XMLNode instance reresenting n-th attribute of node. */
  XMLNode Attribute(int n = 0) const;
  /** Returns XMLNode instance representing first attribute of node with specified by name */
  XMLNode Attribute(const char* name) const; 
  /** Creates new attribute with specified name. */
  XMLNode NewAttribute(const char* name);
  /** Returns number of attributes of node */
  int AttributesSize(void) const;
  /** Assign namespaces of XML document at point specified by this instance.
    If namespace already exists it gets new prefix. New namespaces are added.
    It is usefull to apply this method to XML being processed in order to refer to it's 
    elements by known prefix. */
  void Namespaces(const Arc::NS& namespaces);
  /** Returns prefix of specified namespace. 
    Empty string if no such namespace. */
  std::string NamespacePrefix(const char* urn);
  /** Creates new child XML element at specified position with specified name.
    Default is to put it at end of list. If global order is true position
    applies to whole set of children, otherwise only to children of same name */
  XMLNode NewChild(const std::string& name,int n = -1,bool global_order = false) {
    return NewChild(name.c_str(),n,global_order);
  };
  /** Same as previous method */
  XMLNode NewChild(const char* name,int n = -1,bool global_order = false);
  /** Link a copy of supplied XML node as child.
    Returns instance refering to new child. XML element is a copy of supplied
    one but not owned by returned instance */
  XMLNode NewChild(const XMLNode& node,int n = -1,bool global_order = false);
  /** Makes a copy of supplied node and place it to this one */
  void Replace(const XMLNode& node);
  /** Destroys underlying XML element. 
    XML element is unlinked from XML tree and destroyed.
    After this operation XMLNode instance becomes invalid */
  void Destroy(void);
  /** Uses xPath to look up the whole xml structure,
    Returns a list of XMLNode points. The xpathExpr should be like 
    "//xx:child1/" which indicates the namespace and node that you 
    would like to find; The nsList is the namespace the result should 
    belong to (e.g. xx="uri:test").
  */ 
  std::list<XMLNode> XPathLookup(const std::string& xpathExpr, const Arc::NS& nsList);

  /**Get the root node from any child node of the tree */
  XMLNode GetRoot(void);
  
  /** Do a recursive copy (properties, namespaces and children when applicable)*/
  //void Duplicate(XMLNode& new_node);
};

/** Returns true if underlying XML elements have same names */
bool MatchXMLName(const XMLNode& node1,const XMLNode& node2);

/** Returns true if 'name' matches name of 'node'. If name contains prefix it's checked too */
bool MatchXMLName(const XMLNode& node,const char* name);

/** Returns true if underlying XML elements belong to same namespaces */
bool MatchXMLNamespace(const XMLNode& node1,const XMLNode& node2);

/** Returns true if 'namespace' matches 'node's namespace. */
bool MatchXMLNamespace(const XMLNode& node,const char* uri);


} // namespace Arc 

#endif /* __ARC_XMLNODE_H__ */
