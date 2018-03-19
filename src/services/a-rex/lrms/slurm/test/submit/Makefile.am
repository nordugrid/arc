TESTS = basic-test memory-test count-test queue-test job-name-test \
	cpu-wall-time-test rte-test config-options-test std-io-test user-env-test \
	files-io-test

TESTS_ENVIRONMENT = \
	PYTHONPATH=$(abs_top_srcdir)/src/utils/configparser/ \
	ARC_PLUGIN_PATH=$(abs_top_builddir)/src/hed/acc/JobDescriptionParser/.libs \
	ARC_LOCATION=$(abs_builddir) \
	TEST_WRITE_GRAMI_FILE=$(top_builddir)/src/services/a-rex/grid-manager/test_write_grami_file \
	SRCDIR=$(srcdir) \
	$(SHELL) check_submit_script.sh submit-SLURM-job

SCRIPTSNEEDED = basic-script.sh check_submit_script.sh \
	submit-SLURM-job configure-SLURM-env.sh \
	submit_common.sh $(pkglibexecsubdir)/arcconfig-parser command-simulator.sh

check_SCRIPTS = $(TESTS) $(SCRIPTSNEEDED)

EXTRA_DIST = $(check_SCRIPTS)

check_submit_script.sh: $(srcdir)/../../../test/check_submit_script.sh
	cp $< $@

submit-SLURM-job: $(builddir)/../../submit-SLURM-job
	cp $< $@
	chmod +x $@

configure-SLURM-env.sh: $(srcdir)/../../configure-SLURM-env.sh
	cp $< $@

submit_common.sh: $(builddir)/../../../submit_common.sh
	cp $< $@

$(pkglibexecsubdir)/arcconfig-parser: $(top_srcdir)/src/utils/configparser/arcconfig-parser
	mkdir -p $(pkglibexecsubdir)
	cp $< $(pkglibexecsubdir)/.

command-simulator.sh: $(top_srcdir)/src/tests/lrms/command-simulator.sh
	cp $< $@
	chmod +x $@

CLEANFILES = $(SCRIPTSNEEDED)
