EXAMPLES = basic_job_submission.py

if PYLINT_ENABLED
TESTS_ENVIRONMENT = $(PYLINT) -E
TESTS = $(EXAMPLES)
else
TESTS =
endif

check_SCRIPTS = $(EXAMPLES)

exampledir = $(pkgdatadir)/examples/sdk
example_DATA = $(EXAMPLES)

EXTRA_DIST = $(EXAMPLES)
