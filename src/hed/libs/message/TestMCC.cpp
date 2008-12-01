#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "TestMCC.h"
#include <arc/message/MCCLoader.h>

namespace Test {

TestMCC::TestMCC(Arc::Config *cfg) : Arc::MCC(cfg)
{
    std::string s;
    std::cout << "Init: TestMCC" << std::endl;
    cfg->GetXML(s);
    std::cout << "Configuration:\n" << s << std::endl;
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
    Arc::Message msg;
    return msg;
}

Arc::Plugin *get_mcc_instance(Arc::PluginArgument* arg) 
{
    Arc::MCCPluginArgument* mccarg =
            arg?dynamic_cast<Arc::MCCPluginArgument*>(arg):NULL;   
    if(!mccarg) return NULL;
    return new TestMCC((Arc::Config*)(*mccarg));
}

} // namespace Test

/* MCC plugin descriptor */
Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    {
        "testmcc",                /* name */
        "HED:MCC",                /* kind */
        0,                        /* version */
        &(Test::get_mcc_instance) /* get_instance function */
    },
    { NULL, NULL, 0, NULL }
};
