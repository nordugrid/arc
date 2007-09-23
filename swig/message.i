%{
#include <arc/message/MCC.h>
#include <arc/message/MCC_Status.h>
#include <arc/message/MessageAttributes.h>
#include <arc/message/MessageAuth.h>
#include <arc/message/Message.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/SOAPMessage.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/PayloadStream.h>
#include <arc/message/Service.h>
%}
/* %warnfilter(SWIGWARN_JAVA_MULTIPLE_INHERITANCE) PayloadSOAP; */

%ignore Arc::PayloadStream::Put(char const *);
%ignore Arc::SOAPEnvelop(char const *);
%ignore Arc::PayloadRawInterface;
%ignore Arc::PayloadStreamInterface;
%ignore Arc::ContentFromPayload;
/* %ignore Arc::MessagePayload; */
/* %ignore Arc::PayloadRawInterface::operator [](int) const;
%ignore Arc::PayloadRawInterface::~PayloadRawInterface(void); */

#ifdef SWIGJAVA
%typemap(javainterfaces) Arc::PayloadSOAP "MessagePayload";
%typemap(javaclassmodifiers) Arc::MessagePayload "public interface";
%typemap(javabody) Arc::MessagePayload "";
%typemap(javafinalize) Arc::MessagePayload "";
%typemap(javadestruct,methodname="delete") Arc::MessagePayload "";

/*
%typemap(javaclassmodifiers) Arc::PayloadRawInterface "public interface";
%typemap(javabody) Arc::PayloadRawInterface "";
%typemap(javafinalize) Arc::PayloadRawInterface "";
%typemap(javadestruct) Arc::PayloadRawInterface ""; 

%typemap(javaout) Arc::PayloadRawInterface "";
%typemap(javaout) char *Arc::PayloadRawInterface::Content ";";
%javamethodmodifiers Arc::PayloadRawInterface::Content "abstract public";

%typemap(javaout) int Arc::PayloadRawInterface::Size ";";
%javamethodmodifiers Arc::PayloadRawInterface::Size "abstract public";

%typemap(javaout) char *Arc::PayloadRawInterface::Insert ";";
%javamethodmodifiers Arc::PayloadRawInterface::Insert "abstract public";

%typemap(javaout) char *Arc::PayloadRawInterface::Buffer ";";
%javamethodmodifiers Arc::PayloadRawInterface::Buffer "abstract public";

%typemap(javaout) int Arc::PayloadRawInterface::BufferSize ";";
%javamethodmodifiers Arc::PayloadRawInterface::BufferSize "abstract public";

%rename(inc) operator ++;
*/
#endif

%include "../src/hed/libs/message/MCC.h"
%include "../src/hed/libs/message/MCC_Status.h"
%include "../src/hed/libs/message/MessageAttributes.h"
%include "../src/hed/libs/message/MessageAuth.h"
%include "../src/hed/libs/message/Message.h"
%include "../src/hed/libs/message/PayloadRaw.h"
%include "../src/hed/libs/message/SOAPEnvelope.h"
%include "../src/hed/libs/message/SOAPMessage.h"
%include "../src/hed/libs/message/PayloadSOAP.h"
%include "../src/hed/libs/message/PayloadStream.h"
%include "../src/hed/libs/message/Service.h"
