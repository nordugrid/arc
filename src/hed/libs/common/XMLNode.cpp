// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <cstring>

#include "XMLNode.h"
#include <libxml/xmlschemas.h>

namespace Arc {

  static xmlNsPtr GetNamespace(xmlNodePtr node) {
    if (node == NULL) return NULL;
    xmlNsPtr ns_ = NULL;
    if (node->type == XML_ELEMENT_NODE)
      ns_ = node->ns;
    else if (node->type == XML_ATTRIBUTE_NODE)
      ns_ = ((xmlAttrPtr)node)->ns;
    ;
    if (ns_)
      return ns_;
    if (node->parent)
      return GetNamespace(node->parent);
    return NULL;
  }

  static bool MatchXMLName(xmlNodePtr node1, xmlNodePtr node2) {
    if (node1 == NULL) return false;
    if (node2 == NULL) return false;
    if (strcmp((char*)(node1->name), (char*)(node2->name)) != 0)
      return false;
    if (node1->type != node2->type)
      return false;
    if ((node1->type != XML_ELEMENT_NODE) && (node1->type != XML_ATTRIBUTE_NODE))
      return true;
    xmlNsPtr ns1 = GetNamespace(node1);
    xmlNsPtr ns2 = GetNamespace(node2);
    if (ns1 != ns2)
      return false;
    return true;
  }

  static bool MatchXMLName(xmlNodePtr node, const char *name) {
    if (node == NULL) return false;
    if (name == NULL) return false;
    const char *name_ = strchr(name, ':');
    if (name_ == NULL)
      name_ = name;
    else
      ++name_;
    if (strcmp(name_, (char*)(node->name)) != 0)
      return false;
    if (name_ == name)
      return true;
    xmlNsPtr ns_ = GetNamespace(node);
    std::string ns(name, name_ - name - 1);
    if ((ns_ == NULL) || (ns_->prefix == NULL))
      return ns.empty();
    return (ns == (const char*)(ns_->prefix));
  }

  static bool MatchXMLNamespace(xmlNodePtr node1, xmlNodePtr node2) {
    if (node1 == NULL) return false;
    if (node2 == NULL) return false;
    if (node1->type != node2->type) return false;
    if ((node1->type != XML_ELEMENT_NODE) && (node1->type != XML_ATTRIBUTE_NODE))
      return false;
    xmlNsPtr ns1 = GetNamespace(node1);
    xmlNsPtr ns2 = GetNamespace(node2);
    return (ns1 == ns2);
  }

  static bool MatchXMLNamespace(xmlNodePtr node, const char *uri) {
    if (node == NULL) return false;
    if (uri == NULL) return false;
    xmlNsPtr ns_ = GetNamespace(node);
    if ((ns_ == NULL) || (ns_->href == NULL))
      return false;
    return (strcmp(uri, (const char*)(ns_->href)) == 0);
  }

  bool MatchXMLName(const XMLNode& node1, const XMLNode& node2) {
    return MatchXMLName(node1.node_, node2.node_);
  }

  bool MatchXMLName(const XMLNode& node, const char *name) {
    return MatchXMLName(node.node_, name);
  }

  bool MatchXMLName(const XMLNode& node, const std::string& name) {
    return MatchXMLName(node.node_, name.c_str());
  }

  bool MatchXMLNamespace(const XMLNode& node1, const XMLNode& node2) {
    return MatchXMLNamespace(node1.node_, node2.node_);
  }

  bool MatchXMLNamespace(const XMLNode& node, const char *uri) {
    return MatchXMLNamespace(node.node_, uri);
  }

  bool MatchXMLNamespace(const XMLNode& node, const std::string& uri) {
    return MatchXMLNamespace(node.node_, uri.c_str());
  }

  static void ReplaceNamespace(xmlNsPtr ns, xmlNodePtr node, xmlNsPtr new_ns) {
    if (node->type == XML_ELEMENT_NODE) {
      if (node->ns == ns)
        node->ns = new_ns;
      for (xmlAttrPtr node_ = node->properties; node_; node_ = node_->next)
        ReplaceNamespace(ns, (xmlNodePtr)node_, new_ns);
      for (xmlNodePtr node_ = node->children; node_; node_ = node_->next)
        ReplaceNamespace(ns, node_, new_ns);
    }
    else if (node->type == XML_ATTRIBUTE_NODE) {
      if (((xmlAttrPtr)node)->ns == ns)
        ((xmlAttrPtr)node)->ns = new_ns;
    }
    else
      return;
  }

