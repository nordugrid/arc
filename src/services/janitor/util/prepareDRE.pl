#!/usr/bin/perl -w

use strict;

use Getopt::Long;
use File::Path qw(remove_tree);
use File::Copy::Recursive qw(dirmove);

my $readyDir;
my $re;
my $install;
my $runtime;
my $remove;
my $help;

GetOptions(
	"dir:s"=>\$readyDir,
	"re:s"=>\$re,
	"install"=>\$install,
	"runtime"=>\$runtime,
	"remove"=>\$remove,
	"help"=>\$help
) or die "Could not parse options: $@\n";

=head1 NAME

prepareDRE.pl - transforms a directory into a dynamic runtime environment

=head1 SYNOPSIS

prepareDRE.pl --dir I<path to software> --re I<outputfile> [other options]

=head1 DESCRIPTION

This script takes a directory and transforms it into a dynamic runtime
environment (dRTE). This is particularly helpful for beginners to concentrate
on the binaries rather than on the exact notion of the inner structure
of the dRTEs.

=head1 OPTIONS

=over 4

=item --re I<string>

specifies the filename of the runtime environment ot create. This does
not require to be identical to the name of the runtime environment.
It is only a file which is mapped to a name of a runtime environment only
via its catalog entry.

=item --install I<string>

indication of a script  which shall be
executed after the untaring of the runtime environment (optional).
Use the path relative to the --dir setting or an absolute pathname.

=item --remove I<string>

path to script to execute just before the runtime environment (and the
previously unpacked directory) is removed (optional).
Use the path relative to the --dir setting or an absolute pathname.
The Janitor's default is to remove the whole folder, hence most
dRTEs won't require special treatment.

=item --runtime I<string>

path to script to execute by the job at various stages during its execution.
The default behaviour is to only add the install directory to the PATH 
environment variable. If that is not wanted, then indicate an empty file.
Use the path relative to the --dir setting or an absolute pathname.

=item --path I<string list>

=item --dir I<string>

path relative to the directory harboring all the files that are expected to contribute 
to the runtime environment.

=back

=cut

my $tmpdir=ENV("TMPDIR");

if ( -z "$tmpdir" ) {
	$tmpdir = "/tmp";
}
$tmpdir .= "/prepareDRE_$$";


# the function expects
#	TRUE/FALSE to decide if files should be cleaned.
sub quitMe($$) {
	my $clean=shift;
	my $message=shift;
	if ($clean) {
		foreach my $f ( ("install","remove","runtime") ) {
			my $ff="$tmpdir/control/$f";
			unlink("$ff") if -r "$ff";
		}
		rmdir("$tmpdir/control") if -d "$tmpdir/control";
		remove_tree("$tmpdir/data") if -d "$tmpdir/data";
		rmdir("$tmpdir") if -d "$tmpdir";
	}
	print STDERR "$message\n" if defined($message) and -n "$message";
	die "$@\n" if $@;
	exit -1;
}

if ( -d "$tmpdir") @quitme(FALSE,"The temporary directory '$tmpdir' already exists.");
mkdir "$tmpdir" or @quitme(TRUE,"");
mkdir "$tmpdir/data" or @quitme(TRUE,"");
mkdir "$tmpdir/control" or @quitme(TRUE,"");

if ( -z "$readyDir" ) {
	print STDERR "No directory specified that could be transformed into a runtime environment. Use the --dir option.\n";
	$help=1;
}
elsif ( ! -d "$readyDir" ) {
	@quitMe(TRUE,"Directory '$readyDir' not found.");
}

dircopy("$readyDir","$tmpdir/data" " or @quitMe(TRUE,"Could not change to directory '$readDir'.");

open  INSTALL,">$tmpdir/control/install" or @quitMe(TRUE,"Could not create install config file");
print INSTALL "# Nothing to do.\n";
close(INSTALL);
open  REMOVE,">$tmpdir/control/remove" or @quitMe(TRUE,"Could not create remove config file");
print REMOVE "# Nothing to do.\n";
close(REMOVE);
open  RUNTIME,">$tmpdir/control/runtime" or @quitMe(TRUE,"Could not create runtime config file");
print RUNTIME "# Nothing to do.\n";
close(RUNTIME);

system("tar czvf 

1;
