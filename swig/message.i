// Wrap contents of $(top_srcdir)/src/hed/libs/message/MCC_Status.h
%{
#include <arc/message/MCC_Status.h>
%}
%ignore Arc::MCC_Status::operator!;
%include "../src/hed/libs/message/MCC_Status.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/message/MessageAttributes.h
%rename(next) Arc::AttributeIterator::operator++;
#ifdef SWIGPYTHON
%pythonappend Arc::MessageAttributes::getAll %{
        d = dict()
        while val.hasMore():
            d[val.key()] = val.__ref__()
            val.next()
        return d
%}
#endif
%{
#include <arc/message/MessageAttributes.h>
%}
%include "../src/hed/libs/message/MessageAttributes.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/message/SecAttr.h
#ifdef SWIGPYTHON
%include <typemaps.i>
%apply std::string& OUTPUT { std::string &val };
#endif
%{
#include <arc/message/SecAttr.h>
%}
%include "../src/hed/libs/message/SecAttr.h"
#ifdef SWIGPYTHON
%clear std::string &val;
#endif


// Wrap contents of $(top_srcdir)/src/hed/libs/message/MessageAuth.h
%{
#include <arc/message/MessageAuth.h>
%}
%ignore Arc::MessageAuth::operator[](const std::string&);
#ifdef SWIGPYTHON
%pythonprepend Arc::MessageAuth::Export %{
        x = XMLNode("<dummy/>")
        args = args[:-1] + (args[-1].fget(), x)

%}
%pythonappend Arc::MessageAuth::Export %{
        return x
%}
#endif
%include "../src/hed/libs/message/MessageAuth.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/message/Message.h
%{
#include <arc/message/Message.h>
%}
%ignore Arc::MessageContext::operator[](const std::string&);
%include "../src/hed/libs/message/Message.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/message/MCC.h
/* The 'operator Config*' and 'operator ChainContext*' methods cannot be
 * wrapped. If they are needed in the bindings, they should be renamed.
 */
%ignore Arc::MCCPluginArgument::operator Config*;
%ignore Arc::MCCPluginArgument::operator ChainContext*;
%{
#include <arc/message/MCC.h>
%}
%include "../src/hed/libs/message/MCC.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/message/PayloadRaw.h
%{
#include <arc/message/PayloadRaw.h>
%}
%ignore Arc::PayloadRawInterface::operator[](Size_t) const;
%ignore Arc::PayloadRaw::operator[](Size_t) const;
%include "../src/hed/libs/message/PayloadRaw.h"

// Wrap contents of $(top_srcdir)/src/hed/libs/message/SOAPEnvelope.h
/* The 'operator XMLNode' method cannot be wrapped. If it is needed in
 * the bindings, it should be renamed.
 */
%ignore Arc::SOAPFault::operator XMLNode;
#ifdef SWIGPYTHON
%apply std::string& OUTPUT { std::string& out_xml_str };
#endif
#ifdef SWIGJAVA
%ignore Arc::SOAPFault::Reason(const std::string&); // Reason(const char*) is wrapped instead which is equivalent to this one.
%ignore Arc::SOAPEnvelope::SOAPEnvelope(const char*); // SOAPEnvelope(const std::string&) is wrapped instead which is equivalent to this one.
%ignore Arc::SOAPEnvelope::SOAPEnvelope(const char*, int); // SOAPEnvelope(const std::string& xml) is wrapped instead which is equivalent to this one.
#endif
%{
#include <arc/message/SOAPEnvelope.h>
%}
%include "../src/hed/libs/message/SOAPEnvelope.h"
#ifdef SWIGPYTHON
%clear std::string& out_xml_str;
#endif

// Wrap contents of $(top_srcdir)/src/hed/libs/message/SOAPMessage.h
%{
#include <arc/message/SOAPMessage.h>
%}
%include "../src/hed/libs/message/SOAPMessage.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/message/PayloadSOAP.h
#ifdef SWIGJAVA
/* Multiple inheritance is not supported in Java, so the PayloadSOAP
 * class might not be fully functional. If needed further investigations
 * must be done. Suppress warning for now.
 */
%warnfilter(SWIGWARN_JAVA_MULTIPLE_INHERITANCE) Arc::PayloadSOAP;
#endif
%{
#include <arc/message/PayloadSOAP.h>
%}
%include "../src/hed/libs/message/PayloadSOAP.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/message/PayloadStream.h
%{
#include <arc/message/PayloadStream.h>
%}
%ignore Arc::PayloadStreamInterface::operator!;
%ignore Arc::PayloadStream::operator!;
#ifdef SWIGJAVA
%ignore Arc::PayloadStreamInterface::Put(const char*); // Put(const std::string&) is wrapped instead which is equivalent to this one.
#endif
%include "../src/hed/libs/message/PayloadStream.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/message/Service.h
%{
#include <arc/message/Service.h>
%}
%ignore Arc::Service::operator!;
/* The 'operator Config*' and 'operator ChainContext*' methods cannot be
 * wrapped. If they are needed in the bindings, they should be renamed.
 */
%ignore Arc::ServicePluginArgument::operator Config*;
%ignore Arc::ServicePluginArgument::operator ChainContext*;
%include "../src/hed/libs/message/Service.h"