  static void ReassignNamespace(xmlNsPtr ns, xmlNodePtr node,bool keep = false,int recursion = -1) {
    if(recursion >= 0) keep = true;
    xmlNsPtr ns_cur = node->nsDef;
    xmlNsPtr ns_prev = NULL;
    for (; ns_cur;) {
      if (ns == ns_cur) {
        ns_prev = ns_cur;
        ns_cur = ns_cur->next;
        continue;
      }
      if (ns->href && ns_cur->href && (xmlStrcmp(ns->href, ns_cur->href) == 0)) {
        ReplaceNamespace(ns_cur, node, ns);
        if(!keep) {
          // Unlinking namespace from tree
          if (ns_prev)
            ns_prev->next = ns_cur->next;
          else
            node->nsDef = ns_cur->next;
          xmlNsPtr ns_tmp = ns_cur;
          ns_cur = ns_cur->next;
          xmlFreeNs(ns_tmp);
        } else {
          ns_cur = ns_cur->next;
        }
        continue;
      }
      ns_prev = ns_cur;
      ns_cur = ns_cur->next;
    }
    if(recursion == 0) return;
    if(recursion > 0) --recursion;
    for (xmlNodePtr node_ = node->children; node_; node_ = node_->next)
      ReassignNamespace(ns, node_, keep, recursion);
  }

  // Adds new 'namespaces' to namespace definitions of 'node_'.
  // The 'node_' and its children are converted to new prefixes.
  // If keep == false all existing namespaces with same href
  // defined in 'node_' or children are removed.
  // 'recursion' limits how deep to follow children nodes. 0 for
  // 'node_' only. -1 for unlimited depth. If 'recursion' is set
  //  to >=0 then existing namespaces always kept disregarding
  //  value of 'keep'. Otherwise some XML node would be left
  //  without valid namespaces.
  static void SetNamespaces(const NS& namespaces, xmlNodePtr node_,bool keep = false,int recursion = -1) {
    for (NS::const_iterator ns = namespaces.begin();
         ns != namespaces.end(); ++ns) {
      // First check maybe this namespace is already defined
      xmlNsPtr ns_ = xmlSearchNsByHref(node_->doc, node_, (const xmlChar*)(ns->second.c_str()));
      if (ns_) {
        const char *prefix = (const char*)(ns_->prefix);
        if (!prefix) prefix = "";
        if (ns->first == prefix) {
          // Same namespace with same prefix - doing nothing
        }
        else {
          // Change to new prefix
          ns_ = NULL;
        }
      }
      if (!ns_) {
        // New namespace needed
        // If the namespace's name is defined then pass it to the libxml function else set the value as default namespace
        ns_ = xmlNewNs(node_, (const xmlChar*)(ns->second.c_str()), ns->first.empty() ? NULL : (const xmlChar*)(ns->first.c_str()));
        if (ns_ == NULL)
          // There is already namespace with same prefix (or some other error)
          // TODO: optional change of prefix
          return;
      }
      // Go through all children removing same namespaces and reassigning elements to this one.
      ReassignNamespace(ns_, node_, keep, recursion);
    }
  }

  static void GetNamespaces(NS& namespaces, xmlNodePtr node_) {
    if (node_ == NULL)
      return;
    if (node_->type != XML_ELEMENT_NODE)
      return;
    // TODO: Check for duplicate prefixes
    xmlNsPtr ns = node_->nsDef;
    for (; ns; ns = ns->next) {
      std::string prefix = ns->prefix ? (char*)(ns->prefix) : "";
      if (ns->href)
        if (namespaces[prefix].empty())
          namespaces[prefix] = (char*)(ns->href);
    }
    GetNamespaces(namespaces, node_->parent);
  }

  XMLNode::XMLNode(const std::string& xml)
    : node_(NULL),
      is_owner_(false),
      is_temporary_(false) {
    xmlDocPtr doc = xmlParseMemory((char*)(xml.c_str()), xml.length());
    if (!doc)
      return;
    xmlNodePtr p = doc->children;
    for (; p; p = p->next)
      if (p->type == XML_ELEMENT_NODE) break;
    if (!p) {
      xmlFreeDoc(doc);
      return;
    }
    node_ = p;
    is_owner_ = true;
  }

  XMLNode::XMLNode(const char *xml, int len)
    : node_(NULL),
      is_owner_(false),
      is_temporary_(false) {
    if (!xml)
      return;
    if (len == -1)
      len = strlen(xml);
    xmlDocPtr doc = xmlParseMemory((char*)xml, len);
    if (!doc)
      return;
    xmlNodePtr p = doc->children;
    for (; p; p = p->next)
      if (p->type == XML_ELEMENT_NODE) break;
    if (!p) {
      xmlFreeDoc(doc);
      return;
    }
    node_ = p;
    is_owner_ = true;
  }

  XMLNode::XMLNode(long ptr_addr)
    : node_(NULL),
      is_owner_(false),
      is_temporary_(false) {
    XMLNode *other = (XMLNode *)ptr_addr;
    (*other).New((*this));
  }

