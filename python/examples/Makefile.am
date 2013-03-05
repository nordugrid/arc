EXAMPLES = \
	basic_job_submission.py partial_copy.py copy_file.py job_filtering.py \
	joblist_retrieval.py job_status.py job_submission.py retreiving_results.py \
	service_discovery.py

if PYLINT_ENABLED
TESTS_ENVIRONMENT = $(PYLINT) --errors-only
TESTS = $(EXAMPLES)
else
TESTS =
endif

check_SCRIPTS = $(EXAMPLES)

exampledir = $(pkgdatadir)/examples/sdk
example_DATA = $(EXAMPLES)

EXTRA_DIST = $(EXAMPLES)
