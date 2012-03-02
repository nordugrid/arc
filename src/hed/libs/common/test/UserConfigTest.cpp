#include <cppunit/extensions/HelperMacros.h>

#include <string>
#include <fstream>

#include <arc/UserConfig.h>

class UserConfigTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(UserConfigTest);
  CPPUNIT_TEST(AliasTest);
  CPPUNIT_TEST(ParseRegistryTest);
  CPPUNIT_TEST(ParseComputingTest);
  CPPUNIT_TEST(UnspecifiedInterfaceTest);
  CPPUNIT_TEST(GroupTest);
  CPPUNIT_TEST(PreferredInterfacesTest);
  CPPUNIT_TEST_SUITE_END();

public:
  UserConfigTest() : uc(Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials)), conffile("test-client.conf") {}

  void AliasTest();
  void ParseRegistryTest();
  void ParseComputingTest();
  void UnspecifiedInterfaceTest();
  void GroupTest();
  void PreferredInterfacesTest();

  void setUp() {}
  void tearDown() {}

private:
  Arc::UserConfig uc;
  const std::string conffile;
};

void UserConfigTest::ParseRegistryTest()
{
  std::ofstream f(conffile.c_str(), std::ifstream::trunc);
  f << "[registry/emir1]\nurl=http://emir1.emi-eu.eu\nregistryinterface=EMIR\ndefault=yes\n";  
  f.close();
  uc.LoadConfigurationFile(conffile);
  std::list<Arc::ConfigEndpoint> services;
  services = uc.GetDefaultServices();
  CPPUNIT_ASSERT_EQUAL((int)services.size(), 1);
  CPPUNIT_ASSERT_EQUAL(services.front().URLString, (std::string)"http://emir1.emi-eu.eu");
  CPPUNIT_ASSERT_EQUAL(services.front().InterfaceName, (std::string)"org.nordugrid.emir");  
  
  services = uc.GetDefaultServices(Arc::ConfigEndpoint::REGISTRY);
  CPPUNIT_ASSERT_EQUAL((int)services.size(), 1);
  CPPUNIT_ASSERT_EQUAL(services.front().URLString, (std::string)"http://emir1.emi-eu.eu");
  CPPUNIT_ASSERT_EQUAL(services.front().InterfaceName, (std::string)"org.nordugrid.emir");  

  services = uc.GetDefaultServices(Arc::ConfigEndpoint::COMPUTINGINFO);
  CPPUNIT_ASSERT_EQUAL((int)services.size(), 0);
  
  Arc::ConfigEndpoint service = uc.ResolveService("emir1");
  CPPUNIT_ASSERT_EQUAL(service.URLString, (std::string)"http://emir1.emi-eu.eu");
  CPPUNIT_ASSERT_EQUAL(service.InterfaceName, (std::string)"org.nordugrid.emir");  
  
  remove(conffile.c_str());
}

void UserConfigTest::ParseComputingTest()
{
  std::ofstream f(conffile.c_str(), std::ifstream::trunc);
  f << "[computing/puff]\nurl=ldap://puff.hep.lu.se\ninfointerface=LDAPGLUE2\njobinterface=GRIDFTPJOB\ndefault=yes\n";
  
  f.close();
  uc.LoadConfigurationFile(conffile);
  std::list<Arc::ConfigEndpoint> services;
  services = uc.GetDefaultServices();
  CPPUNIT_ASSERT_EQUAL((int)services.size(), 1);
  CPPUNIT_ASSERT_EQUAL(services.front().URLString, (std::string)"ldap://puff.hep.lu.se");
  CPPUNIT_ASSERT_EQUAL(services.front().InterfaceName, (std::string)"org.nordugrid.ldapglue2");  
  
  services = uc.GetDefaultServices(Arc::ConfigEndpoint::COMPUTINGINFO);
  CPPUNIT_ASSERT_EQUAL((int)services.size(), 1);
  CPPUNIT_ASSERT_EQUAL(services.front().URLString, (std::string)"ldap://puff.hep.lu.se");
  CPPUNIT_ASSERT_EQUAL(services.front().InterfaceName, (std::string)"org.nordugrid.ldapglue2");  

  services = uc.GetDefaultServices(Arc::ConfigEndpoint::REGISTRY);
  CPPUNIT_ASSERT_EQUAL((int)services.size(), 0);
  
  Arc::ConfigEndpoint service = uc.ResolveService("puff");
  CPPUNIT_ASSERT_EQUAL(service.URLString, (std::string)"ldap://puff.hep.lu.se");
  CPPUNIT_ASSERT_EQUAL(service.InterfaceName, (std::string)"org.nordugrid.ldapglue2");  
  CPPUNIT_ASSERT_EQUAL(service.PreferredJobInterfaceName, (std::string)"org.nordugrid.gridftpjob");  
  
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
  service = uc.ResolveService("puff");
  CPPUNIT_ASSERT_EQUAL(service.URLString, (std::string)"ldap://puff.hep.lu.se");
  CPPUNIT_ASSERT_EQUAL(service.InterfaceName, (std::string)"");  
  CPPUNIT_ASSERT_EQUAL(service.PreferredJobInterfaceName, (std::string)"");  

  service = uc.ResolveService("emir1");
  CPPUNIT_ASSERT_EQUAL(service.URLString, (std::string)"http://emir1.nordugrid.org");
  CPPUNIT_ASSERT_EQUAL(service.InterfaceName, (std::string)"");
  
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
  services = uc.ServicesInGroup("hep");
  CPPUNIT_ASSERT_EQUAL((int)services.size(), 2);
  CPPUNIT_ASSERT_EQUAL(services.front().URLString, (std::string)"ldap://puff.hep.lu.se");
  CPPUNIT_ASSERT_EQUAL(services.back().URLString, (std::string)"ldap://paff.hep.lu.se");

  services = uc.ServicesInGroup("niif");
  CPPUNIT_ASSERT_EQUAL((int)services.size(), 1);
  CPPUNIT_ASSERT_EQUAL(services.front().URLString, (std::string)"https://interop.grid.niif.hu");
    
  remove(conffile.c_str());
}

void UserConfigTest::PreferredInterfacesTest()
{
  std::ofstream f(conffile.c_str(), std::ifstream::trunc);
  f << "preferredinfointerface=LDAPGLUE2\npreferredjobinterface=GRIDFTPJOB\n"
    << "[computing/puff]\nurl=ldap://puff.hep.lu.se\n";
  f.close();
  uc.LoadConfigurationFile(conffile);
  
  CPPUNIT_ASSERT_EQUAL(uc.PreferredJobInterface(), (std::string)"org.nordugrid.gridftpjob");
  CPPUNIT_ASSERT_EQUAL(uc.PreferredInfoInterface(), (std::string)"org.nordugrid.ldapglue2");
  Arc::ConfigEndpoint service;
  service = uc.ResolveService("puff");
  CPPUNIT_ASSERT_EQUAL(service.PreferredJobInterfaceName, (std::string)"org.nordugrid.gridftpjob");  
  
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
