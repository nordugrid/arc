package Condor;

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
use POSIX;
our @ISA = ('Exporter');
our @EXPORT_OK = ('cluster_info',
            'queue_info',
            'jobs_info',
            'users_info');
use LogUtils ( 'start_logging', 'error', 'warning', 'debug' ); 
use condor_env;

##########################################
# Saved private variables
##########################################

# contains the requirements string for the current queue.
# It is used by queue-aware functions
my $qdef = '';

my %config = ();
my $arcconf = $ENV{ARC_CONFIG} ? $ENV{ARC_CONFIG} : '/etc/arc.conf';

my %lrms_queue;
my $lrms_queue_initialized = 0;
my @allnodedata = ();
my $allnodedata_initialized = 0;
my %alljobdata = ();
my $alljobdata_initialized = 0;
my @queuenodes = ();
my $queuenodes_initialized = 0;
my @jobids_thisqueue = ();
my @jobids_otherqueue = ();


##########################################
# Private subs
##########################################


# Runs a command. Returns a list of three values:
#
# [0] String containing stdout.
# [1] String containing stderr.
# [2] Program exit code ($?) that was returned to the shell.

sub condor_run($) {
    my $command = shift;
    my $stderr_file = "/tmp/condor_run.$$";
    my $stdout = `$ENV{CONDOR_BIN_PATH}/$command 2>$stderr_file`;
    debug "===condor_run: $command";
    my $ret = $? >> 8;
    local (*ERROR, $/);
    open ERROR, "<$stderr_file";
    my $stderr = <ERROR>;
    close ERROR;
    unlink $stderr_file;
    return $stdout, $stderr, $ret;
}


# String containing LRMS version.  ('UNKNOWN' in case of errors.)
sub type_and_version() {
    my ($out, $err, $ret) = condor_run('condor_version');
    return 'UNKNOWN' if $ret != 0;
    $out =~ /\$CondorVersion:\s+(\S+)/;
    my $version = $1 || 'UNKNOWN';
    my $type = 'Condor';
    return $type, $version;
}

#
# Helper funtion which collects all the information about condor nodes.
#
sub collect_node_data() {
    return if $allnodedata_initialized;
    $allnodedata_initialized = 1;
    my ($out, $err, $ret) = condor_run('condor_status -format "Name = %V\n" Name -format "Machine = %V\n" Machine -format "State = %V\n" State -format "Cpus = %V\n" Cpus -format "TotalCpus = %V\n" TotalCpus -format "SlotType = %V\n\n" SlotType');
    error("Failed collecting node information.") if $ret;
    for (split /\n\n+/, $out) {
        my %target = condor_digest_classad($_);
        next unless defined $target{machine};
        push @allnodedata, \%target;
    }
    debug "===collect_node_data: " . join " ", (map { $$_{machine} } @allnodedata);
}

#
# Helper funtion which collects all the information about condor jobs.
#
sub collect_job_data() {
    return if $alljobdata_initialized;
    $alljobdata_initialized = 1;
    $ENV{_condor_CONDOR_Q_ONLY_MY_JOBS}='false';
    my ($out, $err, $ret) = condor_run('condor_q -format "ClusterId = %V\n" ClusterId -format "ProcId = %V\n" ProcId -format "JobStatus = %V\n" JobStatus -format "CurrentHosts = %V\n" CurrentHosts -format "LastRemoteHost = %V\n" LastRemoteHost -format "RemoteHost = %V\n" RemoteHost -format "ImageSize = %V\n" ImageSize -format "RemoteWallClockTime = %V\n" RemoteWallClockTime -format "RemoteUserCpu = %V\n" RemoteUserCpu -format "RemoteSysCpu = %V\n" RemoteSysCpu -format "JobTimeLimit = %V\n" JobTimeLimit -format "JobCpuLimit = %V\n" JobCpuLimit -format "HoldReasonCode = %V\n\n" HoldReasonCode');
    return if $out =~ m/All queues are empty/;
    error("Failed collecting job information.") if $ret;
    for (split /\n\n+/, $out) {
        my %job = condor_digest_classad($_);
        next unless defined $job{clusterid};
        $job{procid} = "0" unless $job{procid};
        my $jobid = "$job{clusterid}.$job{procid}";
        $alljobdata{$jobid} = \%job;
    }
    debug "===collect_job_data: " . (join " ", keys %alljobdata);
}

