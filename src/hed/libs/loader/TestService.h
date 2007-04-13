#ifndef __ARC_TESTSERVICE_H__
#define __ARC_TESTSERVICE_H__

#include "Service.h"

namespace Test {

class TestService : public Arc::Service 
{
    private:
        int a;
    public:
        TestService(Arc::Config *cfg);
        ~TestService();
        virtual Arc::Message process(Arc::Message msg);
};

}; // namespace Test

#endif /* __ARC_TESTPLUGIN_H__ */
