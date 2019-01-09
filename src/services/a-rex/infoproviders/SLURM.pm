package SLURM;

######################################################################
# DISCLAIMER
######################################################################
# This module depends on ARC0mod.pm which is obsolete and deprecated 
# starting from ARC 6.0 
# Please DO NOT build new LRMS modules based on this one but follow
# the indications in
#                       LRMSInfo.pm
# instead.
######################################################################

use strict;
use POSIX qw(ceil floor);

our @ISA = ('Exporter');

# Module implements these subroutines for the LRMS interface

our @EXPORT_OK = ('cluster_info',
              'queue_info',
              'jobs_info',
              'users_info',
              'nodes_info');

use LogUtils ( 'start_logging', 'error', 'warning', 'debug' ); 

##########################################
# Saved private variables
##########################################

#our(%lrms_queue,%lrms_users);

our(%scont_config, %scont_part, %scont_jobs, %scont_nodes, %sinfo_cpuinfo);

##########################################
# Private subs
##########################################

sub slurm_read_config($){
    my ($config) = shift;
    my ($path) = ($$config{slurm_bin_path} or $$config{SLURM_bin_path} or "/usr/bin");
    # get SLURM config, store dictionary in scont_config
    my %scont_config;
    checkbin("$path/scontrol");
    open (SCPIPE,"$path/scontrol show config| grep -Ev \"primary|Configuration|^\$\"|");
    while(<SCPIPE>){
	chomp;
	my @mrr = split(" = ", $_, 2);
	$mrr[0]=~s/\s+$//;
	$scont_config{$mrr[0]} = $mrr[1];
    }
    close(SCPIPE);

    return %scont_config;
}

sub get_variable($$){
    my $match = shift;
    my $string = shift;
    $string =~ m/(\w\s)*?$match=((\w|\s|\/|,|.|:|;|\[|\]|\(|\)|-)*?)($| \w+=.*)/ ;
    my $var = $2;
    return $var;
}
sub slurm_read_jobs($){
    my ($config) = shift;
    my ($path) = ($$config{slurm_bin_path} or $$config{SLURM_bin_path} or "/usr/bin");
    # get SLURM jobs, store dictionary in scont_jobs
    my %scont_jobs;
    checkbin("$path/squeue");
    open (SCPIPE,"$path/squeue -a -h -t all -o \"JobId=%i TimeUsed=%M Partition=%P JobState=%T ReqNodes=%D ReqCPUs=%C TimeLimit=%l Name=%j NodeList=%N\"|");
    while(<SCPIPE>){
	my %job;
	my $string = $_;
	#Fetching of data from squeue output
	my $JobId = get_variable("JobId",$string);
	$job{JobId} = get_variable("JobId",$string);
	$job{TimeUsed} = get_variable("TimeUsed",$string);
	$job{Partition} = get_variable("Partition",$string);
	$job{JobState} = get_variable("JobState",$string);
	$job{ReqNodes} = get_variable("ReqNodes",$string);
	$job{ReqCPUs} = get_variable("ReqCPUs",$string);
	$job{TimeLimit} = get_variable("TimeLimit",$string);
	$job{Name} = get_variable("Name",$string);
	$job{NodeList} = get_variable("NodeList",$string);

	#Translation of data
	$job{TimeUsed} = slurm_to_arc_time($job{TimeUsed});
	$job{TimeLimit} = slurm_to_arc_time($job{TimeLimit});

	$scont_jobs{$JobId} = \%job;

    }
    close(SCPIPE);

    return %scont_jobs;
}

