#!/usr/bin/perl -w
# Queries NDGF site information and translates it from ARC to GLUE schema
# Prototype by L.Field (2006)
# Corrections and Performance enhancements by M.Flechl (2006-08-17)
# Merged with adjusted bdii-update (David Groep, L.Field, M.Litmaath) by L.Field and M.Flechl (2006-08-21)
# ARC GIIS traversal and some sanity checks for failing GRISes by Mattias Wadenstein (2007-07-03)
# Updated to work at a resource bdii-level by Daniel (2009-2010)
# $Id: glite-info-provider-ndgf,v 1.1 2006/08/30 12:13:15 lfield Exp $ 

use strict;
use LWP::Simple;
use POSIX;
use IO::Handle;

sub translator;

# Global variables for translator
use vars qw($DEFAULT); $DEFAULT = -1;
use vars qw($outbIP $inbIP $glueSubClusterUniqueID $norduBenchmark $norduOpsys $norduNodecput $norduNodecpu);
use vars qw($glueHostMainMemoryRAMSize $glueHostArchitecturePlatformType $glueSubClusterUniqueID $GlueHostBenchmarkSI00 $GlueHostBenchmarkSF00);
use vars qw($glueSubClusterName $glueSubClusterPhysicalCPUs $glueSubClusterLogicalCPUs $glueClusterUniqueID);
use vars qw($AssignedSlots $mappedStatus $waitingJobs $totalJobs $waitingJobs $freeSlots);
use vars qw(%attr @envs);

#Create nordugrid ldif
#TODO read this from optarg
my $ldif_input=`$LDIF_GENERATOR_FILE_NG`;

#Translate and print glue ldif
#TODO this should perhaps be able to write do different rootdn:s, not just mds-vo-name=resource,o=grid.
translator($ldif_input);

exit;

