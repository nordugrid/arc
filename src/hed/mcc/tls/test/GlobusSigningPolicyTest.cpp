#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <string>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <openssl/x509.h>

#include "../GlobusSigningPolicy.h"

static char const policycontent1[] = {
"access_id_CA  X509   /C=ca/O=subject\n"
"pos_rights    globus CA:sign\n"
"cond_subjects globus '\"/O=subject/CN=user1\" \"/O=subject/CN=user2/emailAddress=*\" \"/C=CH/ST=Gen\\xC3\\xA8ve/O=CERN Organisation Europ\\xC3\\xA9enne pour la Recherche Nucl\\xC3\\xA9aire/CN=www.cern.ch\"'\n"
};

static char const policycontent2[] = {
"access_id_CA  X509   '/C=ca/O=subject/CN=\\'-ed'\n"
"pos_rights    globus CA:sign\n"
"cond_subjects globus '\"/O=subject/CN=user1\" \"/O=subject/CN=user2/emailAddress=*\"'\n"
};

static char const policycontent3[] = {
"access_id_CA  X509   \"/C=ca/O=subject/CN=''-ed\"\n"
"pos_rights    globus CA:sign\n"
"cond_subjects globus '\"/O=subject/CN=user1\" \"/O=subject/CN=user2/emailAddress=*\"'\n"
};

/*

namespace ArcMCCTLS {

class GlobusSigningPolicy {
  public:
    GlobusSigningPolicy(): stream_(NULL) { };
    ~GlobusSigningPolicy() { close(); };
    bool open(const X509_NAME* issuer_subject,const std::string& ca_path);
    void close() { delete stream_; stream_ = NULL; };
    bool match(const X509_NAME* issuer_subject,const X509_NAME* subject);
  private:
    GlobusSigningPolicy(GlobusSigningPolicy const &);
    GlobusSigningPolicy& operator=(GlobusSigningPolicy const &);
    std::istream* stream_;
};
*/




class GlobusSigningPolicyTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(GlobusSigningPolicyTest);
  CPPUNIT_TEST(CASubjectMatchTest);
  CPPUNIT_TEST_SUITE_END();

public:
  GlobusSigningPolicyTest() : policyfilename1(NULL), policyfilename2(NULL), policyfilename3(NULL) {}

  void CASubjectMatchTest();

  void setUp();
  void tearDown();

private:
  std::string capath;
  std::string policyfilepath1;
  X509_NAME* policyfilename1;
  std::string policyfilepath2;
  X509_NAME* policyfilename2;
  std::string policyfilepath3;
  X509_NAME* policyfilename3;
};


