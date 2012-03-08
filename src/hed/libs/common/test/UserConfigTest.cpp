#include <cppunit/extensions/HelperMacros.h>

#include <string>
#include <fstream>

#include <arc/UserConfig.h>

class UserConfigTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(UserConfigTest);
  CPPUNIT_TEST(ParseRegistryTest);
  CPPUNIT_TEST(ParseComputingTest);
  CPPUNIT_TEST(UnspecifiedInterfaceTest);
  CPPUNIT_TEST(GroupTest);
  CPPUNIT_TEST(PreferredInterfacesTest);
  // CPPUNIT_TEST(ServiceFromLegacyStringTest);
  CPPUNIT_TEST(LegacyDefaultServicesTest);
  CPPUNIT_TEST(LegacyAliasTest);
  CPPUNIT_TEST(AliasTest);
  CPPUNIT_TEST_SUITE_END();

public:
  UserConfigTest() : uc(Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials)), conffile("test-client.conf") {}

  void ParseRegistryTest();
  void ParseComputingTest();
  void UnspecifiedInterfaceTest();
  void GroupTest();
  void PreferredInterfacesTest();
  // void ServiceFromLegacyStringTest();
  void LegacyDefaultServicesTest();
  void LegacyAliasTest();
  void AliasTest();

  void setUp() {}
  void tearDown() {}

private:
  Arc::UserConfig uc;
  const std::string conffile;
};

