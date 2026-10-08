// -*- indent-tabs-mode: nil -*-
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>
#include <cstdarg>
#include <cstring>

#include <arc/HostnameResolver.h>

#define VERBOSE true

class HostnameResolverTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(HostnameResolverTest);
  CPPUNIT_TEST(CacheInvalidationTest);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();

  void CacheInvalidationTest();
};

static int lookups;
static int gcs;
static int reaps;
static int successes;
static int failures;

static void logit(const char* action, ...) {
  va_list args;
  va_start(args, action);
#ifdef VERBOSE
  std::cerr << "Hostname testing: " << action;
#endif
  if (strcmp(action, "lookup") == 0 || strcmp(action, "reap") == 0) {
    if (strcmp(action, "lookup") == 0) {
      lookups++;
    } else {
      reaps++;
    }
#ifdef VERBOSE
    const char* node = va_arg(args, const char*);
    const char* service = va_arg(args, const char*);
    const char* where = va_arg(args, const char*);
    time_t expires = va_arg(args, time_t);
    std::cerr << " node=" << node
              << " service=" << service
              << " where=" << where
              << " expires=" << expires
              << "\n";
#endif
  } else if (strcmp(action, "gc") == 0) {
    gcs++;
#ifdef VERBOSE
    unsigned long collected = va_arg(args, unsigned long);
    unsigned long remaining = va_arg(args, unsigned long);
    std::cerr << " collected=" << collected << " remaining=" << remaining << "\n";
#endif
  } else {
#ifdef VERBOSE
    std::cerr << "???\n";
#endif
  }
  va_end(args);
}

void HostnameResolverTest::setUp() {
  // Sets TTL=10s, GC interval=5s
  Arc::HostnameResolver::testtune(logit);
}

static bool lookup(const char* node, const char* service, bool local) {
#ifdef VERBOSE
  std::cerr << "Hostname testing: t=" << ::time(nullptr) << "\n";
#endif
  std::list<Arc::HostnameResolver::SockAddr> addrs;
  if (!Arc::HostnameResolver::Resolve(node, service, local, addrs)) {
#ifdef VERBOSE
    std::cerr << "Hostname testing: Lookup failed for " << node << "\n";
#endif
    failures++;
    return false;
  } else {
#ifdef VERBOSE
    std::cerr << "Hostname testing: Lookup succeeded for " << node << "\n";
#endif
    successes++;
    return true;
  }
}

void HostnameResolverTest::CacheInvalidationTest() {
  // This test is a bit timing dependent.  If the system is very busy, it may fail because some work
  // is delayed.  Note though that GCs are not spaced evenly apart: a GC is triggered by a lookup
  // AND the GC interval having expired, and then it resets the interval to now + delta.  If no
  // lookup happens for a while then no GC is run either.  The gc interval is really the minimum
  // time between GCs but the maximum is unbounded.

  // t = 0
  lookup("www.google.com", "443", false); // expires at 10
  sleep(1);
  // t >= 1
  lookup("www.uio.no", "443", false); // expires at 11
  sleep(5);
  // t >= 6
  lookup("www.uit.no", "443", false); // expires at 16
  sleep(1);
  // GC should run before now but should delete nothing
  // t >= 7
  lookup("www.google.com", "443", false); // should be alive, expires at 10
  sleep(1);
  // t >= 8
  lookup("www.uio.no", "443", false); // should be alive, expires at 11
  sleep(1);
  // t >= 9
  lookup("www.uit.no", "443", false); // should be alive, expires at 16
  sleep(5);
  // GC should run before now and should delete 2 objects, but not 3
  // t >= 14
  lookup("www.uit.no", "443", false);     // should be alive, expires at 16
  lookup("www.google.com", "443", false); // should be dead now
  sleep(1);
  // t >= 15
  lookup("www.uio.no", "443", false); // should be dead now
  sleep(6);
  // t >= 20
  lookup("www.uit.no", "443", false);     // should be dead now

  // Expect 4 GCs: initial GC + 3 subsequent.  But be conservative on slow systems.
  CPPUNIT_ASSERT(gcs >= 4);
  if (gcs == 4) {
    CPPUNIT_ASSERT_EQUAL(6, lookups);
    CPPUNIT_ASSERT_EQUAL(10, successes);
    CPPUNIT_ASSERT_EQUAL(0, failures);
  }
}

CPPUNIT_TEST_SUITE_REGISTRATION(HostnameResolverTest);
