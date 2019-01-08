package SLURMmod;

use strict;
use POSIX qw(ceil floor);

our @ISA = ('Exporter');

# Module implements these subroutines for the LRMS interface

our @EXPORT_OK = ('get_lrms_info', 'get_lrms_options_schema');

use LogUtils ( 'start_logging', 'error', 'warning', 'debug' ); 

##########################################
# Public variables
##########################################

our $options;
our $lrms_info;

##########################################
# Saved private variables
##########################################

our $path;
our(%scont_config, %scont_part, %scont_jobs, %scont_nodes, %sinfo_cpuinfo);
our $log = LogUtils->getLogger("SLURMmod");

sub get_lrms_options_schema {
    return {
            'slurm_bin_path'     => '*',
            'queues' => {
                '*' => {
                    'users'       => [ '' ],
                }
            },
            'jobs' => [ '' ]
        };
}

sub get_lrms_info($) {

    $options = shift;

    $path = ($options->{slurm_bin_path} or "/usr/bin");

    slurm_init_check($path);

    slurm_get_data();
    
    cluster_info();
    
    my %qconf = %{$options->{queues}};
    for my $qname ( keys %qconf ) {
        queue_info($qname);
    }

    my $jids = $options->{jobs};
    jobs_info($jids);

    for my $qname ( keys %qconf ) {
        my $users = $qconf{$qname}{users};
        users_info($qname,$users);
    }

    nodes_info();
	
    return $lrms_info;
}

##########################################
# Private subs
##########################################

# checks existence of slurm commands
sub slurm_init_check($) {

   my $path = shift;

   $log->info("Verifying slurm commands...");

   my @slurm_commands = ('scontrol','squeue','sinfo');

   foreach my $slurmcmd (@slurm_commands) {
        $log->error("$path/$slurmcmd command not found. Exiting...") unless (-f "$path/$slurmcmd") ;
   }
}

sub nodes_info() {
    my $lrms_nodes = {};

    # add this cluster to the info tree
    $lrms_info->{nodes} = $lrms_nodes;
    
    for my $host (keys %scont_nodes) {
        my ($isfree, $isavailable) = (0,0);
        $isavailable = 1 unless $scont_nodes{$host}{State} =~ /DOWN|DRAIN|FAIL|MAINT|UNK/;
        $isfree = 1 if $scont_nodes{$host}{State} =~ /IDLE|MIXED/;
        $lrms_nodes->{$host} = {isfree => $isfree, isavailable => $isavailable};

        my $np = $scont_nodes{$host}{CPUTot};
        my $nsock = $scont_nodes{$host}{Sockets};
        my $rmem = $scont_nodes{$host}{RealMemory};
        $lrms_nodes->{$host}{lcpus} = int $np if $np;
        $lrms_nodes->{$host}{slots} = int $np if $np;
        $lrms_nodes->{$host}{pmem} = int $rmem if $rmem;
        $lrms_nodes->{$host}{pcpus} = int $nsock if $nsock;

        $lrms_nodes->{$host}{sysname} = $scont_nodes{$host}{SysName};
        $lrms_nodes->{$host}{machine} = $scont_nodes{$host}{Arch};

    }
    
}

