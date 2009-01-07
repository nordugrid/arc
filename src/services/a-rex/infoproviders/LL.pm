package LL;

@ISA = ('Exporter');

# Module implements these subroutines for the LRMS interface

@EXPORT_OK = ('cluster_info',
              'queue_info',
              'jobs_info',
              'users_info');

use LogUtils ( 'start_logging', 'error', 'warning', 'debug' ); 
use strict;

##########################################
# Saved private variables
##########################################

our(%lrms_queue);

##########################################
# Private subs
##########################################

sub consumable_distribution ($$) {

    my ( $path ) = shift;
    my ( $consumable_type ) = shift;
    my $used = 0;
    my $max  = 0;
    
    unless (open LLSTATUSOUT,  "$path/llstatus -R|") {
	error("Error in executing llstatus");
    }

    my @cons_dist = ();
    while (<LLSTATUSOUT>) {
	if (/^.*$consumable_type.*/) {
	    /[^# ]*(#*) *.*$consumable_type.([0-9]*),([0-9]*).*/;
	    # Check if node is down
	    if ( $1 ne "#" ) {
		my @a = ($3 - $2,$3);
		push @cons_dist, [ @a ];
	    }
	}
    }
    return @cons_dist;
}

sub consumable_total (@) {

    my @dist = @_;
    my ($cpus, $used, $max);
    foreach $cpus (@{dist}) {
	$used += ${$cpus}[0];
        $max  += ${$cpus}[1];
    }
    return ($used,$max)	
}

sub cpudist2str (@) {

    my @dist = @_;
    my @total_dist = ();
    my $str = '';

    # Collect number of available cores
    my ($used,$max,$cpus);
    foreach $cpus (@dist) {
	($used, $max) = @{$cpus};
	$total_dist[$max]++;
    }

    # Turn it into a string
    my $n;
    $n = 0;
    foreach $cpus (@total_dist) {
	if ($cpus ne "") {
	    if ( $str ne '') {
		$str .= ' ';
	    }
	    $str .= $n . "cpu:" . $cpus;
	}
	$n++;
    }
    return $str;
}

sub get_cpu_distribution($) { 

	my ( $path ) = shift;

	my $single_job_per_box = 1;

        if ($single_job_per_box == 1) {

        # Without hyperthreading
	    unless (open LLSTATUSOUT,  "$path/llstatus -f %sta|") {
		error("Error in executing llstatus");
	    }

	} else {

        # Use all available cpus/cores including hyperthreading:
	    unless (open LLSTATUSOUT,  "$path/llstatus -r %cpu %sta|") {
		error("Error in executing llstatus");
	    }
	}

	my %cpudist;
	while (<LLSTATUSOUT>) {
		chomp;
		# We only want CPU lines (and there must be at least one)
		next if !/^[1-9]/;
		# An empty line denotes end of CPUs
		last if /^$/;

		my $cpus;
		my $startd;

		if ($single_job_per_box == 1) {
		    ($startd) = split/\!/;
		    $cpus = 1;
		} else {
		    ($cpus, $startd) = split/\!/;
		}

		# Only count those machines which have startd running
		if ($startd ne "0") {
		    $cpudist{$cpus}++;
		}
	}

	close LLSTATUSOUT;

	return %cpudist;
}

sub get_used_cpus($) { 

	my ( $path ) = shift;

	unless (open LLSTATUSOUT,  "$path/llstatus |") {
		error("Error in executing llstatus");
    	}

	my $cpus_used;

	while (<LLSTATUSOUT>) {
		chomp;
		# We only want CPU lines (and there must be at least one)
		next if !/^Total Machines/;
		tr / //s;
		my @fields = split;
		$cpus_used   = $fields[6];
		last;
	}

	close LLSTATUSOUT;

	return ($cpus_used);
}

sub get_long_status($) { 

	my ( $path ) = shift;

	unless (open LLSTATUSOUT,  "$path/llstatus -l |") {
		error("Error in executing llstatus");
	}

	my %cpudist;
	my $machine_name;
	my %machines;

	while (<LLSTATUSOUT>) {
		# Discard trailing information separated by a newline
		if ( /^$/ ) {
		    next;
		}
		chomp;
		my ($par, $val) = split/\s*=\s*/,$_,2;
		if ($par eq 'Name') {
		    $machine_name=$val;
		    next;
		}
		$machines{$machine_name}{$par} = $val;
	}
	close LLSTATUSOUT;

	return %machines;
}


sub get_long_queue_info($$) { 

	my ( $path ) = shift;
	my ( $queue) = shift;

	unless (open LLCLASSOUT,  "$path/llclass -l $queue |") {
		error("Error in executing llclass");
	}

	my %queue_info;
	my $queue_name;

	while (<LLCLASSOUT>) {
		# Discard trailing information separated by a newline and header
		if ( /^$/ || /^==========/ ) {
		    next;
		}

		# Info ends with a line of dashes
		last if /^----------/;

		s/^\s*//;
		chomp;
		my ($par, $val) = split/\s*:\s*/,$_,2;

		if ($par eq 'Name') {
		    $queue_name=$val;
		    next;
		}

		$queue_info{$queue_name}{$par} = $val;
	}

	close LLCLASSOUT;

	return %queue_info;
}

sub get_queues($) {

	my ( $path ) = shift;

	unless (open LLCLASSOUT,  "$path/llclass |") {
		error("Error in executing llclass");
	}

	# llclass outputs queues (classes) delimited by ---- markers

	my @queues;
	my $queue_sect;
	while (<LLCLASSOUT>) {

		# Now reading queues
		if ( /^----------/ && $queue_sect == 0) {
		    if ($#queues == -1) {
				$queue_sect = 1;
				next;
			}
		}

		# Normal ending after reading final queue
		if ( /^----------/ && $queue_sect == 1) {
			$queue_sect = 0;
			return @queues;
		}

		if ( $queue_sect == 1 ) {
			chomp;
			s/ .*//;
			push @queues, $_;
		}

	}

	# We only end here if there were no queues
	return @queues;
}

sub get_short_job_info($$) { 

    # Path to LRMS commands
    my ($path) = shift;
    # Name of the queue to query
    my ($queue) = shift;

    if ($queue ne "") {
	unless (open LLQOUT, "$path/llq -c $queue |") {
	    error("Error in executing llq");
	}
    } else {
	unless (open LLQOUT, "$path/llq |") {
	    error("Error in executing llq");
	}
    }

    my %jobstatus;

    while (<LLQOUT>) {

        my ($total, $waiting, $pending, $running, $held, $preempted);

        if (/(\d*) .* (\d*) waiting, (\d*) pending, (\d*) running, (\d*) held, (\d*) preempted/) {
          $total     = $1;
          $waiting   = $2;
          $pending   = $3;
          $running   = $4;
          $held      = $5;
          $preempted = $6;
        }
	$jobstatus{total}     = $total;
	$jobstatus{waiting}   = $waiting;
        $jobstatus{pending}   = $pending;
	$jobstatus{running}   = $running;
	$jobstatus{held}      = $held;
	$jobstatus{preempted} = $preempted;
    }
    close LLQOUT;
    return %jobstatus;
}


sub get_long_job_info($$$) { 

    # Path to LRMS commands
    my ($path) = shift;
    # Name of the queue to query
    my ($queue) = shift;
    # LRMS job IDs from Grid Manager (jobs with "INLRMS" GM status)
    my ($lrms_ids) = @_;

    my %jobinfo;
    if ( (@{$lrms_ids})==0){
      return %jobinfo;
    }

    my $lrmsidstr = join(" ", @{$lrms_ids});

    if ($queue == "") {
	unless (open LLQOUT, "$path/llq -l -x $lrmsidstr |") {
	    error("Error in executing llq");
	}
    } else {
	unless (open LLQOUT, "$path/llq -c $queue |") {
	    error("Error in executing llq");
	}
    }

    my $jobid;
    my $skip=0;

    while (<LLQOUT>) {

        # Discard trailing information separated by a newline
        if ( /^$/ ) {
            last;
        }

	# Discard header lines
	if (/^===/) {
	    $skip=0;
	    next;
	}
	if ($skip == 1) {
	    next;
	}
	# skip extra info line
        if (/^------/) {
	    $skip=0;
            next;
	}
	chomp;
	# Create variables using text before colon
	my ($par, $val) = split/: */,$_,2;
	$par =~ s/^ *//g;
	$par =~ s/ /_/g;
	# Assign variables
	if ($par eq 'Job_Step_Id') {
	    $jobid = $val;
	    next;
	}
	$jobinfo{$jobid}{$par} = $val;
    }
    close LLQOUT;
    return %jobinfo;
}

############################################
# Public subs
#############################################

sub cluster_info ($) {

    # Path to LRMS commands
    my ($config) = shift;
    my ($path) = $$config{ll_bin_path};

    my (%lrms_cluster);

    # lrms_type
    $lrms_cluster{lrms_type} = "LoadLeveler";

    # lrms_version
    my $status_string=`$path/llstatus -v`;
    if ( $? != 0 ) {    
	warning("Can't run llstatus");
    }
    $status_string =~ /^\S+\s+(\S+)/;
    $lrms_cluster{lrms_version} = $1;

    my ($ll_consumable_resources) = $$config{ll_consumable_resources};

    if ($ll_consumable_resources ne "yes") {

	# totalcpus
	$lrms_cluster{totalcpus} = 0;
	$lrms_cluster{cpudistribution} = "";
	my %cpudist = get_cpu_distribution($path);
	my $sep = "";
	foreach my $key (keys %cpudist) {
	    $lrms_cluster{cpudistribution} .= $sep.$key."cpu:".$cpudist{$key};
	    if ($sep == "") {
		$sep = " ";
	    }
	    $lrms_cluster{totalcpus} += $key * $cpudist{$key};
	}

	# Simple way to find used cpus (slots/cores) by reading the output of llstatus
	$lrms_cluster{usedcpus} = get_used_cpus($path);

    } else {

	# Find used / max CPUs from cconsumable resources
	my @dist = consumable_distribution($path,"ConsumableCpus");
	my @cpu_total = consumable_total(@dist);

	$lrms_cluster{cpudistribution} = cpudist2str(@dist);
	$lrms_cluster{totalcpus} = $cpu_total[1];
	$lrms_cluster{usedcpus} = $cpu_total[0];
    }

    my %jobstatus = get_short_job_info($path,"");

    # Here waiting actually refers to jobsteps
    $lrms_cluster{queuedcpus} = $jobstatus{waiting};
    # TODO: this is wrong, but we are not worse off than earlier
    # we should count jobs, not cpus
    $lrms_cluster{runningjobs} = $jobstatus{running};
    $lrms_cluster{queuedjobs} = $jobstatus{waiting};
    $lrms_cluster{queue} = [ ];

    return %lrms_cluster;
}

sub queue_info ($$) {

    # Path to LRMS commands
    my ($config) = shift;
    my ($path) = $$config{ll_bin_path};

    # Name of the queue to query
    my ($queue) = shift;

    my %long_queue_info = get_long_queue_info($path,$queue);
    my %jobstatus       = get_short_job_info($path,$queue);

    # Translate between LoadLeveler and ARC

    $lrms_queue{status} = $long_queue_info{$queue}{'Free_slots'};

    # Max_total_tasks seems to give the right queue limit
    #$lrms_queue{maxrunning} = $long_queue_info{$queue}{'Max_total_tasks'};
    # Maximum_slots is really the right parameter to use for queue limit
    $lrms_queue{maxrunning} = $long_queue_info{$queue}{'Maximum_slots'};

    $lrms_queue{maxqueuable} = "";
    $lrms_queue{maxuserrun} = $lrms_queue{maxrunning};

    # Note we use Wall Clock!
    $_ = $long_queue_info{$queue}{'Wall_clock_limit'};
    if (/\((.*) seconds,/) {
	$lrms_queue{maxcputime} = $1 / 60;
    }
    $lrms_queue{maxwalltime} = $lrms_queue{maxcputime};

    # There is no lower limit enforced
    $lrms_queue{mincputime} = 0;
    $lrms_queue{minwalltime} = 0;

    $_ = $long_queue_info{$queue}{'Def_wall_clock_limit'};
    if (/\((.*) seconds,/) {
	$lrms_queue{defaultcput} = $1 / 60;
    }
    $lrms_queue{defaultwallt}= $lrms_queue{defaultcput};
    $lrms_queue{running} = $jobstatus{running}; # + $jobstatus{held} + $jobstatus{preempted};
    $lrms_queue{queued}  = $jobstatus{waiting};
#    $lrms_queue{totalcpus} =  $long_queue_info{$queue}{'Max_processors'};
    $lrms_queue{totalcpus} =  $long_queue_info{$queue}{'Maximum_slots'};
    
    return %lrms_queue;
}

sub jobs_info ($$$) {

    # Path to LRMS commands
    my ($config) = shift;
    my ($path) = $$config{ll_bin_path};
    # Name of the queue to query
    my ($queue) = shift;
    # LRMS job IDs from Grid Manager (jobs with "INLRMS" GM status)
    my ($lrms_ids) = @_;

    my (%lrms_jobs);

    my %jobinfo = get_long_job_info($path,"",$lrms_ids);

    foreach my $id (keys %jobinfo) {
        $lrms_jobs{$id}{status} = "O";
	if (
	    $jobinfo{$id}{Status} eq "Running"
	    ) {
            $lrms_jobs{$id}{status} = "R";
        }
	if (
	    ($jobinfo{$id}{Status} eq "Idle") ||
	    ($jobinfo{$id}{Status} eq "Deferred")
	    ) {
            $lrms_jobs{$id}{status} = "Q";
        }
	if (
	    ($jobinfo{$id}{Status} eq "Completed") ||
	    ($jobinfo{$id}{Status} eq "Canceled") ||
	    ($jobinfo{$id}{Status} eq "Removed") ||
	    ($jobinfo{$id}{Status} eq "Remove Pending") ||
	    ($jobinfo{$id}{Status} eq "Terminated")
	    ) {
            $lrms_jobs{$id}{status} = "E";
	}
	if (
	    ($jobinfo{$id}{Status} eq "System Hold") ||
	    ($jobinfo{$id}{Status} eq "User Hold") ||
	    ($jobinfo{$id}{Status} eq "User and System Hold")
	    ) {
            $lrms_jobs{$id}{status} = "S";
	}
	if (
	    ($jobinfo{$id}{Status} eq "Checkpointing")
	    ) {
            $lrms_jobs{$id}{status} = "O";
        }

	$lrms_jobs{$id}{mem} = -1;
	my $dispt = `date +%s -d "$jobinfo{$id}{Dispatch_Time}\n"`;
	chomp $dispt;
 	$lrms_jobs{$id}{walltime} = POSIX::ceil((time() - $dispt) /60);
 	# Setting cputime, should be converted to minutes
        my (@cput) = split(/:/,$jobinfo{$id}{Step_Total_Time});       
	$lrms_jobs{$id}{cputime} = int($cput[0]*60 + $cput[1] + $cput[2]/60); 
	$lrms_jobs{$id}{reqwalltime} = $jobinfo{$id}{Wall_Clk_Hard_Limit};
	$lrms_jobs{$id}{reqcputime} = $lrms_jobs{$id}{reqwalltime};
	$lrms_jobs{$id}{comment} = [ "LRMS: $jobinfo{$id}{Status}" ];
        $lrms_jobs{$id}{nodes} = ["$jobinfo{$id}{Allocated_Host}"];
        $lrms_jobs{$id}{rank} = -1;
        $lrms_jobs{$id}{cpus} = 0;
        $lrms_jobs{$id}{cpus} = $jobinfo{$id}{Step_Cpus};
    }

    return %lrms_jobs;
}

sub users_info($$@) {

    my ($config) = shift;
    my ($path) = $$config{ll_bin_path};

    my ($qname) = shift;
    my ($accts) = shift;

    my (%lrms_users);

    if ( ! exists $lrms_queue{status} ) {
        %lrms_queue = queue_info( $path, $qname );
    }
    # Using simple estimate. Fair-share value is only known by Administrator.
    foreach my $u ( @{$accts} ) {
        $lrms_users{$u}{freecpus} = $lrms_queue{status};
        $lrms_users{$u}{queuelength} = "$lrms_queue{queued}";
    }
    return %lrms_users;
}

1;
