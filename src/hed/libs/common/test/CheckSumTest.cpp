// -*- indent-tabs-mode: nil -*-

#include <fstream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/CheckSum.h>

class CheckSumTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(CheckSumTest);
  CPPUNIT_TEST(CRC32SumTest);
  CPPUNIT_TEST(MD5SumTest);
  CPPUNIT_TEST(Adler32SumTest);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void CRC32SumTest();
  void MD5SumTest();
  void Adler32SumTest();
};


void CheckSumTest::setUp() {
  std::ofstream f1K("CheckSumTest.f1K.data", std::ios::out), f1M("CheckSumTest.f1M.data", std::ios::out);

  for (int i = 0; i < 1000; ++i) {
    f1K << "0";
    for (int j = 0; j < 1000; ++j) {
      f1M << "0";
    }
  }
  f1K.close();
  f1M.close();
}

void CheckSumTest::tearDown() {
  remove("CheckSumTest.f1K.data");
  remove("CheckSumTest.f1M.data");
}

void CheckSumTest::CRC32SumTest() {
  CPPUNIT_ASSERT_EQUAL((std::string)"acb7ca96", Arc::CheckSumAny::FileChecksum("CheckSumTest.f1K.data", Arc::CheckSumAny::cksum));
  CPPUNIT_ASSERT_EQUAL((std::string)"53a57307", Arc::CheckSumAny::FileChecksum("CheckSumTest.f1M.data", Arc::CheckSumAny::cksum));
  char buf[64];
  Arc::CheckSumAny ck(Arc::CheckSumAny::cksum);
  ck.scan("cksum:53a57307");
  ck.print(buf,sizeof(buf));
  CPPUNIT_ASSERT_EQUAL((std::string)"cksum:53a57307", (std::string)buf);
}

void CheckSumTest::MD5SumTest() {
  CPPUNIT_ASSERT_EQUAL((std::string)"88bb69a5d5e02ec7af5f68d82feb1f1d", Arc::CheckSumAny::FileChecksum("CheckSumTest.f1K.data"));
  CPPUNIT_ASSERT_EQUAL((std::string)"2f54d66538c094bf229e89ed0667b6fd", Arc::CheckSumAny::FileChecksum("CheckSumTest.f1M.data"));
  char buf[64];
  Arc::CheckSumAny ck(Arc::CheckSumAny::md5);
  ck.scan("md5:2f54d66538c094bf229e89ed0667b6fd");
  ck.print(buf,sizeof(buf));
  CPPUNIT_ASSERT_EQUAL((std::string)"md5:2f54d66538c094bf229e89ed0667b6fd", (std::string)buf);
}

void CheckSumTest::Adler32SumTest() {
  CPPUNIT_ASSERT_EQUAL((std::string)"ad1abb81", Arc::CheckSumAny::FileChecksum("CheckSumTest.f1K.data", Arc::CheckSumAny::adler32));
  CPPUNIT_ASSERT_EQUAL((std::string)"471b96e5", Arc::CheckSumAny::FileChecksum("CheckSumTest.f1M.data", Arc::CheckSumAny::adler32));
  //char buf[64];
  //Arc::CheckSumAny ck(Arc::CheckSumAny::adler32);
  //ck.scan("adler32:471b96e5");
  //ck.print(buf,sizeof(buf));
  //CPPUNIT_ASSERT_EQUAL((std::string)"adler32:471b96e5", (std::string)buf);
}

CPPUNIT_TEST_SUITE_REGISTRATION(CheckSumTest);
