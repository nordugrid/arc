#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <string>
#include <fstream>
#include <glibmm.h>

#include <arc/UserConfig.h>

class UserConfigTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(UserConfigTest);
  CPPUNIT_TEST(ParseRegistryTest);
  CPPUNIT_TEST(ParseComputingTest);
  CPPUNIT_TEST(UnspecifiedInterfaceTest);
  CPPUNIT_TEST(GroupTest);
  CPPUNIT_TEST(RequestedInterfacesTest);
  // CPPUNIT_TEST(ServiceFromLegacyStringTest);
  CPPUNIT_TEST(LegacyDefaultServicesTest);
  CPPUNIT_TEST(LegacyAliasTest);
  CPPUNIT_TEST(RejectionTest);
  CPPUNIT_TEST(SaveToFileTest);
  CPPUNIT_TEST_SUITE_END();

public:
  UserConfigTest() : uc(Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials)), conffile("test-client.conf") {}

  void ParseRegistryTest();
  void ParseComputingTest();
  void UnspecifiedInterfaceTest();
  void GroupTest();
  void RequestedInterfacesTest();
  // void ServiceFromLegacyStringTest();
  void LegacyDefaultServicesTest();
  void LegacyAliasTest();
  void RejectionTest();
  void SaveToFileTest();

  void setUp() {}
  void tearDown() {}

private:
  Arc::UserConfig uc;
  const std::string conffile;
};

