#ifdef SWIGPYTHON
%module common

%include "Arc.i"
#endif


/* Python: Avoid creating a new SWIG types for each module, for types that
 * are general for different modules. E.g. StringPair - put it in common, and
 * use the one from common in the other modules.
 */
%template(StringPair) std::pair<std::string, std::string>;
%wraplist(String, std::string)
%template(StringSet) std::set<std::string>;
%template(StringVector) std::vector<std::string>;
%template(StringStringMap) std::map<std::string, std::string>;

#ifndef SWIGIMPORTED

// Wrap contents of $(top_srcdir)/src/hed/libs/common/XMLNode.h
%{
#include <arc/XMLNode.h>
%}
%ignore Arc::MatchXMLName;
%ignore Arc::MatchXMLNamespace;
%ignore Arc::XMLNode::operator!;
%ignore Arc::XMLNode::operator[](const char *) const;
%ignore Arc::XMLNode::operator[](const std::string&) const;
%ignore Arc::XMLNode::operator[](int) const;
%ignore Arc::XMLNode::operator=(const char *);
%ignore Arc::XMLNode::operator=(const std::string&);
%ignore Arc::XMLNode::operator=(const XMLNode&);
%ignore Arc::XMLNode::operator++();
%ignore Arc::XMLNode::operator--();
%ignore Arc::XMLNodeContainer::operator[](int);
%ignore Arc::XMLNodeContainer::operator=(const XMLNodeContainer&);
%ignore operator<<(std::ostream&, const XMLNode&);
%ignore operator>>(std::istream&, XMLNode&);
#ifdef SWIGPYTHON
%include <typemaps.i>
%apply std::string& OUTPUT { std::string& out_xml_str };
#endif
%include "../src/hed/libs/common/XMLNode.h"
%wraplist(XMLNode, Arc::XMLNode);
#ifdef SWIGPYTHON
%clear std::string& out_xml_str;
#endif


