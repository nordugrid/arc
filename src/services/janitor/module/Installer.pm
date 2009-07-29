package Janitor::Installer;

BEGIN {
	if (defined(Janitor::Janitor::resurrect)) { 
		eval 'require Log::Log4perl';
		unless ($@) { 
			import Log::Log4perl qw(:resurrect get_logger);
		}
	}
}


use Exporter;
@ISA = qw(Exporter);	# Inherit from Exporter
@EXPORT_OK = qw();


=head1 NAME

Janitor::Installer - Installs a given package

=head1 SYNOPSIS

use Janitor::Installer;

=head1 DESCRIPTION

This class is used to install a given package into a directory.

=head1 METHODS

=over 4

=cut
use warnings;
use strict;



use File::Temp qw(tempdir);
use Janitor::Execute qw(new);
use Janitor::ArcConfig qw(get_ArcConfig);

#use diagnostics;

use Janitor::Util qw(remove_directory);

###l4p my $logger = get_logger("Janitor::Installer");
my $config = get_ArcConfig();

=item Janitor::Installer::new($instdir)

The consturctor for this class. The argument is the name of the directory in
which the packages will be installed.

=cut

sub new {
	my ($class, $instdir) = @_;
	my $self = {
		_instdir => $instdir,
	};
	
	bless $self, $class;
	return $self;
}

=item $obj->install($file)

This method installes the package $file. Currently it supports tar packages,
which might be compressed with gzip or bzip2.

The installation process includes the unpacking, the execution of the install
script and the preparation and storage of the runtime script. It does not
include the registration of this new package within the registration directory.

=cut

sub install {
	my ($self,$url) = @_;

###l4p 	$logger->info("Installing $url");

	#get the first characters of the package name for tempdir creation
	$url =~ m#/([^/]{4})[^/]*$#;
	my $template = $1;

	#create directories
	my $installdir = tempdir( $template."_XXXXXXXXXX", DIR => $self->{_instdir});	
###l4p 	$logger->debug("Tempdir $installdir");

	#the directory is created with permissions 0700, change this to 0755
	chmod 0755, $installdir;

###l4p 	$logger->debug($self->{_instdir});

	my $tmpdir = "$installdir/temp";
	unless ( mkdir $tmpdir and mkdir($installdir."/pkg") ) {
###l4p 		$logger->error("Can not create directories in $installdir: $!");
		remove_directory($installdir);	# cleanup
		return (0,undef);
	}

	#determine compression type
	my $comptype = "";
	if ( $url =~ /\.tar\.gz$/ or $url =~ /\.tgz$/) { $comptype= "--gzip"; }
	if ( $url =~ /\.tar\.bz2$/ or $url =~ /\.tbz$/) { $comptype= "--bzip2"	}	

	#untar package
	unless ( $self->untar($url,$tmpdir,$comptype) ) {
###l4p 		$logger->error("Failed to extract from $url");
		remove_directory($installdir);	# cleanup
		return (0,undef);	
	} 

	#move data
	unless ( &_movecontent("$tmpdir/data","$installdir/pkg") ) {
###l4p 		$logger->error("Could not move data in $tmpdir");
		remove_directory($installdir);	# cleanup
		return (0,undef);
	}

	my $errorflag = 0;

	#execute the installskript
	my $e = new Janitor::Execute($installdir."/pkg",$installdir."/install.log");
	if ( ! $e->execute("/bin/sh","$tmpdir/control/install") ) {
###l4p 		$logger->error("Error while executing /bin/sh $tmpdir/control/install");
		$errorflag=1;
	}

	# process the runtime script	
	elsif ( ! &_process_runtime_script($tmpdir,$installdir) ) {
###l4p 		$logger->error("Error while processing runtime script");
		$errorflag=1;
	}

	# move the remove script
	elsif ( ! rename ("$tmpdir/control/remove", "$installdir/remove") ) {
###l4p 		$logger->error("Can not rename $tmpdir/control/remove to $installdir/remove: $!");
		$errorflag=1;
	}

	# and finaly remove the temporary directory
	elsif ( ! remove_directory($tmpdir) ) {
###l4p 		$logger->error("Could not remove $tmpdir. $!");
		$errorflag=1;
	}



###l4p	if ($errorflag) {
###l4p 		$logger->error("Some error occured. Keeping installation"
###l4p			. " directory \"$installdir\" for investigation");
###l4p 		$logger->error("Clean up yourself.");
###l4p	} else {
###l4p 		$logger->info("Successfully installed \"$url\" in \"$installdir\"");
###l4p	}

	return (($errorflag==0), $installdir);
}

######################################################################
######################################################################

=item _process_runtime_script($installdir) - intern

Move the runtime script to $installdir and set up the %BASEDIR% 
variable 

=cut

sub _process_runtime_script {
	my ($tmpdir, $installdir) = @_;

	unless ( open RUNTIME_IN, "< $tmpdir/control/runtime" ) {
###l4p 		$logger->error("Could not read $tmpdir/control/runtime. $!");
		return 0;
	}
	my @runtime = <RUNTIME_IN>;
	close RUNTIME_IN;
	
	for (@runtime) {
		s#\%BASEDIR%#$installdir/pkg#g;
	}

	my $pkgdir = $installdir."/runtime";
	unless ( open RUNTIME_OUT, "> $pkgdir" ) {
###l4p 		$logger->error("Could not write $pkgdir. $!");
		return 0;
	}
	print RUNTIME_OUT @runtime;
	unless (close RUNTIME_OUT) {
###l4p 		$logger->error("Error while closing runtime file: $!");
		return 0;
	}

	return 1;
}

######################################################################
######################################################################

=item _movecontent($src, $dst) - intern

Move everything from the $tmpdir/data directory to $installdir.
Returns false if something went wrong

=cut

sub _movecontent {
	my ($src, $dst) = @_;

	unless (opendir(TEMPDIR, "$src")) {
###l4p 		$logger->error("Can't open directory \"$src\": $!");
		return 0;
	}

	my $errorflag = 0;
	while (my $name = readdir(TEMPDIR)) {
		next if $name eq ".";
		next if $name eq "..";

		unless (rename ("$src/$name", "$dst/$name")) {
###l4p 			$logger->error("renaming $src/$name to $dst/$name failed: $!");
			$errorflag = 1;
		}
	}	
	closedir(TEMPDIR);

	return ($errorflag==0);
}

######################################################################
######################################################################

=item untar($url, $tmpdir, $compType)

Extract the tar ball. $compType is depending on the compression
method ('--gzip' | '--bzip2')

=cut

sub untar {
	my ($self, $url, $tmpdir, $compType) = @_;

	my $errorflag = 0;


	my $e = new Janitor::Execute($tmpdir);
	
	unless ( $e->execute("/bin/tar","xf",$url,$compType) ) {
###l4p 		$logger->error("Execution of tar xf $url $compType failed.");
		$errorflag=1;
	}

	return ($errorflag==0);
}

=back

=cut


1;