void UserConfigTest::ParseRegistryTest()
{
  std::ofstream f(conffile.c_str(), std::ifstream::trunc);
  f << "[registry/archery1]\nurl=dns://nordugrid.org\nregistryinterface=org.nordugrid.archery\ndefault=yes\n";  
  f.close();
  uc.LoadConfigurationFile(conffile);
  std::list<Arc::ConfigEndpoint> services;
  services = uc.GetDefaultServices();
  CPPUNIT_ASSERT_EQUAL(1, (int)services.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"dns://nordugrid.org", services.front().URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.archery", services.front().InterfaceName);  
  
  services = uc.GetDefaultServices(Arc::ConfigEndpoint::REGISTRY);
  CPPUNIT_ASSERT_EQUAL(1, (int)services.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"dns://nordugrid.org", services.front().URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.archery", services.front().InterfaceName);  

  services = uc.GetDefaultServices(Arc::ConfigEndpoint::COMPUTINGINFO);
  CPPUNIT_ASSERT_EQUAL(0, (int)services.size());
  
  Arc::ConfigEndpoint service = uc.GetService("archery1");
  CPPUNIT_ASSERT_EQUAL((std::string)"dns://nordugrid.org", service.URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.archery", service.InterfaceName);  
  
  remove(conffile.c_str());
}

void UserConfigTest::ParseComputingTest()
{
  std::ofstream f(conffile.c_str(), std::ifstream::trunc);
  f << "[computing/puff]\nurl=ldap://puff.hep.lu.se\ninfointerface=org.nordugrid.ldapglue2\nsubmissioninterface=org.nordugrid.gridftpjob\ndefault=yes\n";
  
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
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.gridftpjob", service.RequestedSubmissionInterfaceName);  
  
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
  CPPUNIT_ASSERT_EQUAL((std::string)"", service.RequestedSubmissionInterfaceName);  

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

void UserConfigTest::RequestedInterfacesTest()
{
  std::ofstream f(conffile.c_str(), std::ifstream::trunc);
  f << "infointerface=org.nordugrid.ldapglue2\nsubmissioninterface=org.nordugrid.gridftpjob\n"
    << "[computing/puff]\nurl=ldap://puff.hep.lu.se\n";
  f.close();
  uc.LoadConfigurationFile(conffile);
  
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.gridftpjob", uc.SubmissionInterface());
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.ldapglue2", uc.InfoInterface());
  Arc::ConfigEndpoint service;
  service = uc.GetService("puff");
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.gridftpjob", service.RequestedSubmissionInterfaceName);  
  
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
    << "computing:ARC0:ldap://a.org\n";
  f.close();
  
  uc.LoadConfigurationFile(conffile);
  
  std::list<Arc::ConfigEndpoint> services;
  
  services = uc.GetDefaultServices();
  CPPUNIT_ASSERT_EQUAL(3, (int)services.size());
  
  services = uc.GetDefaultServices(Arc::ConfigEndpoint::REGISTRY);  
  CPPUNIT_ASSERT_EQUAL(2, (int)services.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"ldap://index1.nordugrid.org:2135/Mds-Vo-name=NorduGrid,o=grid", services.front().URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.ldapegiis", services.front().InterfaceName);  
  CPPUNIT_ASSERT_EQUAL((std::string)"ldap://index2.nordugrid.org:2135/Mds-Vo-name=NorduGrid,o=grid", services.back().URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.ldapegiis", services.back().InterfaceName);  

  services = uc.GetDefaultServices(Arc::ConfigEndpoint::COMPUTINGINFO);  
  CPPUNIT_ASSERT_EQUAL(1, (int)services.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"ldap://a.org", services.front().URLString);
  CPPUNIT_ASSERT_EQUAL((std::string)"org.nordugrid.ldapng", services.front().InterfaceName);  
  
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

void UserConfigTest::RejectionTest()
{
  std::ofstream f(conffile.c_str(), std::ifstream::trunc);
  f << "rejectdiscovery=ldap://puff.hep.lu.se\nrejectdiscovery=test.nordugrid.org\n";
  f << "rejectmanagement=ldap://puff.hep.lu.se\nrejectmanagement=test.nordugrid.org\n";
  f.close();
  uc.LoadConfigurationFile(conffile);
  
  std::list<std::string> urls = uc.RejectDiscoveryURLs();
  CPPUNIT_ASSERT_EQUAL(2, (int)urls.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"ldap://puff.hep.lu.se", urls.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"test.nordugrid.org", urls.back());

  std::list<std::string> urls2 = uc.RejectManagementURLs();
  CPPUNIT_ASSERT_EQUAL(2, (int)urls2.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"ldap://puff.hep.lu.se", urls2.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"test.nordugrid.org", urls2.back());

  remove(conffile.c_str());
}

void UserConfigTest::SaveToFileTest()
{
  std::ofstream f(conffile.c_str(), std::ifstream::trunc);
  std::string input = 
    "[common]\n"
    "proxypath = /tmp/my-proxy\n"
    "certificatepath = /home/username/cert.pem\n"
    "keypath = /home/username/key.pem\n"
    "cacertificatesdirectory = /home/user/cacertificates\n"
    "causedefalt = 0\n"
    "rejectdiscovery = bad.service.org\n"
    "rejectdiscovery = bad2.service.org\n"
    "rejectmanagement = bad3.service.org\n"
    "rejectmanagement = bad4.service.org\n"
    "timeout = 50\n"
    "brokername = FastestQueue\n"
    "brokerarguments = arg\n"
    "vomsespath = /home/user/vomses\n"
    "submissioninterface = org.nordugrid.gridftpjob\n"
    "infointerface = org.nordugrid.ldapglue2\n"
    "[registry/index1]\n"
    "url = ldap://index1.nordugrid.org:2135/Mds-Vo-name=NorduGrid,o=grid\n"
    "registryinterface = org.nordugrid.ldapegiis\n"
    "default = yes\n"
    "group = index\n"
    "[registry/index2]\n"
    "url = ldap://index2.nordugrid.org:2135/Mds-Vo-name=NorduGrid,o=grid\n"
    "registryinterface = org.nordugrid.ldapegiis\n"
    "group = index\n"
    "group = special\n"
    "[computing/interop]\n"
    "url = https://interop.grid.niif.hu:2010/arex-x509\n"
    "infointerface = org.nordugrid.ldapglue2\n"
    "submissioninterface = org.nordugrid.gridftpjob\n"
    "default = yes\n";
  f <<   input;
  f.close();
  uc.LoadConfigurationFile(conffile);
  remove(conffile.c_str());
  
  uc.SaveToFile(conffile);
  std::ifstream ff(conffile.c_str());
  std::string output;
  std::getline(ff,output,'\0');
  ff.close();
  remove(conffile.c_str());
  CPPUNIT_ASSERT_EQUAL((std::string)input, (std::string)output);
}


CPPUNIT_TEST_SUITE_REGISTRATION(UserConfigTest);
