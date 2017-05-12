#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <stdlib.h>
#include <string.h>
#include "../listfunc.h"

using namespace ArcCredential;

class listfuncTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(listfuncTest);
  CPPUNIT_TEST(listTest);
  CPPUNIT_TEST_SUITE_END();

public:
  listfuncTest() {}
  void setUp() {}
  void tearDown() {}
  void listTest();
};


void f(void *v) {
  if (v==NULL) return;
  free(v);
  return;
}

void listfuncTest::listTest() {

  char **v1,**v2,**v3;
  char *str1,*str2,*str3;

  str1 = (char*) malloc(sizeof(char)*6);
  strncpy(str1,"Hello",6); 
  str2 = (char*) malloc(sizeof(char)*6);
  strncpy(str2,"World",6); 
  str3 = (char*) malloc(sizeof(char)*6);
  strncpy(str3,"World",6); 

  v1 = listadd(NULL,NULL,sizeof(char*));
  CPPUNIT_ASSERT_EQUAL(v1,(char**)NULL);

  v1 = listadd(NULL,str1,sizeof(char*));
  CPPUNIT_ASSERT(v1);

  v2 = listadd(v1,str2,sizeof(char*));
  CPPUNIT_ASSERT(v1);

  v3 = listadd(v2,str3,sizeof(char*));
  CPPUNIT_ASSERT(v3);

  CPPUNIT_ASSERT(v3[0] == str1);
  CPPUNIT_ASSERT(v3[1] == str2);
  CPPUNIT_ASSERT(v3[2] == str3);
  CPPUNIT_ASSERT(v3[3] == NULL);

  listfree(v3,(freefn)f);
}

CPPUNIT_TEST_SUITE_REGISTRATION(listfuncTest);

