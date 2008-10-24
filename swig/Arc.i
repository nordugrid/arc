%module arc
%include <stl.i>

%include "common.i" 
%include "message.i"
%include "client.i"
%include "data.i"
#ifdef SWIGPYTHON
%include "security.i"
#endif
