#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "TestMCC.h"
#include "MCCLoader.h"

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

Arc::Message TestMCC::process(Arc::Message)
{
	std::cout << "process: TestMCC" << std::endl;
    std::cout << "private variable: " << a << std::endl; 
    std::cout << "Defined \"next\"s: " << std::endl;
    for(std::map<std::string,MCCInterface*>::iterator i = next_.begin(); i != next_.end(); ++i) {
        std::cout << "Name: " << i->first << " - " << ((i->second)?"defined":"undefined") << std::endl;
    }
}

Arc::MCC *get_mcc_instance(Arc::Config *cfg,Arc::ChainContext* ctx) 
{
    return (Arc::MCC *)(new TestMCC(cfg));
}

}; // namespace Test

/* MCC plugin descriptor */
mcc_descriptors ARC_MCC_LOADER = {
    {
        "testmcc",                /* name */
        0,                        /* version */
        Test::get_mcc_instance    /* get_instance function */
    },
    { NULL, 0, NULL }
};
