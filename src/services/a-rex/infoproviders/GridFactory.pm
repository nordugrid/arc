package GridFactory;

use File::Basename;
use lib dirname($0);
@ISA = ('Exporter');
@EXPORT_OK = ('cluster_info',
	      'queue_info',
	      'jobs_info',
	      'users_info');
use LogUtils ( 'start_logging', 'error', 'warning', 'debug' ); 
use strict;

##########################################
# Saved private variables
##########################################

our (%lrms_queue);

##########################################
# Private subs
##########################################

sub totalcpus {
    # number of cpus is calculated from the /proc/cpuinfo,
    # by default it is set to 1
    
    my ($cpus) = 0;
    unless (open CPUINFOFILE, "</proc/cpuinfo") {
	error("can't read the /proc/cpuinfo");
    }   
    while(my $line = <CPUINFOFILE>) {
	chomp($line);
	if ($line =~ m/^processor/) {
	    $cpus++;	
	}
    }
    close CPUINFOFILE;
    $cpus || 10;
    return $cpus;
}

sub get_long_job_info ($$$) {

    # Path to LRMS commands
    my ($path) = shift;
    # Name of the queue to query
    my ($queue) = shift;
    # LRMS job IDs from Grid Manager (jobs with "INLRMS" GM status)
    my ($lrms_ids) = @_;

    unless (open PSTATOUT,  'su -c "$path/pstat -b localhost -f dummy $lrms_ids" - apache|') {
	    error("Error in executing pstat");
    }

    my $jobid;
    my %jobinfo;

    while (<PSTATOUT>) {
	chomp;
	# Drop separator lines
	next if /^---/;
	my ($par, $val) = split/\s*:\s*/,$_,2;

	if ($par eq "Job definition ID") {
	    $jobid = $val;
	}
	$jobinfo{$jobid}{$par} = $val;
    }

    close PSTATOUT;
    return %jobinfo;
}

############################################
# Public subs
#############################################

sub cluster_info ($) {
    my ($config) = shift;

    my (%lrms_cluster);

    $lrms_cluster{lrms_type} = "gridfactory";
    $lrms_cluster{lrms_version} = "";
    $lrms_cluster{totalcpus} = totalcpus();
    $lrms_cluster{cpudistribution} = $lrms_cluster{totalcpus}."cpu:1";
    $lrms_cluster{runningjobs} = 0; 
    $lrms_cluster{usedcpus}  = 0;
    $lrms_cluster{queuedcpus} = 0;
    $lrms_cluster{queuedjobs} = 0;   
    $lrms_cluster{queue} = [ ];     

    return %lrms_cluster;
}

sub queue_info ($$) {

    my ($config) = shift;
    my ($qname) = shift;

    # $lrms_queue{running} (number of active jobs in a gridfactory system)
    # is calculated by making use of the 'ps axr' command

    $lrms_queue{running}= 0;
    unless (open PSCOMMAND,  "ps axr |") {
	error("error in executing ps axr");
    }
    while(my $line = <PSCOMMAND>) {
	chomp($line);
	next if ($line =~ m/PID TTY/);
	next if ($line =~ m/ps axr/);
	next if ($line =~ m/cluster-gridfactory/);     
	$lrms_queue{running}++;
    }
    close PSCOMMAND;

    $lrms_queue{totalcpus} = totalcpus();

    $lrms_queue{status} = $lrms_queue{totalcpus}-$lrms_queue{running};

    # reserve negative numbers for error states
    # GridFactory is not real LRMS, and cannot be in error state
    if ($lrms_queue{status}<0) {
	debug("lrms_queue{status} = $lrms_queue{status}");
	$lrms_queue{status} = 0;
    }

    # maxcputime

    unless ( $lrms_queue{maxcputime} = `/bin/sh -c "ulimit -t"` ) {
	debug("Could not determine max cputime with ulimit -t");
	$lrms_queue{maxcputime} = "";
    };
    chomp $lrms_queue{maxcputime};

    # queued
    $lrms_queue{queued} = 0;
    $lrms_queue{maxqueuable} = "";
      
    $lrms_queue{maxrunning} = "";
    $lrms_queue{maxuserrun} = $lrms_queue{maxrunning};
    $lrms_queue{mincputime} = "";
    $lrms_queue{defaultcput} = "";
    $lrms_queue{minwalltime} = "";
    $lrms_queue{defaultwallt} = "";
    $lrms_queue{maxwalltime} = $lrms_queue{maxcputime};

    return %lrms_queue;
}


