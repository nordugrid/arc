#ifndef __ARC_SERVICE_JAVA_WRAPPER_H__
#define __ARC_SERVICE_JAVA_WRAPPER_H__

#include <jni.h>
#include "../../hed/libs/message/Service.h"

namespace Arc {
class Service_JavaWrapper: public Arc::Service {
    protected:
        JavaVM *jvm;
        JNIEnv *jenv;
        jclass serviceClass;
        jobject serviceObj;
        JavaVMInitArgs jvm_args;
        JavaVMOption options[1];

        Arc::XMLNode::NS ns_;
        Arc::MCC_Status make_fault(Arc::Message& outmsg);
    public:
        Service_JavaWrapper(Arc::Config *cfg);
        virtual ~Service_JavaWrapper(void);
        /** Service request processing routine */
        virtual Arc::MCC_Status process(Arc::Message&, Arc::Message&);
};

}; // namespace Arc

#endif // __ARC_SERVICE_JAVA_WRAPPER_H__