// Wrap contents of $(top_srcdir)/src/hed/libs/common/ArcConfig.h
%rename(_print) Arc::Config::print;
%{
#include <arc/ArcConfig.h>
%}
%include "../src/hed/libs/common/ArcConfig.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/common/ArcLocation.h
%{
#include <arc/ArcLocation.h>
%}
%include "../src/hed/libs/common/ArcLocation.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/common/ArcVersion.h
%{
#include <arc/ArcVersion.h>
%}
%include "../src/hed/libs/common/ArcVersion.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/common/IString.h
%{
#include <arc/IString.h>
%}
%ignore Arc::IString::operator=(const IString&);
%ignore operator<<(std::ostream&, const IString&);
%include "../src/hed/libs/common/IString.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/common/Logger.h
%{
#include <arc/Logger.h>
%}
%rename(LogStream_ostream) Arc::LogStream;
%ignore Arc::LogFile::operator!;
%ignore operator<<(std::ostream&, const LoggerFormat&);
%ignore operator<<(std::ostream&, LogLevel);
%ignore operator<<(std::ostream&, const LogMessage&);
#ifdef SWIGPYTHON
// Suppress warnings about unknown classes std::streambuf and std::ostream
%warnfilter(SWIGWARN_TYPE_UNDEFINED_CLASS) CPyOutbuf;
%warnfilter(SWIGWARN_TYPE_UNDEFINED_CLASS) CPyOstream;
// code from: http://www.nabble.com/Using-std%3A%3Aistream-from-Python-ts7920114.html#a7923253
%inline %{
class CPyOutbuf : public std::streambuf {
public:
  CPyOutbuf(PyObject* obj) : m_PyObj(obj) { Py_INCREF(m_PyObj); }
  ~CPyOutbuf() { Py_DECREF(m_PyObj); }
protected:
  int_type overflow(int_type c) {
    // Call to PyGILState_Ensure ensures there is Python
    // thread state created/assigned.
    PyGILState_STATE gstate = PyGILState_Ensure();
    char s = c;
    PyObject_CallMethod(m_PyObj, (char*) "write", (char*) "s#", &s, Py_ssize_t(1));
    PyGILState_Release(gstate);
    return c;
  }
  std::streamsize xsputn(const char* s, std::streamsize count) {
    // Call to PyGILState_Ensure ensures there is Python
    // thread state created/assigned.
    PyGILState_STATE gstate = PyGILState_Ensure();
    PyObject_CallMethod(m_PyObj, (char*) "write", (char*) "s#", s, Py_ssize_t(count));
    PyGILState_Release(gstate);
    return count;
  }
  PyObject* m_PyObj;
};

class CPyOstream : public std::ostream {
public:
  CPyOstream(PyObject* obj) : std::ostream(&m_Buf), m_Buf(obj) {}
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
%include "../src/hed/libs/common/Logger.h"
%wraplist(LogDestination, Arc::LogDestination*);


// Wrap contents of $(top_srcdir)/src/hed/libs/common/DateTime.h
%{
#include <arc/DateTime.h>
%}
%ignore Arc::Time::UNDEFINED;
%ignore Arc::Time::operator=(time_t);
%ignore Arc::Time::operator=(const Time&);
%ignore Arc::Time::operator=(const char*);
%ignore Arc::Time::operator=(const std::string&);
%ignore operator<<(std::ostream&, const Time&);;
%ignore Arc::Period::operator=(time_t);
%ignore Arc::Period::operator=(const Period&);
%ignore operator<<(std::ostream&, const Period&);
%include "../src/hed/libs/common/DateTime.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/common/URL.h
%{
#include <arc/URL.h>
%}
%ignore Arc::URL::operator!;
%ignore Arc::PathIterator::operator++();
%ignore Arc::PathIterator::operator--();
%ignore operator<<(std::ostream&, const URL&);
%include "../src/hed/libs/common/URL.h"
%wraplist(URL, Arc::URL);
%template(URLVector) std::vector<Arc::URL>;
%template(URLListMap) std::map< std::string, std::list<Arc::URL> >;
%wraplist(URLLocation, Arc::URLLocation);


// Wrap contents of $(top_srcdir)/src/hed/libs/common/Utils.h
%ignore Arc::AutoPointer::operator!;
%warnfilter(SWIGWARN_PARSE_NAMED_NESTED_CLASS) Arc::CountedPointer::Base;
%ignore Arc::CountedPointer::operator!;
%ignore Arc::CountedPointer::operator=(T*);
%ignore Arc::CountedPointer::operator=(const CountedPointer<T>&);
// Ignoring functions from Utils.h since swig thinks they are methods of the CountedPointer class, and thus compilation fails.
%ignore Arc::GetEnv;
%ignore Arc::SetEnv;
%ignore Arc::UnsetEnv;
%ignore Arc::EnvLockAcquire;
%ignore Arc::EnvLockRelease;
%ignore Arc::EnvLockWrap;
%ignore Arc::EnvLockUnwrap;
%ignore Arc::EnvLockUnwrapComplete;
%ignore Arc::EnvLockWrapper;
%ignore Arc::InterruptGuard;
%ignore Arc::StrError;
%ignore PersistentLibraryInit;
/* Swig tries to create functions which return a new CountedPointer object.
 * Those functions takes no arguments, and since there is no default
 * constructor for the CountedPointer compilation fails.
 * Adding a "constructor" (used as a function in the cpp file) which
 * returns a new CountedPointer object with newed T object created
 * Ts default constructor. Thus if T has no default constructor,
 * another work around is needed in order to map that CountedPointer
 * wrapped class with swig.
 */
%extend Arc::CountedPointer {
  CountedPointer() { return new Arc::CountedPointer<T>(new T());}
}
%{
#include <arc/Utils.h>
%}
%include "../src/hed/libs/common/Utils.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/common/User.h
%{
#include <arc/User.h>
%}
%ignore Arc::User::operator!;
%include "../src/hed/libs/common/User.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/UserConfig.h>
%rename(toValue) Arc::initializeCredentialsType::operator initializeType; // works with swig 1.3.40, and higher...
%rename(toValue) Arc::initializeCredentialsType::operator Arc::initializeCredentialsType::initializeType; // works with swig 1.3.29
%ignore Arc::UserConfig::operator!;
%ignore Arc::ConfigEndpoint::operator!;
%{
#include <arc/UserConfig.h>
%}
%wraplist(ConfigEndpoint, Arc::ConfigEndpoint);
%include "../src/hed/libs/common/UserConfig.h"
%template(ConfigEndpointMap) std::map<std::string, Arc::ConfigEndpoint>;


// Wrap contents of $(top_srcdir)/src/hed/libs/common/GUID.h
%{
#include <arc/GUID.h>
%}
%include "../src/hed/libs/common/GUID.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/common/CheckSum.h
%{
#include <arc/CheckSum.h>
%}
%include "../src/hed/libs/common/CheckSum.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/common/JobPerfLog.h
%{
#include <arc/JobPerfLog.h>
%}
%include "../src/hed/libs/common/JobPerfLog.h"

#endif // SWIGIMPORTED


// Wrap contents of $(top_srcdir)/src/hed/libs/common/Thread.h
%{
#include <arc/Thread.h>
%}
%ignore Arc::ThreadId;
%include "../src/hed/libs/common/Thread.h"
%template(SimpleConditionList) std::list<Arc::SimpleCondition*>;
