#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "TestMCC.h"

namespace Test {

TestMCC::TestMCC(Arc::Config *cfg) : MCC(cfg)
{
    std::cout << "Init: TestMCC" << std::endl;
    a = 1;
}

TestMCC::~TestMCC(void) 
{
    std::cout << "Destroy: TestMCC " << std::endl;
}

Arc::Message TestMCC::request(Arc::Message)
{
	std::cout << "request: TestMCC" << std::endl;
    std::cout << "private variable: " << a << std::endl; 
}

Arc::MCC *get_mcc_instance(Arc::Config *cfg) 
{
    return (Arc::MCC *)(new TestMCC(cfg));
}

}; // namespace Test

/* MCC plugin descriptor */
mcc_descriptor descriptor = {
    0,                        /* version */
    Test::get_mcc_instance    /* get_instance function */
};
