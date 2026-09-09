TESTS = sge-lrms-regression.sh

TESTS_ENVIRONMENT = \
	SGE_SUBMIT_SCRIPT=$(abs_builddir)/../submit-sge-job \
	SGE_SCAN_SCRIPT=$(abs_builddir)/../scan-sge-job \
	SGE_CANCEL_SCRIPT=$(abs_builddir)/../cancel-sge-job \
	SGE_CONFIGURE_ENV=$(abs_srcdir)/../configure-sge-env.sh \
	SGE_TEST_SRCDIR=$(abs_srcdir) \
	$(SHELL)

EXTRA_DIST = sge-lrms-regression.sh run-tests.sh fake-sge-command \
	fake-lrms-common.sh fake-submit-common.sh fake-scan-common.sh \
	fake-cancel-common.sh fake-gm-kick
