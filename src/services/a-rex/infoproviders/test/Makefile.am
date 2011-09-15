T = $(srcdir)/pbs-test1.t

PERL = /usr/bin/perl
TESTS_ENVIRONMENT = $(PERL) -I$(srcdir) -I$(srcdir)/.. -w
#TESTS = $(T)

check:
	srcdir=$(srcdir) $(PERL) -I$(srcdir) -I$(srcdir)/.. -w -MTest::Harness -e '$$Test::Harness::verbose=0; runtests @ARGV;' $(T)

EXTRA_DIST = pbs-test1.t pbs-test1-uimock.sh setupPBS.pm \
             sge-test1.t sge-test1-uimock.sh setupSGE.pm
