if JAVA_IS_15_OR_ABOVE
JAVA_15_EXAMPLES = ResourceDiscovery.java
else
JAVA_15_EXAMPLES = 
endif

EXAMPLES = BasicJobSubmission.java DTRGenerator.java $(JAVA_15_EXAMPLES)

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

EXTRA_DIST = $(EXAMPLES) ResourceDiscovery.java
