package Janitor::Filefetcher;


use warnings;
use strict;

=head1 NAME

Janitor::Filefetcher - Downloads files for the Janitor.

=head1 SYNOPSIS

use Janitor::Filefetcher;

=head1 DESCRIPTIONS

This class is used for downloading files. Currently it supports  the protocols
http, https and ftp.

=head1 METHODS

=over 4

=cut

use Janitor::Execute;
use File::Temp qw(tempfile);

BEGIN {
	if (defined($main::resurrect)) { 
		eval 'require Log::Log4perl';
		unless ($@) { 
			import Log::Log4perl qw(:resurrect get_logger); 
		}
	}
}
###l4p my $logger = get_logger("Janitor::Filefetcher");

=item Janitor::Filefetcher::new($url, $destination)

The constructor creates an object which will be used to download $url and store
this file in the directory $destination.

=cut 

sub new {
	my ($class, $url, $destination) = @_;

	my $self = {
		_url => $url,
		_destination => $destination,
		_file => undef,
	};
	return bless $self, $class;
}

=item $obj->fetch()

This method is used to actually download. It returns true iff the file was
downloaded successfully. The name of the file will have some random bits. 
The getFile-method can be used to retrieve the actual name.

=cut

sub fetch() { 

	#  Warnings are turned of. Otherwise tempfile will output:
	#    tempfile(): temporary filename requested but not opened.
	#    Possibly unsafe, consider using tempfile() with OPEN set to true
	no warnings;

	my ($self) = @_;

	if (! (-w $self->{_destination} and -d $self->{_destination} ) ) {
###l4p 		$logger->fatal("Cannot access destination directory ".$self->{_destination});
		return 0;
	}
	
	my $url = $self->{_url};

	# Maybe there are site admins around who think it is a good idea to use
	# a world writeable directory as tempdir. So put some unpredictable bits
	# in the filename. Remember: this will be executed as root!
	my $suffix = "";
	$suffix = "_$1" if $url =~ m#.*/[^.]*(\..*)$#;
	my $praefix = "";
	$praefix = $1 if $url =~ m#.*/([^/.]*)\.*[^/]*$#;
	$praefix .= "_XXXXXXXX";  
	(undef, my $file) = tempfile( $praefix, DIR => $self->{_destination},  OPEN => 0, SUFFIX => $suffix );
	if ($url =~ /^(http[s]?)|(ftp):\/\//) {
		my $e = new Janitor::Execute( $self->{_destination} );
		if ( ! $e->execute("/usr/bin/wget",$url,"-O$file") ) {
###l4p			$logger->error("Error while executing 'wget $url -O$file'.");
			return 0;
		}
	}

	$self->{_file} = $file;

	return 1;
}

=item $obj->getFile()

This returns the name of the downloaded file.

=back

=cut

sub getFile {
	return shift->{_file};
}

1;
