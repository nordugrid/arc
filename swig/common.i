/**
 * Note that the order of the "%include" statements are important! If a
 * "%include" depends on other "%include"s, it should be placed after these
 * "%include" dependencies.
 */

#ifdef SWIGJAVA
/* The static logger object will always exist, so do not set any references. See
 * comments in Arc.i.
 */
%typemap(javaout) Arc::Logger& Arc::Logger::getRootLogger() {
  return new $javaclassname($jnicall, $owner);
}
#endif

%{
typedef long time_t;
typedef unsigned int uint32_t;
#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/ArcLocation.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/DateTime.h>
#include <arc/URL.h>
#include <arc/User.h>
#include <arc/UserConfig.h>
#include <arc/GUID.h>
%}
#ifdef SWIGPYTHON
typedef long time_t;
typedef unsigned int uint32_t;
#endif
%include <typemaps.i>
%include <std_vector.i>

%ignore operator !;
%ignore operator [];
%ignore operator =;
%ignore operator ++;
%ignore operator <<;

%ignore *::operator[];
%ignore *::operator++;
%ignore *::operator--;
%ignore *::operator=;

%ignore Arc::MatchXMLName;
%ignore Arc::MatchXMLNamespace;

%template(XMLNodeList) std::list<Arc::XMLNode>;
%template(URLList) std::list<Arc::URL>;
%template(URLVector) std::vector<Arc::URL>;
%template(URLListMap) std::map< std::string, std::list<Arc::URL> >;
%template(URLLocationList) std::list<Arc::URLLocation>;
%template(XMLNodePList) std::list<Arc::XMLNode*>;
%template(LogDestinationList) std::list<Arc::LogDestination*>;


#ifdef SWIGJAVA
%ignore Arc::XMLNode::XMLNode(const char*);
%ignore Arc::XMLNode::XMLNode(const char*, int);
%ignore Arc::XMLNode::NewChild(const std::string&);
%ignore Arc::XMLNode::NewChild(const std::string&, int);
%ignore Arc::XMLNode::NewChild(const std::string&, NS const&, int);
%ignore Arc::XMLNode::NewChild(const std::string&, NS const&);
%ignore Name(const char*);
%ignore Attribute(const char*) const;
%ignore NewAttribute(const char*);
%ignore NewChild(const char*, int, bool);
%ignore NewChild(const char*, const NS&, int, bool);
%ignore Config(const char*);
%ignore Arc::TargetGenerator::ServiceCounter();


%ignore *::operator==;
%ignore *::operator!=;
%ignore *::operator>>;
%ignore *::operator<;
%ignore *::operator>;
%ignore *::operator<=;
%ignore *::operator>=;
%ignore *::operator+;
%ignore *::operator-;

%rename(toString) operator std::string;
%rename(toBool) operator bool;

%template(XMLNodePListIteratorHandler) listiteratorhandler<Arc::XMLNode*>;
%template(LogDestinationListIteratorHandler) listiteratorhandler<Arc::LogDestination*>;
%template(URLLocationListIteratorHandler) listiteratorhandler<Arc::URLLocation>;
#endif

#ifdef SWIGPYTHON
%rename(toBool) operator bool;
%rename(__str__) operator std::string;
%ignore Arc::TargetGenerator::ServiceCounter();
#endif

%rename(_print) Arc::Config::print;

%apply std::string& OUTPUT { std::string& out_xml_str };
%include "../src/hed/libs/common/XMLNode.h"
%clear std::string& out_xml_str;

%include "../src/hed/libs/common/ArcConfig.h"
%include "../src/hed/libs/common/ArcLocation.h"
%include "../src/hed/libs/common/IString.h"
%rename(LogStream_ostream) LogStream;
%include "../src/hed/libs/common/Logger.h"
%include "../src/hed/libs/common/DateTime.h"
%include "../src/hed/libs/common/URL.h"
%include "../src/hed/libs/common/User.h"
%include "../src/hed/libs/common/UserConfig.h"
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
