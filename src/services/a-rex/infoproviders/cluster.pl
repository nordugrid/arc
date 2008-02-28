#!/usr/bin/perl

use File::Basename;
use lib dirname($0);
use Getopt::Long;
use LogUtils ( 'start_logging', 'error', 'warning', 'debug' ); 
use LRMS ('select_lrms', 'cluster_info');
use Shared ( 'print_ldif_data', 'post_process_config');

use strict;

# Constructs clusterldif for NorduGrid ARC Information System
#

my ($totaltime) = time;

####################################################################
# Full definitions of the infosys attributes are documented in
# "The NorduGrid/ARC Information System", 2005-05-09.
####################################################################

####################################################################
# The "output" or interface towards ARC Information system is
# defined by elements in @cen array. The interface towards LRMS is
# defined in LRMS.pm 
####################################################################

my (@cen) = ( 'dn',
	      'objectClass',
	      'nordugrid-cluster-name',
	      'nordugrid-cluster-aliasname',
	      'nordugrid-cluster-comment',
	      'nordugrid-cluster-owner',
	      'nordugrid-cluster-acl',
	      'nordugrid-cluster-location',
	      'nordugrid-cluster-issuerca',
	      'nordugrid-cluster-issuerca-hash',
	      'nordugrid-cluster-trustedca',
	      'nordugrid-cluster-contactstring',
	      'nordugrid-cluster-interactive-contactstring',
	      'nordugrid-cluster-support',
	      'nordugrid-cluster-lrms-type',
	      'nordugrid-cluster-lrms-version',
	      'nordugrid-cluster-lrms-config',
	      'nordugrid-cluster-architecture',
	      'nordugrid-cluster-opsys',
	      'nordugrid-cluster-benchmark',
	      'nordugrid-cluster-homogeneity',
	      'nordugrid-cluster-nodecpu',
	      'nordugrid-cluster-nodememory',
	      'nordugrid-cluster-nodeaccess',
	      'nordugrid-cluster-totalcpus',
	      'nordugrid-cluster-usedcpus',
	      'nordugrid-cluster-cpudistribution',	     
	      'nordugrid-cluster-prelrmsqueued',
	      'nordugrid-cluster-totaljobs',
	      'nordugrid-cluster-localse',
	      'nordugrid-cluster-sessiondir-free',
	      'nordugrid-cluster-sessiondir-total',
	      'nordugrid-cluster-sessiondir-lifetime',
	      'nordugrid-cluster-cache-free',
	      'nordugrid-cluster-cache-total',
	      'nordugrid-cluster-middleware',
	      'nordugrid-cluster-runtimeenvironment',
	      'Mds-validfrom',
	      'Mds-validto');

########################################################
# Package variables
########################################################

my (%config, %gm_info, @runtimeenvironment );

########################################################
# Config defaults
########################################################

$config{ttl}                =  600;
$config{gm_mount_point}     = "/jobs";
$config{gm_port}            = 2811;
$config{homogeneity}        = "TRUE";
$config{providerlog}        = "/var/log/grid/infoprovider.log";
$config{defaultttl}         = "604800";
$config{"x509_user_cert"}   = "/etc/grid-security/hostcert.pem";
$config{"x509_cert_dir"}    = "/etc/grid-security/certificates/";
$config{ng_location}        = $ENV{ARC_LOCATION} ||= "/opt/arc";
$config{gridmap} 	    = "/etc/grid-security/grid-mapfile";

########################################################
# Parse command line options
########################################################

my ($print_help);
GetOptions("dn:s" => \$config{dn},
	   "valid-to:i" => \$config{ttl}, 
	   "config:s" => \$config{conf_file},
	   "loglevel:i" => \$config{loglevel},
	   "help|h" => \$print_help   
	   ); 
    
if ($print_help) { 
    print "\n  
     		script usage: 
		mandatory arguments: --dn
				     --config
				     
		optional arguments:  --valid-to 
		
		this help:	     --help				  		
		\n";
    exit;
}

if (! ( $config{dn} and $config{conf_file} ) ) {
    error("a command line argument is missing, see --help ");
}

##################################################
# Read ARC configuration
##################################################

# Whole content of the config file

my (%parsedconfig) = Shared::arc_conf( $config{conf_file} );

# copy blocks that are relevant to %config

my (@blocks) = ('common', 'grid-manager', 'infosys', 'gridftpd', 'cluster');
foreach my $block ( @blocks ) {
    foreach my $var ( keys %{$parsedconfig{$block}} ) {
	$config{$var} = $parsedconfig{$block}{$var};
    }
}

