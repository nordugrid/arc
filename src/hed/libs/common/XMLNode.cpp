#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "XMLNode.h"
#include <iostream>
#include <fstream>

namespace Arc {

static xmlNsPtr GetNamespace(xmlNodePtr node) {
  xmlNsPtr ns_ = NULL;
  if(node->type == XML_ELEMENT_NODE) {
    ns_=node->ns;
  } else if(node->type == XML_ATTRIBUTE_NODE) {
    ns_=((xmlAttrPtr)node)->ns;
  };
  if(ns_) return ns_;
  if(node->parent) return GetNamespace(node->parent);
  return NULL;
}

static bool MatchXMLName(xmlNodePtr node1,xmlNodePtr node2) {
  if(strcmp((char*)(node1->name),(char*)(node2->name)) != 0) return false;
  if(node1->type != node2->type) return false;
  if((node1->type != XML_ELEMENT_NODE) && (node1->type != XML_ATTRIBUTE_NODE)) return true;
  xmlNsPtr ns1 = GetNamespace(node1);
  xmlNsPtr ns2 = GetNamespace(node2);
  if(ns1 != ns2) return false;
  return true;
}

static bool MatchXMLName(xmlNodePtr node,const char* name) {
  if(name == NULL) return false;
  const char* name_ = strchr(name,':');
  if(name_ == NULL) { name_=name; } else { ++name_; };
  if(strcmp(name_,(char*)(node->name)) != 0) return false;
  if(name_ == name) return true;
  xmlNsPtr ns_ = GetNamespace(node);
  std::string ns(name,name_-name-1);
  if((ns_ == NULL) || (ns_->prefix == NULL)) return ns.empty();
  return (ns == (const char*)(ns_->prefix));
}

static bool MatchXMLNamespace(xmlNodePtr node1,xmlNodePtr node2) {
  if(node1->type != node2->type) return false;
  if((node1->type != XML_ELEMENT_NODE) && (node1->type != XML_ATTRIBUTE_NODE)) return false;
  xmlNsPtr ns1 = GetNamespace(node1);
  xmlNsPtr ns2 = GetNamespace(node2);
  return (ns1 == ns2);
}

static bool MatchXMLNamespace(xmlNodePtr node,const char* uri) {
  if(uri == NULL) return false;
  xmlNsPtr ns_ = GetNamespace(node);
  if((ns_ == NULL) || (ns_->href == NULL)) return false;
  return (strcmp(uri,(const char*)(ns_->href)) == 0);
}

bool MatchXMLName(const XMLNode& node1,const XMLNode& node2) {
  return MatchXMLName(node1.node_,node2.node_);
}

bool MatchXMLName(const XMLNode& node,const char* name) {
  return MatchXMLName(node.node_,name);
}

bool MatchXMLName(const XMLNode& node,const std::string& name) {
  return MatchXMLName(node.node_,name.c_str());
}

bool MatchXMLNamespace(const XMLNode& node1,const XMLNode& node2) {
  return MatchXMLNamespace(node1.node_,node2.node_);
}

bool MatchXMLNamespace(const XMLNode& node,const char* uri) {
  return MatchXMLNamespace(node.node_,uri);
}

bool MatchXMLNamespace(const XMLNode& node,const std::string& uri) {
  return MatchXMLNamespace(node.node_,uri.c_str());
}

static void ReplaceNamespace(xmlNsPtr ns,xmlNodePtr node,xmlNsPtr new_ns) {
  if(node->type == XML_ELEMENT_NODE) {
    if(node->ns == ns) node->ns = new_ns;
  } else if(node->type == XML_ATTRIBUTE_NODE) {
    if(((xmlAttrPtr)node)->ns == ns) ((xmlAttrPtr)node)->ns = new_ns;
  } else {
    return;
  };
  for(xmlNodePtr node_ = node->children;node_;node_=node_->next) {
    ReplaceNamespace(ns,node_,new_ns);
  };
}

static void ReassignNamespace(xmlNsPtr ns,xmlNodePtr node) {
  xmlNsPtr ns_cur = node->nsDef;
  xmlNsPtr ns_prev = NULL;
  for(;ns_cur;) {
    if(ns == ns_cur) { ns_prev=ns_cur; ns_cur=ns_cur->next; continue; };
    if(ns->href && ns_cur->href && (xmlStrcmp(ns->href,ns_cur->href) == 0)) {
      // Unlinking namespace from tree
      ReplaceNamespace(ns_cur,node,ns);
      if(ns_prev) { ns_prev->next=ns_cur->next; } else { node->nsDef=ns_cur->next; };
      xmlNsPtr ns_tmp = ns_cur;
      ns_cur=ns_cur->next;
      xmlFreeNs(ns_tmp);
      continue;
    };
    ns_prev=ns_cur; ns_cur=ns_cur->next;
  };
  for(xmlNodePtr node_ = node->children;node_;node_=node_->next) {
    ReassignNamespace(ns,node_);
  };
}

static void SetNamespaces(const NS& namespaces,xmlNodePtr node_) {
  for(Arc::NS::const_iterator ns = namespaces.begin();
         ns!=namespaces.end();++ns) {
    // First check maybe this namespace is already defined
    xmlNsPtr ns_ = xmlSearchNsByHref(node_->doc,node_,(const xmlChar*)(ns->second.c_str()));
    if(ns_) {
      const char* prefix = (const char*)(ns_->prefix);
      if(!prefix) prefix="";
      if(ns->first == prefix) { 
        // Same namespace with same prefix - doing nothing
      } else {
        // TODO: optional change of prefix
        ns_=NULL;
      };
    };
    if(!ns_) {
      // New namespace needed
      ns_=xmlNewNs(node_,(const xmlChar*)(ns->second.c_str()),(const xmlChar*)(ns->first.c_str()));
      if(ns_ == NULL) {
        // There is already namespace with same prefix (or some other error)
        // TODO: optional change of prefix
        return;
      };
    };
    // Go through all children removing same namespaces and reassigning elements to this one.
    ReassignNamespace(ns_,node_);
  };
}

static void GetNamespaces(NS& namespaces,xmlNodePtr node_) {
  if(node_ == NULL) return;
  if(node_->type != XML_ELEMENT_NODE) return;
  // TODO: Check for duplicate prefixes
  xmlNsPtr ns = node_->nsDef;
  for(;ns;ns=ns->next) {
    std::string prefix = ns->prefix?(char*)(ns->prefix):"";
    if(ns->href) if(namespaces[prefix].empty()) namespaces[prefix]=(char*)(ns->href);
  };
  GetNamespaces(namespaces,node_->parent);
}

XMLNode::XMLNode(const std::string& xml):node_(NULL),is_owner_(false),is_temporary_(false) {
  xmlDocPtr doc = xmlParseMemory((char*)(xml.c_str()),xml.length());
  if(!doc) return;
  if(!doc->children) { xmlFreeDoc(doc); return; };
  node_=doc->children;
  is_owner_=true;
}

XMLNode::XMLNode(const char* xml,int len):node_(NULL),is_owner_(false),is_temporary_(false) {
  if(!xml) return;
  if(len == -1) len=strlen(xml);
  xmlDocPtr doc = xmlParseMemory((char*)xml,len);
  if(!doc) return;
  if(!doc->children) { xmlFreeDoc(doc); return; };
  node_=doc->children;
  is_owner_=true;
}

XMLNode::XMLNode(const Arc::NS& ns,const char* name):node_(NULL),is_owner_(false),is_temporary_(false) {
  xmlDocPtr doc = xmlNewDoc((const xmlChar*)"1.0");
  if(!doc) return;
  if(name == NULL) name="";
  const char* name_ = strchr(name,':');
  std::string node_ns_;
  if(name_ != NULL) {
    node_ns_.assign(name,name_-name);
    ++name_;
  } else {
    name_=name;
  };
  xmlNodePtr new_node = xmlNewNode(NULL,(const xmlChar*)name_);
  if(new_node == NULL) { xmlFreeDoc(doc); return; };
  xmlDocSetRootElement(doc,new_node);
  node_=new_node; is_owner_=true;
  SetNamespaces(ns,node_);
  node_->ns=xmlSearchNs(node_->doc,node_,(const xmlChar*)(node_ns_.c_str()));
}

XMLNode::~XMLNode(void) {
  if(is_owner_ && node_) {
    xmlFreeDoc(node_->doc);
  };
}

XMLNode XMLNode::operator[](int n) const {
  if(!node_) return XMLNode();
  xmlNodePtr p = n<0?NULL:node_;
  for(;p;p=p->next) {
    if((p->type != XML_ELEMENT_NODE) &&
       (p->type != XML_ATTRIBUTE_NODE)) continue;
    if(node_->name) {
      if(!(p->name)) continue;
      if(!MatchXMLName(node_,p)) continue;
    };
    if((--n) < 0) break;
  };
  return XMLNode(p);
}

XMLNode XMLNode::operator[](const char* name) const {
  if(!node_) return XMLNode();
  if((node_->type != XML_ELEMENT_NODE) &&
     (node_->type != XML_ATTRIBUTE_NODE)) return XMLNode();
  xmlNodePtr p = node_->children;
  for(;p;p=p->next) {
    if((p->type != XML_ELEMENT_NODE) &&
       (p->type != XML_ATTRIBUTE_NODE)) continue;
    if(MatchXMLName(p,name)) break;
  };
  return XMLNode(p);
}

void XMLNode::operator++(void) {
  if(!node_) return;
  if(is_owner_) { // top node has no siblings
    xmlFreeDoc(node_->doc);
    node_=NULL; is_owner_=false;
    return;
  };
  xmlNodePtr p = node_->next;
  for(;p;p=p->next) {
    if(node_->type != p->type) continue;
    if(node_->name) {
      if(!(p->name)) continue;
      if(!MatchXMLName(node_,p)) continue;
    };
    break;
  };
  node_=p;
}

void XMLNode::operator--(void) {
  if(!node_) return;
  if(is_owner_) { // top node has no siblings
    xmlFreeDoc(node_->doc);
    node_=NULL; is_owner_=false;
    return;
  };
  xmlNodePtr p = node_->prev;
  for(;p;p=p->prev) {
    if(node_->type != p->type) continue;
    if(node_->name) {
      if(!(p->name)) continue;
      if(!MatchXMLName(node_,p)) continue;
    };
    break;
  };
  node_=p;
}

int XMLNode::Size(void) const {
  if(!node_) return 0;
  int n = 0;
  xmlNodePtr p = node_->children;
  for(;p;p=p->next) {
    if(p->type != XML_ELEMENT_NODE) continue;
    ++n;
  };
  return n;
}

std::string XMLNode::Name(void) const {
  const char* name = (node_)?((node_->name)?(char*)(node_->name):""):"";
  return std::string(name);
}

int XMLNode::AttributesSize(void) const {
    if(!node_) return 0;
    if(node_->type != XML_ELEMENT_NODE) return 0;
    int n = 0;
    xmlAttrPtr p = node_->properties;
    for(;p;p=p->next) {
      if(p->type != XML_ATTRIBUTE_NODE) continue;
      ++n;
    };
    return n;
}

XMLNode XMLNode::Attribute(int n) const {
  if(!node_) return XMLNode();
  if(node_->type != XML_ELEMENT_NODE) return XMLNode();
  xmlAttrPtr p = n<0?NULL:node_->properties;
  for(;p;p=p->next) {
    if(p->type != XML_ATTRIBUTE_NODE) continue;
    if((--n) < 0) break;
  };
  return XMLNode((xmlNodePtr)p);
}

XMLNode XMLNode::Attribute(const char* name) const {
  if(!node_) return XMLNode();
  if(node_->type != XML_ELEMENT_NODE) return XMLNode();
  xmlNodePtr p = (xmlNodePtr)(node_->properties);
  for(;p;p=p->next) {
    if(p->type != XML_ATTRIBUTE_NODE) continue;
    if(MatchXMLName(p,name)) break;
  };
  if(p) return XMLNode(p);
  // New temporary node
  return XMLNode(p);
}

XMLNode XMLNode::NewAttribute(const char* name) {
  if(!node_) return XMLNode();
  if(node_->type != XML_ELEMENT_NODE) return XMLNode();
  const char* name_ = strchr(name,':');
  xmlNsPtr ns = NULL;
  if(name_ != NULL) {
    std::string ns_(name,name_-name);
    ns=xmlSearchNs(node_->doc,node_,(const xmlChar*)(ns_.c_str()));
    ++name_;
  } else {
    name_=name;
  };
  return XMLNode((xmlNodePtr)xmlNewNsProp(node_,ns,(const xmlChar*)name_,NULL));
}

std::string XMLNode::Prefix(void) const {
  if(!node_) return "";
  xmlNsPtr ns = GetNamespace(node_);
  if(!ns) return "";
  if(!(ns->prefix)) return "";
  return (const char*)(ns->prefix);
}

void XMLNode::Name(const char* name) {
  if(!node_) return;
  const char* name_ = strchr(name,':');
  xmlNsPtr ns = NULL;
  if(name_ != NULL) {
    std::string ns_(name,name_-name);
    // ns element is located at same place in Node and Attr elements
    ns=xmlSearchNs(node_->doc,node_,(const xmlChar*)(ns_.c_str()));
    ++name_;
  } else {
    name_=name;
  };
  xmlNodeSetName(node_,(const xmlChar*)name_);
  if(ns) node_->ns=ns;
}

XMLNode XMLNode::Child(int n) const {
  if(!node_) return XMLNode();
  if(node_->type != XML_ELEMENT_NODE) return XMLNode();
  xmlNodePtr p = n<0?NULL:node_->children;
  for(;p;p=p->next) {
    if(p->type != XML_ELEMENT_NODE) continue;
    if((--n) < 0) break;
  };
  return XMLNode(p);
}

XMLNode::operator std::string(void) const {
  std::string content;
  if(!node_) return content;
  for(xmlNodePtr p = node_->children;p;p=p->next) {
    if(p->type != XML_TEXT_NODE) continue;
    xmlChar* buf = xmlNodeGetContent(p);
    if(!buf) continue;
    content+=(char*)buf;
    xmlFree(buf);
  };
  return content;
}

XMLNode& XMLNode::operator=(const char* content) {
  if(!node_) return *this;
  xmlNodeSetContent(node_,(xmlChar*)content);
  return *this;
}


XMLNode XMLNode::NewChild(const char* name,const NS& namespaces,int n,bool global_order) {
  XMLNode x = NewChild("",n,global_order); // placeholder
  x.Namespaces(namespaces);
  x.Name(name);
  return x;
}

XMLNode XMLNode::NewChild(const char* name,int n,bool global_order) {
  if(node_ == NULL) return XMLNode();
  if(node_->type != XML_ELEMENT_NODE) return XMLNode();
  const char* name_ = strchr(name,':');
  xmlNsPtr ns = NULL;
  if(name_ != NULL) {
    std::string ns_(name,name_-name);
    ns=xmlSearchNs(node_->doc,node_,(const xmlChar*)(ns_.c_str()));
    ++name_;
  } else {
    name_=name;
  };
  xmlNodePtr new_node = xmlNewNode(ns,(const xmlChar*)name_);
  if(new_node == NULL) return XMLNode();
  if(n < 0) return XMLNode(xmlAddChild(node_,new_node));
  XMLNode old_node = global_order?Child(n):operator[](name)[n];
  if(!old_node) {
    // TODO: find last old_node
    return XMLNode(xmlAddChild(node_,new_node));
  };
  if(old_node) {
    return XMLNode(xmlAddPrevSibling(old_node.node_,new_node));
  };
  return XMLNode(xmlAddChild(node_,new_node));
}

XMLNode XMLNode::NewChild(const XMLNode& node,int n,bool global_order) {
  if(node_ == NULL) return XMLNode();
  if(node.node_ == NULL) return XMLNode();
  if(node_->type != XML_ELEMENT_NODE) return XMLNode();
  // TODO: Add new attribute if 'node' is attribute
  if(node.node_->type != XML_ELEMENT_NODE) return XMLNode();
  xmlNodePtr new_node = xmlDocCopyNode(node.node_,node_->doc,1);
  if(new_node == NULL) return XMLNode();
  if(n < 0) return XMLNode(xmlAddChild(node_,new_node));
  std::string name;
  xmlNsPtr ns = GetNamespace(new_node);
  if(ns != NULL) {
    if(ns->prefix != NULL) name=(char*)ns->prefix;
    name+=":";
  };
  if(new_node->name) name+=(char*)(new_node->name);
  XMLNode old_node = global_order?Child(n):operator[](name)[n];
  if(!old_node) {
    // TODO: find last old_node
    return XMLNode(xmlAddChild(node_,new_node));
  };
  if(old_node) {
    return XMLNode(xmlAddPrevSibling(old_node.node_,new_node));
  };
  return XMLNode(xmlAddChild(node_,new_node));
}

void XMLNode::Replace(const XMLNode& node) {
  if(node_ == NULL) return;
  if(node.node_ == NULL) return;
  if(node_->type != XML_ELEMENT_NODE) return;
  if(node.node_->type != XML_ELEMENT_NODE) return;
  xmlNodePtr new_node = xmlDocCopyNode(node.node_,node_->doc,1);
  if(new_node == NULL) return;
  xmlReplaceNode(node_,new_node);
  xmlFreeNode(node_); node_=new_node;
  return;
}

void XMLNode::New(XMLNode& new_node) const {
  if(new_node.is_owner_ && new_node.node_) {
    xmlFreeDoc(new_node.node_->doc);
  };
  new_node.is_owner_=false; new_node.node_=NULL;
  if(node_ == NULL) return;
  // TODO: Copy attribute node too
  if(node_->type != XML_ELEMENT_NODE) return;
  xmlDocPtr doc = xmlNewDoc((const xmlChar*)"1.0");
  if(doc == NULL) return;
  new_node.node_=xmlDocCopyNode(node_,doc,1);
  if(new_node.node_ == NULL) return;
  xmlDocSetRootElement(doc,new_node.node_);
  new_node.is_owner_=true;
  return;
}

void XMLNode::Namespaces(const Arc::NS& namespaces) {
  if(node_ == NULL) return;
  if(node_->type != XML_ELEMENT_NODE) return;
  SetNamespaces(namespaces,node_);
}

NS XMLNode::Namespaces(void) {
  NS namespaces;
  if(node_ == NULL) return namespaces;
  if(node_->type != XML_ELEMENT_NODE) return namespaces;
  GetNamespaces(namespaces,node_);
  return namespaces;
}

std::string XMLNode::NamespacePrefix(const char* urn) {
  if(node_ == NULL) return "";
  xmlNsPtr ns_ = xmlSearchNsByHref(node_->doc,node_,(const xmlChar*)urn);
  if(!ns_) return "";
  return (char*)(ns_->prefix);
}

void XMLNode::Destroy(void) {
  if(node_ == NULL) return;
  if(is_owner_) {
    xmlFreeDoc(node_->doc);
    node_=NULL; is_owner_=false; return;
  };
  if(node_->type == XML_ELEMENT_NODE) {
    xmlUnlinkNode(node_);
    xmlFreeNode(node_);
    node_=NULL; return;
  };
  if(node_->type == XML_ATTRIBUTE_NODE) {
    xmlRemoveProp((xmlAttrPtr)node_);
    node_=NULL; return;
  };
}

XMLNodeList XMLNode::XPathLookup(const std::string& xpathExpr, const Arc::NS& nsList) {
  std::list<XMLNode> retlist;
  if(node_ == NULL) return retlist;
  if(node_->type != XML_ELEMENT_NODE) return retlist;
  xmlDocPtr doc = node_->doc;
  if(doc == NULL) return retlist;
  xmlXPathContextPtr xpathCtx=xmlXPathNewContext(doc);

  for(Arc::NS::const_iterator ns = nsList.begin(); ns!=nsList.end(); ++ns) 
    xmlXPathRegisterNs(xpathCtx, (xmlChar*)ns->first.c_str(), (xmlChar*)ns->second.c_str());

  xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((const xmlChar*)(xpathExpr.c_str()),xpathCtx);

  if (xpathObj && xpathObj->nodesetval && xpathObj->nodesetval->nodeNr) {
    xmlNodeSetPtr nodes=xpathObj->nodesetval;
    int size = nodes->nodeNr;
    for(int i = 0; i < size; ++i) {
      if(nodes->nodeTab[i]->type == XML_ELEMENT_NODE){
        xmlNodePtr cur = nodes->nodeTab[i];
        xmlNodePtr parent = cur;
        for(;parent;parent=parent->parent) if(parent == node_) break;
        if(parent) retlist.push_back(XMLNode(cur));
      }
    }
  }
 
  xmlXPathFreeObject(xpathObj);
  xmlXPathFreeContext(xpathCtx); 

  return retlist;
}

XMLNode XMLNode::GetRoot(void) {
  if(node_ == NULL) return XMLNode();
  xmlDocPtr doc = node_->doc;
  if(doc == NULL) return XMLNode();
  return XMLNode(doc->children);
}

XMLNode& XMLNode::operator=(const XMLNode& node) {
  if(is_owner_ && node_) {
    xmlDocPtr doc = node_->doc;
    if(doc != NULL) xmlFreeDoc(doc);
  };
  node_=node.node_;
  is_owner_=false;
  is_temporary_=node.is_temporary_;
  return *this;
}

void XMLNode::GetDoc(std::string& out_xml_str,bool user_friendly) const {
  out_xml_str.resize(0);
  if(!node_) return;
  xmlDocPtr doc = node_->doc;
  if(doc == NULL) return;
  xmlChar* buf = NULL;
  int bufsize = 0;
  if(user_friendly) {
    xmlDocDumpFormatMemory(doc,&buf,&bufsize,1);
  } else {
    xmlDocDumpMemory(doc,&buf,&bufsize);
  };
  if(buf) {
    out_xml_str=(char*)buf;
    xmlFree(buf);
  };
}

void XMLNode::GetXML(std::string& out_xml_str,bool user_friendly) const {
  out_xml_str.resize(0);
  if(!node_) return;
  if(node_->type != XML_ELEMENT_NODE) return;
  xmlDocPtr doc = node_->doc;
  if(doc == NULL) return;
  xmlBufferPtr buf = xmlBufferCreate();
  xmlNodeDump(buf,doc,node_,0,user_friendly?1:0);
  out_xml_str=(char*)(buf->content);
  xmlBufferFree(buf);
}

bool XMLNode::SaveToStream(std::ostream& out) const {
  std::string s;
  GetXML(s);
  out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
  out << s;
  return (bool)out;
}

bool XMLNode::SaveToFile(const std::string &file_name) const {
  std::ofstream out(file_name.c_str(), std::ios::out);
  if (!out) return false;
  bool r = SaveToStream(out);
  out.close();
  return r;
}

std::ostream& operator<<(std::ostream& out,const XMLNode& node) {
  node.SaveToStream(out);
  return out;
}

bool XMLNode::ReadFromStream(std::istream& in) {
  std::string s;
  std::getline<char>(in,s,0);
  if(!in) return false;
  xmlDocPtr doc = xmlParseMemory((char*)(s.c_str()),s.length());
  if(doc == NULL) return false;
  if(node_ != NULL) {
    if(is_owner_) {
      xmlFreeDoc(node_->doc);
      node_=NULL; is_owner_=false;
    };
  };
  node_=doc->children;
  if(node_) is_owner_=true;
  return true;
}

bool XMLNode::ReadFromFile(const std::string &file_name) {
  std::ifstream in(file_name.c_str(), std::ios::in);
  if (!in) return false;
  bool r = ReadFromStream(in);
  in.close();
  return r;
}

std::istream& operator>>(std::istream& in,XMLNode& node) {
  node.ReadFromStream(in);
  return in;
}

XMLNodeContainer::XMLNodeContainer(void) {
}

XMLNodeContainer::XMLNodeContainer(const XMLNodeContainer& container) {
  operator=(container);
}

XMLNodeContainer::~XMLNodeContainer(void) {
  for(std::vector<XMLNode*>::iterator n = nodes_.begin();
              n!=nodes_.end();++n) delete *n;
}

XMLNodeContainer& XMLNodeContainer::operator=(const XMLNodeContainer& container) {
  for(std::vector<XMLNode*>::iterator n = nodes_.begin();
              n!=nodes_.end();++n) delete *n;
  for(std::vector<XMLNode*>::const_iterator n = container.nodes_.begin();
              n!=container.nodes_.end();++n) {
    if((*n)->is_owner_) {
      AddNew(*(*n));
    } else {
      Add(*(*n));
    };
  };
  return *this;
}

void XMLNodeContainer::Add(const XMLNode& node) {
  XMLNode* new_node = new XMLNode(node);
  nodes_.push_back(new_node);
}

void XMLNodeContainer::Add(const std::list<XMLNode>& nodes) {
  for(std::list<XMLNode>::const_iterator n = nodes.begin();
              n!=nodes.end();++n) Add(*n);
}

void XMLNodeContainer::AddNew(const XMLNode& node) {
  XMLNode* new_node = new XMLNode();
  node.New(*new_node);
  nodes_.push_back(new_node);
}

void XMLNodeContainer::AddNew(const std::list<XMLNode>& nodes) {
  for(std::list<XMLNode>::const_iterator n = nodes.begin(); 
       n!=nodes.end();++n) {
       AddNew(*n);
    }
}

int XMLNodeContainer::Size(void) {
  return nodes_.size();
}

XMLNode XMLNodeContainer::operator[](int n) {
  if(n < 0) return XMLNode();
  if(n >= nodes_.size()) return XMLNode();
  return *nodes_[n];
}

std::list<XMLNode> XMLNodeContainer::Nodes(void) {
  std::list<XMLNode> r;
  for(std::vector<XMLNode*>::iterator n = nodes_.begin();
              n!=nodes_.end();++n) r.push_back(**n);
}

} // namespace Arc
