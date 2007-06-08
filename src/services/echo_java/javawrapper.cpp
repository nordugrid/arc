#ifdef HAVE_CONFIG
#include <config.h>
#endif

#include <iostream>
#include "../../hed/libs/loader/Loader.h"
#include "../../hed/libs/loader/ServiceLoader.h"
#include "../../hed/libs/message/PayloadSOAP.h"
#include "javawrapper.h"

static Arc::Service* get_service(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new Arc::Service_JavaWrapper(cfg);
}

service_descriptor __arc_service_modules__[] = {
    { "javawrapper", 0, &get_service },
    { NULL, 0, NULL }
};

namespace Arc {

Service_JavaWrapper::Service_JavaWrapper(Arc::Config *cfg):Service(cfg) 
{

    /* Initiliaze java engine */
    JavaVMInitArgs jvm_args;
    JavaVMOption options[1];

    JNI_GetDefaultJavaVMInitArgs(&jvm_args);
    jvm_args.version = JNI_VERSION_1_2;
    jvm_args.nOptions = 1;
    options[0].optionString = "-Djava.class.path=.;/home/szferi/Projects/knowarc/arc1/src/services/echo_java/";
    jvm_args.options = options;
    jvm_args.ignoreUnrecognized = JNI_FALSE;
    JNI_CreateJavaVM(&jvm, (void **)&jenv, &jvm_args);
    
    std::cout << "JVM started" << std::endl;

    /* Find and construct class */
    serviceClass = jenv->FindClass("EchoService");
    jmethodID constructorID = jenv->GetMethodID(serviceClass, "<init>", "()V");
    serviceObj = jenv->NewObject(serviceClass, constructorID);
    
    std::cout << "EchoService constructed" << std::endl;
}

Service_JavaWrapper::~Service_JavaWrapper(void) {
    std::cout << "Destroy jvm" << std::endl; 
    jvm->DestroyJavaVM();
}
Arc::MCC_Status Service_JavaWrapper::make_fault(Arc::Message& outmsg) 
{
    Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
    Arc::SOAPFault* fault = outpayload->Fault();
    if(fault) {
        fault->Code(Arc::SOAPFault::Sender);
        fault->Reason("Failed processing request");
    };
    outmsg.Payload(outpayload);
    return Arc::MCC_Status();
}

Arc::MCC_Status Service_JavaWrapper::process(Arc::Message& inmsg, Arc::Message& outmsg) 
{
    jmethodID processID = jenv->GetMethodID(serviceClass, "process", "()V");
    jobject ret = jenv->CallObjectMethod(serviceObj, processID);
    jint b = (jint)ret;
    std::cout << "Java return value:" << b;
    // XXX: ECHO code logic which shoud go to java XXX
    ns_["echo"]="urn:echo";
    Arc::PayloadSOAP* inpayload = NULL;
    try {
        inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if(!inpayload) {
        std::cerr << "ECHO: input is not SOAP" << std::endl;
        return make_fault(outmsg);
    };
    // Analyzing request 
    Arc::XMLNode echo_op = (*inpayload)["echo"];
    if(!echo_op) {
        std::cerr << "ECHO: request is not supported - " << echo_op.Name() << std::endl;
        return make_fault(outmsg);
    };
    std::string say = echo_op["say"];
    std::string hear = "Java result: " + b;
    Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);
    outpayload->NewChild("echo:echoResponse").NewChild("echo:hear")=hear;
    outmsg.Payload(outpayload);
    return Arc::MCC_Status();
}

}; // namespace Arc
