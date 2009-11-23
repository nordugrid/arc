#!/usr/bin/perl -w

use strict;

use Getopt::Long;
use File::Path qw(remove_tree);
use File::Copy::Recursive qw(dircopy);
use File::Basename;

my $readyDir;
my $re;
my $install;
my $runtime;
my $remove;
my $help;
my $category;
my $homepage;
my $description;
my $basesystem;
my $catalog;
my $verbose;

GetOptions(
	"dir:s"=>\$readyDir,
	"re:s"=>\$re,
	"install"=>\$install,
	"runtime"=>\$runtime,
	"remove"=>\$remove,
	"category"=>\$category,
	"homepage"=>\$homepage,
	"description"=>\$description,
	"basesystem"=>\$basesystem,
	"catalog:s"=>\$catalog,
	"help"=>\$help,
	"verbose"=>\$verbose
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

The script will adjust the settings of the LD_LIBRARY_PATH environment
variable to the directory that is untared by the Janitor. More correctly
stated: it prepares install, remove and runtime scripts that instruct
the Janitor to parse such and prepare the real scripts that are then
invoked by the backend scripts preparing the use of runtime environments.

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

=item --category I<string>

the path in the runtime environment hierarchy, e.g. "APPS/BIO", defaulting to "APPS/OTHER" if unset, to appear in the catalog entryz that is pasted to stdout.

=item --homepage I<string>

a complete URL to indicate where to learn more about a particular program, e.g. http://autodock.scripps.edu,
defaulting to http://www.nordugrid.org, to appear in the catalog entryz that is pasted to stdout.

=item --description I<string>

a verbose description of the runtime environment, to appear in the catalog enry that is pasted to stdout.

=item --basesystem I<string>

indication of the base system that the particular runtime environment is known to be compatible with, defaults to the RDF ID for Debian Etch, &kb;knowarc_Instance_0.

=item --catalog I<string>

If set, a file that may act as a catalog only for the just prepared runtime environment is prepared.

=back

=cut

my $tmpdir=$ENV{"TMPDIR"};

if ( !defined($tmpdir) or -z "$tmpdir" ) {
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
	print STDERR "$message\n" if defined($message) and ( "" ne "$message" );
	die "$@\n" if $@;
	exit -1;
}

-d "$tmpdir" and quitMe(0,"The temporary directory '$tmpdir' already exists.");
mkdir "$tmpdir" or quitMe(1,"");
mkdir "$tmpdir/data" or quitMe(1,"");
mkdir "$tmpdir/control" or quitMe(1,"");

if (defined($help) and $help) {
	use Pod::Usage;
	pod2usage(-verbose => 2);
}

if ( !defined($re) or -z "$re" ) {
	quitMe(1,"No filename specified under which the runtime environment should be saved.\n"
	        . "Use the --re option.\n");
}

if ( !defined($readyDir) or -z "$readyDir" ) {
	quitMe(1,"No directory specified that could be transformed into a runtime environment. "
	        . "Use the --dir option.\n");
}
elsif ( ! -d "$readyDir" ) {
	quitMe(1,"Directory '$readyDir' not found.");
}

dircopy("$readyDir","$tmpdir/data")
		or quitMe(1,"Could not change to directory '$readyDir'.");

if (defined($install) and "" ne "$install") {
	File::copy("$remove","$tmpdir/control/install")
		or quitMe(1,"Could not copy install config file");
}
else {
	open  INSTALL,">$tmpdir/control/install"
		or quitMe(1,"Could not create install config file");
	print INSTALL "#!/bin/sh\n";
	print INSTALL "# Nothing to do.\n";
	close(INSTALL);
}

if (defined($remove) and "" ne "$remove") {
	File::copy("$remove","$tmpdir/control/remove")
		or quitMe(1,"Could not copy remove config file");
}
else {
	open  REMOVE,">$tmpdir/control/remove"
		or quitMe(1,"Could not create remove config file");
	print REMOVE "#!/bin/sh\n";
	print REMOVE "# Nothing to do.\n";
	close(REMOVE);
}

if (defined($runtime) and "" ne "$runtime") {
	File::copy("$remove","$tmpdir/control/runtime")
		or quitMe(1,"Could not copy runtime config file");
}
else {
	open  RUNTIME,">$tmpdir/control/runtime"
		or quitMe(1,"Could not create runtime config file");
	print RUNTIME "#!/bin/sh\n";
	my $bindir="%BASEDIR%";
	$bindir="%BASEDIR%/bin" if -d "$readyDir/bin";
	my $libdir="%BASEDIR%";
	$bindir="%BASEDIR%/lib" if -d "$readyDir/lib";
	print RUNTIME <<EOSCRIPT;
if [ -n "\$LD_LIBRARY_PATH" ]; then
	export LD_LIBRARY_PATH=$libdir:\$LD_LIBRARY_PATH
else
	export LD_LIBRARY_PATH=$libdir
fi
if [ -n "\$PATH" ]; then
	export PATH=$bindir:\$PATH
else
	export PATH=$bindir
fi
EOSCRIPT
	close(RUNTIME);
}

my $command="tar -C '$tmpdir' -czvf '$re' data control";
system($command) and quitMe(1,"An error occurred while executing '$command'\n");

if (defined($catalog) and "" ne "$catalog") {

	open CATALOG, ">$catalog" or quitMe(1,"Could not create catalog file at '$catalog'.");

	my $REid=$re;
	$REid=~ s/[. \t\n]//g; # delete some characters that seem unappropriate

	my $REname=basename($re);
	$REname = uc($REname);

	my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime time;
	$mon++;
	$year+=1900;

	if (!defined($category) or "" eq "$homepage") {
		$category = "APPS/OTHER";
	}

	if (!defined($homepage) or "" eq "$homepage") {
		$homepage = "http://www.nordugrid.org";
	}

	if (!defined($description) or "" eq "$description") {
		$description="no description available";
	}

	if (!defined($basesystem) or "" eq "$basesystem") {
		$basesystem="&kb;knowarc_Instance_0";
	}

	print CATALOG <<EOCATENTRY;
<?xml version='1.0' encoding='UTF-8'?>
<!DOCTYPE rdf:RDF [
         <!ENTITY rdf 'http://www.w3.org/1999/02/22-rdf-syntax-ns#'>
         <!ENTITY a 'http://protege.stanford.edu/system#'>
         <!ENTITY kb 'http://knowarc.eu/kb#'>
         <!ENTITY rdfs 'http://www.w3.org/2000/01/rdf-schema#'>
]>
<rdf:RDF xmlns:rdf="&rdf;"
         xmlns:a="&a;"
         xmlns:kb="&kb;"
         xmlns:rdfs="&rdfs;">

<kb:MetaPackage rdf:about="&kb;$REid"
         kb:homepage="$homepage"
         kb:lastupdated="$year-$mon-$mday"
         kb:name="$category/$REname"
         rdfs:label="$category/$REname">
        <kb:description>$description</kb:description>
        <kb:instance rdf:resource="&kb;${REid}_TarPackage"/>
</kb:MetaPackage>
<kb:TarPackage rdf:about="&kb;${REid}_TarPackage" kb:url="$re">
        <rdfs:label>Debian Etch (tar based): $REname</rdfs:label>
        <kb:basesystem rdf:resource="$basesystem"/>
</kb:TarPackage>
</rdf:RDF>

EOCATENTRY

	print STDERR "Preparation of dynamic Runtime Environment was successful.\n" if $verbose;
}
1;
