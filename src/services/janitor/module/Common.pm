package Janitor::Common;

BEGIN { 
	if (defined(Janitor::Janitor::resurrect)) { 
		eval 'require Log::Log4perl';
		unless ($@) { 
			import Log::Log4perl qw(:resurrect get_logger);
		}
	}
}
###l4p my $logger = get_logger("Janitor::Common");


use Exporter;
@ISA = qw(Exporter);     # Inherit from Exporter
@EXPORT_OK = qw(get_catalog);


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



use warnings;
use strict;
 





use Janitor::ArcConfig qw(get_ArcConfig);
my $config = get_ArcConfig();

#use Janitor::ArcConfig;
#my $conffile = $ENV{'NORDUGRID_CONFIG'};
#$conffile = "/etc/arc.conf" unless defined $conffile;
#my $config = Janitor::ArcConfig->parse($conffile);

######################################################################
######################################################################



=item janitor_enabled()

Checks, if the parameter in the ArcConfig file says, that Janitor 
is enabled.

=cut

sub janitor_enabled
{
	my $enabled = $config->{"janitor"}{"enabled"};
	if (!defined $enabled){
		return 0;
	} 
	return $enabled;
}


=item get_catalog()

Initializes the catalog. All configured sources are added to the catalog and
the Janitor::Catalog object is returned.

=cut

sub get_catalog
{
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
		# Check if catalog is URL
		if($catalog_file =~ /^[[:alpha:]]*:/) {
			# Looks like URL.
			# Check if we have cache
			my $catalog_cache = $config->{"janitor/$catalog_name"}{"cache"};
			my $catalog_refresh = $config->{"janitor/$catalog_name"}{"refresh"};
			my $cachefile;
			my $need_remove = 0;
			unless($catalog_refresh =~ /^[0-9]+$/) {
				$catalog_refresh = "24*60*60";
			}
			if($catalog_cache) {
				# Find file in cache
				$cachefile = $catalog_file;
				$cachefile =~ s!/!*2F!g;
				$cachefile = "$catalog_cache/$cachefile";
				# Check if file exists and how old it is
				my $need_download = 1;
				my $cachefiletime = (stat($cachefile)) [9];
				if($cachefiletime) {
					my $current_time = time();
					if(($current_time - $cachefiletime) < $catalog_refresh) {
						# 24h refresh time
						$need_download= 0;
					}
				}
				if($need_download) {
					# download to temp file
					my $ff = new Janitor::Filefetcher($catalog_file,$catalog_cache);
					if ($ff->fetch()) {
						# Replace old one
						utime time(), time(), $ff->getFile();
						unless(rename $ff->getFile(), $cachefile) {
							my $msg = "Failed to replace catalog $catalog_name from $catalog_file - using temporary new";
###l4p							$logger->error($msg);
							$cachefile = $ff->getFile();
							$need_remove = 1;
						}
					} else {
						if($cachefiletime) {
							my $msg = "Failed to fetch catalog $catalog_name from $catalog_file - using old";
###l4p							$logger->error($msg);
						} else {
							my $msg = "Failed to fetch catalog $catalog_name from $catalog_file to cache";
###l4p							$logger->error($msg);
							# die "$msg";
							# there is still chance we will be able to dowmload catalog to another place
							$cachefile = "";
						}
					}
				}
			}
			if($cachefile) {
				$catalog->add($cachefile);
				if($need_remove) {
					unlink($cachefile);
				}
			} else {
				# Fetch URL to temporary file and then delete it
				my $ff = new Janitor::Filefetcher($catalog_file,$config->{'janitor'}{'downloaddir'});
				unless ($ff->fetch()) {
					my $msg = "Failed to fetch catalog $catalog_name from $catalog_file";
###l4p					$logger->fatal($msg);
					die "$msg";
				}
				my $catalog_tmp_file = $ff->getFile();
				$catalog->add($catalog_tmp_file);
				unlink($catalog_tmp_file);
			}
		} else {
			# Catalog is not URL
			$catalog->add($catalog_file);
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

