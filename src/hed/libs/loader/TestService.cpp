#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "TestService.h"

namespace Test {

TestService::TestService(Arc::Config *cfg) : Service(cfg)
{
    std::cout << "Init: TestService" << std::endl;
    a = 2;
}

TestService::~TestService(void) 
{
    std::cout << "Destroy: TestService " << std::endl;
}

void TestService::process(void)
{
	std::cout << "process: TestService" << std::endl;
    std::cout << "private variable: " << a << std::endl; 
}

Arc::Service *get_service_instance(Arc::Config *cfg) 
{
    return (Arc::Service *)(new TestService(cfg));
}

}; // namespace Test

/* Service plugin descriptor */
service_descriptor descriptor = {
    0,                            /* version */
    Test::get_service_instance    /* get_instance function */
};
