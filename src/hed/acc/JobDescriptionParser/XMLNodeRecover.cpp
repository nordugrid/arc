// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>
#include <iostream>

#include "XMLNodeRecover.h"

namespace Arc {

  XMLNodeRecover::XMLNodeRecover(const std::string& xml) : XMLNode() {
    xmlSetStructuredErrorFunc(this, &structured_error_handler);
    xmlDocPtr doc = xmlRecoverMemory(xml.c_str(), xml.length());
    xmlSetStructuredErrorFunc(this, NULL);

    if (!doc) {
      return;
    }
    xmlNodePtr p = doc->children;
    for (; p; p = p->next) {
      if (p->type == XML_ELEMENT_NODE) break;
    }
    if (!p) {
      xmlFreeDoc(doc);
      return;
    }

    node_ = p;
    is_owner_ = true;
  }

  XMLNodeRecover::~XMLNodeRecover() {
    for (std::list<xmlErrorPtr>::const_iterator it = errors.begin();
         it != errors.end(); ++it) {
      if(*it) {
        xmlResetError(*it);
        delete *it;
      }
    }
  }

  void XMLNodeRecover::print_error (const xmlError& error) {
    std::cerr << "Domain: " << error.domain << std::endl;
    std::cerr << "Code: " << error.code << std::endl;
    std::cerr << "Message: " << error.message << std::endl;
    std::cerr << "Level: " << error.level << std::endl;
    std::cerr << "Filename: " << error.file << std::endl;
    std::cerr << "Line: " << error.line << std::endl;
    if (error.str1) std::cerr << "Additional info: " << error.str1 << std::endl;
    if (error.str2) std::cerr << "Additional info: " << error.str2 << std::endl;
    if (error.str3) std::cerr << "Additional info: " << error.str3 << std::endl;
    std::cerr << "Extra number: " << error.int1 << std::endl;
    std::cerr << "Column: " << error.int2 << std::endl;
    std::cerr << "Context is " << (error.ctxt == NULL ? "NULL" : "not NULL") << std::endl;
    std::cerr << "Node is " << (error.node == NULL ? "NULL" : "not NULL") << std::endl;
  }

  void XMLNodeRecover::structured_error_handler(void *userData, xmlErrorPtrType error) {
    if (error == NULL) {
      return;
    }

    XMLNodeRecover *xml = static_cast<XMLNodeRecover*>(userData);
    if (xml == NULL) {
      return;
    }
    xmlErrorPtr new_error = new xmlError();
    std::memset(new_error, 0, sizeof(xmlError));
    xmlCopyError(error, new_error);
    xml->errors.push_back(new_error);
  }

} // namespace Arc
