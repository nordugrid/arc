package Janitor::ArcConfig;

# set to one to enable debug output of some operation prior to the
###l4p # initialisation of the logger
my $DEBUG = 0;
######################################################################
###l4p # Loging in this modul is difficult, as the logger might not be
###l4p # ArcConfigured yet. For example if the place of the logger conffile is
# read from the ArcConfig.
#
###l4p # So if the logger is not initialized we just print to stderr.
######################################################################

use Exporter;
@ISA = qw(Exporter);     # Inherit from Exporter
@EXPORT_OK = qw(get_ArcConfig);

=head1 NAME

Janitor::ArcConfig - Provides access to the F<arc.conf>

=head1 SYNOPSIS

use Janitor::ArcConfig qw(get_ArcConfig);

=head1 DESCRIPTION

This class provides access to the F<arc.conf>. It is implemented as a
singleton.

To use this module the following code is needed:

   use Janitor::Catalog;
   my $ArcConfig = Janitor::ArcConfig->parse("/etc/arc.conf");

after this the ArcConfiguration values can be accessed. For example

   my $un = $ArcConfig->{'janitor'}{'uid'};

gives the value of the uid option within the [janitor]-section of
the F<arc.conf>.

This code can be used to get the singleton in another module:

   use Janitor::ArcConfig qw(get_ArcConfig);
   my $ArcConfig = get_ArcConfig();

=head1 METHODS

=over 4

=cut


use XML::Simple;

use warnings;
use strict;



# our singleton :-)
our $singleton = undef;

# time of the last conffile read
our $last = undef;

######################################################################
# This method is used to parse a conffile. we support multiple
# ArcConfiguration files. This is usefull for debugging.
######################################################################

=item Janitor::ArcConfig::parse($file)

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
# The function get and get_ArcConfig always return the singleton. If parse was not
# called yet this hash will be empty. So be carefull.
######################################################################

=item Janitor::ArcConfig::get_ArcConfig()

This function returns a reference to the singleton hash providing access to the
ArcConfiguration file.

=cut

