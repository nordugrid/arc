#! /usr/bin/perl

# unit test of Janitor::Util

use lib "..";
use Janitor::Util qw(sane_rte_name);

my %RTE_NAMES = (
	'ASDF'		=> 1,	# correct
	'ASDF/JKL'	=> 1,	# correct
	'.'		=> 0,	# forbidden
	'/'		=> 0,	# forbidden
	'/ASDF'		=> 0,	# forbidden
	'../ASDF'	=> 0,	# forbidden
	'ASDF/..'	=> 0,	# forbidden
	'ASDF/../'	=> 0,	# forbidden
	'ASDF/../JKLÖ'	=> 0,	# forbidden
	'AS/./DF/.'	=> 0,	# forbidden
	'AS/DF/.'	=> 0,	# forbidden
	''		=> 0,	# forbidden
);

foreach my $rte ( keys %RTE_NAMES ) {
	if (sane_rte_name($rte) != $RTE_NAMES{$rte}){
		printf "failed test on: \"%s\"\n", $rte;
	}
}