#
# Scans grid-manager's controldir for jobs in LRMS state belonging to a
# queue.  Returns a list of their Condor jobids: clusterid.0 (assumes
# procid=0).
#
sub collect_jobids($$) {
    my %pairs;
    my $qname = shift;
    my $controldir = shift;
    my $cmd = "find $controldir/processing -maxdepth 1 -name '?*.status'";
    $cmd   .= ' | xargs grep -l INLRMS ';
    $cmd   .= ' | sed \'s/^.*\/\([^\.]*\)\.status$/\1/\' ';
    $cmd   .= ' | sed -e \'s#\(.\{3\}\)#\1/#3\' -e \'s#\(.\{3\}\)#\1/#2\' -e \'s#\(.\{3\}\)#\1/#1\' -e \'s#$#/local#\' ';
    $cmd   .= " | sed 's\\^\\$controldir/jobs/\\' ";
    $cmd   .= ' | xargs grep -H "^queue=\|^localid="';
    local *LOCAL;
    open(LOCAL, "$cmd |");
    while (<LOCAL>) {
       m#/job\.(\w{10,})\.local:queue=(\S+)# && ($pairs{$1}{queue} = $2);
       m#/job\.(\w{10,})\.local:localid=(\S+)# && ($pairs{$1}{id} = $2);
    }
    close LOCAL;
    foreach my $pair (values %pairs) {
        # get rid of .condor from localid.
        $$pair{id} =~ s/(\d+)\..*/$1.0/;
        if ( $$pair{queue} eq $qname ) {
            push @jobids_thisqueue, $$pair{id};
        } else {
            push @jobids_otherqueue, $$pair{id};
        }
    }
    debug "===collect_jobids: thisqueue: @jobids_thisqueue";
    debug "===collect_jobids: otherqueue: @jobids_otherqueue";
}

#
# Returns a job's rank (place) in the current queue, or False if job is not in
# current queue. Highest rank is 1.  The rank is deduced from jobid, based on
# the assumption that jobs are started sequentially by Condor.
# Input:  jobid (of the form: clusterid.0)
#
sub rank($) {
    my $id = shift;
    my $rank = 0;
    # only calculate rank for queued jobs
    return 0 unless exists $alljobdata{$id};
    return 0 unless $alljobdata{$id}{lc 'JobStatus'} == 1;
    foreach (@jobids_thisqueue) {
        # only include queued jobs in rank
        next unless exists $alljobdata{$_};
        next unless $alljobdata{$_}{lc 'JobStatus'} == 1;
        $rank++;
        last if $id eq $_;
    }
    #debug "===rank($id) = $rank";
    return $rank;
}

#
# Parses long output from condor_q -l
# and condor_status -l into and hash.
# OBS: Field names are lowercased!
# OBS: It removes quotes around strings
#
sub condor_digest_classad($) {
    my %classad;
    for (split /\n+/, shift) {
        next unless /^(\w+)\s*=\s*(.*\S|)\s*$/;
        my ($field, $val) = ($1, $2);
        $val =~ s/"(.*)"/$1/; # remove quotes, if any
        $classad{lc $field} = $val;
    }
    return %classad;
}

#
# Takes an optional constraint description string and returns the names of the
# nodes which satisfy this contraint. If no constraint is given, returns all
# the nodes in the Condor pool
#
sub condor_grep_nodes {
    my $req = shift;
    my $cmd = 'condor_status -format "%s\n" Machine';
    $cmd .= " -constraint '$req'" if $req;
    my ($out, $err, $ret) = condor_run($cmd);
    debug "===condor_grep_nodes: ". (join ', ', split /\n/, $out);
    return () if $ret;
    return split /\n/, $out;
}

