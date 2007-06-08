%{
/* #include "../src/hed/libs/message/AuthNHandler.h"
#include "../src/hed/libs/message/AuthZHandler.h" */
#include "../src/hed/libs/message/MCC.h"
#include "../src/hed/libs/message/MCC_Status.h"
#include "../src/hed/libs/message/MessageAttributes.h"
/* #include "../src/hed/libs/message/MessageAuth.h" */
#include "../src/hed/libs/message/Message.h"
#include "../src/hed/libs/message/PayloadRaw.h"
#include "../src/hed/libs/message/PayloadSOAP.h"
#include "../src/hed/libs/message/PayloadStream.h"
#include "../src/hed/libs/message/Service.h"
#include "../src/hed/libs/message/SOAPMessage.h"
%}
%warnfilter(SWIGWARN_JAVA_MULTIPLE_INHERITANCE) PayloadSOAP;

#ifdef SWIGJAVA
%typemap(javainterfaces) PayloadSOAP "MessagePayload";
%typemap(javaclassmodifiers) MessagePayload "public interface";
%typemap(javabody) MessagePayload "";
%typemap(javafinalize) MessagePayload "";
%typemap(javadestruct) MessagePayload "";
#endif

/* %include "../src/hed/libs/message/AuthNHandler.h"
%include "../src/hed/libs/message/AuthZHandler.h" */
%include "../src/hed/libs/message/MCC.h"
#include "../src/hed/libs/message/MCC_Status.h"
%include "../src/hed/libs/message/MessageAttributes.h"
/* %include "../src/hed/libs/message/MessageAuth.h" */
%include "../src/hed/libs/message/Message.h"
%include "../src/hed/libs/message/PayloadRaw.h"
%include "../src/hed/libs/message/PayloadSOAP.h"
%include "../src/hed/libs/message/PayloadStream.h"
%include "../src/hed/libs/message/Service.h"
%include "../src/hed/libs/message/SOAPMessage.h"
