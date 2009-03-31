package Janitor::Common;

=head1 NAME

Janitor::Common - Provides some common functions

=head1 SYNOPSIS

use Janitor::Common;

=head1 DESCRIPTION

This module contains the function get_catalog() which is not only used by the
Janitor but also by the module RuntimeEnvironments.

=head1 FUNCTIONS

=over 4

=cut

use Exporter;
@ISA = qw(Exporter);     # Inherit from Exporter
@EXPORT_OK = qw(get_catalog refresh_catalog);

use warnings;
use strict;

use POSIX qw(EAGAIN WNOHANG);

BEGIN {
	if (defined($main::resurrect)) { 
		eval 'require Log::Log4perl';
		unless ($@) { 
			import Log::Log4perl qw(:resurrect get_logger); 
		}
	}
}
###l4p my $logger = get_logger("Janitor::Common");

use Janitor::Config qw(get_config);
my $config = get_config();

######################################################################
######################################################################

=item get_catalog()

Initializes the catalog. All configured sources are added to the catalog and
the Janitor::Catalog object is returned.

=cut

sub get_catalog
{
	if (defined $main::GLOBAL_CATALOG) {
		return $main::GLOBAL_CATALOG;
	}

	my $catalog = new Janitor::Catalog;
	unless ( defined($catalog) ) {
###l4p 		$logger->fatal("Could not create Janitor::Catalog object");
		die "Could not create Janitor::Catalog object";
	}

	foreach my $section ( keys %$config ) {
		next unless $section =~ m#^janitor/(.+)$#;

		my ($catalog_name) = $1;

###l4p 	$logger->debug("reading Catalog $catalog_name");

		# get the catalog file
		my $catalog_file = $config->{"janitor/$catalog_name"}{"catalog"};
		if ( !defined $catalog_file ) {
###l4p			$logger->fatal("Catalog $catalog_name has no catalog property");
			die "Catalog $catalog_name has no catalog property";
		}
	
		if (!defined $catalog->add($catalog_file)) {
###l4p			$logger->fatal("Could not add catalog $catalog_name");
			die "Could not add catalog $catalog_name ($catalog_file)";
		}
	}

	# get the allow and deny rules for basesystems 
	my $allow_base = $config->{"janitor"}{"allow_base"};
	my $deny_base = $config->{"janitor"}{"deny_base"};

	# get the allow and deny rules for meta packages
	my $allow_rte = $config->{"janitor"}{"allow_rte"};
	my $deny_rte = $config->{"janitor"}{"deny_rte"};

	if (!defined $allow_base or !defined $allow_rte) {
###l4p		$logger->warn("I'm not allowed to use RTEs from the Catalogs. Configuration error?");
		# This is odd but not an error!	
		return $catalog;
	}

	# process basesystems
	foreach my $b ( split /\[separator\]/, $allow_base ) {
###l4p 		$logger->debug("CONFIG - allowing basesytems matching to $b");
		$b = glob2regex($b);
		$catalog->AllowBaseSystem($b);
	}	
	if (defined $deny_base) {
		foreach my $b ( split /\[separator\]/, $deny_base ) {
###l4p 			$logger->debug("CONFIG - denying basesytems matching to $b");
			$b = glob2regex($b);
			$catalog->DenyBaseSystem($b);
		}	
	}

	# process metapackages
	foreach my $b ( split /\[separator\]/, $allow_rte ) {
###l4p 		$logger->debug("CONFIG - allowing meta packages  matching to $b");
		$b = glob2regex($b);
		$catalog->AllowMetaPackage($b);
	}	
	if (defined $deny_rte) {
		foreach my $b ( split /\[separator\]/, $deny_rte ) {
###l4p 			$logger->debug("CONFIG - denying meta packages matching to $b");
			$b = glob2regex($b);
			$catalog->DenyMetaPackage($b);
		}	
	}

	return $catalog;
}

=item refresh_catalog()

=cut

use Janitor::Filefetcher;
use Digest::MD5;

sub refresh_catalog
{
	my $changes = 0;

	# process the config file
	foreach my $section ( keys %$config ) {
		next unless $section =~ m#^janitor/(.+)$#;
		my $catalog_name = $1;

###l4p 		$logger->debug("checking  Catalog $catalog_name");

		my $catalog_file = $config->{"janitor/$catalog_name"}{"catalog"};
		next unless defined $catalog_file;

		my $catalog_source = $config->{"janitor/$catalog_name"}{"source"};
		next unless defined $catalog_source;

		next unless $catalog_file =~ m#^(.*)/[^/]+$#;
		my $destdir = $1;

		my $ff = new Janitor::Filefetcher($catalog_source,$destdir);

		unless ($ff->fetch) {
###l4p			$logger->error("Can not download catalogue $catalog_name from $catalog_source");
			next;
		}

		if (! -f $catalog_file or md5($catalog_file) ne md5($ff->getFile)) {
###l4p			$logger->info("got new version of Catalog $catalog_name");
			unlink $catalog_file;
			rename $ff->getFile, $catalog_file;
			$changes++;
		} else {
			unlink $ff->getFile;
		}
	}

	return $changes;
}

sub md5 {
	my ($file) = shift;
	open (F, $file);
	binmode F;
	my $ret = Digest::MD5->new->addfile(*F)->hexdigest;
	close F;
	return $ret;
}


######################################################################
# converts a glob to a regex; private, so no pod
######################################################################

sub glob2regex
{
	# quote any char relevant for regexps (besides *)
	# issues with [ ] characters, but they aren't allowed by Janitor::Config
	# anyway
	$_ = $_[0];
	s/([\.\\\/\?\(\)\{\}\[\]\^\$])/\[$1\]/g;
	s/\*/\.\*/g;
	return $_;
}


1;

=back

=name1 SEE ALSO

/etc/arc.conf

=cut

