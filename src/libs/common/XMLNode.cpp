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
};

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
};

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

XMLNode XMLNode::NewChild(const char* name,int n,bool global_order) {
  if(node_ == NULL) return XMLNode();
  if(name == NULL) return XMLNode();
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
    return NewChild(XMLNode(node.node_->children),n,global_order);
  };
  xmlNodePtr new_node = xmlDocCopyNode(node.node_,node_->doc,1);
  if(new_node == NULL) return XMLNode();
  return XMLNode(xmlAddChild(node_,new_node));
}

static void SetNamespaces(const XMLNode::NS& namespaces,xmlNodePtr node_) {
  if(node_ == NULL) return;
  for(XMLNode::NS::const_iterator ns = namespaces.begin();
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

void XMLNode::Namespaces(const XMLNode::NS& namespaces) {
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
  xmlUnlinkNode(node_);
  xmlFreeNode(node_);
  node_=NULL;
}

XMLNode::dateTime::dateTime(time_t val):t_(val) {
}

XMLNode::dateTime::dateTime(const std::string& val):t_((time_t)(-1)) {
  struct tm t;
  int shift = 0;
  const char* p = val.c_str();
  if(sscanf(p,"%04i-%02u-%02uT%02u:%02u:%02u",
     &t.tm_year,&t.tm_mon,&t.tm_mday,&t.tm_hour,&t.tm_min,&t.tm_sec)
     != 6) return;
  p=strchr(p,':');
  if(!p) return;
  p=strchr(p,':');
  if(!p) return;
  const char* f = strchr(p,'.');
  if(f) {
    p=strchr(f,'+');
    if(!p) return;
    ++p;
  };
  if(*p) {
    if(*p != 'Z') {
      if((*p != '+') && (*p != '-')) return;
      unsigned int shift_hour = 0;
      unsigned int shift_min = 0;
      if(sscanf(p+1,"%02u:%02u",&shift_hour,&shift_min) != 2) return;
      shift=shift_min+(shift_hour*60);
      if(*p == '-') shift=-shift;
    };
  };
  t_=timegm(&t);
  t_+=shift;
}

XMLNode::dateTime::operator time_t(void) {
  return t_;
}

XMLNode::dateTime::operator std::string(void) {
  char buf[256];
  struct tm t;
  gmtime_r(&t_,&t);
  snprintf(buf,sizeof(buf)-1,"%04i-%02u-%02uT%02u:%02u:%02uZ",
           t.tm_year,t.tm_mon,t.tm_mday,t.tm_hour,t.tm_min,t.tm_sec);
  buf[sizeof(buf)-1]=0;
  return std::string(buf);
}

} // namespace Arc