if (! exists $config{hostname}){
   chomp ($config{hostname} = `/bin/hostname -f`);
}

post_process_config(\%config);

############################################################

start_logging( $config{provider_loglevel}, $config{providerlog} );

############################################################

############################################################
# Lot's of stuff that just need to be figured out
# Scripting, scripting, scripting, oh hay, oh hay...
############################################################

# Globus location
my ($globus_location) ||= $ENV{GLOBUS_LOCATION} ||= "/opt/globus/";
$ENV{"LD_LIBRARY_PATH"}="$globus_location/lib";

# Hostname
#my ($hostname) = `/bin/hostname -f`;
#chomp $hostname;
my ($hostname) = $config{hostname};
# Hostcert issuer CA, trustedca, issuercahash
# find an openssl							          
my $openssl_command = `which openssl 2>/dev/null`;
chomp $openssl_command;	
if ($? != 0) {
   $openssl_command = "$globus_location/bin/openssl";
}
my $issuerca=`$openssl_command x509 -noout -issuer -in $config{x509_user_cert} 2>/dev/null`;    
if ( $? ) {
    error("error in executing $openssl_command x509 -noout -issuer -in $config{x509_user_cert}");}
if ( $issuerca =~ s/issuer= // ) {
    chomp($issuerca);}
my @certfiles = `ls $config{"x509_cert_dir"}/*.0 2>/dev/null`;
my $issuerca_hash;
my @trustedca;
foreach my $cert ( @certfiles ) {
   my $ca_sn = `$openssl_command x509 -checkend 60 -noout -subject -in $cert`;
   $ca_sn =~ s/subject= //;
   chomp($ca_sn);
   if ( $ca_sn eq $issuerca) {
      $issuerca_hash = `$openssl_command x509 -checkend 60 -noout -hash -in $cert`;
      chomp ($issuerca_hash);
   }
   if ($?) {
     $config{loglevel} and warning("CA $ca_sn is expired");
     next;
   }
   push @trustedca, $ca_sn;
}

#nordugrid-cluster-sessiondirusage
my $session_diskfree  = 0;
my $session_disktotal = 0;
if ( -d "$config{sessiondir}") {
    # afs  
    if ($config{sessiondir} =~ /\/afs\//) {
	$session_diskfree =`fs listquota $config{sessiondir} 2>/dev/null`;
	if ($session_diskfree) {
	    $session_diskfree =~ /\n\S+\s+(\d+)\s+(\d+)\s+\d+%\s+\d+%/;
	    $session_diskfree = int (($1 - $2)/1024);
	    $session_disktotal= int $1/1024;
	} 
    } 
    # "ordinary" disk 
    else {
	my (@dfstring) =`df -k -P $config{sessiondir} 2>/dev/null`;
	if ($dfstring[1]) {
	    my ( $filesystem, $onekblocks, $used, $available, $usepercentage,
		 $mounted ) = split " ", $dfstring[1];
	    $session_diskfree = int $available/1024;
	    $session_disktotal = int $onekblocks/1024;
	}
    }
}
else {
    warning("no sessiondir found");
}
    
#nordugrid-cluster-cacheusage
my ($cache_statfile) = "statistics";
my ($cache_disktotal, $cache_diskfree);
if ( $config{cachedir} ){
    (my $path1, my $path2) = split /\s+/, $config{cachedir};
    my ($dfstring) = join '', `cat $path1/$cache_statfile 2>/dev/null`;
    if ( $? ) {
	warning("error in reading the cache status file $path1/$cache_statfile");
    } 
    $dfstring =~ /softfree=(\d+)/;
    $cache_diskfree = int $1/1024/1024;
    $dfstring =~ /unclaimed=(\d+)/;
    $cache_diskfree += int $1/1024/1024;
    $dfstring =~ /softsize=(\d+)/;
    $cache_disktotal = int $1/1024/1024;
}

#NorduGrid middleware
#nordugridlocation/bin/grid-manager -v
$ENV{"LD_LIBRARY_PATH"}="$globus_location/lib:$config{ng_location}/lib";
my ($ngversion) = `$config{ng_location}/sbin/grid-manager -v 2>/dev/null`;
if ($?) { warning("Can't execute the $config{ng_location}/sbin/grid-manager")}
$ngversion =~ s/grid-manager: version\s*(.+)$/$1/;
chomp $ngversion;

