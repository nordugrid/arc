#!/usr/bin/perl -w

=head1 NAME

update_catalog.pl - retrieve catalogue files stored remotely

=head1 SYNOPSIS

update_catalog.pl pathToArc.conf

=head1 DESCRIPTION

The script performs the automated download of catalog files from external source,
according to the paths identified in the Janitor blocks of an arc.conf-formatted
file.

=head1 AUTHOR

Daniel Bayer, LE<uuml>beck, 2007

=cut

use strict;
use File::Basename;

my $lastBlock=undef;
my $catalog=undef;

my %usedFilenames;
my $catalogdir="/var/spool/nordugrid/janitor/catalog";

while(<>) {
	next if /^\s*#/;
	my $tmp;
	# A new block is defined
	if (/^\s*\[.*\]/) {
		if (defined($lastBlock)) {
			print STDERR "lastBlock: $lastBlock\n";
			print STDERR "catalog=$catalog\n" if defined($catalog);
			if ($catalog =~ /^(ht|f)tp:\/\//) {
				unless (defined($catalog)) {
					$catalog=$catalogdir."/".basename($catalog);
				}

				my $d = $catalogdir;
				die "\n\nCould not access catalog root directory '$d'.\n\n"
					unless ( -d $d );

				unless ( -d "$d/$lastBlock") {
					mkdir("$d/$lastBlock") or die "Could not create subdir '"
								."$d/$lastBlock"
								."' for automated download\n";
				}
				chdir "$d/$lastBlock" or die "Could not access directory "
								."$d/$lastBlock\n";
				my $cmd="wget -N -nH -nd $catalog";
				print "Executing: '$cmd'";
				if (0 == system($cmd)) {
					print "...ok.\n";
				}
				else {
					print "...failed.\n";
				}
				chdir("..");
				link("$lastBlock/".basename($catalog),"$lastBlock.rdf");
			}
			print STDERR "\n";
			$lastBlock=undef;
			$catalog=undef;
		}
		if (($tmp)= /^\s*\[\s*janitor\/([^\s]+)\s*\]/) {
			$lastBlock=$tmp;
		}
	}
	# a regular line is read, but only if we are in a block
	# of our interest
	elsif (defined($lastBlock)) {
		my $tmp;
		if (($tmp)=/^\s*catalog\s*=\s*"([^"]*)"/) {
			$catalog=$tmp;
		}
	}
	else {
		my $tmp;
		if (($tmp)=/^\s*catalogdir\s*=\s*"([^"]*)"/) {
			$catalogdir=$tmp;
			print STDERR "Catalogdir assigned to $catalogdir in line ".$_."\n";
		}
	}

}