void UserConfigTest::ParseRegistryTest()
{
  std::ofstream f(conffile.c_str(), std::ifstream::trunc);
  f << "[registry/emir1]\nurl=http://emir1.emi-eu.eu\nregistryinterface=org.nordugrid.emir\ndefault=yes\n";  
  f.close();
  uc.LoadConfigurationFile(conffile);
  std::list<Arc::ConfigEndpoint> services;
  services = uc.GetDefaultServices();
  CPPUNIT_ASSERT_EQUAL(1, (int)services.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"http://emir1.emi-eu.eu", services.front().URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.emir", services.front().InterfaceName);  
  
  services = uc.GetDefaultServices(Arc::ConfigEndpoint::REGISTRY);
  CPPUNIT_ASSERT_EQUAL(1, (int)services.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"http://emir1.emi-eu.eu", services.front().URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.emir", services.front().InterfaceName);  

  services = uc.GetDefaultServices(Arc::ConfigEndpoint::COMPUTINGINFO);
  CPPUNIT_ASSERT_EQUAL(0, (int)services.size());
  
  Arc::ConfigEndpoint service = uc.GetService("emir1");
  CPPUNIT_ASSERT_EQUAL((std::string)"http://emir1.emi-eu.eu", service.URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.emir", service.InterfaceName);  
  
  remove(conffile.c_str());
}

void UserConfigTest::ParseComputingTest()
{
  std::ofstream f(conffile.c_str(), std::ifstream::trunc);
  f << "[computing/puff]\nurl=ldap://puff.hep.lu.se\ninfointerface=org.nordugrid.ldapglue2\njobinterface=org.nordugrid.gridftpjob\ndefault=yes\n";
  
  f.close();
  uc.LoadConfigurationFile(conffile);
  std::list<Arc::ConfigEndpoint> services;
  services = uc.GetDefaultServices();
  CPPUNIT_ASSERT_EQUAL(1, (int)services.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"ldap://puff.hep.lu.se", services.front().URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.ldapglue2", services.front().InterfaceName);  
  
  services = uc.GetDefaultServices(Arc::ConfigEndpoint::COMPUTINGINFO);
  CPPUNIT_ASSERT_EQUAL(1, (int)services.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"ldap://puff.hep.lu.se", services.front().URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.ldapglue2", services.front().InterfaceName);  

  services = uc.GetDefaultServices(Arc::ConfigEndpoint::REGISTRY);
  CPPUNIT_ASSERT_EQUAL(0, (int)services.size());
  
  Arc::ConfigEndpoint service = uc.GetService("puff");
  CPPUNIT_ASSERT_EQUAL((std::string)"ldap://puff.hep.lu.se", service.URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.ldapglue2", service.InterfaceName);  
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.gridftpjob", service.PreferredJobInterfaceName);  
  
  remove(conffile.c_str());
}

void UserConfigTest::UnspecifiedInterfaceTest()
{
  std::ofstream f(conffile.c_str(), std::ifstream::trunc);
  f << "[computing/puff]\nurl=ldap://puff.hep.lu.se\n"
    << "[registry/emir1]\nurl=http://emir1.nordugrid.org\n";
  
  f.close();
  uc.LoadConfigurationFile(conffile);
  
  Arc::ConfigEndpoint service;
  service = uc.GetService("puff");
  CPPUNIT_ASSERT_EQUAL((std::string)"ldap://puff.hep.lu.se", service.URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"", service.InterfaceName);  
  CPPUNIT_ASSERT_EQUAL((std::string)"", service.PreferredJobInterfaceName);  

  service = uc.GetService("emir1");
  CPPUNIT_ASSERT_EQUAL((std::string)"http://emir1.nordugrid.org", service.URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"", service.InterfaceName);
  
  remove(conffile.c_str());
}


void UserConfigTest::GroupTest()
{
  std::ofstream f(conffile.c_str(), std::ifstream::trunc);
  f << "[computing/puff]\nurl=ldap://puff.hep.lu.se\ngroup=hep\n"
    << "[computing/paff]\nurl=ldap://paff.hep.lu.se\ngroup=hep\n"
    << "[computing/interop]\nurl=https://interop.grid.niif.hu\ngroup=niif\n";
  f.close();
  uc.LoadConfigurationFile(conffile);
  std::list<Arc::ConfigEndpoint> services;
  services = uc.GetServicesInGroup("hep");
  CPPUNIT_ASSERT_EQUAL(2, (int)services.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"ldap://puff.hep.lu.se", services.front().URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"ldap://paff.hep.lu.se", services.back().URLString);

  services = uc.GetServicesInGroup("niif");
  CPPUNIT_ASSERT_EQUAL(1, (int)services.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"https://interop.grid.niif.hu", services.front().URLString);
    
  remove(conffile.c_str());
}

void UserConfigTest::PreferredInterfacesTest()
{
  std::ofstream f(conffile.c_str(), std::ifstream::trunc);
  f << "preferredinfointerface=org.nordugrid.ldapglue2\npreferredjobinterface=org.nordugrid.gridftpjob\n"
    << "[computing/puff]\nurl=ldap://puff.hep.lu.se\n";
  f.close();
  uc.LoadConfigurationFile(conffile);
  
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.gridftpjob", uc.PreferredJobInterface());
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.ldapglue2", uc.PreferredInfoInterface());
  Arc::ConfigEndpoint service;
  service = uc.GetService("puff");
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.gridftpjob", service.PreferredJobInterfaceName);  
  
  remove(conffile.c_str());
}

// void UserConfigTest::ServiceFromLegacyStringTest()
// {
//   Arc::ConfigEndpoint service;
//   service = Arc::UserConfig::ServiceFromLegacyString("computing:ARC0:http://a.org");
//   CPPUNIT_ASSERT_EQUAL(Arc::ConfigEndpoint::COMPUTINGINFO, service.type);
//   CPPUNIT_ASSERT_EQUAL((std::string)"http://a.org", service.URLString);
//   CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.ldapng", service.InterfaceName);
// 
//   service = Arc::UserConfig::ServiceFromLegacyString("computing:ARC1:http://a.org");
//   CPPUNIT_ASSERT_EQUAL(Arc::ConfigEndpoint::COMPUTINGINFO, service.type);
//   CPPUNIT_ASSERT_EQUAL((std::string)"http://a.org", service.URLString);
//   CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.wsrfglue2", service.InterfaceName);
// 
//   service = Arc::UserConfig::ServiceFromLegacyString("index:ARC0:http://a.org");
//   CPPUNIT_ASSERT_EQUAL(Arc::ConfigEndpoint::REGISTRY, service.type);
//   CPPUNIT_ASSERT_EQUAL((std::string)"http://a.org", service.URLString);
//   CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.ldapegiis", service.InterfaceName);
// 
//   service = Arc::UserConfig::ServiceFromLegacyString("index:EMIES:http://a.org");
//   CPPUNIT_ASSERT_EQUAL(Arc::ConfigEndpoint::REGISTRY, service.type);
//   CPPUNIT_ASSERT_EQUAL((std::string)"http://a.org", service.URLString);
//   CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.emir", service.InterfaceName);
// }

void UserConfigTest::LegacyDefaultServicesTest()
{
  std::ofstream f(conffile.c_str(), std::ifstream::trunc);
  f << "defaultservices="
    << "index:ARC0:ldap://index1.nordugrid.org:2135/Mds-Vo-name=NorduGrid,o=grid "
    << "index:ARC0:ldap://index2.nordugrid.org:2135/Mds-Vo-name=NorduGrid,o=grid "
    << "computing:ARC0:ldap://a.org "
    << "computing:ARC1:http://b.org\n"; 
  f.close();
  
  uc.LoadConfigurationFile(conffile);
  
  std::list<Arc::ConfigEndpoint> services;
  
  services = uc.GetDefaultServices();
  CPPUNIT_ASSERT_EQUAL(4, (int)services.size());
  
  services = uc.GetDefaultServices(Arc::ConfigEndpoint::REGISTRY);  
  CPPUNIT_ASSERT_EQUAL(2, (int)services.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"ldap://index1.nordugrid.org:2135/Mds-Vo-name=NorduGrid,o=grid", services.front().URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.ldapegiis", services.front().InterfaceName);  
  CPPUNIT_ASSERT_EQUAL((std::string)"ldap://index2.nordugrid.org:2135/Mds-Vo-name=NorduGrid,o=grid", services.back().URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.ldapegiis", services.back().InterfaceName);  

  services = uc.GetDefaultServices(Arc::ConfigEndpoint::COMPUTINGINFO);  
  CPPUNIT_ASSERT_EQUAL(2, (int)services.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"ldap://a.org", services.front().URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.ldapng", services.front().InterfaceName);  
  CPPUNIT_ASSERT_EQUAL((std::string)"http://b.org", services.back().URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.wsrfglue2", services.back().InterfaceName);  
  
  remove(conffile.c_str()); 
}

void UserConfigTest::LegacyAliasTest()
{
  std::ofstream f(conffile.c_str(), std::ifstream::trunc);
  f << "[ alias ]" << std::endl;
  f << "a = computing:ARC0:http://a.org" << std::endl;
  f << "b = computing:ARC0:http://b.org" << std::endl;
  f << "c = a b computing:ARC0:http://c.org" << std::endl;
  f << "invalid = compute:ARC0:http://invalid.org" << std::endl;
  f << "i = index:ARC0:http://i.org" << std::endl;
  f << "j = index:ARC0:http://j.org" << std::endl;
  f << "k = i j index:ARC0:http://k.org" << std::endl;
  f << "mixed = a b i j" << std::endl;
  f << "loop = a b link" << std::endl;
  f << "link = loop" << std::endl;
  f.close();

  uc.LoadConfigurationFile(conffile);

  std::list<Arc::ConfigEndpoint> services;
  
  // legacy aliases become groups in the new config
  
  services = uc.GetServices("a");
  CPPUNIT_ASSERT_EQUAL(1, (int)services.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"http://a.org", services.front().URLString);

  services = uc.GetServices("b");
  CPPUNIT_ASSERT_EQUAL(1, (int)services.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"http://b.org", services.front().URLString);

  services = uc.GetServices("c");
  CPPUNIT_ASSERT_EQUAL(3, (int)services.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"http://a.org", services.front().URLString);
  services.pop_front();
  CPPUNIT_ASSERT_EQUAL((std::string)"http://b.org", services.front().URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"http://c.org", services.back().URLString);

  services = uc.GetServices("i");
  CPPUNIT_ASSERT_EQUAL(1, (int)services.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"http://i.org", services.front().URLString);

  services = uc.GetServices("j");
  CPPUNIT_ASSERT_EQUAL(1, (int)services.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"http://j.org", services.front().URLString);

  services = uc.GetServices("k");
  CPPUNIT_ASSERT_EQUAL(3, (int)services.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"http://i.org", services.front().URLString);
  services.pop_front();
  CPPUNIT_ASSERT_EQUAL((std::string)"http://j.org", services.front().URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"http://k.org", services.back().URLString);

  services = uc.GetServices("invalid");
  CPPUNIT_ASSERT_EQUAL(0, (int)services.size());

  services = uc.GetServices("mixed");
  CPPUNIT_ASSERT_EQUAL(4, (int)services.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"http://a.org", services.front().URLString);
  services.pop_front();
  CPPUNIT_ASSERT_EQUAL((std::string)"http://b.org", services.front().URLString);
  services.pop_front();
  CPPUNIT_ASSERT_EQUAL((std::string)"http://i.org", services.front().URLString);
  services.pop_front();
  CPPUNIT_ASSERT_EQUAL((std::string)"http://j.org", services.front().URLString);

  services = uc.GetServices("loop");
  CPPUNIT_ASSERT_EQUAL(2, (int)services.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"http://a.org", services.front().URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"http://b.org", services.back().URLString);

  services = uc.GetServices("undefined");
  CPPUNIT_ASSERT_EQUAL(0, (int)services.size());
  
  remove(conffile.c_str());
}

void UserConfigTest::AliasTest()
{
  std::ofstream f(conffile.c_str(), std::ifstream::trunc);
  f << "[ alias ]" << std::endl;
  f << "a = computing:TEST:http://a.org" << std::endl;
  f << "b = computing:TEST:http://b.org" << std::endl;
  f << "c = a b computing:TEST:http://c.org" << std::endl;
  f << "invalid = compute:TEST:http://invalid.org" << std::endl;
  f << "i = index:TEST:http://i.org" << std::endl;
  f << "j = index:TEST:http://j.org" << std::endl;
  f << "k = i j index:TEST:http://k.org" << std::endl;
  f << "mixed = a b i j" << std::endl;
  f << "loop = a b link" << std::endl;
  f << "link = loop" << std::endl;
  f.close();

  uc.LoadConfigurationFile(conffile);

  std::list<std::string> s;

  // Checking normal computing alias.
  s.push_back("a");
  s.push_back("b");
  CPPUNIT_ASSERT(uc.ResolveAliases(s, Arc::COMPUTING));
  CPPUNIT_ASSERT_EQUAL(2, (int)s.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://a.org", s.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://b.org", s.back());
  s.clear();

  // Checking computing alias referring to computing alias.
  s.push_back("c");
  CPPUNIT_ASSERT(uc.ResolveAliases(s, Arc::COMPUTING));
  CPPUNIT_ASSERT_EQUAL(3, (int)s.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://a.org", s.front());
  s.pop_front();
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://b.org", s.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://c.org", s.back());
  s.clear();

  // Checking normal index alias.
  s.push_back("i");
  s.push_back("j");
  CPPUNIT_ASSERT(uc.ResolveAliases(s, Arc::INDEX));
  CPPUNIT_ASSERT_EQUAL(2, (int)s.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://i.org", s.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://j.org", s.back());
  s.clear();

  // Checking index alias referring to computing alias.
  s.push_back("k");
  CPPUNIT_ASSERT(uc.ResolveAliases(s, Arc::INDEX));
  CPPUNIT_ASSERT_EQUAL(3, (int)s.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://i.org", s.front());
  s.pop_front();
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://j.org", s.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://k.org", s.back());
  s.clear();

  // Checking alias referring to an alias.
  s.push_back("a");
  s.push_back("invalid");
  CPPUNIT_ASSERT(uc.ResolveAliases(s, Arc::COMPUTING));
  CPPUNIT_ASSERT_EQUAL(1, (int)s.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://a.org", s.front());
  s.clear();

  // Checking mixed alias.
  s.push_back("mixed");
  CPPUNIT_ASSERT(uc.ResolveAliases(s, Arc::COMPUTING));
  CPPUNIT_ASSERT_EQUAL(2, (int)s.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://a.org", s.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://b.org", s.back());
  s.clear();
  s.push_back("mixed");
  CPPUNIT_ASSERT(uc.ResolveAliases(s, Arc::INDEX));
  CPPUNIT_ASSERT_EQUAL(2, (int)s.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://i.org", s.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://j.org", s.back());
  s.clear();
  s.push_back("a");
  CPPUNIT_ASSERT(uc.ResolveAliases(s, Arc::INDEX));
  CPPUNIT_ASSERT_EQUAL(0, (int)s.size());
  s.clear();
  s.push_back("i");
  CPPUNIT_ASSERT(uc.ResolveAliases(s, Arc::COMPUTING));
  CPPUNIT_ASSERT_EQUAL(0, (int)s.size());
  s.clear();

  // Check if loops are detected.
  s.push_back("loop");
  CPPUNIT_ASSERT(!uc.ResolveAliases(s, Arc::COMPUTING));
  s.clear();

  // Specifying an undefined alias should return false.
  s.push_back("undefined");
  CPPUNIT_ASSERT(!uc.ResolveAliases(s, Arc::COMPUTING));
  s.clear();

  remove(conffile.c_str());
}

CPPUNIT_TEST_SUITE_REGISTRATION(UserConfigTest);
