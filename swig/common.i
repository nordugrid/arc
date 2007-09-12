%{
#include <arc/ArcConfig.h>
/* #include "../src/libs/common/BasicDT.h" */
#include <arc/Logger.h>
/* #include "../src/libs/common/Thread.h" */
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

%include <arc/XMLNode.h>
/*
#ifdef SWIGPYTHON
%extend Arc::XMLNode {
    Arc::XMLNode __getitem__(const char *name) const {
        return self->operator[](name);
    }
    
    Arc::XMLNode __getitem__(const std::string& name) const {
        return self->operator[](name.c_str());
    }
} 
#endif */
%include <arc/ArcConfig.h>
/* %include "../src/libs/common/BasicDT.h" */
%include <arc/Logger.h>
/* %include "../src/libs/common/Thread.h" */