sub slurm_read_partitions($){
    my ($config) = shift;
    my ($path) = ($$config{slurm_bin_path} or $$config{SLURM_bin_path} or "/usr/bin");
    # get SLURM partitions, store dictionary in scont_part
    my %scont_part;
    checkbin("$path/sinfo");
    open (SCPIPE,"$path/sinfo -a -h -o \"PartitionName=%P TotalCPUs=%C TotalNodes=%D MaxTime=%l DefTime=%L\"|");
    while(<SCPIPE>){
	my %part;
	my $string = $_;
	my $PartitionName = get_variable("PartitionName",$string);
	$PartitionName =~ s/\*$//;
	#Fetch data from sinfo
	$part{PartitionName} = $PartitionName;
	my $totalcpus = get_variable("TotalCPUs",$string);
	$part{TotalNodes} = get_variable("TotalNodes",$string);
	$part{MaxTime} = get_variable("MaxTime",$string);
	$part{DefTime} = get_variable("DefTime",$string);

	#Translation of data
	$part{MaxTime} = slurm_to_arc_time($part{MaxTime});
	$part{DefTime} = slurm_to_arc_time($part{DefTime});
	# Format of "%C" is: Number of CPUs by state in the format "allocated/idle/other/total"
	# We only care about total:
	######
	($part{AllocatedCPUs},$part{IdleCPUs},$part{OtherCPUs},$part{TotalCPUs}) = split('/',$totalcpus);

	# Neither of these fields probably need this in SLURM 1.3, but it doesn't hurt.
	$part{AllocatedCPUs} = slurm_parse_number($part{AllocatedCPUs});
	$part{IdleCPUs} = slurm_parse_number($part{IdleCPUs});
	$part{OtherCPUs} = slurm_parse_number($part{OtherCPUs});
	$part{TotalCPUs} = slurm_parse_number($part{TotalCPUs});

	$part{TotalNodes} = slurm_parse_number($part{TotalNodes});

	$scont_part{$PartitionName} = \%part;
    }
    close(SCPIPE);

    return %scont_part;
}

sub slurm_read_cpuinfo($){
    my ($config) = shift;
    my ($path) = ($$config{slurm_bin_path} or $$config{SLURM_bin_path} or "/usr/bin");
    # get SLURM partitions, store dictionary in scont_part
    my %sinfo_cpuinfo;
    my $cpuinfo;
    checkbin("$path/sinfo");
    open (SCPIPE,"$path/sinfo -a -h -o \"cpuinfo=%C\"|");
    while(<SCPIPE>){
	my $string = $_;
	$cpuinfo = get_variable("cpuinfo",$string);
    }
    close(SCPIPE);
    ($sinfo_cpuinfo{AllocatedCPUs},$sinfo_cpuinfo{IdleCPUs},$sinfo_cpuinfo{OtherCPUs},$sinfo_cpuinfo{TotalCPUs}) = split('/',$cpuinfo);

    $sinfo_cpuinfo{AllocatedCPUs} = slurm_parse_number($sinfo_cpuinfo{AllocatedCPUs});
    $sinfo_cpuinfo{IdleCPUs} = slurm_parse_number($sinfo_cpuinfo{IdleCPUs});
    $sinfo_cpuinfo{OtherCPUs} = slurm_parse_number($sinfo_cpuinfo{OtherCPUs});
    $sinfo_cpuinfo{TotalCPUs} = slurm_parse_number($sinfo_cpuinfo{TotalCPUs});

    return %sinfo_cpuinfo;
}    

