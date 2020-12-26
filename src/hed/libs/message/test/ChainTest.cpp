#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <arc/message/MCCLoader.h>

class ChainTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(ChainTest);
  CPPUNIT_TEST(TestPlugin);
  CPPUNIT_TEST_SUITE_END();

public:
  void TestPlugin();
};

class ChainTestLoader: Arc::Loader {
public:
  ChainTestLoader(Arc::XMLNode cfg):Arc::Loader(cfg) {
  };
  Arc::PluginsFactory* factory(void) {
    return factory_;
  };
};

void ChainTest::TestPlugin() {
  {
    std::ofstream apd(".libs/libtestmcc.apd",std::ios::trunc);
    apd<<"name=\"testmcc\""<<std::endl;
    apd<<"kind=\"HED:MCC\""<<std::endl;
    apd<<"version=\"0\""<<std::endl;
    apd<<"priority=\"128\""<<std::endl;
  }

  {
    std::ofstream apd(".libs/libtestservice.apd",std::ios::trunc);
    apd<<"name=\"testservice\""<<std::endl;
    apd<<"kind=\"HED:SERVICE\""<<std::endl;
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
    <Name>testmcc</Name>\n\
    <Name>testservice</Name>\n\
  </Plugins>\n\
  <Chain>\n\
    <Component name=\"testmcc\" id=\"mcc1\">\n\
      <next id=\"mcc2\"/>\n\
    </Component>\n\
    <Component name=\"testmcc\" id=\"mcc2\">\n\
      <next id=\"mcc3\"/>\n\
    </Component>\n\
    <Plexer id=\"mcc3\">\n\
      <next id=\"mcc4\">/service1</next>\n\
      <next id=\"mcc5\">/service2</next>\n\
    </Plexer>\n\
    <Service name=\"testservice\" id=\"mcc4\">\n\
    </Service>\n\
    <Service name=\"testservice\" id=\"mcc5\">\n\
    </Service>\n\
  </Chain>\n\
</ArcConfig>");
  Arc::Config cfg(config_xml);
  Arc::MCCLoader loader(cfg);
  CPPUNIT_ASSERT(loader);
}

CPPUNIT_TEST_SUITE_REGISTRATION(ChainTest);
