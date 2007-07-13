#ifndef __ARC_TESTSERVICE_H__
#define __ARC_TESTSERVICE_H__

#include "ServiceLoader.h"

namespace Test {

class TestService : public Arc::Service 
{
    private:
        int a;
    public:
        TestService(Arc::Config *cfg);
        ~TestService();
        virtual Arc::MCC_Status process(Arc::Message& request,Arc::Message& response);
};

} // namespace Test

#endif /* __ARC_TESTPLUGIN_H__ */
