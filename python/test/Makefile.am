if MACOSX
DYLD_LIBRARY_PATH = "$(top_builddir)/src/hed/libs/credentialstore/.libs:$(top_builddir)/src/hed/libs/client/.libs:$(top_builddir)/src/hed/libs/data/.libs:$(top_builddir)/src/hed/libs/security/.libs:$(top_builddir)/src/hed/libs/credential/.libs:$(top_builddir)/src/hed/libs/crypto/.libs:$(top_builddir)/src/hed/libs/message/.libs:$(top_builddir)/src/hed/libs/loader/.libs:$(top_builddir)/src/hed/libs/common/.libs:$(top_builddir)/src/libs/data-staging/.libs"
else
DYLD_LIBRARY_PATH =
endif

TESTSCRIPTS = DeletingSwigIteratorObtainedFromPublicListRegTest.py UsingTheNextMethodOnASwigIteratorRegTest.py JobSupervisorTest.py EndpointContainerTest.py EndpointTest.py ServiceEndpointRetrieverTest.py TargetInformationRetrieverTest.py
if PYTHON_ENABLED
TESTS_ENVIRONMENT = ARC_PLUGIN_PATH=$(top_builddir)/src/hed/acc/TEST/.libs DYLD_LIBRARY_PATH="$(DYLD_LIBRARY_PATH)" PYTHONPATH=..:../.libs:$(top_srcdir)/src/hed/libs/python $(PYTHON)
TESTS = $(TESTSCRIPTS)
else
TESTS =
endif
check_SCRIPTS = $(TESTS)

EXTRA_DIST = $(TESTSCRIPTS)
