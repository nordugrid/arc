#!/usr/bin/perl

use File::Basename;
use lib dirname($0);

use Getopt::Long;
use XML::Simple;
use Data::Dumper;

########################################################
# Driver for classic Nordugrid  information collection
# Reads old style arc.config and prints out XML
########################################################

use ARC0ClusterInfo;
use ConfigParser;
use LogUtils; 

use strict;

our $log = LogUtils->getLogger(__PACKAGE__);

########################################################
# Config defaults
########################################################

my %config = (
               ttl            =>  600,
               gm_mount_point => "/jobs",
               gm_port        => 2811,
               homogeneity    => "TRUE",
               providerlog    => "/var/log/grid/infoprovider.log",
               defaultttl     => "604800",
               x509_user_cert => "/etc/grid-security/hostcert.pem",
               x509_cert_dir  => "/etc/grid-security/certificates/",
               ng_location    => $ENV{ARC_LOCATION} ||= "/usr/local",
               gridmap        => "/etc/grid-security/grid-mapfile",
               processes      => [qw(gridftpd grid-manager arched)]

);

########################################################
# Parse command line options
########################################################

my $configfile;
my $print_help;
GetOptions("config:s" => \$configfile,
           "help|h"   => \$print_help ); 
    
if ($print_help) { 
    print "
                script usage: 
                                   --config
                this help:         --help
    ";
    exit 1;
}

unless ( $configfile ) {
    $log->error("a command line argument is missing, see --help ") and die;
}

##################################################
# Read ARC configuration
##################################################

my $parser = ConfigParser->new($configfile)
    or $log->error("Cannot open $configfile") and die;
%config = (%config, $parser->get_section('common'));
%config = (%config, $parser->get_section('cluster'));
%config = (%config, $parser->get_section('grid-manager'));
%config = (%config, $parser->get_section('infosys'));

$config{queues} = {};
for my $qname ($parser->list_subsections('queue')) {
    my %queue_config = $parser->get_section("queue/$qname");
    $config{queues}{$qname} = \%queue_config;

    # put this option in the proper place
    if ($queue_config{maui_bin_path}) {
        $config{maui_bin_path} = $queue_config{maui_bin_path};
        delete $queue_config{maui_bin_path};
    }
    # maui should not be a per-queue option
    if (lc($queue_config{scheduling_policy}) eq 'maui') {
        $config{pbs_scheduler} = 'maui';
    }
}

# throw away default queue string following lrms
($config{lrms}) = split " ", $config{lrms};


##################################################
# Collect information & print XML
##################################################

$log->level($LogUtils::DEBUG);
$log->debug("From config file:\n". Dumper \%config);
my $cluster_info = ARC0ClusterInfo->new()->get_info(\%config);

my $xml = new XML::Simple(NoAttr => 0, ForceArray => 1, RootName => 'n:nordugrid', KeyAttr => ['name']);
print $xml->XMLout($cluster_info);

