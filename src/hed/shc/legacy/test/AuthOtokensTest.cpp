#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <../auth.h>

using namespace ArcSHCLegacy;

class AuthOtokensTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(AuthOtokensTest);
  CPPUNIT_TEST(TestAuthTokensGen);
  CPPUNIT_TEST_SUITE_END();

public:
  void TestAuthTokensGen();
};

class SecAttrMock: public Arc::SecAttr {
private:
  std::map< std::string,std::list<std::string> > attrs_;
public:
  SecAttrMock() {
    attrs_["sub"].push_back("e83eec5a-e2e3-43c6-bb67-df8f5ec3e8d0");
    attrs_["iss"].push_back("https://wlcg.cloud.cnaf.infn.it/");
    attrs_["aud"].push_back("aud");
    attrs_["email"].push_back("live.evil.com");
  }

  std::string get(const std::string& id) const {
    if(attrs_.find(id) == attrs_.end()) return "";
    return attrs_.at(id).front();
  }

  std::map< std::string,std::list<std::string> > getAll() const {
    return attrs_;
  }
};

void AuthOtokensTest::TestAuthTokensGen() {
  Arc::Message message;
  SecAttrMock* attr = new SecAttrMock();
  message.Auth()->set("OTOKENS", attr);

  AuthUser user(message);
  AuthResult result;
  result = user.evaluate(R"+++(authtokensgen !(email="live.evil.com"))+++");
  CPPUNIT_ASSERT(result == AAA_NO_MATCH);
  result = user.evaluate(R"+++(authtokensgen email="live.evil.com")+++");
  CPPUNIT_ASSERT(result == AAA_POSITIVE_MATCH);
  result = user.evaluate(R"+++(authtokensgen email~".*\.evil\.com")+++");
  CPPUNIT_ASSERT(result == AAA_POSITIVE_MATCH);
  result = user.evaluate(R"+++(authtokensgen !(email~".*\.evil\.com"))+++");
  CPPUNIT_ASSERT(result == AAA_NO_MATCH);
  result = user.evaluate(R"+++(authtokensgen (email~".*\.evil\.com"))+++");
  CPPUNIT_ASSERT(result == AAA_POSITIVE_MATCH);
  result = user.evaluate(R"+++(authtokensgen (sub=e83eec5a-e2e3-43c6-bb67-df8f5ec3e8d0) & (iss="https://wlcg.cloud.cnaf.infn.it/") & !(email~".*\.evil\.com"))+++");
  CPPUNIT_ASSERT(result == AAA_NO_MATCH);
  result = user.evaluate(R"+++(authtokensgen (sub=e83eec5a-e2e3-43c6-bb67-df8f5ec3e8d0) & (iss="https://wlcg.cloud.cnaf.infn.it/") & (email~".*\.evil\.com"))+++");
  CPPUNIT_ASSERT(result == AAA_POSITIVE_MATCH);
  result = user.evaluate(R"+++(authtokensgen (sub=e83eec5a-e2e3-43c6-bb67-df8f5ec3e8d0) & (iss="https://wlcg.cloud.cnaf.infn.it/") & !(email~".*\.evil\.com))+++");
  CPPUNIT_ASSERT(result == AAA_FAILURE);
  result = user.evaluate(R"+++(authtokensgen (sub=e83eec5a-e2e3-43c6-bb67-df8f5ec3e8d0) & (iss="https://wlcg.cloud.cnaf.infn.it/") & !(email~".*\.evil\.com")+++");
  CPPUNIT_ASSERT(result == AAA_FAILURE);
}

CPPUNIT_TEST_SUITE_REGISTRATION(AuthOtokensTest);
