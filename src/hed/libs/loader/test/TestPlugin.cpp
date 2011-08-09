#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "../Plugin.h"

namespace Test {

class TestPlugin: Arc::Plugin {
    public:
        TestPlugin();
        ~TestPlugin();
};

TestPlugin::TestPlugin() {
}

TestPlugin::~TestPlugin(void) {
}

Arc::Plugin *get_instance(Arc::PluginArgument*) 
{
    return (Arc::Plugin *)(new TestPlugin());
}

} // namespace Test

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    {
        "testplugin",         /* name */
        "TEST",               /* kind */
        0,                    /* version */
        Test::get_instance    /* get_instance function */
    },
    { NULL, NULL, 0, NULL }
};