  XMLNode::XMLNode(const NS& ns, const char *name)
    : node_(NULL),
      is_owner_(false),
      is_temporary_(false) {
    xmlDocPtr doc = xmlNewDoc((const xmlChar*)"1.0");
    if (!doc)
      return;
    if (name == NULL)
      name = "";
    const char *name_ = strchr(name, ':');
    std::string node_ns_;
    if (name_ != NULL) {
      node_ns_.assign(name, name_ - name);
      ++name_;
    }
    else
      name_ = name;
    xmlNodePtr new_node = xmlNewNode(NULL, (const xmlChar*)name_);
    if (new_node == NULL) {
      xmlFreeDoc(doc);
      return;
    }
    xmlDocSetRootElement(doc, new_node);
    node_ = new_node;
    is_owner_ = true;
    SetNamespaces(ns, node_);
    node_->ns = xmlSearchNs(node_->doc, node_, (const xmlChar*)(node_ns_.c_str()));
  }

  XMLNode::~XMLNode(void) {
    if (is_owner_ && node_)
      xmlFreeDoc(node_->doc);
  }

  XMLNode XMLNode::operator[](int n) const {
    if (!node_)
      return XMLNode();
    xmlNodePtr p = n < 0 ? NULL : node_;
    for (; p; p = p->next) {
      if ((p->type != XML_ELEMENT_NODE) &&
          (p->type != XML_ATTRIBUTE_NODE))
        continue;
      if (node_->name) {
        if (!(p->name))
          continue;
        if (!MatchXMLName(node_, p))
          continue;
      }
      if ((--n) < 0)
        break;
    }
    return XMLNode(p);
  }

  XMLNode XMLNode::operator[](const char *name) const {
    if (!node_)
      return XMLNode();
    if ((node_->type != XML_ELEMENT_NODE) &&
        (node_->type != XML_ATTRIBUTE_NODE))
      return XMLNode();
    xmlNodePtr p = node_->children;
    for (; p; p = p->next) {
      if ((p->type != XML_ELEMENT_NODE) &&
          (p->type != XML_ATTRIBUTE_NODE))
        continue;
      if (MatchXMLName(p, name))
        break;
    }
    return XMLNode(p);
  }

  void XMLNode::operator++(void) {
    if (!node_)
      return;
    if (is_owner_) { // top node has no siblings
      xmlFreeDoc(node_->doc);
      node_ = NULL;
      is_owner_ = false;
      return;
    }
    xmlNodePtr p = node_->next;
    for (; p; p = p->next) {
      if (node_->type != p->type)
        continue;
      if (node_->name) {
        if (!(p->name))
          continue;
        if (!MatchXMLName(node_, p))
          continue;
      }
      break;
    }
    node_ = p;
  }

  void XMLNode::operator--(void) {
    if (!node_)
      return;
    if (is_owner_) { // top node has no siblings
      xmlFreeDoc(node_->doc);
      node_ = NULL;
      is_owner_ = false;
      return;
    }
    xmlNodePtr p = node_->prev;
    for (; p; p = p->prev) {
      if (node_->type != p->type)
        continue;
      if (node_->name) {
        if (!(p->name))
          continue;
        if (!MatchXMLName(node_, p))
          continue;
      }
      break;
    }
    node_ = p;
  }

  int XMLNode::Size(void) const {
    if (!node_)
      return 0;
    int n = 0;
    xmlNodePtr p = node_->children;
    for (; p; p = p->next) {
      if (p->type != XML_ELEMENT_NODE)
        continue;
      ++n;
    }
    return n;
  }

  std::string XMLNode::Name(void) const {
    const char *name = (node_) ? ((node_->name) ? (char*)(node_->name) : "") : "";
    return std::string(name);
  }

  int XMLNode::AttributesSize(void) const {
    if (!node_)
      return 0;
    if (node_->type != XML_ELEMENT_NODE)
      return 0;
    int n = 0;
    xmlAttrPtr p = node_->properties;
    for (; p; p = p->next) {
      if (p->type != XML_ATTRIBUTE_NODE)
        continue;
      ++n;
    }
    return n;
  }

  XMLNode XMLNode::Attribute(int n) {
    if (!node_)
      return XMLNode();
    if (node_->type != XML_ELEMENT_NODE)
      return XMLNode();
    xmlAttrPtr p = n < 0 ? NULL : node_->properties;
    for (; p; p = p->next) {
      if (p->type != XML_ATTRIBUTE_NODE)
        continue;
      if ((--n) < 0)
        break;
    }
    return XMLNode((xmlNodePtr)p);
  }

  XMLNode XMLNode::Attribute(const char *name) {
    if (!node_)
      return XMLNode();
    if (node_->type != XML_ELEMENT_NODE)
      return XMLNode();
    xmlNodePtr p = (xmlNodePtr)(node_->properties);
    for (; p; p = p->next) {
      if (p->type != XML_ATTRIBUTE_NODE)
        continue;
      if (MatchXMLName(p, name))
        break;
    }
    if (p)
      return XMLNode(p);
    // New temporary node
    return XMLNode(p);
  }

