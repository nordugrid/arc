%{
#include "../src/libs/common/ArcConfig.h"
/* #include "../src/libs/common/BasicDT.h" */
#include "../src/libs/common/Logger.h"
/* #include "../src/libs/common/Thread.h" */
#include "../src/libs/common/XMLNode.h"
%}

%ignore Arc::Config(char const *);
%ignore Arc::XMLNode(char const *);
%ignore Arc::XMLNode::operator [](char const *) const;
%ignore Arc::XMLNode::operator =(char const *);
%ignore Arc::NewChild(char const *,int,bool);
%ignore Arc::NewChild(char const *,int);
%ignore Arc::NewChild(char const *);

%template(NS) std::map<std::string, std::string>;

#ifdef SWIGJAVA
%rename(not_equal) operator !;
%rename(Get) operator []; 
%rename(toString) operator std::string;
%rename(equal) operator =;
%rename(Set) Arc::XMLNode::operator =;
%rename(toBool) operator bool;
#endif

#ifdef SWIGPYTHON
%rename(__ne__) operator !;
%rename(__getitem__) operator[];
%rename(__str__) operator std::string;
%rename(__eq__) operator =;
%rename(toBool) operator bool;
#endif

%include "../src/libs/common/XMLNode.h"
%include "../src/libs/common/ArcConfig.h"
/* %include "../src/libs/common/BasicDT.h" */
%include "../src/libs/common/Logger.h"
/* %include "../src/libs/common/Thread.h" */