static void make_policy_file(char const * capath, X509_NAME* policyfilename, char const * policycontent, std::string& policyfilepath) {
  char hash_str[32];
  unsigned long hash = X509_NAME_hash(policyfilename);
  snprintf(hash_str,sizeof(hash_str)-1,"%08lx",hash);
  hash_str[sizeof(hash_str)-1]=0;
  policyfilepath = std::string(capath)+"/"+hash_str+".signing_policy";
  unlink(policyfilepath.c_str());
  int h = open(policyfilepath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
  if (h != -1) {
    write(h, policycontent, strlen(policycontent));
    close(h);
  }
}

void GlobusSigningPolicyTest::setUp() {
  char const * tmppath = getenv("TMPDIR");
  if (!tmppath) tmppath = "/tmp";
  capath = tmppath;

  X509_NAME* caName = X509_NAME_new();
  X509_NAME_add_entry_by_txt(caName, "C", MBSTRING_ASC, (unsigned char const *)"ca", -1, -1, 0);
  X509_NAME_add_entry_by_txt(caName, "O", MBSTRING_ASC, (unsigned char const *)"subject", -1, -1, 0);
  policyfilename1 = caName;
  caName = X509_NAME_new();
  X509_NAME_add_entry_by_txt(caName, "C", MBSTRING_ASC, (unsigned char const *)"ca", -1, -1, 0);
  X509_NAME_add_entry_by_txt(caName, "O", MBSTRING_ASC, (unsigned char const *)"subject", -1, -1, 0);
  X509_NAME_add_entry_by_txt(caName, "CN", MBSTRING_ASC, (unsigned char const *)"'-ed", -1, -1, 0);
  policyfilename2 = caName;
  caName = X509_NAME_new();
  X509_NAME_add_entry_by_txt(caName, "C", MBSTRING_ASC, (unsigned char const *)"ca", -1, -1, 0);
  X509_NAME_add_entry_by_txt(caName, "O", MBSTRING_ASC, (unsigned char const *)"subject", -1, -1, 0);
  X509_NAME_add_entry_by_txt(caName, "CN", MBSTRING_ASC, (unsigned char const *)"''-ed", -1, -1, 0);
  policyfilename3 = caName;

  make_policy_file(capath.c_str(), policyfilename1, policycontent1, policyfilepath1);
  make_policy_file(capath.c_str(), policyfilename2, policycontent2, policyfilepath2);
  make_policy_file(capath.c_str(), policyfilename3, policycontent3, policyfilepath3);
}

void GlobusSigningPolicyTest::tearDown() {
  X509_NAME_free(policyfilename1);
  policyfilename1 =  NULL;
  X509_NAME_free(policyfilename2);
  policyfilename2 =  NULL;
  X509_NAME_free(policyfilename3);
  policyfilename3 =  NULL;
  if(!policyfilepath1.empty()) unlink(policyfilepath1.c_str());
  policyfilepath1.clear();
  if(!policyfilepath2.empty()) unlink(policyfilepath2.c_str());
  policyfilepath2.clear();
  if(!policyfilepath3.empty()) unlink(policyfilepath3.c_str());
  policyfilepath3.clear();
}

void GlobusSigningPolicyTest::CASubjectMatchTest() {
  ArcMCCTLS::GlobusSigningPolicy policy;

  X509_NAME* caGood1 = X509_NAME_new();
  X509_NAME_add_entry_by_txt(caGood1, "C", MBSTRING_ASC, (unsigned char const *)"ca", -1, -1, 0);
  X509_NAME_add_entry_by_txt(caGood1, "O", MBSTRING_ASC, (unsigned char const *)"subject", -1, -1, 0);

  X509_NAME* caGood2 = X509_NAME_new();
  X509_NAME_add_entry_by_txt(caGood2, "C", MBSTRING_ASC, (unsigned char const *)"ca", -1, -1, 0);
  X509_NAME_add_entry_by_txt(caGood2, "O", MBSTRING_ASC, (unsigned char const *)"subject", -1, -1, 0);
  X509_NAME_add_entry_by_txt(caGood2, "CN", MBSTRING_ASC, (unsigned char const *)"'-ed", -1, -1, 0);

  X509_NAME* caGood3 = X509_NAME_new();
  X509_NAME_add_entry_by_txt(caGood3, "C", MBSTRING_ASC, (unsigned char const *)"ca", -1, -1, 0);
  X509_NAME_add_entry_by_txt(caGood3, "O", MBSTRING_ASC, (unsigned char const *)"subject", -1, -1, 0);
  X509_NAME_add_entry_by_txt(caGood3, "CN", MBSTRING_ASC, (unsigned char const *)"''-ed", -1, -1, 0);

  X509_NAME* caBad1 = X509_NAME_new();
  X509_NAME_add_entry_by_txt(caBad1, "C", MBSTRING_ASC, (unsigned char const *)"ca", -1, -1, 0);
  X509_NAME_add_entry_by_txt(caBad1, "O", MBSTRING_ASC, (unsigned char const *)"subject", -1, -1, 0);
  X509_NAME_add_entry_by_txt(caBad1, "CN", MBSTRING_ASC, (unsigned char const *)"''-ed", -1, -1, 0);


  X509_NAME* userGood1 = X509_NAME_new();
  X509_NAME_add_entry_by_txt(userGood1, "O", MBSTRING_ASC, (unsigned char const *)"subject", -1, -1, 0);
  X509_NAME_add_entry_by_txt(userGood1, "CN", MBSTRING_ASC, (unsigned char const *)"user1", -1, -1, 0);

  X509_NAME* userGood2 = X509_NAME_new();
  X509_NAME_add_entry_by_txt(userGood2, "O", MBSTRING_ASC, (unsigned char const *)"subject", -1, -1, 0);
  X509_NAME_add_entry_by_txt(userGood2, "CN", MBSTRING_ASC, (unsigned char const *)"user2", -1, -1, 0);
  X509_NAME_add_entry_by_txt(userGood2, "emailAddress", MBSTRING_ASC, (unsigned char const *)"alt1", -1, -1, 0);

  X509_NAME* userGood3 = X509_NAME_new();
  X509_NAME_add_entry_by_txt(userGood3, "C", MBSTRING_ASC, (unsigned char const *)"CH", -1, -1, 0);
  X509_NAME_add_entry_by_txt(userGood3, "ST", MBSTRING_UTF8, (unsigned char const *)"Gen" "\xC3" "\xA8" "ve", -1, -1, 0);
  X509_NAME_add_entry_by_txt(userGood3, "O", MBSTRING_UTF8, (unsigned char const *)"CERN Organisation Europ" "\xC3" "\xA9" "enne pour la Recherche Nucl" "\xC3" "\xA9" "aire", -1, -1, 0);
  X509_NAME_add_entry_by_txt(userGood3, "CN", MBSTRING_ASC, (unsigned char const *)"www.cern.ch", -1, -1, 0);

  X509_NAME* userBad1 = X509_NAME_new();
  X509_NAME_add_entry_by_txt(userBad1, "O", MBSTRING_ASC, (unsigned char const *)"subject", -1, -1, 0);
  X509_NAME_add_entry_by_txt(userBad1, "CN", MBSTRING_ASC, (unsigned char const *)"bad", -1, -1, 0);

  CPPUNIT_ASSERT(policy.open(policyfilename1, capath));
  CPPUNIT_ASSERT(policy.match(caGood1, userGood1));
  CPPUNIT_ASSERT(policy.open(policyfilename1, capath));
  CPPUNIT_ASSERT(policy.match(caGood1, userGood2));
  CPPUNIT_ASSERT(policy.open(policyfilename1, capath));
  CPPUNIT_ASSERT(policy.match(caGood1, userGood3));

  CPPUNIT_ASSERT(policy.open(policyfilename2, capath));
  CPPUNIT_ASSERT(policy.match(caGood2, userGood1));
  CPPUNIT_ASSERT(policy.open(policyfilename2, capath));
  CPPUNIT_ASSERT(policy.match(caGood2, userGood2));

  CPPUNIT_ASSERT(policy.open(policyfilename3, capath));
  CPPUNIT_ASSERT(policy.match(caGood3, userGood1));
  CPPUNIT_ASSERT(policy.open(policyfilename3, capath));
  CPPUNIT_ASSERT(policy.match(caGood3, userGood2));

  CPPUNIT_ASSERT(policy.open(policyfilename1, capath));
  CPPUNIT_ASSERT(!policy.match(caGood1, userBad1));
  CPPUNIT_ASSERT(policy.open(policyfilename1, capath));
  CPPUNIT_ASSERT(!policy.match(caBad1, userGood1));
}

CPPUNIT_TEST_SUITE_REGISTRATION(GlobusSigningPolicyTest);

