%{
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
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
/* %rename(Get) operator []; */
%rename(toString) operator std::string;
/* %rename(equal) operator =; */
/* %rename(Set) Arc::XMLNode::operator =; */
%rename(toBool) operator bool;
#endif

#ifdef SWIGPYTHON
%ignore Arc::XMLNode::operator [](const std::string&) const;
%ignore Arc::XMLNode::operator [](int) const;
%rename(__ne__) operator !;
/* %rename(__getitem__) operator[];  */
%rename(__str__) operator std::string;
/* %rename(Set) Arc::XMLNode::operator =; */
/* %rename(__eq__) operator =; */
%rename(toBool) operator bool;
#endif

%include "../src/hed/libs/common/ArcConfig.h"
%include "../src/hed/libs/common/Logger.h"
%include "../src/hed/libs/common/XMLNode.h"