  XMLNode XMLNode::NewAttribute(const char *name) {
    if (!node_)
      return XMLNode();
    if (node_->type != XML_ELEMENT_NODE)
      return XMLNode();
    const char *name_ = strchr(name, ':');
    xmlNsPtr ns = NULL;
    if (name_ != NULL) {
      std::string ns_(name, name_ - name);
      ns = xmlSearchNs(node_->doc, node_, (const xmlChar*)(ns_.c_str()));
      ++name_;
    }
    else
      name_ = name;
    return XMLNode((xmlNodePtr)xmlNewNsProp(node_, ns, (const xmlChar*)name_, NULL));
  }

  std::string XMLNode::Prefix(void) const {
    if (!node_)
      return "";
    xmlNsPtr ns = GetNamespace(node_);
    if (!ns)
      return "";
    if (!(ns->prefix))
      return "";
    return (const char*)(ns->prefix);
  }

  std::string XMLNode::Namespace(void) const {
    if (!node_)
      return "";
    xmlNsPtr ns = GetNamespace(node_);
    if (!ns)
      return "";
    if (!(ns->href))
      return "";
    return (const char*)(ns->href);
  }

  void XMLNode::Name(const char *name) {
    if (!node_)
      return;
    const char *name_ = strchr(name, ':');
    xmlNsPtr ns = NULL;
    if (name_ != NULL) {
      std::string ns_(name, name_ - name);
      // ns element is located at same place in Node and Attr elements
      ns = xmlSearchNs(node_->doc, node_, (const xmlChar*)(ns_.c_str()));
      ++name_;
    }
    else
      name_ = name;
    xmlNodeSetName(node_, (const xmlChar*)name_);
    if (ns)
      node_->ns = ns;
  }

  XMLNode XMLNode::Child(int n) {
    if (!node_)
      return XMLNode();
    if (node_->type != XML_ELEMENT_NODE)
      return XMLNode();
    xmlNodePtr p = n < 0 ? NULL : node_->children;
    for (; p; p = p->next) {
      if (p->type != XML_ELEMENT_NODE)
        continue;
      if ((--n) < 0)
        break;
    }
    return XMLNode(p);
  }

  XMLNode::operator std::string(void) const {
    std::string content;
    if (!node_)
      return content;
    for (xmlNodePtr p = node_->children; p; p = p->next) {
      if (p->type != XML_TEXT_NODE)
        continue;
      xmlChar *buf = xmlNodeGetContent(p);
      if (!buf)
        continue;
      content += (char*)buf;
      xmlFree(buf);
    }
    return content;
  }

  XMLNode& XMLNode::operator=(const char *content) {
    if (!node_)
      return *this;
    if (!content)
      content = "";
    xmlChar *encode = xmlEncodeSpecialChars(node_->doc, (xmlChar*)content);
    if (!encode)
      encode = (xmlChar*)"";
    xmlNodeSetContent(node_, encode);
    xmlFree(encode);
    return *this;
  }


  XMLNode XMLNode::NewChild(const char *name, const NS& namespaces, int n, bool global_order) {
    XMLNode x = NewChild("", n, global_order); // placeholder
    x.Namespaces(namespaces);
    x.Name(name);
    return x;
  }

  XMLNode XMLNode::NewChild(const char *name, int n, bool global_order) {
    if (node_ == NULL)
      return XMLNode();
    if (node_->type != XML_ELEMENT_NODE)
      return XMLNode();
    const char *name_ = strchr(name, ':');
    xmlNsPtr ns = NULL;
    if (name_ != NULL) {
      std::string ns_(name, name_ - name);
      ns = xmlSearchNs(node_->doc, node_, (const xmlChar*)(ns_.c_str()));
      ++name_;
    }
    else
      name_ = name;
    xmlNodePtr new_node = xmlNewNode(ns, (const xmlChar*)name_);
    if (new_node == NULL)
      return XMLNode();
    if (n < 0)
      return XMLNode(xmlAddChild(node_, new_node));
    XMLNode old_node = global_order ? Child(n) : operator[](name)[n];
    if (!old_node)
      // TODO: find last old_node
      return XMLNode(xmlAddChild(node_, new_node));
    if (old_node)
      return XMLNode(xmlAddPrevSibling(old_node.node_, new_node));
    return XMLNode(xmlAddChild(node_, new_node));
  }