sub slurm_read_nodes($){
    my ($config) = shift;
    my ($path) = ($$config{slurm_bin_path} or $$config{SLURM_bin_path} or "/usr/bin");
    # get SLURM nodes, store dictionary in scont_nodes
    my %scont_nodes;
    checkbin("$path/scontrol");
    open (SCPIPE,"$path/scontrol show node --oneliner|");
    while(<SCPIPE>){
	my %record;
	my $string = $_;

	my $node = get_variable("NodeName",$string);
	# We have to keep CPUs key name for not to break other
	# functions that use this key
	$record{CPUs} = get_variable("CPUTot",$string);
	$record{RealMemory} = get_variable("RealMemory",$string);

	my $StateName = get_variable("State",$string);
	# Node status can be followed by different symbols
	# according to it being unresponsive, powersaving, etc.
	# Get rid of them
	$StateName =~ s/[\*~#\+]$//;
	$record{State} = $StateName;

	$record{Sockets} = get_variable("Sockets",$string);
	$record{SysName} = get_variable("OS",$string);
	$record{Arch} = get_variable("Arch",$string);

	$scont_nodes{$node} = \%record;

    }
    close(SCPIPE);

    return %scont_nodes;
}

#Function for retrieving used and queued cpus from slurm
sub slurm_get_jobs {
    my $queue = shift;

    my $queuedjobs=0, my $usedcpus=0, my $nocpus=0, my $jqueue=0;
    my $runningjobs=0;

    foreach my $i (keys %scont_jobs){

	$jqueue = $scont_jobs{$i}{"Partition"};
	next if (defined($queue) && !($jqueue =~ /$queue/));
        if ($scont_jobs{$i}{"JobState"} =~ /^PENDING$/){
                $queuedjobs++;
        }
        if (($scont_jobs{$i}{"JobState"} =~ /^RUNNING$/) ||
 	($scont_jobs{$i}{"JobState"} =~ /^COMPLETING$/)){
		$runningjobs++;
        }


    }
    return ($queuedjobs, $runningjobs);
}

sub slurm_get_data($){
    my $config = shift;
    %scont_config = slurm_read_config($config);
    %scont_part = slurm_read_partitions($config);
    %scont_jobs = slurm_read_jobs($config);
    %scont_nodes = slurm_read_nodes($config);
    %sinfo_cpuinfo = slurm_read_cpuinfo($config);

}

sub slurm_to_arc_time($){
    my $timeslurm = shift;
    my $timearc = 0;
    # $timeslurm can be "infinite" or "UNLIMITED"
    if (($timeslurm =~ "UNLIMITED")
	or ($timeslurm =~ "infinite")) {
	#Max number allowed by ldap
	$timearc = 2**31-1;
    }
    # days-hours:minutes:seconds
    elsif ( $timeslurm =~ /(\d+)-(\d+):(\d+):(\d+)/ ) {
	$timearc = $1*24*60*60 + $2*60*60 + $3*60 + $4;
    }
    # hours:minutes:seconds
    elsif ( $timeslurm =~ /(\d+):(\d+):(\d+)/ ) {
	$timearc = $1*60*60 + $2*60 + $3;
    }
    # minutes:seconds
    elsif ( $timeslurm =~ /(\d+):(\d+)/ ) {
	$timearc = $1*60 + $2;
    }
    # ARC infosys uses minutes as the smallest allowed value.
    $timearc = floor($timearc/60);
    return $timearc;
}

# SLURM outputs some values as 12.3K where K is 1024. Include T, G, M
# as well in case they become necessary in the future.

sub slurm_parse_number($){
    my $value = shift;
    if ( $value =~ /(\d+\.?\d*)K$/ ){
	$value = floor($1 * 1024);
    }
    if ( $value =~ /(\d+\.?\d*)M$/ ){
	$value = floor($1 * 1024 * 1024);
    }
    if ( $value =~ /(\d+\.?\d*)G$/ ){
	$value = floor($1 * 1024 * 1024 * 1024);
    }
    if ( $value =~ /(\d+\.?\d*)T$/ ){
	$value = floor($1 * 1024 * 1024 * 1024 * 1024);
    }
    return $value;
}

sub slurm_get_first_node($){
    my $nodes = shift;
    my @enodes = split(",", slurm_expand_nodes($nodes));
    return " NoNode " if ! @enodes;
    return $enodes[0];
}
#translates a list like n[1-2,5],n23,n[54-55] to n1,n2,n5,n23,n54,n55 
sub slurm_expand_nodes($){
    my $nodes = shift;
    my $enodes = "";
    $nodes =~ s/,([a-zA-Z])/ $1/g;
    foreach my $node (split(" ",$nodes)){

	if( $node =~ m/([a-zA-Z0-9-_]*)\[([0-9\-,]*)\]/ ){
	    my $name = $1;
	    my $list = $2;
	        
	    foreach my $element (split(",",$list)){
		if($element =~ /(\d*)-(\d*)/){
		    my $start=$1;
		    my $end=$2;
		    my $l = length($start);
		    for (my $i=$start;$i<=$end;$i++){
			# Preserve leading zeroes in sequence, if needed
			$enodes .= sprintf("%s%0*d,", $name, $l, $i);
		    }
		}
		else {
		    $enodes .= $name.$element.",";
		}
	    }
	} else { 
	    $enodes .= $node . ",";
	}
    }
    chop $enodes;
    return $enodes;
}

# check the existence of a passed binary, full path.
sub checkbin ($) {
    my $apbin = shift;
    error("Can't find $apbin , check slurm_bin_path. Exiting...") unless -f $apbin;
}

############################################
# Public subs
#############################################

sub cluster_info ($) {

    # config array
    my ($config) = shift;
    my ($path) = ($$config{slurm_bin_path} or $$config{SLURM_bin_path} or "/usr/bin");
    # Get Data needed by this function, stored in the global variables
    # scont_nodes, scont_part, scont_jobs
    slurm_get_data($config);

    # Return data structure %lrms_cluster{$keyword}
    #
    # lrms_type          LRMS type (eg. LoadLeveler)
    # lrms_version       LRMS version
    # totalcpus          Total number of cpus in the system
    # queuedjobs         Number of queueing jobs in LRMS 
    # runningjobs        Number of running jobs in LRMS 
    # usedcpus           Used cpus in the system
    # cpudistribution    CPU distribution string
    #
    # All values should be defined, empty values "" are ok if field
    # does not apply to particular LRMS.
    my (%lrms_cluster);

    #determine the version of SLURM
    $lrms_cluster{lrms_type} = "SLURM";
    $lrms_cluster{lrms_version} = $scont_config{"SLURM_VERSION"};

    # SLURM has no cputime limit at all
    $lrms_cluster{has_total_cputime_limit} = 0;

    #determine number of processors
    my $totalcpus=0;

    foreach my $i (keys %scont_nodes){
	$totalcpus += $scont_nodes{$i}{"CPUs"};
    }
    $lrms_cluster{totalcpus} = $totalcpus;
    
    $lrms_cluster{usedcpus} = $sinfo_cpuinfo{AllocatedCPUs};
    
    # TODO: investigate if this can be calculated for SLURM
    # this is a quick and dirty fix for a warning, might be fixed somewhere else
    $lrms_cluster{queuedcpus} = 0;
    
    ($lrms_cluster{queuedjobs}, $lrms_cluster{runningjobs}) = slurm_get_jobs();

    #NOTE: should be on the form "8cpu:800 2cpu:40"
    my @queue=();
    foreach my $i (keys %scont_part){
	unshift (@queue,$i);
    }
    my %cpudistribution;
    $lrms_cluster{cpudistribution} = "";
    foreach my $key (keys %scont_nodes){
	if(exists $cpudistribution{$scont_nodes{$key}{CPUs}}){
	    $cpudistribution{$scont_nodes{$key}{CPUs}} +=1;
	}
	else{
	    $cpudistribution{$scont_nodes{$key}{CPUs}} = 1;
	}
    }
    foreach my $key (keys %cpudistribution){
	$lrms_cluster{cpudistribution}.= $key ."cpu:" . $cpudistribution{$key} . " ";
    }
    $lrms_cluster{queue} = [@queue];

    return %lrms_cluster;
}

sub queue_info ($$) {

    # config array
    my ($config) = shift;
    my ($path) = ($$config{slurm_bin_path} or $$config{SLURM_bin_path} or "/usr/bin");

    # Name of the queue to query
    my ($queue) = shift;

    # Get data needed by this function, stored in global variables
    # scont_nodes, scont_part, scont_jobs
    slurm_get_data($config);

    # The return data structure is %lrms_queue.
    my (%lrms_queue);
    # Return data structure %lrms_queue{$keyword}
    #
    # status        available slots in the queue, negative number signals
    #               some kind of LRMS error state for the queue
    # maxrunning    queue limit for number of running jobs
    # maxqueuable   queue limit for number of queueing jobs
    # maxuserrun    queue limit for number of running jobs per user
    # maxcputime    queue limit for max cpu time for a job
    # mincputime    queue limit for min cpu time for a job
    # defaultcput   queue default for cputime
    # maxwalltime   queue limit for max wall time for a job
    # minwalltime   queue limit for min wall time for a job
    # defaultwalltime   queue default for walltime
    # running       number of procs used by running jobs in the queue
    # queued        number of procs requested by queueing jobs in the queue
    # totalcpus     number of procs in the queue

    #
    # All values should be defined, empty values "" are ok if field
    # does not apply to particular LRMS.
    
    #TODO available slots, not max jobs.
    $lrms_queue{status} = $scont_config{"MaxJobCount"};
    $lrms_queue{maxrunning} = $scont_config{"MaxJobCount"};
    $lrms_queue{maxqueuable} = $scont_config{"MaxJobCount"};
    $lrms_queue{maxuserrun} = $scont_config{"MaxJobCount"};

    my $maxtime = $scont_part{$queue}{"MaxTime"};
    my $deftime = $scont_part{$queue}{"DefTime"};
    
    $lrms_queue{maxcputime} = $maxtime;
    $lrms_queue{mincputime} = 0;
    $lrms_queue{defaultcput} = $deftime;
    $lrms_queue{maxwalltime} = $maxtime;
    $lrms_queue{minwalltime} = 0;
    $lrms_queue{defaultwallt} = $deftime;

    ($lrms_queue{queued}, $lrms_queue{running}) = slurm_get_jobs($queue);

    $lrms_queue{totalcpus} = $scont_part{$queue}{TotalCPUs};


    return %lrms_queue;
}

sub jobs_info ($$$) {

    # config array
    my ($config) = shift;
    my ($path) = ($$config{slurm_bin_path} or $$config{SLURM_bin_path} or "/usr/bin");
    # Name of the queue to query
    my ($queue) = shift;
    # LRMS job IDs from Grid Manager (jobs with "INLRMS" GM status)
    my ($jids) = shift;

    #Get data needed by this function, stored in global variables
    # scont_nodes, scont_part, scont_jobs
    slurm_get_data($config);

    # status        Status of the job: Running 'R', Queued'Q',
    #                                  Suspended 'S', Exiting 'E', Other 'O'
    # rank          Position in the queue
    # mem           Used (virtual) memory
    # walltime      Used walltime
    # cputime       Used cpu-time
    # reqwalltime   Walltime requested from LRMS
    # reqcputime    Cpu-time requested from LRMS
    # nodes         List of execution hosts. 
    # comment       Comment about the job in LRMS, if any
    # cpus          Number of cpus requested/used by the job

    my (%lrms_jobs);

    #$queue is not used to keep jobs from different queues separate
    #jobs can't have overlapping job-ids between queues in SLURM

    foreach my $jid (@{$jids}){
	if ($scont_jobs{$jid}{"JobState"} eq "RUNNING") {
	    $lrms_jobs{$jid}{status} = "R";
	}
	elsif ($scont_jobs{$jid}{"JobState"} eq "COMPLETED") {
	    $lrms_jobs{$jid}{status} = "E";
	}
	elsif ($scont_jobs{$jid}{"JobState"} eq "CANCELLED") {
	    $lrms_jobs{$jid}{status} = "O";
	}
	elsif ($scont_jobs{$jid}{"JobState"} eq "FAILED") {
	    $lrms_jobs{$jid}{status} = "O";
	}
	elsif ($scont_jobs{$jid}{"JobState"} eq "PENDING") {
	    $lrms_jobs{$jid}{status} = "Q";
	}
	elsif ($scont_jobs{$jid}{"JobState"} eq "TIMEOUT") {
	    $lrms_jobs{$jid}{status} = "O";
	}
	else {
	    $lrms_jobs{$jid}{status} = "O";
	}
	#TODO: calculate rank? Probably not possible.
	$lrms_jobs{$jid}{rank} = 0;

	#TODO: This gets the memory from the first node in a job
	#allocation which will not be correct on a heterogenous
	#cluster
	my $node = slurm_get_first_node($scont_jobs{$jid}{"NodeList"});
	$lrms_jobs{$jid}{mem} = $scont_nodes{$node}{"RealMemory"};

	my $walltime = $scont_jobs{$jid}{"TimeUsed"};
	my $count = $scont_jobs{$jid}{ReqCPUs};
	$lrms_jobs{$jid}{walltime} = $walltime;
	# TODO: multiply walltime by number of cores to get cputime?
	$lrms_jobs{$jid}{cputime} = $walltime*$count;

	$lrms_jobs{$jid}{reqwalltime} = $scont_jobs{$jid}{"TimeLimit"};
	# TODO: cputime/walltime confusion again...
	$lrms_jobs{$jid}{reqcputime} = $scont_jobs{$jid}{"TimeLimit"}*$count;
	$lrms_jobs{$jid}{nodes} = [ split(",",slurm_expand_nodes($scont_jobs{$jid}{"NodeList"}) ) ];
	$lrms_jobs{$jid}{comment} = [$scont_jobs{$jid}{"Name"}];

	$lrms_jobs{$jid}{cpus} = $scont_jobs{$jid}{ReqCPUs};
    }

    return %lrms_jobs;
}

sub users_info($$@) {
    # config array
    my ($config) = shift;
    my ($path) = ($$config{slurm_bin_path} or $$config{SLURM_bin_path} or "/usr/bin");
    # name of queue to query
    my ($queue) = shift;
    # user accounts 
    my ($accts) = shift;

    #Get data needed by this function, stored in global variables
    # scont_nodes, scont_part, scont_jobs
    slurm_get_data($config);

    my (%lrms_users);

    # freecpus for given account
    # queue length for given account
    #

    foreach my $u ( @{$accts} ) {
        $lrms_users{$u}{freecpus} = $sinfo_cpuinfo{IdleCPUs};
        $lrms_users{$u}{queuelength} = 0;
    }
    return %lrms_users;
}

sub nodes_info($) {

    my $config = shift;
    my %hoh_slurmnodes = slurm_read_nodes($config);

    my %nodes;
    for my $host (keys %hoh_slurmnodes) {
        my ($isfree, $isavailable) = (0,0);
        $isavailable = 1 unless $hoh_slurmnodes{$host}{State} =~ /DOWN|DRAIN|FAIL|MAINT|UNK/;
        $isfree = 1 if $hoh_slurmnodes{$host}{State} =~ /IDLE|MIXED/;
        $nodes{$host} = {isfree => $isfree, isavailable => $isavailable};

        my $np = $hoh_slurmnodes{$host}{CPUs};
        my $nsock = $hoh_slurmnodes{$host}{Sockets};
        my $rmem = $hoh_slurmnodes{$host}{RealMemory};
        $nodes{$host}{lcpus} = int $np if $np;
        $nodes{$host}{slots} = int $np if $np;
        $nodes{$host}{pmem} = int $rmem if $rmem;
        $nodes{$host}{pcpus} = int $nsock if $nsock;

        $nodes{$host}{sysname} = $hoh_slurmnodes{$host}{SysName};
        $nodes{$host}{machine} = $hoh_slurmnodes{$host}{Arch};

    }
    return %nodes;
}

1;
