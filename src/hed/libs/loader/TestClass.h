#ifndef __ARC_TESTPLUGIN_H__
#define __ARC_TESTPLUGIN_H__

#include "MCC.h"

namespace TestPlugin {

class TestClass : public Arc::MCC 
{
    private:
        int a;
    public:
        TestClass(Arc::Config *cfg);
        ~TestClass();
        void request(void);
};

}; // namespace TestPlugin

#endif /* __ARC_TESTPLUGIN_H__ */
