#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pwd.h>
#include <unistd.h>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/User.h>
#include <arc/Utils.h>

class UserTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(UserTest);
  CPPUNIT_TEST(OwnUserTest);
  CPPUNIT_TEST_SUITE_END();

public:

  void OwnUserTest();
};

void UserTest::OwnUserTest() {

  uid_t uid_ = getuid();
  // My username
  struct passwd pwd;
  char pwdbuf[2048];
  struct passwd *pwd_p;
  CPPUNIT_ASSERT_EQUAL(0, getpwuid_r(uid_, &pwd, pwdbuf, sizeof(pwdbuf), &pwd_p));
  int uid = (int)uid_;
  int gid = pwd_p->pw_gid;
  std::string username = pwd_p->pw_name;
  std::string home = Arc::GetEnv("HOME");
  if (home.empty()) home = pwd_p->pw_dir;

  // User using this user's uid
  Arc::User user;
  CPPUNIT_ASSERT(user);
  CPPUNIT_ASSERT_EQUAL(uid, user.get_uid());
  CPPUNIT_ASSERT_EQUAL(gid, user.get_gid());
  CPPUNIT_ASSERT_EQUAL(username, user.Name());
  CPPUNIT_ASSERT_EQUAL(home, user.Home());

  // User with specified uid and gid
  Arc::User user2(uid, gid);
  CPPUNIT_ASSERT(user2);
  CPPUNIT_ASSERT_EQUAL(uid, user2.get_uid());
  CPPUNIT_ASSERT_EQUAL(gid, user2.get_gid());
  CPPUNIT_ASSERT_EQUAL(username, user2.Name());
  CPPUNIT_ASSERT_EQUAL(home, user2.Home());

  // User with specified username
  Arc::User user3(username);
  CPPUNIT_ASSERT(user3);
  CPPUNIT_ASSERT_EQUAL(uid, user3.get_uid());
  CPPUNIT_ASSERT_EQUAL(gid, user3.get_gid());
  CPPUNIT_ASSERT_EQUAL(username, user3.Name());
  CPPUNIT_ASSERT_EQUAL(home, user3.Home());
}

CPPUNIT_TEST_SUITE_REGISTRATION(UserTest);
