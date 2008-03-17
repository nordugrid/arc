%{
#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/IString.h>
#include <arc/Logger.h>
%}
%include <typemaps.i>

%template(NS) std::map<std::string, std::string>;

#ifdef SWIGJAVA
%ignore Arc::MatchXMLName(const XMLNode&, const char*);
%ignore Arc::MatchXMLNamespace(const XMLNode&, const char*);
%ignore XMLNode(const char*, int);
%ignore Name(const char*);
%ignore Attribute(const char*) const;
%ignore NewAttribute(const char*);
%ignore NewChild(const char*, int, bool);
%ignore NewChild(const char*, const NS&, int, bool);
%ignore Config(const char*);
%ignore operator !;
%ignore operator [];
%ignore operator =;
%ignore operator <<;
%ignore operator ++;
%rename(toString) operator std::string;
%rename(toBool) operator bool;
#endif

#ifdef SWIGPYTHON
%include <std_list.i>
%template(XMLNodeList) std::list<Arc::XMLNode>;
%ignore operator !;
%ignore operator [];
%ignore operator =;
%ignore operator ++;
%ignore operator <<;
%rename(toBool) operator bool;
%rename(__str__) operator std::string;
#endif

%apply std::string& OUTPUT { std::string& out_xml_str }; 
%include "../src/hed/libs/common/XMLNode.h"
%clear std::string& out_xml_str;

%include "../src/hed/libs/common/ArcConfig.h"
%include "../src/hed/libs/common/IString.h"
%include "../src/hed/libs/common/Logger.h"
