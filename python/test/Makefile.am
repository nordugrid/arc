if MACOSX
DYLD_LIBRARY_PATH = "$(top_builddir)/src/hed/libs/credentialstore/.libs:$(top_builddir)/src/hed/libs/communication/.libs:$(top_builddir)/src/hed/libs/compute/.libs:$(top_builddir)/src/hed/libs/data/.libs:$(top_builddir)/src/hed/libs/security/.libs:$(top_builddir)/src/hed/libs/credential/.libs:$(top_builddir)/src/hed/libs/crypto/.libs:$(top_builddir)/src/hed/libs/message/.libs:$(top_builddir)/src/hed/libs/loader/.libs:$(top_builddir)/src/hed/libs/common/.libs:$(top_builddir)/src/libs/data-staging/.libs"
else
DYLD_LIBRARY_PATH =
endif

TESTSCRIPTS = \
	DeletingSwigIteratorObtainedFromPublicListRegTest.py \
	UsingTheNextMethodOnASwigIteratorRegTest.py \
	JobSupervisorTest.py EndpointContainerTest.py \
	EndpointTest.py ServiceEndpointRetrieverTest.py \
	TargetInformationRetrieverTest.py \
	MappingOf_time_t_and_uint32_t_CTypesToPythonRegressionTest.py


if PYTHON_ENABLED
TESTS_ENVIRONMENT = \
        test -r arc || ln -s .. arc ; \
        test -r ../_arc.so || ln -s .libs/_arc.so ../_arc.so ; \
	ARC_PLUGIN_PATH=$(top_builddir)/src/hed/acc/TEST/.libs \
	DYLD_LIBRARY_PATH="$(DYLD_LIBRARY_PATH)" \
	$(PYTHON)
TESTS = $(TESTSCRIPTS)
else
TESTS =
endif
check_SCRIPTS = $(TESTS)

EXTRA_DIST = $(TESTSCRIPTS) testutils.py
