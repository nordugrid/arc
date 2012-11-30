#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "../Plugin.h"

namespace Test {

class TestPlugin: Arc::Plugin {
    public:
        TestPlugin(Arc::PluginArgument*);
        ~TestPlugin();
};

TestPlugin::TestPlugin(Arc::PluginArgument* parg): Plugin(parg) {
}

TestPlugin::~TestPlugin(void) {
}

Arc::Plugin *get_instance(Arc::PluginArgument* arg) 
{
    return (Arc::Plugin *)(new TestPlugin(arg));
}

} // namespace Test

Arc::PluginDescriptor ARC_PLUGINS_TABLE_NAME[] = {
    {
        "testplugin",         /* name */
        "TEST",               /* kind */
        NULL,                 /* description */
        0,                    /* version */
        Test::get_instance    /* get_instance function */
    },
    { NULL, NULL, NULL, 0, NULL }
};

