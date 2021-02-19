#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/loader/Loader.h>

class PluginTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(PluginTest);
  CPPUNIT_TEST(TestPlugin);
  CPPUNIT_TEST_SUITE_END();

public:
  void TestPlugin();
};

class PluginTestLoader: Arc::Loader {
public:
  PluginTestLoader(Arc::XMLNode cfg):Arc::Loader(cfg) {
  };
  Arc::PluginsFactory* factory(void) {
    return factory_;
  };
};

void PluginTest::TestPlugin() {
  {
    std::ofstream apd(".libs/libtestplugin.apd",std::ios::trunc);
    apd<<"name=\"testplugin\""<<std::endl;
    apd<<"kind=\"TEST\""<<std::endl;
    apd<<"version=\"0\""<<std::endl;
    apd<<"priority=\"128\""<<std::endl;
  }
  std::string config_xml("\
<?xml version=\"1.0\"?>\n\
<ArcConfig xmlns=\"http://www.nordugrid.org/schemas/ArcConfig/2007\">\n\
  <ModuleManager>\n\
    <Path>.libs/</Path>\n\
  </ModuleManager>\n\
  <Plugins>\n\
    <Name>testplugin</Name>\n\
  </Plugins>\n\
</ArcConfig>");
  Arc::XMLNode cfg(config_xml);
  PluginTestLoader loader(cfg);
  CPPUNIT_ASSERT(loader.factory());
  std::string plugin_name = "testplugin";
  std::string plugin_kind = "TEST";
  Arc::PluginArgument* plugin_arg = NULL;
  CPPUNIT_ASSERT(loader.factory()->get_instance(plugin_kind,plugin_name,plugin_arg,false));
}

CPPUNIT_TEST_SUITE_REGISTRATION(PluginTest);
