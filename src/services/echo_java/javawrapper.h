#ifndef __ARC_SERVICE_JAVA_WRAPPER_H__
#define __ARC_SERVICE_JAVA_WRAPPER_H__

#include <jni.h>
#include "message/Service.h"
#include "common/Logger.h"

namespace Arc {
class Service_JavaWrapper: public Arc::Service {
    protected:
        JavaVM *jvm;
        jclass serviceClass;
        jobject serviceObj;
        
        Arc::MCC_Status make_fault(Arc::Message& outmsg);
        Arc::MCC_Status java_error(JNIEnv *jenv, const char *str);
	static Arc::Logger logger;
    public:
        Service_JavaWrapper(Arc::Config *cfg);
        virtual ~Service_JavaWrapper(void);
        /** Service request processing routine */
        virtual Arc::MCC_Status process(Arc::Message&, Arc::Message&);
};

}; // namespace Arc

#endif // __ARC_SERVICE_JAVA_WRAPPER_H__