#
# Takes one argument:
# 1. The LRMS job id as represented in the GM.  (In Condor terms,
#    it's <clusterid>.<proc>)
#
# Returns the current status of the job by mapping Condor's JobStatus
# integer into corresponding one-letter codes used by ARC:
#
#   1 (Idle)       --> Q (job is queuing, waiting for a node, etc.)
#   2 (Running)    --> R (running on a host controlled by the LRMS)
#   2 (Suspended)  --> S (an already running job in a suspended state)
#   3 (Removed)    --> E (finishing in the LRMS)
#   4 (Completed)  --> E (finishing in the LRMS)
#   5 (Held)       --> H --> S ( some jobs are stuck in the queue)
#                  --> H --> O if HoldReasonCode == 16  jobs are datastaging
#   6 (Transfer)   --> O (other, almost finished. Transferring output.)
#   7 (Suspended)  --> S (newer condor version support suspended)
#
# If the job couldn't be found, E is returned since it is probably finished.
#
sub condor_get_job_status($) {
    my $id = shift;
    my %num2letter = qw(1 Q 2 R 3 E 4 E 5 H 6 O 7 S);
    return 'E' unless $alljobdata{$id};
    my $s = $alljobdata{$id}{jobstatus};
    return 'E' if !defined $s;
    $s = $num2letter{$s};
    if ($s eq 'R') {
        $s = 'S' if condor_job_suspended($id);
    }
    # Takes care of HOLD jobs
    if ($s eq 'H') {
        $s = condor_job_hold_isstaging($id) ? 'O' : 'S';
    }

    debug "===condor_get_job_status $id: $s";
    return $s;
}

#
# Returns the list of nodes belonging to the current queue
#
sub condor_queue_get_nodes() {
    return @queuenodes if $queuenodes_initialized;
    $queuenodes_initialized = 1;
    @queuenodes = condor_grep_nodes($qdef);
    debug "===condor_queue_get_nodes @queuenodes";
    return @queuenodes;
}

#
# Count queued jobs (idle or held) within the current queue.
#
sub condor_queue_get_queued() {
    my $gridqueued = 0;
    my $localqueued = 0;
    my $qfactor = 0;
    if(condor_cluster_totalcpus() != 0){
        $qfactor = condor_queue_get_nodes() / condor_cluster_totalcpus();
    }
    for (values %alljobdata) {
        my %job = %$_;
        # only include jobs which are idle. 
        # TODO: Held jobs (ID=5) removed upon WLCG request, maybe they need to be counted elsewhere
        next unless $job{jobstatus} == 1;
        my $clusterid = "$job{clusterid}.$job{procid}";
        if (grep { $_ eq $clusterid } @jobids_thisqueue) {
            $gridqueued += 1;
        } elsif (grep { $_ eq $clusterid } @jobids_otherqueue) {
            # this is a grid job, but in a different queue
        } else {
            $localqueued += 1;
        }
    }
    # for locally queued jobs, we don't know to which queue it belongs
    # try guessing the odds
    my $total = $gridqueued + int($localqueued * $qfactor);
    debug "===condor_queue_get_queued: $total = $gridqueued+int($localqueued*$qfactor)";
    return $total;
}

#
# Counts all queued cpus (idle) in the cluster.
# TODO: this counts jobs, not cpus.
# TODO: Held jobs (ID=5) removed upon WLCG request, maybe they need to be counted elsewhere
sub condor_cluster_get_queued_cpus() {
    my $sum = 0;
    do {$sum++ if $$_{jobstatus} == 1} for values %alljobdata;
    debug "===condor_cluster_get_queued_cpus: $sum";
    return $sum;
}

#
# Counts all queued jobs (idle) in the cluster.
#
sub condor_cluster_get_queued_jobs() {
    my $sum = 0;
    do {$sum++ if $$_{jobstatus} == 1} for values %alljobdata;
    debug "===condor_cluster_get_queued_jobs: $sum";
    return $sum;
}

# Counts all held jobs (ID=5) in the cluster.
#
sub condor_cluster_get_held_jobs() {
    my $sum = 0;
    do {$sum++ if $$_{jobstatus} == 5} for values %alljobdata;
    debug "===condor_cluster_get_held_jobs: $sum";
    return $sum;
}

