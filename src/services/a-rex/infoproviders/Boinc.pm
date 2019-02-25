package Boinc;

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
use DBI;
use POSIX qw(ceil floor);
use Sys::Hostname;
our @ISA = ('Exporter');
our @EXPORT_OK = ('cluster_info',
	      'queue_info',
	      'jobs_info',
	      'users_info');
use LogUtils ( 'start_logging', 'error', 'warning', 'debug' ); 

##########################################
# Saved private variables
##########################################

our (%lrms_queue);
our $running = undef; # total running jobs in a queue

# the queue passed in the latest call to queue_info, jobs_info or users_info
my $currentqueue = undef;

# Resets queue-specific global variables if
# the queue has changed since the last call
sub init_globals($) {
    my $qname = shift;
    if (not defined $currentqueue or $currentqueue ne $qname) {
        $currentqueue = $qname;
        %lrms_queue = ();
        $running = undef;
    }
}

##########################################
# Private subs
##########################################
sub db_conn($){
    my $config=shift;
    my $DB_HOST=$$config{boinc_db_host};
    my $DB_PORT=$$config{boinc_db_port};
    my $DB_NAME=$$config{boinc_db_name};
    my $DB_USER=$$config{boinc_db_user};
    my $DB_PASS=$$config{boinc_db_pass};
    my $dbh = DBI->connect("DBI:mysql:$DB_NAME;host=$DB_HOST:$DB_PORT","$DB_USER","$DB_PASS",{RaiseError=>1});
    return $dbh;
}

sub get_total_cpus($){
    # Total number of hosts that finished jobs recently
    my $config=shift;
    my $dbh = db_conn($config);
    my $where = "";
    if (defined($$config{boinc_app_id})) { $where = " and appid=$$config{boinc_app_id}"; }
    my $sth = $dbh->prepare('select count(distinct hostid) from result where server_state=4'.$where);
    $sth->execute();
    my $result = $sth->fetchrow_array();
    if(defined($result)){
        # Return a reasonable number to allow bootstrapping projects
        if($result < 100) {$result = 100;}
        return $result;
    }
    return 0;
}
sub get_max_cpus($){
    # Total number of hosts with running and finished jobs
    my $config=shift;
    my $dbh = db_conn($config);
    my $where = "";
    if (defined($$config{boinc_app_id})) { $where = " and appid=$$config{boinc_app_id}"; }
    my $sth = $dbh->prepare('select count(distinct hostid) from result where server_state=4'.$where);
    $sth->execute();
    my $result = $sth->fetchrow_array();
    if(defined($result)){return $result;}
    else{ return 0;}
}
sub get_jobs_in_que($){
    # Unsent jobs on BOINC server
    my $config=shift;
    my $dbh = db_conn($config);
    my $where = "";
    if (defined($$config{boinc_app_id})) { $where = " and appid=$$config{boinc_app_id}"; }
    my $sth = $dbh->prepare('select count(*) from result where server_state in (1,2)'.$where);
    $sth->execute();
    my $result = $sth->fetchrow_array();
    if(defined($result)){return $result;}
    else{ return 0;}
}
sub get_jobs_in_run($){
    # Jobs in progress
    my $config=shift;
    my $dbh = db_conn($config);
    my $where = "";
    if (defined($$config{boinc_app_id})) { $where = " and appid=$$config{boinc_app_id}"; }
    my $sth = $dbh->prepare('select count(*) from result where server_state=4'.$where);
    $sth->execute();
    my $result = $sth->fetchrow_array();
    if(defined($result)){return $result;}
    else{ return 0;}
}
sub get_jobs_status($){
    # Convert BOINC status into ARC LRMS state for each job
    my $config=shift;
    my $dbh = db_conn($config);
    my $where = "";
    if (defined($$config{boinc_app_id})) { $where = " and appid=$$config{boinc_app_id}"; }
    my $sth = $dbh->prepare('SELECT result.server_state, result.name, user.name FROM result LEFT JOIN user ON result.userid=user.id WHERE server_state in (1,2,4)'.$where);
    $sth->execute();
    my (%jobs_status, @result,$job_status, $job_state, $job_name, $user_name);
    while(($job_state, $job_name, $user_name) = $sth->fetchrow_array())
    {
        $job_status="Q";
        my @tmp=split(/_/,$job_name);
        $job_name=$tmp[0];
        if($job_state==4){$job_status="R";}
        # Strip out non-ascii
        $user_name =~ s/[^[:ascii:]]/_/g if $user_name;
        $jobs_status{$job_name}=[$job_status, $user_name];
    }
    return \%jobs_status;
}


############################################
# Public subs
#############################################

