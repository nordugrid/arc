// -*- indent-tabs-mode: nil -*-
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <string>
#include <unistd.h>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/Thread.h>

class ThreadTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(ThreadTest);
  CPPUNIT_TEST(TestThread);
  CPPUNIT_TEST(TestBroadcast);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void TestThread();
  void TestBroadcast();

private:
  static void func(void*);
  static void func_wait(void* arg);
  static int counter;
  static Glib::Mutex* lock;
  Arc::SimpleCondition cond;
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
  tid = parent.tid;
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
  cond.reset();
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
  // Wait for all threads
  // In worst case it should be no more than one thread simultaneously.
  for(int n = 0; n<(500*10); ++n) {
    sleep(1);
    if(counter >= 500) break;
  }
  sleep(1);
  CPPUNIT_ASSERT_EQUAL(500,counter);
  CPPUNIT_ASSERT_EQUAL(501,titem_created);
  CPPUNIT_ASSERT_EQUAL(500,titem_deleted);
}

void ThreadTest::TestBroadcast() {
  // Create 2 threads which wait and check broadcast wakes them both up
  CPPUNIT_ASSERT(Arc::CreateThreadFunction(&func_wait, this));
  CPPUNIT_ASSERT(Arc::CreateThreadFunction(&func_wait, this));
  // Wait for threads to start
  sleep(1);
  cond.broadcast();
  // Wait for result
  sleep(1);
  CPPUNIT_ASSERT_EQUAL(2, counter);
}

void ThreadTest::func_wait(void* arg) {
  ThreadTest* test = (ThreadTest*)arg;
  test->cond.wait();
  lock->lock();
  test->counter++;
  lock->unlock();
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

