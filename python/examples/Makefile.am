EXAMPLES = \
	basic_job_submission.py partial_copy.py copy_file.py job_filtering.py \
	joblist_retrieval.py job_status.py job_submission.py retrieving_results.py \
	service_discovery.py dtr_generator.py

if PYLINT_ENABLED
TESTS_ENVIRONMENT = \
        test -r arc || ln -s .. arc ; \
        test -r ../_arc.so || ln -s .libs/_arc.so ../_arc.so ; \
        $(PYLINT) --errors-only
TESTS = $(EXAMPLES)
else
TESTS =
endif

check_SCRIPTS = $(EXAMPLES)

CLEANFILES = arc

exampledir = $(pkgdatadir)/examples/sdk
example_DATA = $(EXAMPLES)

EXTRA_DIST = $(EXAMPLES)