  XMLNode XMLNode::NewChild(const XMLNode& node, int n, bool global_order) {
    if (node_ == NULL)
      return XMLNode();
    if (node.node_ == NULL)
      return XMLNode();
    if (node_->type != XML_ELEMENT_NODE)
      return XMLNode();
    // TODO: Add new attribute if 'node' is attribute
    if (node.node_->type != XML_ELEMENT_NODE)
      return XMLNode();
    xmlNodePtr new_node = xmlDocCopyNode(node.node_, node_->doc, 1);
    if (new_node == NULL)
      return XMLNode();
    if (n < 0)
      return XMLNode(xmlAddChild(node_, new_node));
    std::string name;
    xmlNsPtr ns = GetNamespace(new_node);
    if (ns != NULL) {
      if (ns->prefix != NULL)
        name = (char*)ns->prefix;
      name += ":";
    }
    if (new_node->name)
      name += (char*)(new_node->name);
    XMLNode old_node = global_order ? Child(n) : operator[](name)[n];
    if (!old_node)
      // TODO: find last old_node
      return XMLNode(xmlAddChild(node_, new_node));
    if (old_node)
      return XMLNode(xmlAddPrevSibling(old_node.node_, new_node));
    return XMLNode(xmlAddChild(node_, new_node));
  }

  void XMLNode::Replace(const XMLNode& node) {
    if (node_ == NULL)
      return;
    if (node.node_ == NULL)
      return;
    if (node_->type != XML_ELEMENT_NODE)
      return;
    if (node.node_->type != XML_ELEMENT_NODE)
      return;
    xmlNodePtr new_node = xmlDocCopyNode(node.node_, node_->doc, 1);
    if (new_node == NULL)
      return;
    xmlReplaceNode(node_, new_node);
    xmlFreeNode(node_);
    node_ = new_node;
    return;
  }

  void XMLNode::New(XMLNode& new_node) const {
    if (new_node.is_owner_ && new_node.node_)
      xmlFreeDoc(new_node.node_->doc);
    new_node.is_owner_ = false;
    new_node.node_ = NULL;
    if (node_ == NULL)
      return;
    // TODO: Copy attribute node too
    if (node_->type != XML_ELEMENT_NODE)
      return;
    xmlDocPtr doc = xmlNewDoc((const xmlChar*)"1.0");
    if (doc == NULL)
      return;
    new_node.node_ = xmlDocCopyNode(node_, doc, 1);
    if (new_node.node_ == NULL)
      return;
    xmlDocSetRootElement(doc, new_node.node_);
    new_node.is_owner_ = true;
    return;
  }

  void XMLNode::Move(XMLNode& node) {
    if (node.is_owner_ && node.node_)
      xmlFreeDoc(node.node_->doc);
    node.is_owner_ = false;
    node.node_ = NULL;
    if (node_ == NULL)
      return;
    // TODO: Copy attribute node too
    if (node_->type != XML_ELEMENT_NODE) {
      return;
    }
    if(is_owner_) {
      // Owner also means top level. So just copy and clean.
      node.node_=node_;
      node.is_owner_=true;
      node_=NULL; is_owner_=false;
      return;
    }
    // Otherwise unlink this node and make a new document of it
    // New(node); Destroy();
    xmlDocPtr doc = xmlNewDoc((const xmlChar*)"1.0");
    if (doc == NULL) return;
    xmlUnlinkNode(node_);
    node.node_ = node_; node_ = NULL;
    xmlDocSetRootElement(doc, node.node_);
    node.is_owner_ = true;
    return;
  }

  void XMLNode::Swap(XMLNode& node) {
    xmlNodePtr tmp_node_ = node.node_;
    bool tmp_is_owner_ = node.is_owner_;
    node.node_ = node_;
    node.is_owner_ = is_owner_;
    node_ = tmp_node_;
    is_owner_ = tmp_is_owner_;
  }

  void XMLNode::Exchange(XMLNode& node) {
    if(((node_ == NULL) || is_owner_) &&
       ((node.node_ == NULL) || node.is_owner_)) {
      Swap(node); // ?
      return;
    }
    xmlNodePtr node1 = node_;
    xmlNodePtr node2 = node.node_;
    if(node1 && (node1->type != XML_ELEMENT_NODE)) return;
    if(node2 && (node2->type != XML_ELEMENT_NODE)) return;
    node_ = NULL; node.node_ = NULL;
    xmlNodePtr neighb1 = node1?(node1->next):NULL;
    xmlNodePtr neighb2 = node2?(node2->next):NULL;
    xmlNodePtr parent1 = node1?(node1->parent):NULL;
    xmlNodePtr parent2 = node2?(node2->parent):NULL;
    xmlDocPtr doc1 = node1?(node1->doc):NULL;
    xmlDocPtr doc2 = node2?(node2->doc):NULL;
    // In current implementation it is dangerous to move
    // top level element if node is not owning document
    if((parent1 == NULL) && (!is_owner_)) return;
    if((parent2 == NULL) && (!node.is_owner_)) return;
    xmlUnlinkNode(node1);
    xmlUnlinkNode(node2);
    if(parent1) {
      if(neighb1) {
        xmlAddPrevSibling(neighb1,node2);
      } else {
        xmlAddChild(parent1,node2);
      }
    } else if(doc1) {
      xmlDocSetRootElement(doc1,node2);
    } else {
      // Should not happen
      xmlFreeNode(node2); node2 = NULL;
    }
    if(parent2) {
      if(neighb2) {
        xmlAddPrevSibling(neighb2,node1);
      } else {
        xmlAddChild(parent2,node1);
      }
    } else if(doc2) {
      xmlDocSetRootElement(doc2,node1);
    } else {
      // Should not happen
      xmlFreeNode(node1); node1 = NULL;
    }
    node_ = node2;
    node.node_ = node1;
  }