#Globus Toolkit version
#globuslocation/share/doc/VERSION
my $globusversion;
if (-r "$globus_location/share/doc/VERSION" ) {
   chomp ( $globusversion =  `cat $globus_location/share/doc/VERSION 2>/dev/null`);
   if ($?) { warning ("error in reading the globus version file")}   
}
#globuslocation/bin/globus-version
elsif (-x "$globus_location/bin/globus-version" ) {
   chomp ( $globusversion =  `$globus_location/bin/globus-version 2>/dev/null`);
   if ($?) { warning ("error in executing the $globus_location/bin/globus-version command")}
}

#nordugrid-cluster-runtimeenvironment
if ($config{runtimedir}){
   if (opendir DIR, $config{runtimedir}) {
      @runtimeenvironment = `find $config{runtimedir} -type f ! -name ".*" ! -name "*~"` ;
      closedir DIR;
      foreach my $listentry (@runtimeenvironment) {
         chomp($listentry);
         $listentry=~s/$config{runtimedir}\/*//;
      }
   }else{
     # $listentry="";
      warning("Can't acess $config{runtimedir}");
   }
}

###############################################################
# Information from Grid Manager via the gm-jobs command
###############################################################

#read the gm-jobs information about the number of jobs in different gm states

unless (open GMJOBSOUTPUT, 
    "$config{ng_location}/bin/gm-jobs -Z -c $config{conf_file} -u 0  2>/dev/null |") {
    error("Error in executing gm-jobs")
}

while (my $line= <GMJOBSOUTPUT>) {
    if ($line =~ /^Job:/) {next};					  
    ($line =~ /Jobs total: (\d+)/)    and $gm_info{"totaljobs"}=$1; 	  
    ($line =~ /ACCEPTED: (\d+)/)      and $gm_info{"accepted"}=$1; 	  
    ($line =~ /PREPARING: (\d+)/)     and $gm_info{"preparing"}=$1;	  
    ($line =~ /SUBMIT: (\d+)/)        and $gm_info{"submit"}=$1; 	  
    ($line =~ /INLRMS: (\d+)/)        and $gm_info{"inlrms"}=$1; 	  
    ($line =~ /FINISHING: (\d+)/)     and $gm_info{"finishing"}=$1;	  
    ($line =~ /FINISHED: (\d+)/)      and $gm_info{"finished"}=$1;	  
    ($line =~ /DELETED: (\d+)/)       and $gm_info{"deleted"}=$1;	  
    ($line =~ /CANCELING: (\d+)/)     and $gm_info{"canceling"}=$1;	  
}

close GMJOBSOUTPUT;


##################################################
# Select lrms
##################################################
select_lrms(\%config);
###########################################
# Ask LRMS to fill %lrms_cluster
###########################################

my (%lrms_cluster) = cluster_info( \%config );
debug("LRMS_cluster info retrieved");
################################################################
# arc.conf config-file data overrides the values given by LRMS
################################################################

if ($config{totalcpus}) {
    $lrms_cluster{totalcpus} = $config{totalcpus}}
if ($config{cpudistribution}) {
    $lrms_cluster{cpudistribution} = $config{cpudistribution}}

#######################################
# The Beef
#######################################

# Constract the ldif data (hash of arrays)
my (%c);
$c{'dn'} = [ "$config{dn}" ];
$c{'objectClass'} = [ 'Mds', 'nordugrid-cluster' ];
$c{'nordugrid-cluster-name'} = [ "$hostname" ];
$c{'nordugrid-cluster-aliasname'} = [ "$config{cluster_alias}" ];
if ( defined $config{comment} ) {
    $c{'nordugrid-cluster-comment'} = [ "$config{comment}" ];
}
$c{'nordugrid-cluster-owner'} =
    [ split /\[separator\]/, $config{cluster_owner} ];
my @cluster_acl = split /\[separator\]/, $config{authorizedvo};
foreach my $listentry (@cluster_acl) {
   push @{$c{'nordugrid-cluster-acl'}}, "VO:$listentry";       
}
$c{'nordugrid-cluster-location'} = [ $config{cluster_location} ];
$c{'nordugrid-cluster-issuerca'} = [ "$issuerca" ];
$c{'nordugrid-cluster-issuerca-hash'} = [ "$issuerca_hash" ];
foreach my $listentry (@trustedca) {
   push @{$c{'nordugrid-cluster-trustedca'}}, $listentry;
}
$c{'nordugrid-cluster-contactstring'} =
    ["gsiftp://$hostname:$config{gm_port}$config{gm_mount_point}"];
if ($config{interactive_contactstring}) {
    $c{'nordugrid-cluster-interactive-contactstring'} =
	[ split /\[separator\]/, $config{interactive_contactstring} ];
} else {
    $c{'nordugrid-cluster-interactive-contactstring'} = [ "" ];
}
$c{'nordugrid-cluster-support'} =
    [ split /\[separator\]/, $config{clustersupport} ];
$c{'nordugrid-cluster-lrms-type'} = [ "$lrms_cluster{lrms_type}" ];
$c{'nordugrid-cluster-lrms-version'} = [ "$lrms_cluster{lrms_version}" ];
$c{'nordugrid-cluster-lrms-config'} = [ "$config{lrmsconfig}" ];
if ( defined $config{architecture} ) {
    $c{'nordugrid-cluster-architecture'} = [ "$config{architecture}" ];
}
$c{'nordugrid-cluster-opsys'} = [ split /\[separator\]/, $config{opsys} ];

$c{'nordugrid-cluster-benchmark'} = [];
if ( defined  $config{benchmark} ) {
    foreach my $listentry (split /\[separator\]/, $config{benchmark}) {
	my ($bench_name,$bench_value) = split(/\s+/, $listentry);
	push @{$c{'nordugrid-cluster-benchmark'}}, "$bench_name \@ $bench_value";}
}
$c{'nordugrid-cluster-homogeneity'} = [ "$config{homogeneity}" ];
$c{'nordugrid-cluster-nodecpu'} = [ "$config{nodecpu}" ];
$c{'nordugrid-cluster-nodememory'} = [ "$config{nodememory}" ];
$c{'nordugrid-cluster-nodeaccess'} = 
    [ split /\[separator\]/, $config{nodeaccess} ];

$c{'nordugrid-cluster-totalcpus'} = [ "$lrms_cluster{totalcpus}" ];
$c{'nordugrid-cluster-usedcpus'} = [ "$lrms_cluster{usedcpus}" ];
$c{'nordugrid-cluster-cpudistribution'} =
    [ "$lrms_cluster{cpudistribution}" ];
$c{'nordugrid-cluster-prelrmsqueued'} =
    [ ($gm_info{accepted} + $gm_info{preparing} + $gm_info{submit}) ];
$c{'nordugrid-cluster-totaljobs'} =
    [ ($gm_info{totaljobs} - $gm_info{finishing} - $gm_info{finished} - $gm_info{deleted}
      + $lrms_cluster{queuedcpus} + $lrms_cluster{usedcpus} - $gm_info{inlrms})  ];
    
if ($config{localse}) {
    $c{'nordugrid-cluster-localse'} =
	[ split /\[separator\]/, $config{localse} ];
} else {
    $c{'nordugrid-cluster-localse'} = [ "" ];
} 
$c{'nordugrid-cluster-sessiondir-free'} = [ "$session_diskfree" ];
$c{'nordugrid-cluster-sessiondir-total'} =
    [ "$session_disktotal" ];
$config{defaultttl} =~ /(\d+)/;
$c{'nordugrid-cluster-sessiondir-lifetime'} = [ int $1/60 ];

$c{'nordugrid-cluster-cache-free'} = [ "$cache_diskfree" ];
$c{'nordugrid-cluster-cache-total'} = [ "$cache_disktotal" ];

if ( defined $ngversion ) {
   push @{$c{'nordugrid-cluster-middleware'}},  "nordugrid-arc-$ngversion";
}

if ( $globusversion ) {
   push @{$c{'nordugrid-cluster-middleware'}},  "globus-$globusversion";
}

@{$c{'nordugrid-cluster-runtimeenvironment'}} =
    @runtimeenvironment;

if ( defined $config{middleware} ) {
    foreach my $listentry (split /\[separator\]/, $config{middleware}) {
	next if ($ngversion and ($listentry =~ /nordugrid/i)); 
	push @{$c{'nordugrid-cluster-middleware'}}, $listentry;
    }
}

my ( $valid_from, $valid_to ) =
    Shared::mds_valid($config{ttl});
$c{'Mds-validfrom'} = [ "$valid_from" ];
$c{'Mds-validto'} = [ "$valid_to" ];

###################################################
# Print ldif data
###################################################

print_ldif_data(\@cen,\%c);

# log execution time

$totaltime = time - $totaltime;
debug("TOTALTIME in cluster.pl: $totaltime");


exit;
