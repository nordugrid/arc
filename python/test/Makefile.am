if MACOSX
DYLD_LIBRARY_PATH = "$(top_builddir)/src/hed/libs/credentialstore/.libs:$(top_builddir)/src/hed/libs/client/.libs:$(top_builddir)/src/hed/libs/data/.libs:$(top_builddir)/src/hed/libs/security/.libs:$(top_builddir)/src/hed/libs/credential/.libs:$(top_builddir)/src/hed/libs/crypto/.libs:$(top_builddir)/src/hed/libs/message/.libs:$(top_builddir)/src/hed/libs/loader/.libs:$(top_builddir)/src/hed/libs/common/.libs"
else
DYLD_LIBRARY_PATH =
endif

TESTSCRIPTS = SwigPyIteratorSegFaultRegTest.py JobSupervisorTest.py
if PYTHON_ENABLED
TESTS_ENVIRONMENT = ARC_PLUGIN_PATH=$(top_builddir)/src/hed/acc/TEST/.libs DYLD_LIBRARY_PATH="$(DYLD_LIBRARY_PATH)" PYTHONPATH=..:../.libs:../../src/hed/libs/python $(PYTHON)
TESTS = $(TEST_SCRIPTS)
else
TESTS =
endif
check_SCRIPTS = $(TESTS)

EXTRA_DIST = $(TESTSCRIPTS)