#
# Counts all running jobs in the cluster.
# TODO: also counts suspended jobs apparently.
# only counts suspended jobs in earlier versions of Condor
# Newer versions have separate state (7) for supended jobs

sub condor_cluster_get_running_jobs() {
    my $sum = 0;
    do {$sum++ if $$_{jobstatus} == 2} for values %alljobdata;
    debug "===condor_cluster_get_running_jobs: $sum";
    return $sum;
}

#
# Counts nodes in the current queue with state other than 'Unclaimed'
# Every running job is automatically included, plus nodes used
# interactively by their owners
#
sub condor_queue_get_running() {
    my $running = 0;
    my @qnod = condor_queue_get_nodes();

    for (@allnodedata)
    {
        my %node = %$_;
        next unless grep { $_ eq $node{machine} } @qnod;
        $running += $node{cpus} if ($node{slottype} !~ /^Partitionable/i && $node{state} !~ /^Unclaimed/i);
    }

    debug "===condor_queue_get_running: $running";
    return $running;
}

#
# Same as above, but for the whole cluster
#
sub condor_cluster_get_usedcpus() {
    my $used = 0;
    for (@allnodedata)
    {
       $used += $$_{cpus} if ($$_{slottype} !~ /^Partitionable/i && $$_{state} !~ /^Unclaimed/i);
    }
    debug "===condor_cluster_get_usedcpus: $used";
    return $used;
}

#
# returns the total number of CPUs in the cluster
#
sub condor_queue_totalcpus() {
    my @qnod = condor_queue_get_nodes();

    # List all machines in the pool. Create a hash specifying the TotalCpus
    # for each machine.
    my %machines;
    $machines{$$_{machine}} = $$_{totalcpus} for @allnodedata;

    my $totalcpus = 0;
    for (keys %machines) {
        my $machine = $_;
        next unless grep { $machine eq $_ } @qnod;
        $totalcpus += $machines{$_};
    }

    return $totalcpus;
}

#
# returns the total number of nodes in the cluster
#
sub condor_cluster_totalcpus() {
    # List all machines in the pool. Create a hash specifying the TotalCpus
    # for each machine.
    my %machines;
    $machines{$$_{machine}} = $$_{totalcpus} for @allnodedata;

    my $totalcpus = 0;
    for (keys %machines) {
        $totalcpus += $machines{$_};
    }

    return $totalcpus;
}

#
# This function parses the condor log to see if the job has been suspended.
# (condor_q reports 'R' for running even when the job is suspended, so we need
# to parse the log to be sure that 'R' actually means running.)
#
# Argument: the condor job id
# Returns: true if the job is suspended, and false if it's running.
#

sub condor_job_suspended($) {
    my $id = shift;
    return 0 unless $alljobdata{$id};
    my $logfile = $alljobdata{$id}{lc 'UserLog'};
    return 0 unless $logfile;
    local *LOGFILE;
    open LOGFILE, "<$logfile" or return 0;
    my $suspended = 0;
    while (my $line = <LOGFILE>) {
        $suspended = 1 if $line =~ /Job was suspended\.$/;
        $suspended = 0 if $line =~ /Job was unsuspended\.$/;
    }
    close LOGFILE;
    return $suspended;
}

#
# This function parses the condor log to see if a job in HOLD state
# has been kept in HOLD because HoldReasonCode == 16.
# In this case the job is staging so it should not be discarded.
#
# Argument: the condor job id
# Returns: E if the job is in a terminal state, O if not.
#

sub condor_job_hold_isstaging($) {
    my $id = shift;
    return 0 unless $alljobdata{$id};
    return 1 if ($alljobdata{$id}{lc 'HoldReasonCode'} == '16');
    
    # E state means the job will not exit the HOLD state.
    # O state means the job can be out of the HOLD state.
    # If HoldReasonCode == 16 --> staging --> O
    my $substate = 'E';
    $substate = 'O' if $alljobdata{$id}{lc 'HoldReasonCode'} == '16';
    return $substate;
}