sub jobs_info ($$$) {
    # Path to LRMS commands
    my ($config) = shift;
    my ($path) = $$config{gridfactory_bin_path};

    # Name of the queue to query
    my ($queue) = shift;

    # LRMS job IDs from Grid Manager (jobs with "INLRMS" GM status)
    my ($lrms_ids) = @_;

    my %jobinfo;

    if ( (@{$lrms_ids})==0){
      return %jobinfo;
    }

    my $lrmsidstr = join(" ", @{$lrms_ids});

    %jobinfo = get_long_job_info($path,"",$lrmsidstr);

    # Return data structure %lrms_jobs{$lrms_local_job_id}{$keyword}
    # should contain the keywords listed in LRMS.pm:
    # status        Status of the job: Running 'R', Queued'Q',
    #                                  Suspended 'S', Exiting 'E', Other 'O'
    # rank          Position in the queue
    # mem           Used (virtual) memory
    # walltime      Used walltime
    # cputime       Used cpu-time
    # reqwalltime   Walltime requested from LRMS
    # reqcputime    Cpu-time requested from LRMS
    # nodes         List of execution hosts. (one host in case of serial jobs)
    # cpus          Number of cpus used by job           
    # comment       Comment about the job in LRMS, if any

    # lrms_jobs: ass array containing above expr
    my (%lrms_jobs);

    # s: array used for time formats
    #my (@s);

    foreach my $id (keys %jobinfo) {

        # Default status is "Other"
        $lrms_jobs{$id}{status} = "O";
	# Drop sub-status
	$jobinfo{$id}{Status} =~ s/:.*//;
        if (
            $jobinfo{$id}{Status} eq "ready" ||
            $jobinfo{$id}{Status} eq "requested" ||
            $jobinfo{$id}{Status} eq "prepared"
            ) {
            $lrms_jobs{$id}{status} = "Q";
        }
        if (
            $jobinfo{$id}{Status} eq "submitted" ||
            $jobinfo{$id}{Status} eq "running"
            ) {
            $lrms_jobs{$id}{status} = "R";
        }
        if (
            $jobinfo{$id}{Status} eq "executed" ||
            $jobinfo{$id}{Status} eq "uploaded" ||
            $jobinfo{$id}{Status} eq "requestKill" ||
            $jobinfo{$id}{Status} eq "aborted" ||
            $jobinfo{$id}{Status} eq "failed" ||
            $jobinfo{$id}{Status} eq "done"
            ) {
            $lrms_jobs{$id}{status} = "E";
        }
        if (
            $jobinfo{$id}{Status} eq "paused"
            ) {
            $lrms_jobs{$id}{status} = "S";
        }

	#$lrms_jobs{$id}{status} = "R";
        $lrms_jobs{$id}{mem} = "";
        my $dispt = `date +%s -d "$jobinfo{$id}{'Job record creation time'}"`;
        chomp $dispt;
        $lrms_jobs{$id}{walltime} = POSIX::ceil((time() - $dispt) /60);
        $lrms_jobs{$id}{cputime} = "";
        $lrms_jobs{$id}{reqwalltime} = "";
        $lrms_jobs{$id}{reqcputime} = "";
        $lrms_jobs{$id}{comment} = [ "LRMS: $jobinfo{$id}{Status}" ];

        $lrms_jobs{$id}{rank} = "0";
        $lrms_jobs{$id}{cpus} = "1";

        # We need workernode hostname
        my $node = "localhost";
        $lrms_jobs{$id}{nodes} = [ $node ];

    }

    return %lrms_jobs;

}


sub users_info($$@) {
    # Path to LRMS commands
    my ($config) = shift;
    #my ($path) = $$config{pbs_bin_path};

    # Name of the queue to query
    my ($qname) = shift;

    # Unix user names mapped to grid users
    my ($accts) = shift;

    # Return data structure %lrms_users{$unix_local_username}{$keyword}
    # should contain the keywords listed in LRMS.pm:
    # freecpus
    # queue length
    my (%lrms_users);


    if ( ! exists $lrms_queue{status} ) {
	%lrms_queue = queue_info( $config, $qname );
    }
    
    my $job_limit;
    if ( ! exists $$config{gridfactory_job_limit} ) {
       $job_limit = 1;
    }
    elsif ($$config{gridfactory_job_limit} eq "cpunumber") {
        $job_limit = $lrms_queue{totalcpus};
    }
    else {
       $job_limit = $$config{gridfactory_job_limit};
    }
    foreach my $u ( @{$accts} ) {
	$lrms_users{$u}{freecpus} = $job_limit - $lrms_queue{running};        
	$lrms_users{$u}{queuelength} = "$lrms_queue{queued}";
    }
    return %lrms_users;
}
	      
1;
