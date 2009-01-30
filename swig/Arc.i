%module arc

%include <stl.i>

#ifdef SWIGPYTHON
%include <std_list.i>
%template(StringList) std::list<std::string>;
#endif

%include "common.i" 
%include "message.i"
%include "client.i"
%include "data.i"
%include "delegation.i"
#ifdef SWIGPYTHON
%include "security.i"
#endif
