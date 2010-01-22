T = pbs-test1.t

PERL = /usr/bin/perl
TESTS_ENVIRONMENT = $(PERL) -I$(srcdir) -I$(srcdir)/.. -w
#TESTS = $(T)

check:
	srcdir=$(srcdir) $(PERL) -I$(srcdir) -I$(srcdir)/.. -w -MTest::Harness -e '$$Test::Harness::verbose=0; runtests @ARGV;' $(T)
