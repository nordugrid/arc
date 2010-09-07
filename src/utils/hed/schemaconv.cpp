#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>

#include <arc/XMLNode.h>
#include <arc/StringConv.h>

#include "schemaconv.h"

using namespace Arc;


bool schemaconv(XMLNode wsdl,std::ostream& h_file,std::ostream& cpp_file,const std::string& name) {
  h_file<<"\
#include <arc/XMLNode.h>\n\
\n\
namespace "<<name<<" {\n\
\n\
";
  cpp_file<<"\
#include \""<<name<<".h\"\n\
\n\
namespace "<<name<<" {\n\
\n\
";
  
  Arc::NS ns;
  ns["wsdl"]="http://schemas.xmlsoap.org/wsdl/";
  ns["xsd"]="http://www.w3.org/2001/XMLSchema";
  wsdl.Namespaces(ns);
  XMLNode schema;
  if(wsdl.FullName() == "wsdl:definitions") {
    schema = wsdl["wsdl:types"]["xsd:schema"];
    if(!schema) {
      std::cerr<<"No Schema in WSDL"<<std::endl;
      return false;
    };  
  } else if(wsdl.FullName() == "xsd:schema") {
    schema = wsdl;
  } else {
    std::cerr<<"Neither WSDL nor XSD found"<<std::endl;
  }
  std::string nnamespace = (std::string)(schema.Attribute("targetNamespace"));
  if(nnamespace.empty()) {
    std::cerr<<"No namespace for new elements in Schema"<<std::endl;
    return false;
  };
  // Declaring classes 
  for(int n = 0;;++n) {
    XMLNode type = schema.Child(n);
    if(!type) break;
    std::string ntype = type.Attribute("name");
    if(ntype.empty()) continue;
    strprintf(h_file,"class %s;\n",ntype);
  };
  // Simple types
  for(XMLNode stype = schema["xsd:simpleType"];(bool)stype;++stype) {
    simpletypeprint(stype,nnamespace,h_file,cpp_file);
  };
  // Complex types
  for(XMLNode ctype = schema["xsd:complexType"];(bool)ctype;++ctype) {
    complextypeprint(ctype,nnamespace,h_file,cpp_file);
  };

  h_file<<"\
} // namespace A\n\
";
  cpp_file<<"\
} // namespace A\n\
\n\
";

  return true;
}


