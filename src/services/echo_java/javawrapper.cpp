#ifdef HAVE_CONFIG
#include <config.h>
#endif

#include <iostream>
#include "../../hed/libs/loader/Loader.h"
#include "../../hed/libs/loader/ServiceLoader.h"
#include "../../hed/libs/message/SOAPMessage.h"
#include "javawrapper.h"

static Arc::Service* get_service(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new Arc::Service_JavaWrapper(cfg);
}

service_descriptor __arc_service_modules__[] = {
    { "arcservice_javawrapper", 0, &get_service },
    { NULL, 0, NULL }
};

namespace Arc {


Service_JavaWrapper::Service_JavaWrapper(Arc::Config *cfg):Service(cfg) 
{
    JNIEnv *jenv = NULL;
    JavaVMInitArgs jvm_args;
    JavaVMOption options[1];
    /* Initiliaze java engine */
    JNI_GetDefaultJavaVMInitArgs(&jvm_args);
    jvm_args.version = JNI_VERSION_1_2;
    jvm_args.nOptions = 1;
    options[0].optionString = "-Djava.class.path=.:/home/szferi/Projects/knowarc/arc1/src/services/echo_java/:/home/szferi/Projects/knowarc/arc1/java/arc.jar";
    jvm_args.options = options;
    jvm_args.ignoreUnrecognized = JNI_FALSE;
    JNI_CreateJavaVM(&jvm, (void **)&jenv, &jvm_args);
    
    std::cout << "JVM started" << std::endl;

    /* Find and construct class */
    serviceClass = jenv->FindClass("EchoService");
    if (serviceClass == NULL) {
        std::cerr << "There is no service: " << "EchoService" << " in your java class search path" << std::endl;
        if (jenv->ExceptionOccurred()) {
            jenv->ExceptionDescribe();
        }
        return;
    }
    jmethodID constructorID = jenv->GetMethodID(serviceClass, "<init>", "()V");
    if (constructorID == NULL) {
        std::cerr << "There is no constructor function" << std::endl;
        if (jenv->ExceptionOccurred()) {
            jenv->ExceptionDescribe();
        }
        return;
    }
    serviceObj = jenv->NewObject(serviceClass, constructorID);
    std::cout << "EchoService constructed" << std::endl;
}

Service_JavaWrapper::~Service_JavaWrapper(void) {
    std::cout << "Destroy jvm" << std::endl; 
    jvm->DestroyJavaVM();
}

Arc::MCC_Status Service_JavaWrapper::make_fault(Arc::Message& outmsg) 
{
    Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(Arc::NS(),true);
    Arc::SOAPFault* fault = outpayload->Fault();
    if(fault) {
        fault->Code(Arc::SOAPFault::Sender);
        fault->Reason("Failed processing request");
    };
    outmsg.Payload(outpayload);
    return Arc::MCC_Status();
}

Arc::MCC_Status Service_JavaWrapper::java_error(JNIEnv *jenv, const char *str) {
    std::cerr << str << std::endl;
    if (jenv->ExceptionOccurred()) {
        jenv->ExceptionDescribe();
    }
    /* Cleanup */
    jvm->DetachCurrentThread();
    return Arc::MCC_Status(Arc::GENERIC_ERROR);
}

Arc::MCC_Status Service_JavaWrapper::process(Arc::Message& inmsg, Arc::Message& outmsg) 
{
    JNIEnv *jenv = NULL;

    /* Attach to the current java engine thread */
    jint aret = jvm->AttachCurrentThread((void **)&jenv, NULL);
    printf("JEnv: %p\n");
    /* Get the process function of service */
    jmethodID processID = jenv->GetMethodID(serviceClass, "process", "(Lorg/nordugrid/arc/SOAPMessage;Lorg/nordugrid/arc/SOAPMessage;)Lorg/nordugrid/arc/MCC_Status;");
    if (processID == NULL) {
        return java_error(jenv,"Cannot find process method of java class");
    }
    /* convert inmsg and outmsg to java objects */
    Arc::SOAPMessage *inmsg_ptr = NULL;
    Arc::SOAPMessage *outmsg_ptr = NULL;
    try {
        inmsg_ptr = new Arc::SOAPMessage(inmsg);
        outmsg_ptr = new Arc::SOAPMessage(outmsg);
    } catch(std::exception& e) { };
    if(!inmsg_ptr) {
        std::cerr << "ECHO: input is not SOAP" << std::endl;
        return make_fault(outmsg);
    };
    if(!outmsg_ptr) {
        std::cerr << "ECHO: output is not SOAP" << std::endl;
        return make_fault(outmsg);
    };

    jclass JSOAPMessageClass = jenv->FindClass("org/nordugrid/arc/SOAPMessage");
    if (JSOAPMessageClass == NULL) {
        return java_error(jenv,"Cannot find SOAPMessage object");
    }
    /* Get the constructor of java object */
    jmethodID constructorID = jenv->GetMethodID(JSOAPMessageClass, "<init>", "(I)V");
    if (constructorID == NULL) {
        return java_error(jenv,"Cannot find constructor function of message");
    }
    /* Convert C++ object to Java objects */
    jobject jinmsg = jenv->NewObject(JSOAPMessageClass, constructorID, (jlong)((long int)inmsg_ptr));
    if (jinmsg == NULL) {
        return java_error(jenv,"Cannot convert input message to java object");
    }
    jobject joutmsg = jenv->NewObject(JSOAPMessageClass, constructorID, (jlong)((long int)outmsg_ptr));
    if (jinmsg == NULL) {
        return java_error(jenv,"Cannot convert output message to java object");
    }
    /* Create arguments for java process function */
    jvalue args[2];
    args[0].l = jinmsg;
    args[1].l = joutmsg;
    /* Call the process method of Java object */
    jobject jmcc_status = jenv->CallObjectMethodA(serviceObj, processID, args);
    if (jmcc_status == NULL) {
        return java_error(jenv,"Error in call process function of java object");
    }
    /* Get SWIG specific getCPtr function of Message class */
    jmethodID msg_getCPtrID = jenv->GetStaticMethodID(JSOAPMessageClass, "getCPtr", "(Lorg/nordugrid/arc/SOAPMessage;)J");
    if (msg_getCPtrID == NULL) {
        return java_error(jenv,"Cannot find getCPtr method of java Message class");
    }
    /* Get Java MCC_Status class */
    jclass JMCC_StatusClass = jenv->FindClass("org/nordugrid/arc/MCC_Status");
    if (JMCC_StatusClass == NULL) {
        std::cerr << "Cannot find MCC_Status object" << std::endl;
        /* Cleanup */
        jvm->DetachCurrentThread();
        return Arc::MCC_Status(Arc::GENERIC_ERROR);
    }
    /* Get SWIG specific getCPtr function of MCC_Status class */
    jmethodID mcc_status_getCPtrID = jenv->GetStaticMethodID(JMCC_StatusClass, "getCPtr", "(Lorg/nordugrid/arc/MCC_Status;)J");
    if (mcc_status_getCPtrID == NULL) {
        return java_error(jenv,"Cannot find getCPtr method of java MCC_Status class");
    }
    
    /* Convert Java status object to C++ class */
    jlong mcc_status_addr = jenv->CallStaticLongMethod(JMCC_StatusClass, mcc_status_getCPtrID, jmcc_status);
    printf("%p\n", mcc_status_addr);
    Arc::MCC_Status status(*((Arc::MCC_Status *)(long)mcc_status_addr));
    /* Convert Java output message object to C++ class */
    jlong outmsg_addr = jenv->CallStaticLongMethod(JSOAPMessageClass, msg_getCPtrID, joutmsg);
     
    Arc::SOAPMessage *outmsg_ptr2 = (Arc::SOAPMessage *)(long)outmsg_addr;
    // printf("%p\n", outmsg_ptr2);
    // std::string xml;
    // outmsg_ptr2->Payload()->GetXML(xml);   
    // std::cout << xml << std::endl;
    Arc::PayloadSOAP *pl = new Arc::PayloadSOAP(*(outmsg_ptr2->Payload()));
    outmsg.Payload((MessagePayload *)pl);
#if 0
    ->GetXML(xml);
    std::cout << "XML: "<< xml << std::endl;
#endif
    // XXX: how to handle error?
    /* Detach from the Java engine */
    jvm->DetachCurrentThread();
    return status;
}

}; // namespace Arc
