JAVAUNITTESTS = \
	MappingOf_time_t_and_uint32_t_CTypesToJavaRegressionTest
nodist_TESTSCRIPTS = \
	$(JAVAUNITTESTS:=.class)

if JUNIT_ENABLED
CLASSPATH = $(top_builddir)/java/arc.jar:.:/usr/share/java/junit.jar
TESTS_ENVIRONMENT = \
	ARC_PLUGIN_PATH=$(top_builddir)/src/hed/acc/TEST/.libs \
	CLASSPATH=$(CLASSPATH) \
	LD_LIBRARY_PATH=$(top_builddir)/java/.libs
	$(SHELL) $(top_builddir)/java/test/strip_test_file_name_and_run_junit
TESTS = $(nodist_TESTSCRIPTS)
else
CLASSPATH =
TESTS =
endif

check_SCRIPTS = $(nodist_TESTSCRIPTS)

CLEANFILES = *.class

.java.class:
	$(JAVAC) $(JAVAC_FLAGS) -cp $(CLASSPATH) -d . $<

EXTRA_DIST = $(JAVAUNITTESTS:=.java)
