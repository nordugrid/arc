TESTS_ENVIRONMENT = \
	$(PERL) -I$(srcdir) -I$(srcdir)/.. -w -MTest::Harness -e '$$Test::Harness::verbose=0; runtests @ARGV;'

TESTS = pbs.t slurm.t sge.t

PERL = @PERL@

check_SCRIPTS = InfoproviderTestSuite.pm command-simulator.sh \
                $(TESTS)

EXTRA_DIST = InfoproviderTestSuite.pm command-simulator.sh pbs.t slurm.t sge.t

command-simulator.sh: $(top_srcdir)/src/tests/lrms/command-simulator.sh
	cp $< $@
	chmod +x $@
