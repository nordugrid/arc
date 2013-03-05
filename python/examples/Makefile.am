EXAMPLES = \
	basic_job_submission.py partial_copy.py copy_file.py job_filtering.py \
	joblist_retrieval.py job_status.py job_submission.py retreiving_results.py \
	service_discovery.py

if PYLINT_ENABLED
TESTS_ENVIRONMENT = PYTHONPATH=..:../.libs $(PYLINT) --errors-only
TESTS = $(EXAMPLES)
else
TESTS =
endif

check_SCRIPTS = ../arc.py $(EXAMPLES)

../arc.py : $(top_srcdir)/python/__init__.py
	cp $< $@

CLEANFILES = ../arc.py ../arc.pyc

exampledir = $(pkgdatadir)/examples/sdk
example_DATA = $(EXAMPLES)

EXTRA_DIST = $(EXAMPLES)
