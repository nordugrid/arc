#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "TestClass.h"

namespace TestPlugin {

TestClass::TestClass(Arc::Config *cfg) : MCC(cfg)
{
    std::cout << "Init: TestClass" << std::endl;
    a = 1;
}

TestClass::~TestClass(void) 
{
    std::cout << "Destroy: TestClass " << std::endl;
}

void TestClass::request(void)
{
	std::cout << "request: TestClass" << std::endl;
    std::cout << "private variable: " << a << std::endl; 
}

Arc::MCC *get_instance(Arc::Config *cfg) 
{
    return (Arc::MCC *)(new TestClass(cfg));
}

}; // namespace TestClass

/* MCC plugin descriptor */
mcc_descriptor descriptor = {
    0,                  /* version */
    TestPlugin::get_instance    /* get_instance function */
};
