package Janitor::Config;

=head1 NAME

Janitor::Config - Provides access to the F<arc.conf>

=head1 SYNOPSIS

use Janitor::Config qw(get_config);

=head1 DESCRIPTION

This class provides access to the F<arc.conf>. It is implemented as a
singleton.

To use this module the following code is needed:

   use Janitor::Catalog;
   my $config = Janitor::Config->parse("/etc/arc.conf");

after this the configuration values can be accessed. For example

   my $un = $config->{'janitor'}{'uid'};

gives the value of the uid option within the [janitor]-section of
the F<arc.conf>.

This code can be used to get the singleton in another module:

   use Janitor::Config qw(get_config);
   my $config = get_config();

=head1 METHODS

=over 4

=cut

use Exporter;
@ISA = qw(Exporter);     # Inherit from Exporter
@EXPORT_OK = qw(get_config);

use warnings;
use strict;

# set to one to enable debug output of some operation prior to the
###l4p # initialisation of the logger
my $DEBUG = 0;
######################################################################
###l4p # Loging in this modul is difficult, as the logger might not be
###l4p # configured yet. For example if the place of the logger conffile is
# read from the config.
#
###l4p # So if the logger is not initialized we just print to stderr.
######################################################################
BEGIN {
	if (defined($main::resurrect)) { 
		eval 'require Log::Log4perl';
		unless ($@) { 
			import Log::Log4perl qw(:resurrect get_logger); 
		}
	}
}
###l4p my $logger = get_logger("Janitor::Config");


# our singleton :-)
our $singleton = undef;

# time of the last conffile read
our $last = undef;

######################################################################
# This method is used to parse a conffile. we support multiple
# configuration files. This is usefull for debugging.
######################################################################

=item Janitor::Config::parse($file)

This method parses the given file and adds its content to the singleton.
For debugging purposes this method might be called multiple times for different
files. Please note this feature is not tested well enough for production use.

=cut

sub parse {
	my ($class, $file) = @_;

	get($class);

	$last = time;
	$singleton->_parse($file);

	return $singleton;
}

######################################################################
# The function get and get_config always return the singleton. If parse was not
# called yet this hash will be empty. So be carefull.
######################################################################

=item Janitor::Config::get_config()

This function returns a reference to the singleton hash providing access to the
configuration file.

=cut

sub get_config {
	if (!defined $singleton) {
		my $msg = "Janitor::Config::get_config: returning " .
			"reference to empty hash; call parse() to fill it";
###l4p		if (Log::Log4perl::initialized) {
###l4p 			$logger->debug($msg);
###l4p		} else {
###l4p			printf STDERR "DEBUG: %s\n", $msg if $DEBUG;
###l4p		}
	}
	return Janitor::Config->get();
}
sub get {
	my ($class) = @_;
	unless (defined $singleton) {
		$singleton = bless {}, $class;
	}
	return $singleton;	
}

######################################################################
# Checks if $file was changed more recently then the last read of the
# configuration file
######################################################################

=item Janitor::Config::changed($file)

This function returns a scalar which is true iff the given file was changend
more recently than the last read of the configuration file.

=cut
sub changed {
	my ($file) = @_;

	my $mtime = (stat($file)) [9];

	return (!defined $last or $mtime > $last);
}

######################################################################
# cleans the current configuration and reads it again from file
######################################################################

=item Janitor::Config::reinitialize($file)

This method reinitializes the configuration hash, i.e. its content is replaced
with the content of $file.

=cut

sub reinitialize {
	my ($class, $file) = @_;

	get($class);

	%$singleton = ();
	$last = time;
	$singleton->_parse($file);

	return $singleton;
}
	
######################################################################
# This function was stolen from the nordugrid::Shared
# parse the config file (applied to arc.conf)
######################################################################
sub _parse {
	my ($self,$conf_file) = @_;
	my ( $variable_name, $variable_value);

	# Parse the arc.conf directly into a hash
	# $self->{blockname}{variable_name}

	unless ( open (CONFIGFILE, "<$conf_file") ) {
		my $msg = "can not open $conf_file: $!";
###l4p		if (Log::Log4perl::initialized) {
###l4p 			$logger->fatal($msg);
###l4p		} else {
			printf STDERR "FATAL: %s\n", $msg;
###l4p		}
		exit 1;
	}

	my $blockname;
	my $c = 0;
	while (my $line =<CONFIGFILE>) {
		$c++;
		next if $line =~/^#/;
		next if $line =~/^$/;
		next if $line =~/^\s+$/;

		if ($line =~/^\s*\[(.+)\]\s*$/ ) {
			$blockname = $1;
			next;}

		unless ($line =~ /=\s*".*"\s*$/) {
			my $msg =  "skipping incorrect arc.conf line ($c): $line";
###l4p			if (Log::Log4perl::initialized) {
###l4p 				$logger->warn($msg);
###l4p			} else {
				printf STDERR "WARNING: %s\n", $msg;
###l4p			}
			next;
		}

		$line =~m/^(\w+)\s*=\s*"(.*)"\s*$/;
		$variable_name=$1;
		$variable_value=$2;
		unless ($self->{$blockname}{$variable_name}) {
			$self->{$blockname}{$variable_name} = $variable_value;}
		else {
			$self->{$blockname}{$variable_name} .= "[separator]".$variable_value;
		}
	}
	close CONFIGFILE;
}

=back

=name1 SEE ALSO

/etc/arc.conf

=cut

1;
