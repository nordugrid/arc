%{
#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
%}
%template(NS) std::map<std::string, std::string>;

#ifdef SWIGJAVA
%ignore Arc::MatchXMLName(const XMLNode& node1,const XMLNode& node2);
%ignore Arc::MatchXMLName(const XMLNode& node,const char* name);
%ignore XMLNode(const char* xml,int len = -1);
%ignore Name(const char *name);
%ignore NewAttribute(const char *);
%ignore NewChild(const char *);
%ignore NewChild(const char *, int);
%ignore NewChild(const char *, int, bool);
%ignore Config(const char *);
%ignore operator !;
%ignore operator [];
%ignore operator =;
%ignore operator <<;
%ignore operator ++;
%rename(toString) operator std::string;
%rename(toBool) operator bool;
#endif
#ifdef SWIGPYTHON
%ignore operator !;
%ignore operator [];
%ignore operator =;
%ignore operator ++;
%ignore operator <<;
%rename(toBool) operator bool;
%rename(__str__) operator std::string;
#endif

%include "../src/hed/libs/common/XMLNode.h"
%include "../src/hed/libs/common/XMLNode.h"
%include "../src/hed/libs/common/ArcConfig.h"
%include "../src/hed/libs/common/Logger.h"