sub users_info($@) {

    # name of queue to query
    my ($queue) = shift;
    # user accounts 
    my ($accts) = shift;

    my $lrms_users = {};

    # add users to the info tree
    my $lrms_queue = $lrms_info->{queues}{$queue};
    $lrms_queue->{users} = $lrms_users;
    
    # freecpus for given account
    # queue length for given account
    #

    foreach my $u ( @{$accts} ) {
        $lrms_users->{$u}{freecpus} = { $sinfo_cpuinfo{IdleCPUs} => 0 };
        $lrms_users->{$u}{queuelength} = 0;
    }
    
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


sub jobs_info ($) {
	
    my $jids = shift;

    my $lrms_jobs = {};

    # add jobs to the info tree
    $lrms_info->{jobs} = $lrms_jobs;

    #jobs can't have overlapping job-ids between queues in SLURM

    foreach my $jid (@{$jids}){
        if ($scont_jobs{$jid}{"JobState"} eq "RUNNING") {
            $lrms_jobs->{$jid}{status} = "R";
        }
        elsif ($scont_jobs{$jid}{"JobState"} eq "COMPLETED") {
        	$lrms_jobs->{$jid}{status} = "E";
        }
        elsif ($scont_jobs{$jid}{"JobState"} eq "CANCELLED") {
        	$lrms_jobs->{$jid}{status} = "O";
        }
        elsif ($scont_jobs{$jid}{"JobState"} eq "FAILED") {
        	$lrms_jobs->{$jid}{status} = "O";
        }
        elsif ($scont_jobs{$jid}{"JobState"} eq "PENDING") {
        	$lrms_jobs->{$jid}{status} = "Q";
        }
        elsif ($scont_jobs{$jid}{"JobState"} eq "TIMEOUT") {
        	$lrms_jobs->{$jid}{status} = "O";
        }
        else {
        	$lrms_jobs->{$jid}{status} = "O";
        }
        
        #TODO: calculate rank? Probably not possible.
        $lrms_jobs->{$jid}{rank} = 0;
        $lrms_jobs->{$jid}{cpus} = $scont_jobs{$jid}{ReqCPUs};
        
        #TODO: This gets the memory from the first node in a job
        #allocation which will not be correct on a heterogenous
        #cluster
        my $node = slurm_get_first_node($scont_jobs{$jid}{"NodeList"});

        # Only jobs that got the nodes can report the memory of
        # their nodes
        if($node ne " NoNode "){
            $lrms_jobs->{$jid}{mem} = $scont_nodes{$node}{"RealMemory"};
        }

        my $walltime = $scont_jobs{$jid}{"TimeUsed"};
        my $count = $scont_jobs{$jid}{ReqCPUs};
        $lrms_jobs->{$jid}{walltime} = $walltime;
        # TODO: multiply walltime by number of cores to get cputime?
        $lrms_jobs->{$jid}{cputime} = $walltime*$count;
        
        $lrms_jobs->{$jid}{reqwalltime} = $scont_jobs{$jid}{"TimeLimit"};
        # TODO: cputime/walltime confusion again...
        $lrms_jobs->{$jid}{reqcputime} = $scont_jobs{$jid}{"TimeLimit"}*$count;
        $lrms_jobs->{$jid}{nodes} = [ split(",",slurm_expand_nodes($scont_jobs{$jid}{"NodeList"}) ) ];
        $lrms_jobs->{$jid}{comment} = [$scont_jobs{$jid}{"Name"}];
    }
}

sub queue_info ($) {
	
    # Name of the queue to query
    my ($queue) = shift;

    my $lrms_queue = {};

    # add this queue to the info tree
    $lrms_info->{queues}{$queue} = $lrms_queue;

    $lrms_queue->{status} = $scont_config{"MaxJobCount"};
    $lrms_queue->{maxrunning} = $scont_config{"MaxJobCount"};
    $lrms_queue->{maxqueuable} = $scont_config{"MaxJobCount"};
    $lrms_queue->{maxuserrun} = $scont_config{"MaxJobCount"};
	
    my $maxtime = $scont_part{$queue}{"MaxTime"};
    my $deftime = $scont_part{$queue}{"DefTime"};
    
    $lrms_queue->{maxcputime} = $maxtime;
    $lrms_queue->{mincputime} = 0;
    $lrms_queue->{defaultcput} = $deftime;
    $lrms_queue->{maxwalltime} = $maxtime;
    $lrms_queue->{minwalltime} = 0;
    $lrms_queue->{defaultwallt} = $deftime;
    ($lrms_queue->{queued}, $lrms_queue->{running}) = slurm_get_jobs($queue);
    $lrms_queue->{totalcpus} = $scont_part{$queue}{TotalCPUs};
    $lrms_queue->{freeslots} = $scont_part{$queue}{IdleCPUs};
    
}

#Function for retrieving running and queued jobs from slurm
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

sub cluster_info () {
    my $lrms_cluster = {};

    # add this cluster to the info tree
    $lrms_info->{cluster} = $lrms_cluster;
    
    #determine the version of SLURM
    $lrms_cluster->{lrms_type} = "SLURM";
    $lrms_cluster->{lrms_version} = $scont_config{"SLURM_VERSION"};

    #determine number of processors
    my $totalcpus=0;
    foreach my $i (keys %scont_nodes){
    	$totalcpus += $scont_nodes{$i}{CPUTot};
    }
    $lrms_cluster->{totalcpus} = $totalcpus;

    # TODO: investigate if this can be calculated for SLURM
    # this is a quick and dirty fix for a warning, might be fixed somewhere else
    $lrms_cluster->{queuedcpus} = 0;
    
    $lrms_cluster->{usedcpus} = $sinfo_cpuinfo{AllocatedCPUs};
    
    ($lrms_cluster->{queuedjobs}, $lrms_cluster->{runningjobs}) = slurm_get_jobs();

    #NOTE: should be on the form "8cpu:800 2cpu:40"
    
    my %cpudistribution;
    $lrms_cluster->{cpudistribution} = "";
    foreach my $key (keys %scont_nodes){
    	if(exists $cpudistribution{$scont_nodes{$key}{CPUTot}}){
    		$cpudistribution{$scont_nodes{$key}{CPUTot}} +=1;
    	}
    	else{
    		$cpudistribution{$scont_nodes{$key}{CPUTot}} = 1;
    	}
    }
    foreach my $key (keys %cpudistribution){
    	$lrms_cluster->{cpudistribution}.= $key ."cpu:" . $cpudistribution{$key} . " ";
    }   
}

sub slurm_get_data(){
    %scont_config = slurm_read_config();
    %scont_part = slurm_read_partitions();
    %scont_jobs = slurm_read_jobs();
    %scont_nodes = slurm_read_nodes();
    %sinfo_cpuinfo = slurm_read_cpuinfo();
}

sub slurm_read_config(){
    # get SLURM config, store dictionary in scont_config
    my %scont_config;
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

sub slurm_to_arc_time($){
    my $timeslurm = shift;
    my $timearc = 0;
    # $timeslurm can be "infinite" or "UNLIMITED"
    if (($timeslurm =~ "UNLIMITED") or ($timeslurm =~ "infinite")) {
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

sub slurm_read_partitions(){
    # get SLURM partitions, store dictionary in scont_part
    my %scont_part;
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

sub slurm_read_jobs($){
    # get SLURM jobs, store dictionary in scont_jobs
    my %scont_jobs;
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

sub slurm_read_nodes($){
    # get SLURM nodes, store dictionary in scont_nodes
    my %scont_nodes;
    open (SCPIPE,"$path/scontrol show node --oneliner|");
    while(<SCPIPE>){
	my %record;
	my $string = $_;

	my $node = get_variable("NodeName",$string);
	# We have to keep CPUs key name for not to break other
	# functions that use this key
	$record{CPUTot} = get_variable("CPUTot",$string);
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

sub slurm_read_cpuinfo($){
    my %sinfo_cpuinfo;
    my $cpuinfo;
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



1;
