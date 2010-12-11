// -*- indent-tabs-mode: nil -*-

#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/Thread.h>

class ThreadTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(ThreadTest);
  CPPUNIT_TEST(TestThread);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void TestThread();

private:
  static void func(void*);
  static int counter;
  static Glib::Mutex* lock;
};

static int titem_created = 0;
static int titem_deleted = 0;

class TItem: public Arc::ThreadDataItem {
private:
  std::string tid;
  int id;
  ~TItem(void);
public:
  TItem(void);
  TItem(const TItem& parent);
  virtual void Dup(void);
};

TItem::TItem(void) {
  Attach(tid);
  id=(++titem_created);
}

TItem::TItem(const TItem& parent):Arc::ThreadDataItem(parent.tid) {
  id=(++titem_created);
}

TItem::~TItem(void) {
  ++titem_deleted;
}

void TItem::Dup(void) {
  new TItem(*this);
}

void ThreadTest::setUp() {
  counter = 0;
  lock = new Glib::Mutex;
  new TItem;
}


void ThreadTest::tearDown() {
  if(lock) delete lock;
}

void ThreadTest::TestThread() {
  // Simply run 500 threads and see if executable crashes
  for(int n = 0;n<500;++n) {
    CPPUNIT_ASSERT(Arc::CreateThreadFunction(&func,NULL));
  }
  sleep(30);
  CPPUNIT_ASSERT_EQUAL(500,counter);
  CPPUNIT_ASSERT_EQUAL(501,titem_created);
  CPPUNIT_ASSERT_EQUAL(500,titem_deleted);
}

void ThreadTest::func(void*) {
  sleep(1);
  lock->lock();
  ++counter;
  lock->unlock();
}

int ThreadTest::counter = 0;
Glib::Mutex* ThreadTest::lock = NULL;

CPPUNIT_TEST_SUITE_REGISTRATION(ThreadTest);

