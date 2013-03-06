EXAMPLES = BasicJobSubmission.java DTRGenerator.java

if JAVA_SWIG_ENABLED
CLASSPATH = $(top_builddir)/java/arc.jar:.
TESTS_ENVIRONMENT = $(JAVAC) $(JAVAC_FLAGS) -cp $(CLASSPATH) -d .
TESTS = $(EXAMPLES)
else
CLASSPATH =
TESTS =
endif

check_SCRIPTS = $(EXAMPLES)

CLEANFILES = *.class

exampledir = $(pkgdatadir)/examples/sdk
example_DATA = $(EXAMPLES)

EXTRA_DIST = $(EXAMPLES)
