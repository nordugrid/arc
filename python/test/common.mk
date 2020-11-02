if MACOSX
DYLD_LIBRARY_PATH = "$(top_builddir)/src/hed/libs/credentialstore/.libs:$(top_builddir)/src/hed/libs/communication/.libs:$(top_builddir)/src/hed/libs/compute/.libs:$(top_builddir)/src/hed/libs/data/.libs:$(top_builddir)/src/hed/libs/security/.libs:$(top_builddir)/src/hed/libs/credential/.libs:$(top_builddir)/src/hed/libs/crypto/.libs:$(top_builddir)/src/hed/libs/message/.libs:$(top_builddir)/src/hed/libs/loader/.libs:$(top_builddir)/src/hed/libs/common/.libs:$(top_builddir)/src/libs/data-staging/.libs"
else
DYLD_LIBRARY_PATH =
endif

include $(srcdir)/../tests.mk

$(TESTSCRIPTS) testutils.py: %: $(srcdir)/../% testutils.py
	cp -p $< $@

CLEANFILES = $(TESTSCRIPTS) testutils.py*

clean-local:
	-rm -rf __pycache__

if PYTHON_SWIG_ENABLED
TESTS_ENVIRONMENT = \
	ARC_PLUGIN_PATH=$(top_builddir)/src/hed/acc/TEST/.libs \
	DYLD_LIBRARY_PATH="$(DYLD_LIBRARY_PATH)" \
	PYTHONPATH=$(PYTHONPATH) $(PYTHON)

TESTS = $(TESTSCRIPTS)
else
TESTS =
endif
check_SCRIPTS = $(TESTS)
