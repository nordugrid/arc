#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "XMLNode.h"

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

bool MatchXMLName(const XMLNode& node1,const XMLNode& node2) {
  return MatchXMLName(node1.node_,node2.node_);
}

bool MatchXMLName(const XMLNode& node,const char* name) {
  return MatchXMLName(node.node_,name);
}

bool MatchXMLName(xmlNodePtr node1,xmlNodePtr node2) {
  if(strcmp((char*)(node1->name),(char*)(node2->name)) != 0) return false;
  if(node1->type != node2->type) return false;
  if((node1->type != XML_ELEMENT_NODE) && (node1->type != XML_ATTRIBUTE_NODE)) return true;
  xmlNsPtr ns1 = GetNamespace(node1);
  xmlNsPtr ns2 = GetNamespace(node2);
  if(ns1 != ns2) return false;
  return true;
}

bool MatchXMLName(xmlNodePtr node,const char* name) {
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

bool MatchXMLNamespace(const XMLNode& node1,const XMLNode& node2) {
  return MatchXMLNamespace(node1.node_,node2.node_);
}

bool MatchXMLNamespace(const XMLNode& node,const char* uri) {
  return MatchXMLNamespace(node.node_,uri);
}

bool MatchXMLNamespace(xmlNodePtr node1,xmlNodePtr node2) {
  if(node1->type != node2->type) return false;
  if((node1->type != XML_ELEMENT_NODE) && (node1->type != XML_ATTRIBUTE_NODE)) return false;
  xmlNsPtr ns1 = GetNamespace(node1);
  xmlNsPtr ns2 = GetNamespace(node2);
  return (ns1 == ns2);
}

bool MatchXMLNamespace(xmlNodePtr node,const char* uri) {
  if(uri == NULL) return false;
  xmlNsPtr ns_ = GetNamespace(node);
  if((ns_ == NULL) || (ns_->href == NULL)) return false;
  return (strcmp(uri,(const char*)(ns_->href)) == 0);
}

XMLNode::XMLNode(const std::string& xml):node_(NULL),is_owner_(false),is_temporary_(false) {
  xmlDocPtr doc = xmlParseMemory((char*)(xml.c_str()),xml.length());
  if(!doc) return;
  node_=(xmlNodePtr)doc;
  is_owner_=true;
}

XMLNode::XMLNode(const char* xml,int len):node_(NULL),is_owner_(false),is_temporary_(false) {
  if(!xml) return;
  if(len == -1) len=strlen(xml);
  xmlDocPtr doc = xmlParseMemory((char*)xml,len);
  if(!doc) return;
  node_=(xmlNodePtr)doc;
  is_owner_=true;
}

XMLNode::XMLNode(const Arc::NS& ns):node_(NULL),is_owner_(false),is_temporary_(false) {
  node_=(xmlNodePtr)xmlNewDoc((const xmlChar*)"1.0");
  if(node_ != NULL) is_owner_=true;
  // Hack - temporary
  NewChild("");
  Namespaces(ns);
}

XMLNode::~XMLNode(void) {
  if(is_owner_ && node_) {
    if(node_->type == XML_DOCUMENT_NODE) {
      xmlFreeDoc((xmlDocPtr)node_);
    } else if(node_->type == XML_ELEMENT_NODE) {
      Destroy();
    };
  };
}

XMLNode XMLNode::operator[](int n) const {
  if(!node_) return XMLNode();
  xmlNodePtr p = n<0?NULL:node_;
  for(;p;p=p->next) {
    if(p->type == XML_TEXT_NODE) continue;
    if(node_->name) {
      if(!(p->name)) continue;
      //if(strcmp((char*)(node_->name),(char*)(p->name))) continue;
      if(!MatchXMLName(node_,p)) continue;
    };
    if((--n) < 0) break;
  };
  if(p) return XMLNode(p);
  // New (temporary) node
  return XMLNode(p);
}

XMLNode XMLNode::operator[](const char* name) const {
  if(!node_) return XMLNode();
  xmlNodePtr p = node_->children;
  for(;p;p=p->next) {
    if(p->type == XML_TEXT_NODE) continue;
    //if((p->name) && (!strcmp(name,(char*)(p->name)))) break;
    if(MatchXMLName(p,name)) break;
  };
  if(p) return XMLNode(p);
  // New temporary node
  return XMLNode(p);
}

int XMLNode::AttributesSize(void)
{
    if(!node_) {
        return 0;
    }
    if(node_->type != XML_ELEMENT_NODE) {
        return 0;
    }
    int n = 0;
    xmlAttrPtr p = node_->properties;
    for(;p;p=p->next) {
      if(p->type == XML_TEXT_NODE) continue;
      ++n;
    };
    return n;
}

XMLNode XMLNode::Attribute(int n) {
  if(!node_) return XMLNode();
  if(node_->type != XML_ELEMENT_NODE) return XMLNode();
  xmlAttrPtr p = n<0?NULL:node_->properties;
  for(;p;p=p->next) {
    if(p->type == XML_TEXT_NODE) continue;
    if((--n) < 0) break;
  };
  if(p) return XMLNode((xmlNodePtr)p);
  // New temporary node
  return XMLNode((xmlNodePtr)p);
}

XMLNode XMLNode::Attribute(const std::string& name) {
  if(!node_) return XMLNode();
  if(node_->type != XML_ELEMENT_NODE) return XMLNode();
  xmlNodePtr p = (xmlNodePtr)(node_->properties);
  for(;p;p=p->next) {
    if(p->type == XML_TEXT_NODE) continue;
    if(MatchXMLName(p,name.c_str())) break;
  };
  if(p) return XMLNode(p);
  // New temporary node
  return XMLNode(p);
}

XMLNode XMLNode::NewAttribute(const std::string& name) {
  return NewAttribute(name.c_str());
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

void XMLNode::Name(const std::string& name) {
  Name(name.c_str());
}

void XMLNode::Name(const char* name) {
  if(!node_) return;
  const char* name_ = strchr(name,':');
  xmlNsPtr ns = NULL;
  if(name_ != NULL) {
    std::string ns_(name,name_-name);
    ns=xmlSearchNs(node_->doc,node_,(const xmlChar*)(ns_.c_str()));
    ++name_;
  } else {
    name_=name;
  };
  xmlNodeSetName(node_,(const xmlChar*)name_);
  if(ns) node_->ns=ns;
}


XMLNode XMLNode::NewChild(const char* name,int n,bool global_order) {
  if(node_ == NULL) return XMLNode();
  if(name == NULL) return XMLNode();
  const char* name_ = strchr(name,':');
  xmlNsPtr ns = NULL;
  if(name_ != NULL) {
    std::string ns_(name,name_-name);
    if(node_->type == XML_DOCUMENT_NODE) {
      ns=xmlSearchNs((xmlDocPtr)node_,node_->children,(const xmlChar*)(ns_.c_str()));
    } else {
      ns=xmlSearchNs(node_->doc,node_,(const xmlChar*)(ns_.c_str()));
    };
    ++name_;
  } else {
    name_=name;
  };
  xmlNodePtr new_node = xmlNewNode(ns,(const xmlChar*)name_);
  if(new_node == NULL) return XMLNode();
  if(node_->type == XML_DOCUMENT_NODE) {
    XMLNode root = Child();
    if(root) {
      // Hack - root element must be always present
      if(root.Name().empty()) {
        root.Name(name); return root; 
      };
      xmlFreeNode(new_node); return XMLNode(); 
    };
    xmlDocSetRootElement((xmlDocPtr)node_,new_node); 
    return Child();
  };
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
  if(node.node_->type == XML_DOCUMENT_NODE) {
    return NewChild(node.Child(),n,global_order);
  };
  if(node.Name().empty()) return XMLNode();
  if(node_->type == XML_DOCUMENT_NODE) {
    XMLNode root = Child();
    if(!root) return XMLNode();
    if(!(root.Name().empty())) return XMLNode();
    root.Replace(node);
    return Child();
  };
  xmlNodePtr new_node = xmlDocCopyNode(node.node_,node_->doc,1);
  if(new_node == NULL) return XMLNode();
  return XMLNode(xmlAddChild(node_,new_node));
}

void XMLNode::Replace(const XMLNode& node) {
  if(node_ == NULL) return;
  if(node.node_ == NULL) return;
  if(node_->type == XML_DOCUMENT_NODE) {
    XMLNode(((xmlDocPtr)node_)->children).Replace(node);
  };
  if(node.node_->type == XML_DOCUMENT_NODE) {
    Replace(XMLNode(((xmlDocPtr)(node.node_))->children));
    return;
  };
  xmlNodePtr new_node = xmlDocCopyNode(node.node_,node_->doc,1);
  if(new_node == NULL) return;
  xmlReplaceNode(node_,new_node);
  xmlFreeNode(node_); node_=new_node;
  return;
}

void XMLNode::New(XMLNode& new_node) const {
  if(new_node.is_owner_ && new_node.node_) {
    xmlFreeDoc((xmlDocPtr)(new_node.node_));
  };
  new_node.is_owner_=false; new_node.node_=NULL;
  if(!(*this)) return;
  if(node_->type == XML_DOCUMENT_NODE) {
    new_node.node_=(xmlNodePtr)xmlCopyDoc((xmlDocPtr)node_,1);
  } else {
    new_node.node_=(xmlNodePtr)xmlNewDoc((const xmlChar*)"1.0");
    new_node.NewChild(*this);
  };
  if(!new_node) return;
  new_node.is_owner_=true;
  return;
}

static void SetNamespaces(const Arc::NS& namespaces,xmlNodePtr node_) {
  if(node_ == NULL) return;
  for(Arc::NS::const_iterator ns = namespaces.begin();
         ns!=namespaces.end();++ns) {
    xmlNsPtr ns_ = xmlSearchNsByHref(node_->doc,node_,(const xmlChar*)(ns->second.c_str()));
    if(ns_) {
      xmlFree((void*)(ns_->prefix));
      ns_->prefix=(xmlChar*)xmlMemStrdup(ns->first.c_str());
    } else {
      xmlNewNs(node_,(const xmlChar*)(ns->second.c_str()),(const xmlChar*)(ns->first.c_str()));
    };
  };
}

void XMLNode::Namespaces(const Arc::NS& namespaces) {
  if(node_ == NULL) return;
  if(node_->type == XML_DOCUMENT_NODE) {
    for(int n = 0;;++n) {
      XMLNode n_ = Child(n);
      if(!n_) break;
      n_.Namespaces(namespaces);
    };
    return;
  };
  SetNamespaces(namespaces,node_);
}

std::string XMLNode::NamespacePrefix(const char* urn) {
  xmlNsPtr ns_ = xmlSearchNsByHref(node_->doc,node_,(const xmlChar*)urn);
  if(!ns_) return "";
  return (char*)(ns_->prefix);
}

void XMLNode::Destroy(void) {
  if(node_ == NULL) return;
  // TODO: check for doc node
  xmlUnlinkNode(node_);
  xmlFreeNode(node_);
  node_=NULL;
}

std::list<XMLNode> XMLNode::XPathLookup(const std::string& xpathExpr, const Arc::NS& nsList) {
  std::list<XMLNode> retlist;
  if(node_ == NULL) return retlist;
  if(node_->type != XML_DOCUMENT_NODE) return retlist;
  xmlDocPtr doc = (xmlDocPtr)node_;
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
        retlist.push_back(XMLNode(cur));   
      }
    }
  }
 
  xmlXPathFreeObject(xpathObj);
  xmlXPathFreeContext(xpathCtx); 

  return retlist;
}

XMLNode XMLNode::GetRoot(void) {
  xmlNodePtr parent = node_->parent;
  if(parent){
    while(parent->parent){
      parent= parent->parent;
    }
  }
  if(!(node_->parent)) parent = node_;

  return XMLNode(parent);
}

XMLNode& XMLNode::operator=(const XMLNode& node) {
  if(is_owner_ && node_) {
    if(node_->type == XML_DOCUMENT_NODE) {
      xmlFreeDoc((xmlDocPtr)node_);
    } else if(node_->type == XML_ELEMENT_NODE) {
      Destroy();
    };
  };
  node_=node.node_;
  is_owner_=false;
  is_temporary_=node.is_temporary_;
  return *this;
}

/*
void XMLNode::Duplicate(XMLNode& new_node) {
  if(!(*this)) return;
  new_node.Destroy();
  new_node.is_owner_=false; new_node.node_=NULL;
  new_node.node_=(xmlNodePtr)xmlCopyNode((xmlNodePtr)node_,1);
  if(!new_node) return;
  new_node.is_owner_=true;
  return;
}
*/
} // namespace Arc
