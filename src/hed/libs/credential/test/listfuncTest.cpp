#include <cppunit/extensions/HelperMacros.h>

#include <malloc.h>
#include <string.h>
#include "../listfunc.h"

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

  char **v1,**v2,**v3,**v4;
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

  v2 = listadd(NULL,str2,sizeof(char*));
  CPPUNIT_ASSERT(v2);

  v3 = listjoin(v1,v2,sizeof(char*));
  CPPUNIT_ASSERT(v3);

  v4 = listadd(v3,str3,sizeof(char*));
  CPPUNIT_ASSERT(v4);

  CPPUNIT_ASSERT(v4[0] == str1);
  CPPUNIT_ASSERT(v4[1] == str2);
  CPPUNIT_ASSERT(v4[2] == str3);

  CPPUNIT_ASSERT_EQUAL(listjoin(NULL,NULL,0),(char**)NULL);

  listfree(v4,(freefn)f);
}

CPPUNIT_TEST_SUITE_REGISTRATION(listfuncTest);

