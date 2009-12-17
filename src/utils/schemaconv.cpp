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

// -------------- Complex type --------------

// 1 - class/parent name (C++,XML)
static const char* complex_type_header_pattern_h = "\
class %1$s: public Arc::XMLNode {\n\
 public:\n\
";

// 1 - class/parent name (C++,XML)
static const char* complex_type_footer_pattern_h = "\
  static %1$s New(Arc::XMLNode parent);\n\
  %1$s(Arc::XMLNode node);\n\
};\n\
\n\
";

// 1 - class/parent name (C++,XML)
// 2 - class namespace (XML)
static const char* complex_type_constructor_header_pattern_cpp = "\
%1$s::%1$s(Arc::XMLNode node) {\n\
  Arc::NS ns;\n\
  ns[\"ns\"]=\"%2$s\";\n\
  Namespaces(ns);\n\
";

// 1 - class/parent name (C++,XML)
// 2 - class namespace (XML)
static const char* complex_type_constructor_footer_pattern_cpp = "\
}\n\
\n\
%1$s %1$s::New(Arc::XMLNode parent) {\n\
  Arc::NS ns;\n\
  ns[\"ns\"]=\"%2$s\";\n\
  %1$s el(parent.NewChild(\"ns:%1$s\",ns));\n\
  return el;\n\
}\n\
";

// 1 - element name (C++,XML)
// 2 - element type (C++)
static const char* mandatory_element_pattern_h = "\
  %2$s %1$s(void);\n\
";

// 1 - element name (C++,XML)
// 2 - element type (C++)
static const char* optional_element_pattern_h = "\
  %2$s %1$s(bool create = false);\n\
";

// 1 - element name (C++,XML)
// 2 - element type (C++)
static const char* array_element_pattern_h = "\
  %2$s %1$s(int index,bool create = false);\n\
";

// 1 - element name (C++,XML)
static const char* mandatory_element_constructor_pattern_cpp = "\
  (void)%1$s();\n\
";

// 1 - element name (C++,XML)
// 2 - element type (C++)
// 3 - class/parent name (C++,XML)
// 4 - element namespace prefix (XML)
// 5 - element type (XML)
static const char* mandatory_element_method_pattern_cpp = "\
%2$s %3$s::%1$s(void) {\n\
  Arc::XMLNode node = operator[](\"%4$s:%5$s\");\n\
  if(!node) node = NewChild(\"%4$s:%5$s\");\n\
  return node;\n\
}\n\
";

// 1 - element name (C++,XML)
static const char* optional_element_constructor_pattern_cpp = "\
";

// 1 - element name (C++,XML)
// 2 - element type (C++)
// 3 - class/parent name (C++,XML)
// 4 - element namespace prefix (XML)
// 5 - element type (XML)
static const char* optional_element_method_pattern_cpp = "\
%2$s %3$s::%1$s(bool create) {\n\
  Arc::XMLNode node = operator[](\"%4$s:%5$s\");\n\
  if(create && !node) node = NewChild(\"%4$s:%5$s\");\n\
  return node;\n\
}\n\
";

// 1 - element name (C++,XML)
// 2 - minimal number of elements
static const char* array_element_constructor_pattern_cpp = "\
  if(%2$s > 0) (void)%1$s(%2$s - 1);\n\
";

// 1 - element name (C++,XML)
// 2 - element type (C++)
// 3 - class/parent name (C++,XML)
// 4 - element namespace prefix (XML)
// 5 - element type (XML)
// 6 - minimal number of elements
static const char* array_element_method_pattern_cpp = "\
%2$s %3$s::%1$s(int index,bool create) {\n\
  if(index < %6$s) create = true;\n\
  Arc::XMLNode node = operator[](\"%4$s:%5$s\")[index];\n\
  if(create && !node) {\n\
    for(int n = 0;n<index;++n) {\n\
      node = operator[](\"%4$s:%5$s\")[index];\n\
      if(!node) {\n\
        node = NewChild(\"%4$s:%5$s\");\n\
        if(!node) break; // protection\n\
      };\n\
    };\n\
  };\n\
  return node;\n\
}\n\
";


static void strprintf(std::ostream& out,const char* fmt,
                const std::string& arg1 = "",const std::string& arg2 = "",
                const std::string& arg3 = "",const std::string& arg4 = "",
                const std::string& arg5 = "",const std::string& arg6 = "",
                const std::string& arg7 = "",const std::string& arg8 = "",
                const std::string& arg9 = "",const std::string& arg10 = "") {
  char buf[65536];
  buf[0]=0;
  snprintf(buf,sizeof(buf)-1,fmt,arg1.c_str(),arg2.c_str(),arg3.c_str(),
                                 arg4.c_str(),arg5.c_str(),arg6.c_str(),
                                 arg7.c_str(),arg8.c_str(),arg9.c_str(),
                                 arg10.c_str());
  buf[sizeof(buf)-1]=0;
  out<<buf;
}

