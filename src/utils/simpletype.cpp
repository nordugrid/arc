#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>

#include <arc/XMLNode.h>
#include <arc/StringConv.h>

#include "schemaconv.h"

using namespace Arc;

// -------- Simple type ------------

// 1 - class/parent name (C++,XML)
static const char* simple_type_pattern_h = "\
class %1$s: public Arc::XMLNode {\n\
 public:\n\
  static %1$s New(Arc::XMLNode parent);\n\
  %1$s(Arc::XMLNode node);\n\
};\n\
\n\
";

// 1 - class/parent name (C++,XML)
// 2 - class namespace (XML)
static const char* simple_type_pattern_cpp = "\
%1$s %1$s::New(Arc::XMLNode parent) {\n\
  Arc::NS ns;\n\
  ns[\"ns\"]=\"%2$s\";\n\
  %1$s el(parent.NewChild(\"ns:%1$s\",ns));\n\
  return el;\n\
}\n\
\n\
%1$s::%1$s(Arc::XMLNode node):Arc::XMLNode(node){\n\
  Arc::NS ns;\n\
  ns[\"ns\"]=\"%2$s\";\n\
  Namespaces(ns);\n\
}\n\
\n\
";

void simpletypeprint(XMLNode stype,const std::string& ns,std::ostream& h_file,std::ostream& cpp_file) {
  std::string ntype = stype.Attribute("name");
  h_file<<"//simple type: "<<ntype<<std::endl;
  strprintf(h_file,simple_type_pattern_h,ntype);
  strprintf(cpp_file,simple_type_pattern_cpp,ntype,ns);
}

