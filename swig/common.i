%{
#include "../src/libs/common/ArcConfig.h"
#include "../src/libs/common/XMLNode.h"
%}
#ifdef SWIGJAVA
%rename(not_equal) operator !;
%rename(Get) operator [];
%rename(to_string) operator std::string;
%rename(equal) operator =;
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