static void strprintf(std::string& out,const char* fmt,
                const std::string& arg1 = "",const std::string& arg2 = "",
                const std::string& arg3 = "",const std::string& arg4 = "",
                const std::string& arg5 = "",const std::string& arg6 = "",
                const std::string& arg7 = "",const std::string& arg8 = "",
                const std::string& arg9 = "",const std::string& arg10 = "") {
  char buf[65536];
  buf[0]=0;
  snprintf(buf,sizeof(buf)-1,fmt,arg1.c_str(),arg2.c_str(),arg3.c_str(),
                                 arg4.c_str(),arg5.c_str(),arg6.c_str(),
                                 arg7.c_str(),arg8.c_str(),arg9.c_str(),
                                 arg10.c_str());
  buf[sizeof(buf)-1]=0;
  out+=buf;
}

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
  if(wsdl.FullName() != "wsdl:definitions") {
    std::cerr<<"Not a WSDL"<<std::endl;
    return false;
  };
  XMLNode schema = wsdl["wsdl:types"]["xsd:schema"];
  if(!schema) {
    std::cerr<<"No Schema in WSDL"<<std::endl;
    return false;
  };  
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
    std::string ntype = stype.Attribute("name");
    h_file<<"//simple type: "<<ntype<<std::endl;
    strprintf(h_file,simple_type_pattern_h,ntype);
    strprintf(cpp_file,simple_type_pattern_cpp,ntype,nnamespace);
  };
  // Complex types
  for(XMLNode ctype = schema["xsd:complexType"];(bool)ctype;++ctype) {
    std::string ntype = ctype.Attribute("name");
    h_file<<"//complex type: "<<ntype<<std::endl;
    XMLNode sequence = ctype["xsd:sequence"];
    if(!sequence) {
      std::cout<<"complex type is not sequence - not supported"<<std::endl;
    };
    strprintf(h_file,complex_type_header_pattern_h,ntype);
    std::string complex_file_constructor_cpp;
    for(XMLNode element = sequence["xsd:element"];(bool)element;++element) {
      int minoccurs = 1;
      int maxoccurs = 1;
      std::string ename = element.Attribute("name");
      std::string etype = element.Attribute("type");
      std::string eprefix;
      std::string::size_type p = etype.find(':');
      if(p != std::string::npos) {
        eprefix=etype.substr(0,p);
        etype.erase(0,p+1);
      };
      std::string enamespace = element.Namespaces()[eprefix];
      std::string cpptype = etype;
      if(enamespace != nnamespace) {
        cpptype = "Arc::XMLNode";
      } else {
        eprefix = "ns";
      };
      std::string n;
      n=(std::string)(element.Attribute("minOccurs"));
      if(!n.empty()) {
        minoccurs=stringto<int>((std::string)n);
      };
      n=(std::string)(element.Attribute("maxOccurs"));
      if(!n.empty()) {
        if(n == "unbounded") {
          maxoccurs=-1;
        } else {
          maxoccurs=stringto<int>((std::string)n);
        };
      };
      if(maxoccurs != -1) {
        if(maxoccurs < minoccurs) {
          std::cout<<"  maxOccurs is smaller than minOccurs"<<std::endl;
          continue;
        };
      };
      if(maxoccurs == 0) {
        std::cout<<"  maxOccurs is zero"<<std::endl;
        continue;
      }; 
      h_file<<"//  element: "<<ename<<std::endl;
      h_file<<"//    type: "<<etype<<std::endl;
      h_file<<"//    prefix: "<<eprefix<<std::endl;
      h_file<<"//    namespace: "<<enamespace<<std::endl;
      h_file<<"//    minOccurs: "<<minoccurs<<std::endl;
      h_file<<"//    maxOccurs: "<<maxoccurs<<std::endl;
      // header
      if((maxoccurs == 1) && (minoccurs == 1)) {
        strprintf(h_file,mandatory_element_pattern_h,ename,cpptype);
      } else if((maxoccurs == 1) && (minoccurs == 0)) {
        strprintf(h_file,optional_element_pattern_h,ename,cpptype);
      } else {
        strprintf(h_file,array_element_pattern_h,ename,cpptype);
      };
      // constructor
      if((maxoccurs == 1) && (minoccurs == 1)) {
        strprintf(complex_file_constructor_cpp,
                  mandatory_element_constructor_pattern_cpp,ename);
      } else if((maxoccurs == 1) && (minoccurs == 0)) {
        strprintf(complex_file_constructor_cpp,
                  optional_element_constructor_pattern_cpp,ename);
      } else {
        strprintf(complex_file_constructor_cpp,
                  array_element_constructor_pattern_cpp,ename,
                  tostring(minoccurs));
      };
      // method
      if((maxoccurs == 1) && (minoccurs == 1)) {
        strprintf(cpp_file,mandatory_element_method_pattern_cpp,
           ename,cpptype,ntype,eprefix,etype);
      } else if((maxoccurs == 1) && (minoccurs == 0)) {
        strprintf(cpp_file,optional_element_method_pattern_cpp,
           ename,cpptype,ntype,eprefix,etype);
      } else {
        strprintf(cpp_file,array_element_method_pattern_cpp,
           ename,cpptype,ntype,eprefix,etype,
           tostring(minoccurs));
      };
    };
    strprintf(h_file,complex_type_footer_pattern_h,ntype);
    strprintf(cpp_file,complex_type_constructor_header_pattern_cpp,
                               ntype,nnamespace);
    cpp_file<<complex_file_constructor_cpp;
    strprintf(cpp_file,complex_type_constructor_footer_pattern_cpp,
                               ntype,nnamespace);
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