  void XMLNode::Namespaces(const NS& namespaces, bool keep, int recursion) {
    if (node_ == NULL)
      return;
    if (node_->type != XML_ELEMENT_NODE)
      return;
    SetNamespaces(namespaces, node_, keep, recursion);
  }

  NS XMLNode::Namespaces(void) {
    NS namespaces;
    if (node_ == NULL)
      return namespaces;
    if (node_->type != XML_ELEMENT_NODE)
      return namespaces;
    GetNamespaces(namespaces, node_);
    return namespaces;
  }

  std::string XMLNode::NamespacePrefix(const char *urn) {
    if (node_ == NULL)
      return "";
    xmlNsPtr ns_ = xmlSearchNsByHref(node_->doc, node_, (const xmlChar*)urn);
    if (!ns_)
      return "";
    return (char*)(ns_->prefix);
  }

  void XMLNode::Destroy(void) {
    if (node_ == NULL)
      return;
    if (is_owner_) {
      xmlFreeDoc(node_->doc);
      node_ = NULL;
      is_owner_ = false;
      return;
    }
    if (node_->type == XML_ELEMENT_NODE) {
      xmlNodePtr p = node_->prev;
      if(p && (p->type == XML_TEXT_NODE)) {
        xmlChar *buf = xmlNodeGetContent(p);
        if (buf) {
          while(*buf) {
            if(!isspace(*buf)) {
              p = NULL;
              break;
            }
            ++buf;
          }
        }
      } else {
        p = NULL;
      }
      xmlUnlinkNode(node_);
      xmlFreeNode(node_);
      node_ = NULL;
      // Remove beautyfication text too.
      if(p) {
        xmlUnlinkNode(p);
        xmlFreeNode(p);
      }
      return;
    }
    if (node_->type == XML_ATTRIBUTE_NODE) {
      xmlRemoveProp((xmlAttrPtr)node_);
      node_ = NULL;
      return;
    }
  }

  XMLNodeList XMLNode::Path(const std::string& path) {
    XMLNodeList res;
    std::string::size_type name_s = 0;
    std::string::size_type name_e = path.find('/', name_s);
    if (name_e == std::string::npos)
      name_e = path.length();
    res.push_back(*this);
    for (;;) {
      if (res.size() <= 0)
        return res;
      XMLNodeList::iterator node = res.begin();
      std::string node_name = path.substr(name_s, name_e - name_s);
      int nodes_num = res.size();
      for (int n = 0; n < nodes_num; ++n) {
        for (int cn = 0;; ++cn) {
          XMLNode cnode = (*node).Child(cn);
          if (!cnode)
            break;
          if (MatchXMLName(cnode, node_name))
            res.push_back(cnode);
        }
        ++node;
      }
      res.erase(res.begin(), node);
      if (name_e >= path.length())
        break;
      name_s = name_e + 1;
      name_e = path.find('/', name_s);
      if (name_e == std::string::npos)
        name_e = path.length();
    }
    return res;
  }

  XMLNodeList XMLNode::XPathLookup(const std::string& xpathExpr, const NS& nsList) {
    std::list<XMLNode> retlist;
    if (node_ == NULL)
      return retlist;
    if (node_->type != XML_ELEMENT_NODE)
      return retlist;
    xmlDocPtr doc = node_->doc;
    if (doc == NULL)
      return retlist;
    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);

    for (NS::const_iterator ns = nsList.begin(); ns != nsList.end(); ++ns)
      xmlXPathRegisterNs(xpathCtx, (xmlChar*)ns->first.c_str(), (xmlChar*)ns->second.c_str());

    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((const xmlChar*)(xpathExpr.c_str()), xpathCtx);

