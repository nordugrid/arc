#ifndef __ARC_TESTMCC_H__
#define __ARC_TESTMCC_H__

#include "MCC.h"

namespace Test {

class TestMCC : public Arc::MCC 
{
    private:
        int a;
    public:
        TestMCC(Arc::Config *cfg);
        ~TestMCC();
        Arc::Message process(Arc::Message);
};

}; // namespace Test

#endif /* __ARC_TESTMCC_H__ */
