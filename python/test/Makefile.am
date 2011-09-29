if MACOSX
DYLD_LIBRARY_PATH = "$(top_builddir)/src/hed/libs/credentialstore/.libs:$(top_builddir)/src/hed/libs/client/.libs:$(top_builddir)/src/hed/libs/data/.libs:$(top_builddir)/src/hed/libs/security/.libs:$(top_builddir)/src/hed/libs/credential/.libs:$(top_builddir)/src/hed/libs/crypto/.libs:$(top_builddir)/src/hed/libs/message/.libs:$(top_builddir)/src/hed/libs/loader/.libs:$(top_builddir)/src/hed/libs/common/.libs"
else
DYLD_LIBRARY_PATH =
endif

if PYTHON_ENABLED


TESTS_ENVIRONMENT = DYLD_LIBRARY_PATH="$(DYLD_LIBRARY_PATH)" PYTHONPATH=../.:../.libs/. $(PYTHON)
TESTS = SwigPyIteratorSegFaultRegTest.py
else
TESTS =
endif
check_SCRIPTS = $(TESTS)

EXTRA_DIST = SwigPyIteratorSegFaultRegTest.py