    if (xpathObj && xpathObj->nodesetval && xpathObj->nodesetval->nodeNr) {
      xmlNodeSetPtr nodes = xpathObj->nodesetval;
      int size = nodes->nodeNr;
      for (int i = 0; i < size; ++i)
        if (nodes->nodeTab[i]->type == XML_ELEMENT_NODE) {
          xmlNodePtr cur = nodes->nodeTab[i];
          xmlNodePtr parent = cur;
          for (; parent; parent = parent->parent)
            if (parent == node_)
              break;
          if (parent)
            retlist.push_back(XMLNode(cur));
        }
    }

    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);

    return retlist;
  }

  XMLNode XMLNode::GetRoot(void) {
    if (node_ == NULL)
      return XMLNode();
    xmlDocPtr doc = node_->doc;
    if (doc == NULL)
      return XMLNode();
    return XMLNode(doc->children);
  }

  XMLNode XMLNode::Parent(void) {
    if (node_ == NULL)
      return XMLNode();
    if (node_->type == XML_ELEMENT_NODE)
      return XMLNode(node_->parent);
    if (node_->type == XML_ATTRIBUTE_NODE)
      return XMLNode(((xmlAttrPtr)node_)->parent);
    return XMLNode();
  }

  XMLNode& XMLNode::operator=(const XMLNode& node) {
    if (is_owner_ && node_) {
      xmlDocPtr doc = node_->doc;
      if (doc != NULL)
        xmlFreeDoc(doc);
    }
    node_ = node.node_;
    is_owner_ = false;
    is_temporary_ = node.is_temporary_;
    return *this;
  }

  static int write_to_string(void* context,const char* buffer,int len) {
    if(!context) return -1;
    std::string* str = (std::string*)context;
    if(len <= 0) return 0;
    if(!buffer) return -1;
    str->append(buffer,len);
    return len;
  }

  static int close_string(void* context) {
    if(!context) return -1;
    return 0;
  }

  void XMLNode::GetDoc(std::string& out_xml_str, bool user_friendly) const {
    out_xml_str.resize(0);
    if (!node_)
      return;
    xmlDocPtr doc = node_->doc;
    if (doc == NULL)
      return;
    xmlOutputBufferPtr buf =
     xmlOutputBufferCreateIO(&write_to_string,&close_string,&out_xml_str,NULL);
    if(buf == NULL)
      return;
/*
    xmlChar *buf = NULL;
    int bufsize = 0;
    if (user_friendly)
      xmlDocDumpFormatMemory(doc, &buf, &bufsize, 1);
    else
      xmlDocDumpMemory(doc, &buf, &bufsize);
    if (buf) {
      out_xml_str = (char*)buf;
      xmlFree(buf);
    }
*/
    // Note xmlSaveFormatFileTo/xmlSaveFileTo call xmlOutputBufferClose
    if (user_friendly)
      xmlSaveFormatFileTo(buf, doc, (const char*)(doc->encoding), 1);
    else
      xmlSaveFileTo(buf, doc, (const char*)(doc->encoding));
  }

  void XMLNode::GetXML(std::string& out_xml_str, bool user_friendly) const {
    out_xml_str.resize(0);
    if (!node_)
      return;
    if (node_->type != XML_ELEMENT_NODE)
      return;
    xmlDocPtr doc = node_->doc;
    if (doc == NULL)
      return;
/*
    xmlBufferPtr buf = xmlBufferCreate();
    xmlNodeDump(buf, doc, node_, 0, user_friendly ? 1 : 0);
    out_xml_str = (char*)(buf->content);
    xmlBufferFree(buf);
*/
    xmlOutputBufferPtr buf =
     xmlOutputBufferCreateIO(&write_to_string,&close_string,&out_xml_str,NULL);
    if(buf == NULL)
      return;
    xmlNodeDumpOutput(buf, doc, node_, 0, user_friendly ? 1 : 0, (const char*)(doc->encoding));
    xmlOutputBufferClose(buf);
  }

  void XMLNode::GetXML(std::string& out_xml_str, const std::string& encoding, bool user_friendly) const {
    out_xml_str.resize(0);
    if (!node_)
      return;
    if (node_->type != XML_ELEMENT_NODE)
      return;
    xmlDocPtr doc = node_->doc;
    if (doc == NULL)
      return;
    xmlCharEncodingHandlerPtr handler = NULL;
    handler = xmlFindCharEncodingHandler(encoding.c_str());
    if (handler == NULL)
      return;
    //xmlOutputBufferPtr buf = xmlAllocOutputBuffer(handler);
    xmlOutputBufferPtr buf =
     xmlOutputBufferCreateIO(&write_to_string,&close_string,&out_xml_str,NULL);
    if(buf == NULL)
      return;
    xmlNodeDumpOutput(buf, doc, node_, 0, user_friendly ? 1 : 0, encoding.c_str());
    xmlOutputBufferFlush(buf);
    //out_xml_str = (char*)(buf->conv ? buf->conv->content : buf->buffer->content);
    xmlOutputBufferClose(buf);
  }

  bool XMLNode::SaveToStream(std::ostream& out) const {
    std::string s;
    GetXML(s);
    out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
    out << s;
    return (bool)out;
  }

  bool XMLNode::SaveToFile(const std::string& file_name) const {
    std::ofstream out(file_name.c_str(), std::ios::out);
    if (!out)
      return false;
    bool r = SaveToStream(out);
    out.close();
    return r;
  }

  std::ostream& operator<<(std::ostream& out, const XMLNode& node) {
    node.SaveToStream(out);
    return out;
  }

  bool XMLNode::ReadFromStream(std::istream& in) {
    std::string s;
    std::getline<char>(in, s, 0);
    if (!in)
      return false;
    xmlDocPtr doc = xmlParseMemory((char*)(s.c_str()), s.length());
    if (doc == NULL)
      return false;
    xmlNodePtr p = doc->children;
    for (; p; p = p->next)
      if (p->type == XML_ELEMENT_NODE) break;
    if (!p) {
      xmlFreeDoc(doc);
      return false;
    }
    if (node_ != NULL)
      if (is_owner_) {
        xmlFreeDoc(node_->doc);
        node_ = NULL;
        is_owner_ = false;
      }
    node_ = p;
    if (node_)
      is_owner_ = true;
    return true;
  }

  bool XMLNode::ReadFromFile(const std::string& file_name) {
    std::ifstream in(file_name.c_str(), std::ios::in);
    if (!in)
      return false;
    bool r = ReadFromStream(in);
    in.close();
    return r;
  }

  std::istream& operator>>(std::istream& in, XMLNode& node) {
    node.ReadFromStream(in);
    return in;
  }

  bool XMLNode::Validate(const std::string &schema_file_name, std::string &err_msg) {
    // create parser ctxt for schema accessible on schemaPath
    xmlSchemaParserCtxtPtr schemaParser = xmlSchemaNewParserCtxt(schema_file_name.c_str());
    if (!schemaParser) {
        err_msg = "Cannot load schema";
        return false;
    }
    // parse schema
    xmlSchemaPtr schema = xmlSchemaParse(schemaParser);
    if (!schema) {
        xmlSchemaFreeParserCtxt(schemaParser);
        err_msg = "Cannot parse schmea";
        return false;
    }
    xmlSchemaFreeParserCtxt(schemaParser);

    // create schema validation context
    xmlSchemaValidCtxtPtr validityCtx = xmlSchemaNewValidCtxt(schema);
    if (!validityCtx) {
        xmlSchemaFree(schema);
        err_msg = "Cannot create validation context";
        return false;
    }

    // Set contect collectoors
    xmlSchemaSetValidErrors(validityCtx,
                            (xmlSchemaValidityErrorFunc) fprintf,
                            (xmlSchemaValidityWarningFunc) fprintf,
                            stderr);
    // validate against schema
    bool result = (xmlSchemaValidateDoc(validityCtx, node_->doc) == 0);

    // free resources and return result
    xmlSchemaFreeValidCtxt(validityCtx);
    xmlSchemaFree(schema);

    return result;
  }

  XMLNodeContainer::XMLNodeContainer(void) {}

  XMLNodeContainer::XMLNodeContainer(const XMLNodeContainer& container) {
    operator=(container);
  }

  XMLNodeContainer::~XMLNodeContainer(void) {
    for (std::vector<XMLNode*>::iterator n = nodes_.begin();
         n != nodes_.end(); ++n)
      delete *n;
  }

  XMLNodeContainer& XMLNodeContainer::operator=(const XMLNodeContainer& container) {
    for (std::vector<XMLNode*>::iterator n = nodes_.begin();
         n != nodes_.end(); ++n)
      delete *n;
    for (std::vector<XMLNode*>::const_iterator n = container.nodes_.begin();
         n != container.nodes_.end(); ++n) {
      if ((*n)->is_owner_)
        AddNew(*(*n));
      else
        Add(*(*n));
    }
    return *this;
  }

  void XMLNodeContainer::Add(const XMLNode& node) {
    XMLNode *new_node = new XMLNode(node);
    nodes_.push_back(new_node);
  }

  void XMLNodeContainer::Add(const std::list<XMLNode>& nodes) {
    for (std::list<XMLNode>::const_iterator n = nodes.begin();
         n != nodes.end(); ++n)
      Add(*n);
  }

  void XMLNodeContainer::AddNew(const XMLNode& node) {
    XMLNode *new_node = new XMLNode();
    node.New(*new_node);
    nodes_.push_back(new_node);
  }

  void XMLNodeContainer::AddNew(const std::list<XMLNode>& nodes) {
    for (std::list<XMLNode>::const_iterator n = nodes.begin();
         n != nodes.end(); ++n)
      AddNew(*n);
  }

  int XMLNodeContainer::Size(void) const {
    return nodes_.size();
  }

  XMLNode XMLNodeContainer::operator[](int n) {
    if (n < 0)
      return XMLNode();
    if (n >= nodes_.size())
      return XMLNode();
    return *nodes_[n];
  }

  std::list<XMLNode> XMLNodeContainer::Nodes(void) {
    std::list<XMLNode> r;
    for (std::vector<XMLNode*>::iterator n = nodes_.begin();
         n != nodes_.end(); ++n)
      r.push_back(**n);
    return r;
  }

} // namespace Arc