#
# CPU distribution string (e.g., '1cpu:5 2cpu:1').
#
sub cpudistribution {
    # List all machines in the pool. Create a hash specifying the TotalCpus
    # for each machine.
    my %machines;
    $machines{$$_{machine}} = $$_{totalcpus} for @allnodedata;

    # Count number of machines with one CPU, number with two, etc.
    my %dist;
    for (keys %machines) {
        $dist{$machines{$_}}++;
    }

    # Generate CPU distribution string.
    my $diststr = '';
    for (sort { $a <=> $b } keys %dist) {
        $diststr .= ' ' unless $diststr eq '';
        $diststr .= "${_}cpu:$dist{$_}";
    }

    return $diststr;
}




############################################
# Public subs
#############################################

sub cluster_info ($) {
    my $config = shift;

    my %lrms_cluster;

    configure_condor_env(%$config) or error("Condor executables (in condor_bin_path) or config file (condor_config) not found, check configuration. Exiting...");

    collect_node_data();
    collect_job_data();

    ( $lrms_cluster{lrms_type}, $lrms_cluster{lrms_version} ) = type_and_version();

    # not sure how Condor counts RemoteUserCpu and RemoteSysCpu but it should
    # not matter anyway since we don't support parallel jobs under Condor
    $lrms_cluster{has_total_cputime_limit} = 0;

    # Count used/free CPUs and queued jobs in the cluster
    
    # Note: SGE has the concept of "slots", which roughly corresponds to
    # concept of "cpus" in ARC (PBS) LRMS interface.

    $lrms_cluster{totalcpus} = condor_cluster_totalcpus();
    $lrms_cluster{cpudistribution} = cpudistribution();
    $lrms_cluster{usedcpus} = condor_cluster_get_usedcpus();
    #NOTE: counts jobs, not cpus.
    $lrms_cluster{queuedcpus} = condor_cluster_get_queued_cpus();

    $lrms_cluster{queuedjobs} = condor_cluster_get_queued_jobs();
    $lrms_cluster{runningjobs} = condor_cluster_get_running_jobs();


    # List LRMS queues.
    # This does not seem to be used in cluster.pl!
    
    @{$lrms_cluster{queue}} = ();

    return %lrms_cluster;
}


sub queue_info ($$) {
    return %lrms_queue if $lrms_queue_initialized;
    $lrms_queue_initialized = 1;

    my $config = shift;
    my $qname = shift;

    $qdef = join "", split /\[separator\]/, ($$config{condor_requirements} || '');
    warning("Option 'condor_requirements' is not defined for queue $qname") unless $qdef;
    debug("===Requirements for queue $qname: $qdef");
    
    configure_condor_env(%$config) or error("Condor executables (in condor_bin_path) or config file (condor_config) not found, check configuration. Exiting...");

    collect_node_data();
    collect_job_data();
    collect_jobids($qname, $$config{controldir});

    # Number of available (free) cpus can not be larger that
    # free cpus in the whole cluster
    my $totalcpus = condor_queue_totalcpus();
    my $usedcpus = condor_queue_get_running();
    my $queuedcpus = condor_queue_get_queued();

    $lrms_queue{freecpus} = $totalcpus - $usedcpus;
    $lrms_queue{running} = $usedcpus;
    $lrms_queue{totalcpus} = $totalcpus;
    # In theory any job in some circumstances can consume all available slots
    $lrms_queue{MaxSlotsPerJob} = $totalcpus;
    $lrms_queue{queued} = $queuedcpus;

    # reserve negative numbers for error states 
    if ($lrms_queue{freecpus}<0) {
        warning("lrms_queue{freecpus} = $lrms_queue{freecpus}")
    }

    # nordugrid-queue-maxrunning
    # nordugrid-queue-maxqueuable
    # nordugrid-queue-maxuserrun
    # nordugrid-queue-mincputime
    # nordugrid-queue-defaultcputime
    $lrms_queue{maxrunning} = $totalcpus;
    $lrms_queue{maxqueuable} = 2 * $lrms_queue{maxrunning};
    $lrms_queue{maxuserrun} = $lrms_queue{maxrunning};

    $lrms_queue{maxwalltime} = '';
    $lrms_queue{minwalltime} = '';
    $lrms_queue{defaultwallt} = '';
    $lrms_queue{maxcputime} = '';
    $lrms_queue{mincputime} = '';
    $lrms_queue{defaultcput} = '';

    $lrms_queue{status} = 1;

    return %lrms_queue;
}


