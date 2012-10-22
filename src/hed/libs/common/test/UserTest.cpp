#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef WIN32
#include <glibmm/miscutils.h>
#else
#include <pwd.h>
#include <unistd.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <arc/User.h>

class UserTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(UserTest);
  CPPUNIT_TEST(OwnUserTest);
  CPPUNIT_TEST_SUITE_END();

public:

  void OwnUserTest();
};

void UserTest::OwnUserTest() {

#ifdef WIN32
  // uid/gid not implemented in win32 version of User
  int uid = 0;
  int gid = 0;
  bool found;
  std::string username = Glib::getenv("USERNAME", found);
  CPPUNIT_ASSERT(found);
#else
  uid_t uid_ = getuid();
  gid_t gid_ = getgid();
  // My username
  struct passwd pwd;
  char pwdbuf[2048];
  struct passwd *pwd_p;
  CPPUNIT_ASSERT_EQUAL(0, getpwuid_r(uid_, &pwd, pwdbuf, sizeof(pwdbuf), &pwd_p));
  int uid = (int)uid_;
  int gid = (int)gid_;
  std::string username = pwd_p->pw_name;
#endif

  // User using this user's uid
  Arc::User user;
  CPPUNIT_ASSERT(user);
  CPPUNIT_ASSERT_EQUAL(uid, user.get_uid());
  CPPUNIT_ASSERT_EQUAL(gid, user.get_gid());
  CPPUNIT_ASSERT_EQUAL(username, user.Name());

  // User with specified uid and gid
  Arc::User user2(uid, gid);
  CPPUNIT_ASSERT(user2);
  CPPUNIT_ASSERT_EQUAL(uid, user2.get_uid());
  CPPUNIT_ASSERT_EQUAL(gid, user2.get_gid());
  CPPUNIT_ASSERT_EQUAL(username, user2.Name());

  // User with specified username
  Arc::User user3(username);
  CPPUNIT_ASSERT(user3);
  CPPUNIT_ASSERT_EQUAL(uid, user3.get_uid());
  CPPUNIT_ASSERT_EQUAL(gid, user3.get_gid());
  CPPUNIT_ASSERT_EQUAL(username, user3.Name());
}

CPPUNIT_TEST_SUITE_REGISTRATION(UserTest);