#translator takes an ldif-output in a scalar variable as input and prints the output to stdout
sub translator(){

    my $temp=$_[0];

    #$DEFAULT = -1;

    my @ldif;

    #Remove black space at the start of the line
    $temp=~s/\n //gm;
    @ldif=split "\n", $temp;
    push @ldif, "EOF";
    
    #my $hostname=hostname();

    %attr=(
	'nordugrid-cluster-location' => '',
	'nordugrid-cluster-support' => '',
	'nordugrid-cluster-name' => '',
	'nordugrid-cluster-runtimeenvironment' => '',
	'nordugrid-cluster-contactstring' => '',
	'nordugrid-cluster-aliasname' => '',
	'nordugrid-cluster-lrms-type' => '',
	'nordugrid-cluster-lrms-version' => '',
	'nordugrid-cluster-totalcpus' => '',
	'nordugrid-cluster-opsys' => '',
	'nordugrid-cluster-benchmark' => '',
	'nordugrid-cluster-nodecpu' => '',
	'nordugrid-cluster-nodememory' => '',
	'nordugrid-cluster-nodeaccess' => '',
	'nordugrid-cluster-architecture' => '',
	'nordugrid-cluster-acl' => '',
	'nordugrid-cluster-homogeneity' => '',
	'nordugrid-cluster-comment' => '',
	'nordugrid-cluster-owner' => '',
	);

    #all these values will be checked if they are numeric only:
    my @attr_num = ('totalcpus','nodememory');
    for (my $i=0; $i<=$#attr_num; $i++){ $attr_num[$i] = 'nordugrid-cluster-'.$attr_num[$i]; }
    
    my $queue="false";
    my @queue;
    my %queue=(
	'nordugrid-queue-name' => '',
	'nordugrid-queue-running' => '',
	'nordugrid-queue-maxrunning' => '',
	'nordugrid-queue-maxcputime' => '',
	'nordugrid-queue-maxqueuable' => '',
	'nordugrid-queue-totalcpus' => '',
	'nordugrid-queue-opsys' => '',
	'nordugrid-queue-nodecpu' => '',           
	'nordugrid-queue-nodememory' => '',
	'nordugrid-queue-architecture' => '',
	'nordugrid-queue-status' => '',
	'nordugrid-queue-gridqueued' => '',
	'nordugrid-queue-localqueued' => '',
	'nordugrid-queue-prelrmsqueued' => '',
	);

    #all these values will be checked if they are numeric only:
    my @queue_num = ('running','maxrunning','maxcputime','maxqueuable','totalcpus','nodememory','gridqueued','localqueued','prelrmsqueued');
    for (my $i=0; $i<=$#queue_num; $i++){ $queue_num[$i] = 'nordugrid-queue-'.$queue_num[$i]; }


    #set the attributes from the ldif
    for my $key ( keys %attr ) {
	$attr{$key} = join (" ", grep { /^$key/ } @ldif);
	chomp $attr{$key};
	if ($key eq "nordugrid-cluster-opsys" or $key eq "nordugrid-cluster-owner"
	    or $key eq "nordugrid-cluster-benchmark") {
	    $attr{$key}=~s/ ?$key//g;
	} else {
	    $attr{$key}=~s/$key: //g;
	}
	if ($attr{$key}=~/^$/) { $attr{$key}="$DEFAULT" }
    }
    my $glue_site_unique_id="$GLUESITEUNIQUEID";
    @envs = split / /, $attr{'nordugrid-cluster-runtimeenvironment'};
    
    $outbIP = "FALSE";
    $inbIP = "FALSE";
    if ($attr{'nordugrid-cluster-nodeaccess'} eq "outbound"){
	$outbIP = "TRUE";
    } elsif ($attr{'nordugrid-cluster-nodeaccess'} eq "inbound"){
	$inbIP = "TRUE";
    }
    if ($attr{'nordugrid-cluster-acl'} eq "$DEFAULT") {
	$attr{'nordugrid-cluster-acl'}="VO:ops";
    }
    
    my @owner  = split /: /, $attr{'nordugrid-cluster-owner'};
    
    my $nclocation=$attr{'nordugrid-cluster-location'};
    
    my $loc = $LOC; 
    my $lat = $LAT; 
    my $long = $LONG;

    #set numeric values to $DEFAULT if they are on the list and not numeric
	for (my $i=0; $i<=$#attr_num; $i++){
	    if (! ($attr{$attr_num[$i]} =~ /^\d+$/) ){ $attr{$attr_num[$i]} = $DEFAULT; }
    }

    # Write Site Entries
    print "
dn: GlueSiteUniqueID=$glue_site_unique_id,mds-vo-name=resource,o=grid
objectClass: GlueTop
objectClass: GlueSite
objectClass: GlueKey
objectClass: GlueSchemaVersion
GlueSiteUniqueID: $glue_site_unique_id
GlueSiteName: $glue_site_unique_id
GlueSiteDescription: ARC-$attr{'nordugrid-cluster-comment'}
GlueSiteUserSupportContact: mailto: $attr{'nordugrid-cluster-support'}
GlueSiteSysAdminContact: mailto: $attr{'nordugrid-cluster-support'}
GlueSiteSecurityContact: mailto: $attr{'nordugrid-cluster-support'}
GlueSiteLocation: $loc
GlueSiteLatitude: $lat
GlueSiteLongitude: $long
GlueSiteWeb: $GLUESITEWEB";

    for (my $i=1; $i<=$#owner; $i++){
	print "\nGlueSiteSponsor: $owner[$i]";
    }

    print "
GlueSiteOtherInfo: Middleware=ARC
GlueForeignKey: None
GlueSchemaVersionMajor: 1
GlueSchemaVersionMinor: 2

";
    

    # Write Cluster Entries
    
    print "
dn: GlueClusterUniqueID=$attr{'nordugrid-cluster-name'},mds-vo-name=resource,o=grid
objectClass: GlueClusterTop
objectClass: GlueCluster
objectClass: GlueSchemaVersion
objectClass: GlueInformationService
objectClass: GlueKey
GlueClusterName: $glue_site_unique_id
GlueClusterService: $attr{'nordugrid-cluster-name'}
GlueClusterUniqueID: $attr{'nordugrid-cluster-name'}
GlueForeignKey: GlueCEUniqueID=$attr{'nordugrid-cluster-contactstring'}
GlueForeignKey: GlueSiteUniqueID=$glue_site_unique_id
GlueSchemaVersionMajor: 1
GlueSchemaVersionMinor: 2

";


    if ($attr{'nordugrid-cluster-homogeneity'} =~ /true/i){
	$glueSubClusterUniqueID=$attr{'nordugrid-cluster-name'};
	$norduOpsys=$attr{'nordugrid-cluster-opsys'};
	$norduBenchmark=$attr{'nordugrid-cluster-benchmark'};
	$norduNodecpu=$attr{'nordugrid-cluster-nodecpu'};
	$glueHostMainMemoryRAMSize=$attr{'nordugrid-cluster-nodememory'};
	$glueHostArchitecturePlatformType=$attr{'nordugrid-cluster-architecture'};
	$glueSubClusterUniqueID=$attr{'nordugrid-cluster-name'};
	$glueSubClusterName=$glue_site_unique_id;
	$glueSubClusterPhysicalCPUs=$attr{'nordugrid-cluster-totalcpus'};
	$glueSubClusterLogicalCPUs=$attr{'nordugrid-cluster-totalcpus'};
	$glueClusterUniqueID=$attr{'nordugrid-cluster-name'};
    
	WriteSubCluster();
    }

    #Do GlueCE entry for each nordugrid queue
    foreach(@ldif){
	push @queue, $_;
	if(m/^dn:\s+nordugrid-queue-name/){
	    $queue="true";
	    undef @queue;
	}
	
	if( ( (m/^\s*$/) || (m/^EOF/) ) && ( $queue eq "true" )  ){
	    $queue = "false";
	    
	    #Set the queue attributes from the ldif
	    for my $key ( keys %queue ) {
		$queue{$key} = join (" ", grep { /^$key/ } @queue);
		chomp $queue{$key};
		
		if ($key eq "nordugrid-queue-opsys"){
		    $queue{$key}=~s/$key//g;
		} else {
		    $queue{$key}=~s/$key: //g;
		}
		if ($queue{$key}=~/^$/) { $queue{$key}="$DEFAULT" }
		
	    }
	    
	    #set non-numeric values to $DEFAULT if they are on the list
	    for (my $i=0; $i<=$#queue_num; $i++){
		if (! ($queue{$queue_num[$i]} =~ /^\d+$/) ){
		    #print "XXX Change $queue{$queue_num[$i]} to $DEFAULT\n";
		    $queue{$queue_num[$i]} = $DEFAULT;
		}
	    }


	    if ($attr{'nordugrid-cluster-homogeneity'} =~ /false/i){

		$glueSubClusterUniqueID=$queue{'nordugrid-queue-name'}; ##XX
		$norduOpsys=$queue{'nordugrid-queue-opsys'}; ##XX
		$norduBenchmark=$queue{'nordugrid-queue-benchmark'}; ##XX
		$norduNodecpu=$queue{'nordugrid-queue-nodecpu'}; ##XX
		$glueHostMainMemoryRAMSize=$queue{'nordugrid-queue-nodememory'}; ##XX
		$glueHostArchitecturePlatformType=$queue{'nordugrid-queue-architecture'}; ##XX
		$glueSubClusterUniqueID=$queue{'nordugrid-queue-name'};  ##XX
		$glueSubClusterName=$queue{'nordugrid-queue-name'};  ##XX
		$glueSubClusterPhysicalCPUs=$queue{'nordugrid-queue-totalcpus'};  ##XX
		$glueSubClusterLogicalCPUs=$queue{'nordugrid-queue-totalcpus'};  ##XX
		$glueClusterUniqueID=$attr{'nordugrid-cluster-name'};  ##XX

		WriteSubCluster();
	    }
	    

	    $AssignedSlots = 0;
	    if ($queue{'nordugrid-queue-totalcpus'} ne $DEFAULT){
		$AssignedSlots = $queue{'nordugrid-queue-totalcpus'};
	    } elsif ($queue{'nordugrid-queue-maxrunning'} ne $DEFAULT)  {
		$AssignedSlots = $queue{'nordugrid-queue-maxrunning'};
	    } elsif ($attr{'nordugrid-cluster-totalcpus'} ne $DEFAULT)  {
		$AssignedSlots = $attr{'nordugrid-cluster-totalcpus'};
	    }
	    
	    if ($queue{'nordugrid-queue-totalcpus'} eq $DEFAULT){
		$queue{'nordugrid-queue-totalcpus'} = $attr{'nordugrid-cluster-totalcpus'};
	    }
	    
	    $mappedStatus="";
	    if ($queue{'nordugrid-queue-status'} eq "active"){
		$mappedStatus = "Production";
	    } else{
		$mappedStatus = "Closed";
	    }
	    
	    $waitingJobs = 0;
	    if ($queue{'nordugrid-queue-gridqueued'} ne $DEFAULT) {$waitingJobs += $queue{'nordugrid-queue-gridqueued'};}
	    if ($queue{'nordugrid-queue-localqueued'} ne $DEFAULT) {$waitingJobs += $queue{'nordugrid-queue-localqueued'};}
	    if ($queue{'nordugrid-queue-prelrmsqueued'} ne $DEFAULT) {$waitingJobs += $queue{'nordugrid-queue-prelrmsqueued'};}
	    
	    $totalJobs = $waitingJobs;
	    if ($queue{'nordugrid-queue-prelrmsqueued'} ne $DEFAULT) { $totalJobs += $queue{'nordugrid-queue-running'}; }

	    $freeSlots=$DEFAULT;
	    if ( ($queue{'nordugrid-queue-totalcpus'} ne $DEFAULT) && ($queue{'nordugrid-queue-running'} ne $DEFAULT) ){
		$freeSlots = $queue{'nordugrid-queue-totalcpus'} - $queue{'nordugrid-queue-running'};
	    }

	    # Write CE Entries
	    
	    print "
dn: GlueCEUniqueID=$attr{'nordugrid-cluster-contactstring'}?queue\\=$queue{'nordugrid-queue-name'},mds-vo-name=resource,o=grid
objectClass: GlueCETop
objectClass: GlueCE
objectClass: GlueSchemaVersion
objectClass: GlueCEAccessControlBase
objectClass: GlueCEInfo
objectClass: GlueCEPolicy
objectClass: GlueCEState
objectClass: GlueInformationService
objectClass: GlueKey
GlueCEUniqueID: $attr{'nordugrid-cluster-contactstring'}?queue=$queue{'nordugrid-queue-name'}
GlueCEHostingCluster: $attr{'nordugrid-cluster-name'}
GlueCEName: $queue{'nordugrid-queue-name'}
GlueCEInfoGatekeeperPort: 2811
GlueCEInfoHostName: $attr{'nordugrid-cluster-name'}
GlueCEInfoLRMSType: $attr{'nordugrid-cluster-lrms-type'}
GlueCEInfoLRMSVersion: $attr{'nordugrid-cluster-lrms-version'}
GlueCEInfoGRAMVersion: $DEFAULT
GlueCEInfoTotalCPUs: $queue{'nordugrid-queue-totalcpus'}
GlueCECapability: CPUScalingReferenceSI00=$CPUSCALINGREFERENCESI00
GlueCEInfoJobManager: arc
GlueCEInfoContactString: $attr{'nordugrid-cluster-contactstring'}?queue=$queue{'nordugrid-queue-name'}
GlueInformationServiceURL: ldap://$attr{'nordugrid-cluster-name'}:$BDIIPORT/mds-vo-name=resource,o=grid
GlueCEStateEstimatedResponseTime: 1000
GlueCEStateRunningJobs: $queue{'nordugrid-queue-running'}
GlueCEStateStatus: $mappedStatus
GlueCEStateTotalJobs: $totalJobs
GlueCEStateWaitingJobs: $waitingJobs
GlueCEStateWorstResponseTime: 2000
GlueCEStateFreeJobSlots: $freeSlots
GlueCEPolicyMaxCPUTime: $queue{'nordugrid-queue-maxcputime'}
GlueCEPolicyMaxRunningJobs: $queue{'nordugrid-queue-maxrunning'}
GlueCEPolicyMaxTotalJobs: $queue{'nordugrid-queue-maxqueuable'}
GlueCEPolicyMaxWallClockTime: $queue{'nordugrid-queue-maxcputime'}
GlueCEPolicyPriority: 1
GlueCEPolicyAssignedJobSlots: $AssignedSlots
GlueCEAccessControlBaseRule: $attr{'nordugrid-cluster-acl'}
GlueForeignKey: GlueClusterUniqueID=$attr{'nordugrid-cluster-name'}
GlueSchemaVersionMajor: 1
GlueSchemaVersionMinor: 2
";
	    
	}
    }
}


# Write SubCluster Entries

sub WriteSubCluster {
    
#dn: GlueSubClusterUniqueID=$glueSubClusterUniqueID,GlueClusterUniqueID=$attr{'nordugrid-cluster-name'},mds-vo-name=resource,o=grid

    print "
dn: GlueSubClusterUniqueID=$glueSubClusterUniqueID,GlueClusterUniqueID=$attr{'nordugrid-cluster-name'},mds-vo-name=resource,o=grid
objectClass: GlueClusterTop
objectClass: GlueSubCluster
objectClass: GlueSchemaVersion
objectClass: GlueHostApplicationSoftware
objectClass: GlueHostArchitecture
objectClass: GlueHostBenchmark
objectClass: GlueHostMainMemory
objectClass: GlueHostNetworkAdapter
objectClass: GlueHostOperatingSystem
objectClass: GlueHostProcessor
objectClass: GlueInformationService
objectClass: GlueKey
";
 
    foreach (@envs){
	chomp;
	print "GlueHostApplicationSoftwareRunTimeEnvironment:  $_\n"
    }
 
    print "GlueHostArchitectureSMPSize: 2
GlueHostNetworkAdapterInboundIP: $inbIP
GlueHostNetworkAdapterOutboundIP: $outbIP";
 
    my @opsys = split /: /, $norduOpsys;
    for (my $i=0; $i<=3; $i++){
	if ($i > $#opsys) { $opsys[$i]=$DEFAULT; }
	chomp($opsys[$i]);
	$opsys[$i]=~s/\s+$//;
    }
    $GlueHostBenchmarkSI00 = $DEFAULT;
    $GlueHostBenchmarkSF00 = $DEFAULT;
    my $benchmark;
    if($norduBenchmark) {
	foreach $benchmark (split /: /, $norduBenchmark) {
	    if($benchmark =~ /SPECINT2000 @ (\d+)/) {
		$GlueHostBenchmarkSI00 = $1;
	    }
	    if($benchmark =~ /SPECFP2000 @ (\d+)/) {
		$GlueHostBenchmarkSF00 = $1;
	    }
	}
    }
 
    my @nodecpu  = split / /, $norduNodecpu;
    my $clockSpeed=$DEFAULT;
    my $cpuVer=$DEFAULT;
    for (my $i=0; $i<=$#nodecpu; $i++){
	if ($i >= 2){
	    if ($nodecpu[$i -2] =~ /@/) { $clockSpeed = $nodecpu[$i -1]." ".$nodecpu[$i]; }
	}
	if ($nodecpu[$i] =~ /I$/) { $cpuVer = $nodecpu[$i]; }
    }
    for (my $i=0; $i<=1; $i++){ if ($i > $#nodecpu) { $nodecpu[$i]=$DEFAULT; } }
    
    $clockSpeed=~s/\.[0-9]*//;
    $clockSpeed=~s/MHz//i;
    $clockSpeed=~s/GHz/000/i;
    $clockSpeed=~s/[^0-9]//g;
    if (! $clockSpeed=~m/[0-9]/) { $clockSpeed=$DEFAULT; } 
    print "
GlueHostOperatingSystemName: $opsys[1]
GlueHostOperatingSystemRelease: $opsys[2]
GlueHostOperatingSystemVersion: $opsys[3]
GlueHostProcessorVendor: $nodecpu[0]
GlueHostProcessorModel: $nodecpu[1]
GlueHostProcessorVersion: $cpuVer
GlueHostProcessorClockSpeed: $clockSpeed
GlueHostProcessorOtherDescription: $PROCESSOROTHERDESCRIPTION
GlueHostMainMemoryRAMSize: $glueHostMainMemoryRAMSize
GlueHostArchitecturePlatformType: $glueHostArchitecturePlatformType
GlueHostBenchmarkSI00: $GlueHostBenchmarkSI00
GlueHostBenchmarkSF00: $GlueHostBenchmarkSF00
GlueSubClusterUniqueID: $glueSubClusterUniqueID
GlueSubClusterName: $glueSubClusterName
GlueSubClusterPhysicalCPUs: $glueSubClusterPhysicalCPUs
GlueSubClusterLogicalCPUs: $glueSubClusterLogicalCPUs
GlueChunkKey: GlueClusterUniqueID=$glueClusterUniqueID
GlueSchemaVersionMajor: 1
GlueSchemaVersionMinor: 2
";
}

#EOF