sub cluster_info ($) {
    my ($config) = shift;

    my (%lrms_cluster);

    $lrms_cluster{lrms_type} = "boinc";
    $lrms_cluster{lrms_version} = "1";

    # only enforcing per-process cputime limit
    $lrms_cluster{has_total_cputime_limit} = 0;

    my ($total_cpus) = get_total_cpus($config);
    my ($max_cpus) = get_max_cpus($config);
    $lrms_cluster{totalcpus} = $total_cpus;

    $lrms_cluster{cpudistribution} = $lrms_cluster{totalcpus}."cpu:1";

    my $que_jobs = get_jobs_in_que($config);
    my $run_jobs = get_jobs_in_run($config);
    $lrms_cluster{usedcpus} = $run_jobs;

    $lrms_cluster{runningjobs} = $lrms_cluster{usedcpus};

    $lrms_cluster{queuedcpus} = $max_cpus-$total_cpus;
    $lrms_cluster{queuedjobs} = $que_jobs;
    $lrms_cluster{queue} = [ ];
    return %lrms_cluster;
}

sub queue_info ($$) {
    my ($config) = shift;
    my ($qname) = shift;

    init_globals($qname);

    my ($total_cpus) = get_total_cpus($config);
    my ($max_cpus) = get_max_cpus($config);
    my $que_jobs = get_jobs_in_que($config);
    my $running = get_jobs_in_run($config);
    if (defined $running) {
        # job_info was already called, we know exactly how many grid jobs
        # are running
        $lrms_queue{running} = $running;

    } else {
        # assuming that the submitted grid jobs are cpu hogs, approximate
        # the number of running jobs with the number of running processes

        $lrms_queue{running}= 0;
    }
    $lrms_queue{totalcpus} = $total_cpus;

    $lrms_queue{status} = $lrms_queue{totalcpus}-$lrms_queue{running};

    # reserve negative numbers for error states
    # Fork is not real LRMS, and cannot be in error state
    if ($lrms_queue{status}<0) {
	debug("lrms_queue{status} = $lrms_queue{status}");
	$lrms_queue{status} = 0;
    }

    my $job_limit;
    $job_limit = 1000;

    $lrms_queue{maxrunning} = $job_limit;
    $lrms_queue{maxuserrun} = $job_limit;
    $lrms_queue{maxqueuable} = $job_limit;

    $lrms_queue{maxcputime} = "";

    $lrms_queue{queued} = $que_jobs;
    $lrms_queue{mincputime} = "";
    $lrms_queue{defaultcput} = "";
    $lrms_queue{minwalltime} = "";
    $lrms_queue{defaultwallt} = "";
    $lrms_queue{maxwalltime} = $lrms_queue{maxcputime};

    return %lrms_queue;
}


sub jobs_info ($$@) {
    my ($config) = shift;
    my ($qname) = shift;
    my ($jids) = shift;

    init_globals($qname);

    my (%lrms_jobs,$jstatus);
    $jstatus=get_jobs_status($config);
    foreach my $id (@$jids){
        # Real hostname will be published when job finishes
        my $host = 'unknown@unknown';
        if ($$jstatus{$id} and $$jstatus{$id}[1]) {
            $host = $$jstatus{$id}[1].'@unknown';
        }
        $lrms_jobs{$id}{nodes} = [ $host ];
        $lrms_jobs{$id}{mem} = 2000000000;
        $lrms_jobs{$id}{walltime} = "";
        $lrms_jobs{$id}{cputime} = "";
        $lrms_jobs{$id}{comment} = [ "LRMS: Running under boinc" ];

        $lrms_jobs{$id}{reqwalltime} = "";
        $lrms_jobs{$id}{reqcputime} = "";   
        $lrms_jobs{$id}{rank} = "0";
        # Fix cores to 1 since volunteers download 1 task per core
        $lrms_jobs{$id}{cpus} = "1"; 
        #if (! exists $$jstatus{$id} || $$jstatus{$id}[1] == 0) {
        #    $lrms_jobs{$id}{cpus} = "1"; 
        #} else {
        #    $lrms_jobs{$id}{cpus} = "$$jstatus{$id}[1]"; 
        #} 
        if(! exists $$jstatus{$id}) {
            $lrms_jobs{$id}{status} = "O";
        } elsif($$jstatus{$id}[0] eq "R") {
            $lrms_jobs{$id}{status} = "R";
        } elsif($$jstatus{$id}[0] eq "Q") {
            $lrms_jobs{$id}{status} = "Q";
        } else {
            $lrms_jobs{$id}{status} = "O";
        }
    }

    return %lrms_jobs;
}


sub users_info($$@) {
    my ($config) = shift;
    my ($qname) = shift;
    my ($accts) = shift;

    init_globals($qname);

    my (%lrms_users);

    # freecpus
    # queue length

    if ( ! exists $lrms_queue{status} ) {
	%lrms_queue = queue_info( $config, $qname );
    }
    
    foreach my $u ( @{$accts} ) {
	$lrms_users{$u}{freecpus} = $lrms_queue{maxuserrun} - $lrms_queue{running};        
	$lrms_users{$u}{queuelength} = "$lrms_queue{queued}";
    }
    return %lrms_users;
}
	      
1;
