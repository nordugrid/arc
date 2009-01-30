%{
#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/DateTime.h>
#include <arc/URL.h>
#include <arc/GUID.h>
%}
%include <typemaps.i>

%template(NS_map) std::map<std::string, std::string>;


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
%rename(LogStream_ostream) LogStream;
%include "../src/hed/libs/common/Logger.h"
%include "../src/hed/libs/common/DateTime.h"
%include "../src/hed/libs/common/URL.h"
%include "../src/hed/libs/common/GUID.h"


#ifdef SWIGPYTHON

// code from: http://www.nabble.com/Using-std%3A%3Aistream-from-Python-ts7920114.html#a7923253
%inline %{ 
class CPyOutbuf : public std::streambuf 
{ 
public: 
     CPyOutbuf(PyObject* obj) { 
         m_PyObj = obj; 
         Py_INCREF(m_PyObj); 
     } 
     ~CPyOutbuf() { 
         Py_DECREF(m_PyObj); 
     } 
protected: 
     int_type overflow(int_type c) { 
         // Call to PyGILState_Ensure ensures there is Python 
         // thread state created/assigned.
         PyGILState_STATE gstate = PyGILState_Ensure();
         PyObject_CallMethod(m_PyObj, (char*) "write", (char*) "c", c); 
         PyGILState_Release(gstate);
         return c; 
     } 
     std::streamsize xsputn(const char* s, std::streamsize count) { 
         // Call to PyGILState_Ensure ensures there is Python 
         // thread state created/assigned.
         PyGILState_STATE gstate = PyGILState_Ensure();
         PyObject_CallMethod(m_PyObj, (char*) "write", (char*) "s#", s, int(count)); 
         PyGILState_Release(gstate);
         return count; 
     } 
     PyObject* m_PyObj; 
}; 

class CPyOstream : public std::ostream 
{ 
public: 
     CPyOstream(PyObject* obj) : m_Buf(obj), std::ostream(&m_Buf) {} 
private: 
     CPyOutbuf m_Buf; 
}; 

%} 

%pythoncode %{
    def LogStream(file):
        os = CPyOstream(file)
        os.thisown = False
        ls = LogStream_ostream(os)
        ls.thisown = False
        return ls

%}
#endif
