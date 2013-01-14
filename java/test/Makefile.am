TESTSCRIPTS = 

if JUNIT_ENABLED
CLASSPATH = $(top_builddir)/java/arc.jar:.:/usr/share/java/junit.jar
TESTS_ENVIRONMENT = \
	ARC_PLUGIN_PATH=$(top_builddir)/src/hed/acc/TEST/.libs \
	CLASSPATH=$(CLASSPATH) \
	$(SHELL) $(top_builddir)/java/test/strip_test_file_name_and_run_junit
TESTS = $(TESTSCRIPTS)
else
CLASSPATH =
TESTS =
endif

check_SCRIPTS = $(TESTSCRIPTS)

CLEANFILES = *.class

.java.class:
	$(JAVAC) $(JAVAC_FLAGS) -cp $(CLASSPATH) -d . $<

EXTRA_DIST = $(TESTSCRIPTS)