sub get_ArcConfig {
	if (!defined $singleton) {
		my $msg = "Janitor::ArcConfig::get_ArcConfig: returning " .
			"reference to empty hash; call parse() to fill it";
			printf STDERR "DEBUG: %s\n", $msg if $DEBUG;

	}
	return Janitor::ArcConfig->get();
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
# ArcConfiguration file
######################################################################

=item Janitor::ArcConfig::changed($file)

This function returns a scalar which is true iff the given file was changend
more recently than the last read of the ArcConfiguration file.

=cut
sub changed {
	my ($file) = @_;

	my $mtime = (stat($file)) [9];

	return (!defined $last or $mtime > $last);
}

######################################################################
# cleans the current ArcConfiguration and reads it again from file
######################################################################

=item Janitor::ArcConfig::reinitialize($file)

This method reinitializes the ArcConfiguration hash, i.e. its content is replaced
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

sub _parse {
    my ($self, $file) = @_;
    if (isXML($file)) {
        $self->_parse_xml($file);
        my $inifile = $self->{gmconfig};
        $self->_parse_ini($inifile) if $inifile;
    } else {
        $self->_parse_ini($file);
    }
}

############# Generic functions for handling parsed XML ##############

# Check whether a file is XML
sub isXML {
    my $file = shift;
    unless (open CONFIGFILE, "<$file") {
        print STDERR "FATAL: can not open $file: $!\n";
        exit 1;
    }
    my $isxml = 0;
    while (my $line = <CONFIGFILE>) {
        chomp $line;
        next unless $line;
        if ($line =~ m/^\s*<\?xml/) {$isxml = 1; last};
        if ($line =~ m/^\s*<!--/)   {$isxml = 1; last};
        last;
    }
    close CONFIGFILE;
    return $isxml;
}

# walks a tree of hashes and arrays while applying a function to each hash.
sub hash_tree_apply {
    my ($ref, $func) = @_;
    if (not ref($ref)) {
        return;
    } elsif (ref($ref) eq 'ARRAY') {
        map {hash_tree_apply($_,$func)} @$ref;
        return;
    } elsif (ref($ref) eq 'HASH') {
        &$func($ref);
        map {hash_tree_apply($_,$func)} values %$ref;
        return;
    } else {
        return;
    }
}

# Strips namespace prefixes from the keys of the hash passed by reference
sub hash_strip_prefixes {
    my ($h) = @_;
    my %t;
    while (my ($k,$v) = each %$h) {
        next if $k =~ m/^xmlns/;
        $k =~ s/^\w+://;
        $t{$k} = $v;
    }
    %$h=%t;
    return;
}

######################################################################

# Parse the a-rex part of the XML config file into a hash
sub read_arex_xml {
    my ($file) = @_;
    my %xmlopts = (NSexpand => 0, ForceArray => 1, KeepRoot => 1, KeyAttr => {});
    my $xml;
    eval { $xml = XML::Simple->new(%xmlopts) };
    if ($@) {
        print STDERR "FATAL: $@\n";
        exit 1;
    }
    my $data;
    eval { $data = $xml->XMLin($file) };
    if ($@) {
        print STDERR "FATAL: $@\n";
        exit 1;
    }
    hash_tree_apply $data, \&hash_strip_prefixes;
    my $services;
    $services = $data->{Service}
        if  ref $data eq 'HASH'
        and ref $data->{Service} eq 'ARRAY';
    $services = $data->{ArcConfig}[0]{Chain}[0]{Service}
        if  not $services
        and ref $data eq 'HASH'
        and ref $data->{ArcConfig} eq 'ARRAY'
        and ref $data->{ArcConfig}[0] eq 'HASH'
        and ref $data->{ArcConfig}[0]{Chain} eq 'ARRAY'
        and ref $data->{ArcConfig}[0]{Chain}[0] eq 'HASH'
        and ref $data->{ArcConfig}[0]{Chain}[0]{Service} eq 'ARRAY';
    return undef unless $services;
    for my $srv (@$services) {
        next unless ref $srv eq 'HASH';
        return $srv if $srv->{name} eq 'a-rex';
    }

    printf STDERR "FATAL: A-REX config not found in $file\n";
    exit 1;
}

######################################################################

sub _parse_xml {
    my ($self,$file) = @_;
    my $arex = read_arex_xml($file);

    # Check for the reference to the INI file
    if (ref $arex->{'gmconfig'} eq 'ARRAY') {
        my $gmconfig = $arex->{'gmconfig'}[0];
        if (not ref $gmconfig) {
            $self->{'gmconfig'} = $gmconfig;
        } elsif (ref $gmconfig eq 'HASH') {
            $self->{'gmconfig'} = $gmconfig->{'content'}
                if $gmconfig->{'content'} and $gmconfig->{'type'}
                                          and $gmconfig->{'type'} eq 'INI';
        }
    }

    sub get_scalar {
        my ($h, $key) = @_;
        return '' unless ref $h->{$key} eq 'ARRAY';
        return join '[separator]', grep {not ref $_} @{$h->{$key}};
    }

    my $lrms = $arex->{'LRMS'};
    if (ref $lrms eq 'ARRAY' and @$lrms and ref $lrms->[0] eq 'HASH') {
        my $dir = get_scalar($lrms->[0], 'runtimeDir');
        $self->{'grid-manager'}{'runtimedir'} = $dir if $dir;
    }

    my $jani = $arex->{'janitor'};
    if (ref $jani eq 'ARRAY' and @$jani and ref $jani->[0] eq 'HASH') {
        $self->{'janitor'}{$_} = get_scalar($jani->[0], $_)
            for grep {$_ ne 'catalog'} keys %{$jani->[0]};

        my $catalogs = $jani->[0]{'catalog'};
        if (ref $catalogs eq 'ARRAY') {
            for my $cat (@$catalogs) {
                next unless ref $cat eq 'HASH';
                my $name = $cat->{'name'};
                my $loc = $cat->{'location'};
                delete $cat->{'name'};
                delete $cat->{'location'};
                next unless $name and not ref $name;
                next unless ref $loc eq 'ARRAY' and @$loc and not ref $loc->[-1];
                $self->{"janitor/$name"}{'catalog'} = $loc->[-1];
                $self->{"janitor/$name"}{$_} = get_scalar($cat, $_) for keys %$cat;
            }
        }
    }
}

######################################################################
# This function was stolen from the nordugrid::Shared
# parse the ArcConfig file (applied to arc.conf)
######################################################################

sub _parse_ini {
	my ($self,$conf_file) = @_;
	my ( $variable_name, $variable_value);

	# Parse the arc.conf directly into a hash
	# $self->{blockname}{variable_name}

	unless ( open (ArcConfigFILE, "<$conf_file") ) {
		my $msg = "can not open $conf_file: $!";
		printf STDERR "FATAL: %s\n", $msg;
		exit 1;
	}

	my $blockname;
	my $c = 0;
	while (my $line =<ArcConfigFILE>) {
		$c++;
		next if $line =~/^#/;
		next if $line =~/^$/;
		next if $line =~/^\s+$/;

		if ($line =~/^\s*\[(.+)\]\s*$/ ) {
			$blockname = $1;
			next;}

		unless ($line =~ /=\s*".*"\s*$/) {
			my $msg =  "skipping incorrect $conf_file line ($c): $line";
			printf STDERR "WARNING: %s\n", $msg;
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
	close ArcConfigFILE;
}

=back

=name1 SEE ALSO

/etc/arc.conf

=cut

1;
