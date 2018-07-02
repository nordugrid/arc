TESTS = basic-test

TESTS_ENVIRONMENT = \
	PYTHONPATH=$(abs_top_srcdir)/src/utils/python/ \
	SRCDIR=$(srcdir) \
	$(SHELL) check_scan_script.sh scan-SLURM-job

SCRIPTSNEEDED = check_scan_script.sh \
	scan-SLURM-job configure-SLURM-env.sh \
	lrms_common.sh scan_common.sh arcconfig-parser \
	command-simulator.sh gm-kick

check_SCRIPTS = $(TESTS) $(SCRIPTSNEEDED)

EXTRA_DIST = $(TESTS)

check_scan_script.sh: $(builddir)/../../../test/check_scan_script.sh
	cp $< $@

scan-SLURM-job: $(builddir)/../../scan-SLURM-job
	cp $< $@
	chmod +x $@

configure-SLURM-env.sh: $(srcdir)/../../configure-SLURM-env.sh
	cp $< $@

scan_common.sh: $(builddir)/../../../scan_common.sh
	cp $< $@

lrms_common.sh: $(builddir)/../../../lrms_common.sh
	cp $< $@

arcconfig-parser: $(top_srcdir)/src/utils/python/arcconfig-parser
	cp $< $@

command-simulator.sh: $(top_srcdir)/src/tests/lrms/command-simulator.sh
	cp $< $@
	chmod +x $@

gm-kick: $(srcdir)/../../../test/test-gm-kick.sh
	cp $< $@
	chmod +x $@

CLEANFILES = $(SCRIPTSNEEDED)