sub jobs_info ($$@) {
    my $config = shift;
    my $qname = shift;
    my $jids = shift;

    my %lrms_jobs;

    queue_info($config, $qname);

    foreach my $id ( @$jids ) {
        # submit-condor-job might return identifiers of the form ClusterId.condor
        # Replace .hostname with .0. It is safe to assume that ProcId is 0 because
        # we only submit one job at a time.
        my $id0 = $id;
        $id0 =~ s/(\d+)\..*/$1.0/;
        debug "===jobs_info: Mapping $id to $id0";

        if ( $alljobdata{$id0} ) {
            my %job = %{$alljobdata{$id0}};
            $lrms_jobs{$id}{status} = condor_get_job_status($id0);
            $lrms_jobs{$id}{mem} = $job{lc 'ImageSize'};
            $lrms_jobs{$id}{walltime} = floor($job{lc 'RemoteWallClockTime'} / 60);
            $lrms_jobs{$id}{cputime} = floor(($job{lc 'RemoteUserCpu'} + $job{lc 'RemoteSysCpu'}) / 60);
            $lrms_jobs{$id}{nodes} = [];
            $lrms_jobs{$id}{nodes} = [$job{lc 'LastRemoteHost'}] if ($job{lc 'LastRemoteHost'} ne "undefined");
            $lrms_jobs{$id}{nodes} = [$job{lc 'RemoteHost'}] if ($job{lc 'RemoteHost'} ne "undefined");
            if ($job{lc 'JobTimeLimit'} ne "undefined") {
                $lrms_jobs{$id}{reqwalltime} = floor($job{lc 'JobTimeLimit'} / 60); # caller knows these better
            }
            if ($job{lc 'JobCpuLimit'} ne "undefined") {
                $lrms_jobs{$id}{reqcputime} = floor($job{lc 'JobCpuLimit'} / 60); # caller knows these better
            }
            $lrms_jobs{$id}{rank} = rank($id0) ? rank($id0) : '';
            $lrms_jobs{$id}{comment} = []; # TODO
            $lrms_jobs{$id}{cpus} = $job{lc 'CurrentHosts'};

            # For queued jobs, unset meanigless values
            if ($lrms_jobs{$id}{status} eq 'Q') {
                $lrms_jobs{$id}{mem} = '';
                $lrms_jobs{$id}{walltime} = '';
                $lrms_jobs{$id}{cputime} = '';
                $lrms_jobs{$id}{nodes} = [];
            }
        } else {
            # Job probably already finished
            debug("===Condor job $id not found. Probably it has finished");
            $lrms_jobs{$id}{status} = '';
            $lrms_jobs{$id}{mem} = '';
            $lrms_jobs{$id}{walltime} = '';
            $lrms_jobs{$id}{cputime} = '';
            $lrms_jobs{$id}{reqwalltime} = '';
            $lrms_jobs{$id}{reqcputime} = '';
            $lrms_jobs{$id}{rank} = '';
            $lrms_jobs{$id}{nodes} = [];
            $lrms_jobs{$id}{comment} = [];
        }
    } 

    return %lrms_jobs;
}


sub users_info($$@) {
    my $config = shift;
    my $qname = shift;
    my $accts = shift;

    my %lrms_users;

    queue_info($config, $qname);

    foreach my $u ( @{$accts} ) {
        # all users are treated as equals
        # there is no maximum walltime/cputime limit in Condor
    $lrms_users{$u}{freecpus} = $lrms_queue{freecpus};
    $lrms_users{$u}{queuelength} = "$lrms_queue{queued}";
    }
    return %lrms_users;
}


1;
