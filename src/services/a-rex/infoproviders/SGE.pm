package SGE;

use POSIX qw(floor ceil);
@ISA = ('Exporter');
@EXPORT_OK = ('lrms_init',
              'cluster_info',
	      'queue_info',
	      'jobs_info',
	      'users_info');
use LogUtils ( 'start_logging', 'error', 'warning', 'debug' ); 
use strict;

##########################################
# Saved private variables
##########################################

our %lrms_queue;
our $config;
our $path;

our $maxwalltime;
our $max_u_jobs;
our %total_user_jobs;

# Regular expression used for interpreting output from qstat.
# The simpler approach of splitting the line will not work if a job's name
# includes a space
our $name_re = '.*\S'; # may contain space
our $ts_re = '\d\d\/\d\d\/\d{4} \d\d:\d\d:\d\d';
our $queue_re = '\S*\D\S*'; # not only digits
our $task_re = '[-,:\d]+'; # example: 4,6-8:1
our $job_re = '^\s*(\d+)\s+([.\d]+|nan)\s+('.$name_re.')\s+(\S+)\s+(\w+)'
            . '\s+('.$ts_re.')(?:\s+('.$queue_re.'))??\s+(\d+)(?:\s+('.$task_re.'))?\s*$';

##########################################
# Private subs
##########################################

sub type_and_version () {

    my ($type, $version);
    my ($command) = "$path/qstat -help";

    open QSTAT, "$command 2>/dev/null |";
    my ($line) = <QSTAT>;
    ($type, $version) = split " ", $line;
    close QSTAT;
    error("Cannot indentify SGE version from output of '$command': $line")
        unless $type =~ /GE/ and $version;
    warning("Unsupported SGE version: $version") unless $version =~ /^6/;
    return $type, $version;
}

sub queues () {
    my (@queue, $line);
    my ($command) = "$path/qconf -sql";
    unless ( open QCONF, "$command 2>/dev/null |" ) {
	error("$command failed.");}
    while ( $line = <QCONF> ) {
	chomp $line;
	push @queue, $line;
    }
    close QCONF;
    return @queue;
}

#
# Processes an array task definition (i.e.: '3,4,6-8,10-20:2')
# and returns the number of individual tasks
#

sub count_array_spec($) {
    my $count = 0;
    for my $spec (split ',', shift) {
        # handles expressions like '6-10:2' and '6-10' and '6'
        return 0 unless $spec =~ '^(\d+)(?:-(\d+)(?::(\d+))?)?$';
        my ($lower,$upper,$step) = ($1,$2,$3);
        $upper = $lower unless defined $upper;
        $step = 1 unless $step;
        $count += 1 + floor(abs($upper-$lower)/$step);
    }
    return $count;
}

sub cpudist (@) {
    my (@cpuarray) = @_;
    my (%cpuhash) = ();
    my ($cpudistribution) ="";
    while ( @cpuarray ) {
	$cpuhash{ pop(@cpuarray) }++; }
    while ( my ($cpus,$count)  = each %cpuhash ) {
	if ( $cpus > 0 ) { 
	    $cpudistribution .=  $cpus . 'cpu:' . $count . " ";
	}
    }
    chop $cpudistribution;
    return $cpudistribution;
}

sub slots () {
    my ($totalcpus, $distr, $usedslots, $queued);
    my ($foo, $line, @slotarray);
    my $queuetotalslots = {};

    # Number of slots in execution hosts

    my ($command) = "$path/qconf -sep";

    unless ( open QQ, "$command 2>/dev/null |" ) {
	error("$command failed.");
    }
    while ( $line = <QQ> ) {
	if ( $line =~ /^HOST/ || $line =~ /^=+/ ) { next; }
	if ( $line =~ /^SUM\s+(\d+)/ ) {
	    $totalcpus = $1;
	    next;
	}
	my ($name, $ncpu, $arch ) = split " ", $line;
	push @slotarray, $ncpu;
    }
    close QQ;

    $distr = cpudist(@slotarray);

    # Used slots in all queues
    $command = "$path/qstat -g c";
    unless ( open QSTAT, "$command 2>/dev/null |" ) {
	error("$command failed.");}
    $usedslots = 0;
    while ( $line = <QSTAT> ) {
	if ( $line =~ /^CLUSTER QUEUE/ || $line =~ /^-+/) { next; }
	my ($name, $cqload, $used, $avail, $total, $aoACDS, $cdsuE )
	    = split " ", $line;
	$usedslots += $used;
        $$queuetotalslots{$name} = $total;
    }
    close QSTAT;

    # List all jobs

    $command = "$path/qstat -u '*'";
    unless ( open QSTAT, "$command 2>/dev/null |" ) {
	error("$command failed.");}
    my $queuedcpus = 0;
    my $queuedjobs = 0;
    my $runningjobs = 0;

    while ( $line = <QSTAT> ) {
	# assume that all lines beginning with an integer are describing jobs
	next unless $line =~ /^\s*\d+\s+/;
        if ($line =~ /$job_re/) {
            my ($id,$state,$slots,$tasks) = ($1,$5,$8,$9); 
            # for interpreting state codes:
            # http://gridengine.sunsource.net/nonav/source/browse/~checkout~/gridengine/source/libs/japi/jobstates.html
            if ($state =~ /[rtsSTR]/) {
                # This should match the jobs that would be listed by qstat -s rs
                # Job is either running, transfering or suspended for some reason.
                # It may also be waiting to be deleted while in the above states.
                # OBS: hr (hold running) state are also counted here.
                #      R is state for restarted job.
                $runningjobs++;
            } elsif ($state =~ /[hw]/) {
                # This should match the jobs that would be listed by qstat -s p
                # Job is pending in the queue. It's possibly being hold or it can be in an error state.
                my $ntasks = 1;
                $ntasks = count_array_spec($tasks) if defined $tasks;
                error("Cannot understand array job specification '$tasks' from qstat line: $line") unless $ntasks;
                $queuedjobs += $ntasks;
                $queuedcpus += $ntasks * $slots;
            } else {
                # should not happen
                warning("SGE job $id is in an unexpected state: $state");
            }
        } else {
            error("cannot parse an output line from $command: $line");
        }
    }
    close QSTAT;
    return ($totalcpus, $distr, $usedslots, $runningjobs, $queuedjobs, $queuedcpus, $queuetotalslots);
}

sub global_limits() {
    my ($command) = "$path/qconf -sconf global";

    unless ( open QQ, "$command 2>/dev/null |" ) {
        error("$command failed.");
    }
    my $max_jobs = 0;
    while ( my $line = <QQ> ) {
        $max_jobs = $1 if $line =~ /^max_jobs\s+(\d+)/;
        $max_u_jobs = $1 if $line =~ /^max_u_jobs\s+(\d+)/;
    }
    close QQ;
    return ($max_jobs, $max_u_jobs);
}

sub req_limits ($) {
    my ($s) = shift;
    my ($reqcputime, $reqwalltime);

    # required cputime 
    if ($s =~ /h_cpu=(\d+)/ ) {
	$reqcputime=ceil($1/60);
    } elsif ($s =~ /s_cpu=(\d+)/ ) {
	$reqcputime=ceil($1/60);
    } else {
	$reqcputime="";
    }

    # required walltime 
    if ($s =~ /h_rt=(\d+)/ ) {
	$reqwalltime=ceil($1/60);
    } elsif ($s =~ /s_rt=(\d+)/ ) {
	$reqwalltime=ceil($1/60);
    } else {
	$reqwalltime="";
    }

    return ($reqcputime, $reqwalltime);
}

############################################
# Public subs
#############################################

sub configure_sge_env(%) {
    my %config = @_;
    $ENV{SGE_ROOT} = $config{sge_root} || $ENV{SGE_ROOT};
    error("sge_root must be defined in arc.conf") unless $ENV{SGE_ROOT};

    $ENV{SGE_CELL} = $config{sge_cell} || $ENV{SGE_CELL} || 'default';
    $ENV{SGE_QMASTER_PORT} = $config{sge_qmaster_port} if $config{sge_qmaster_port};
    $ENV{SGE_EXECD_PORT} = $config{sge_execd_port} if $config{sge_execd_port};

    for (split ':', $ENV{PATH}) {
        $ENV{SGE_BIN_PATH} = $_ and last if -x "$_/qsub";
    }
    $ENV{SGE_BIN_PATH} = $config{sge_bin_path} || $ENV{SGE_BIN_PATH};

    error("SGE executables not found") unless -x "$ENV{SGE_BIN_PATH}/qsub";

    $path = $ENV{SGE_BIN_PATH};
}


sub cluster_info ($) {
    $config = shift;

    configure_sge_env(%$config);

    my (%lrms_cluster);

    # Figure out SGE type and version

    ( $lrms_cluster{lrms_type}, $lrms_cluster{lrms_version} )
	= type_and_version();

    # SGE has per-slot cputime limit
    $lrms_cluster{has_total_cputime_limit} = 0;

    # Count used/free CPUs and queued jobs in the cluster
    
    # Note: SGE has the concept of "slots", which roughly corresponds to
    # concept of "cpus" in ARC (PBS) LRMS interface.

    ( $lrms_cluster{totalcpus},
      $lrms_cluster{cpudistribution},
      $lrms_cluster{usedcpus},
      $lrms_cluster{runningjobs},
      $lrms_cluster{queuedjobs},
      $lrms_cluster{queuedcpus})
	= slots ();

    # List LRMS queues
    
    @{$lrms_cluster{queue}} = queues();

    return %lrms_cluster;
}

sub queue_info ($$) {
    $config = shift;
    configure_sge_env(%$config);

    my ($qname) = shift;

    # status
    # running
    # totalcpus

    my ($command) = "$path/qstat -g c";
    unless ( open QSTAT, "$command 2> /dev/null |" ) {
	error("$command failed.");}
    my ($line, $used);
    my ($sub_running, $sub_status, $sub_totalcpus);
    while ( $line = <QSTAT> ) {
	if ( $line =~ /^(CLUSTER QUEUE)/  || $line =~ /^-+$/ ) {next}
	my (@a) = split " ", $line;
	if ( $a[0] eq $qname ) {
	    # SGE 6.2 has an extra column between USED and AVAIL
	    $lrms_queue{status} = $a[-4];
	    $lrms_queue{running} = $a[2];
	    $lrms_queue{totalcpus} = $a[-3];
        }
    }
    close QSTAT;

    # settings in the config file override
    $lrms_queue{totalcpus} = $$config{totalcpus} if $$config{totalcpus};

    # Number of available (free) cpus can not be larger that
    # free cpus in the whole cluster
    my ($totalcpus, $distr, $usedslots, $runningjobs, $queuedjobs, $queuedcpus, $queuetotalslots) = slots ( );
    if ( $lrms_queue{status} > $totalcpus-$usedslots ) {
	$lrms_queue{status} = $totalcpus-$usedslots;
	$lrms_queue{status} = 0 if $lrms_queue{status} < 0;
    }
    # reserve negative numbers for error states
    if ($lrms_queue{status}<0) {
	warning("lrms_queue{status} = $lrms_queue{status}")}

    # Grid Engine has hard and soft limits for both CPU time and
    # wall clock time. Nordugrid schema only has CPU time.
    # The lowest of the 2 limits is returned by this code.

    my $qlist = $qname;
    $command = "$path/qconf -sq $qlist";
    open QCONF, "$command 2>/dev/null |" or error("$command failed.");
    while ( $line = <QCONF> ) {
        if ($line =~ /^[sh]_rt\s+(.*)/) {
            next if $1 eq 'INFINITY';
            my @a = split ":", $1;
            my $timelimit;
            if (@a == 3) {
                $timelimit = 60*$a[0]+$a[1];
            } elsif (@a == 4) {
                $timelimit = 24*60*$a[0]+60*$a[1]+$a[2];
            } elsif (@a == 1) {
                $timelimit = int($a[0]/60);
            } else {
                warning("Error parsing time info in output of $command");
                next;
            }
            if (not defined $lrms_queue{maxwalltime} || $lrms_queue{maxwalltime} > $timelimit) {
                $lrms_queue{maxwalltime} = $timelimit;
            }
        }
        if ($line =~ /^[sh]_cpu\s+(.*)/) {
            next if $1 eq 'INFINITY';
            my @a = split ":", $1;
            my $timelimit;
            if (@a == 3) {
                $timelimit = 60*$a[0]+$a[1];
            } elsif (@a == 4) {
                $timelimit = 24*60*$a[0]+60*$a[1]+$a[2];
            } elsif (@a == 1) {
                $timelimit = int($a[0]/60);
            } else {
                warning("Error parsing time info in output of $command");
                next;
            }
            if (not defined $lrms_queue{maxcputime} || $lrms_queue{maxcputime} > $timelimit) {
                $lrms_queue{maxcputime} = $timelimit;
            }
        }
    }
    close QCONF;

    # Global variable; saved for later use in users_info()
    $maxwalltime = $lrms_queue{maxwalltime};

    $lrms_queue{maxwalltime} = "" unless $lrms_queue{maxwalltime};
    $lrms_queue{maxcputime} = "" unless $lrms_queue{maxcputime};
    $lrms_queue{minwalltime} = "";
    $lrms_queue{mincputime} = "";
    $lrms_queue{defaultcput} = "";
    $lrms_queue{defaultwallt} = "";

    # Grid Engine puts queueing jobs in single "PENDING" state pool,
    # so here we report the total number queueing jobs in the cluster.

    $lrms_queue{queued} = $queuedjobs;

    # nordugrid-queue-maxqueuable

    # SGE has a global limit on total number of jobs, but not per-queue limit.
    # This global limit can be used as an upper bound for nordugrid-queue-maxqueuable
    my ($max_jobs,$max_u_jobs) = global_limits();
    $max_jobs = "" unless $max_jobs;
    $lrms_queue{maxqueuable} = $max_jobs;
    
    # nordugrid-queue-maxrunning

    # The total max running jobs is the number of slots for this queue
    $lrms_queue{maxrunning} = $$queuetotalslots{$qname} || "";

    # nordugrid-queue-maxuserrun
    $lrms_queue{maxuserrun} = "";

    return %lrms_queue;
}


sub jobs_info ($$@) {
    $config = shift;
    my ($qname) = shift;
    my ($jids) = shift;

    my $line;
    my (%lrms_jobs);

    # Running jobs
    my ($command) = "$path/qstat -u '*' -s rs";
    unless ( open QSTAT, "$command 2>/dev/null |" ) {
	error("$command failed.");}

    while ( $line = <QSTAT>) {
	# assume that all lines beginning with an integer are describing jobs
	next unless $line =~ /^\s*\d+\s+/;
        if ($line =~ /$job_re/) {
            my ($id,$user,$state,$queue,$slots) = ($1,$4,$5,$7,$8);
	    $total_user_jobs{$user}++;
            if (grep { $id == $_ } @$jids) {
                # it's a grid job.
                # grid jobs are never array jobs so we don't worry about multiple lines for the same job id.
	        if ($state =~ /^R?[rt]$/) {
                    $lrms_jobs{$id}{status} = 'R';
                } else {
                    $lrms_jobs{$id}{status} = 'S';
                }
	        my ($cluster_queue, $exec_host) = split '@', $queue;
	        $lrms_jobs{$id}{nodes} = [ $exec_host ]; # master node for parellel runs
	        $lrms_jobs{$id}{cpus} = $slots;
	        $lrms_jobs{$id}{walltime} = "";
	        $lrms_jobs{$id}{rank} = "";
	    }
        } else {
            error("cannot parse an output line from $command: $line");
        }
    }
    close QSTAT;


    # lrms_jobs{$id}{status}
    # lrms_jobs{$id}{rank}

    # Pending (queued) jobs
    # NOTE: Counting rank based on all queues.
    $command = "$path/qstat -u '*' -s p";
    unless ( open QSTAT, "$command 2>/dev/null |" ) {
	error("$command failed.");}
    my ($rank) = 1;
    while ( $line = <QSTAT> ) {
        # assume that all lines beginning with an integer are describing jobs
        next unless $line =~ /^\s*\d+\s+/;
        if ($line =~ /$job_re/) {
            my ($id,$user,$state,$slots,$tasks) = ($1,$4,$5,$8,$9);
            if (grep { $id == $_ } @$jids) {
                # it's a grid job.
                # grid jobs are never array jobs so we don't worry about multiple lines for the same job id.
                $lrms_jobs{$id}{rank} = $rank;
                if ( $state =~ /E/ ) {
                    $lrms_jobs{$id}{status} = 'O';
                } else { # Normally queued
                    $lrms_jobs{$id}{status} = 'Q';
                }
            }
            my $ntasks = 1;
            $ntasks = count_array_spec($tasks) if defined $tasks;
            error("Cannot understand array job specification '$tasks' from qstat line: $line") unless $ntasks;
            $total_user_jobs{$user} += $ntasks;
            $rank += $ntasks;
        } else {
            error("cannot parse an output line from $command: $line");
        }
    }
    close QSTAT;
    
    # lrms_jobs{$id}{mem}
    # lrms_jobs{$id}{walltime}
    # lrms_jobs{$id}{cputime}
    # lrms_jobs{$id}{reqwalltime}
    # lrms_jobs{$id}{reqcputime}
    # lrms_jobs{$id}{comment}

    my (@running, @queueing, @otherlrms, @notinlrms);
    foreach my $jid ( @{$jids} ) {
        next unless exists $lrms_jobs{$jid};
	if ($lrms_jobs{$jid}{status} eq 'R') {
	    push @running, $jid;
	} elsif ($lrms_jobs{$jid}{status} eq 'S') {
	    push @running, $jid;
	} elsif ($lrms_jobs{$jid}{status} eq 'Q') {
	    push @queueing, $jid;
	} elsif ($lrms_jobs{$jid}{status} eq 'O') {
	    push @otherlrms, $jid;
	} else {
	    push @notinlrms, $jid;
	}
    }

    # If qstat does not match, job has finished already.

    foreach my $jid ( @{$jids} ) {
        next if exists $lrms_jobs{$jid};
	debug("Job executed $jid");
        $lrms_jobs{$jid}{status} = '';
        $lrms_jobs{$jid}{mem} = '';
        $lrms_jobs{$jid}{walltime} = '';
        $lrms_jobs{$jid}{cputime} = '';
        $lrms_jobs{$jid}{reqwalltime} = '';
        $lrms_jobs{$jid}{reqcputime} = '';
        $lrms_jobs{$jid}{rank} = '';
        $lrms_jobs{$jid}{nodes} = [];
        $lrms_jobs{$jid}{comment} = [];
    }


    my ($jid);
    
    # Running jobs

    my ($jidstr) = join ',', @running;
    $command = "$path/qstat -j $jidstr";
    $jid = "";

    unless ( open QSTAT, "$command 2>/dev/null |" ) {
	debug("Command $command failed.")}

    while ( $line = <QSTAT>) {

	if ( $line =~ /^job_number:\s+(\d+)/) {
	    $jid=$1;
	    $lrms_jobs{$jid}{comment} = [ ];
	    next;
	}

	if ( $line =~ /^usage/) {
	    # Memory usage in kB
	    # SGE reports mem, vmem and maxvmem
	    # maxvmem chosen here
	    $line =~ /maxvmem=(\d+)\.?(\d*)\s*(\w+)/;
	    my ($mult) = 1024; 
	    if ($3 eq "M") {$mult = 1024} 
	    if ($3 eq "G") {$mult = 1024*1024} 
	    $lrms_jobs{$jid}{mem} = int($mult*$1 + $2*$mult/1000);
	    # used cpu time
	    $line =~ /cpu=((\d+:?)*)/;
	    my (@a) = $1 =~ /(\d+):?/g;
	    if ( @a == 4 ) {
		$lrms_jobs{$jid}{cputime} =
		    60 * ( $a[0]*24 + $a[1] ) + $a[2] ;
	    } else {
		$lrms_jobs{$jid}{cputime} =
		    60 * $a[0] + $a[1];}
	    next;
	}
	
	if ($line =~ /^hard resource_list/) {
	    ( $lrms_jobs{$jid}{reqcputime},
	      $lrms_jobs{$jid}{reqwalltime} ) =
		  req_limits($line);
	}
	next;
    }
    close QSTAT;

    # Normally queueing job

    $jidstr = join ',', @queueing;
    $command = "$path/qstat -j $jidstr";
    $jid = "";
    
    unless ( open QSTAT, "$command 2>/dev/null |" ) {
	debug("Command $command failed.")}

    while ( $line = <QSTAT>) {

	if ( $line =~ /^job_number:\s+(\d+)/) {
	    $jid=$1;
	    next;
	}

	if ($line =~ /^hard resource_list/) {
	    ( $lrms_jobs{$jid}{reqcputime},
	      $lrms_jobs{$jid}{reqwalltime} ) =
		  req_limits($line);
	    next;
	}

	# Reason for beeing held in queue
	if ( $line =~ /^\s*(cannot run because.*)/ ) {
	    if ( exists $lrms_jobs{$jid}{comment} ) {
		push @{ $lrms_jobs{$jid}{comment} }, "LRMS: $1";
	    } else {
		$lrms_jobs{$jid}{comment} = [ "LRMS: $1" ];
	    }
	}
	next;
    }
    close QSTAT;

    # Other LRMS state, often jobs pending in error state 'Eqw'
    # Skip the rest if no jobs are in this state
    return %lrms_jobs unless @otherlrms;

    $jidstr = join ',', @otherlrms;
    $command = "$path/qstat -j $jidstr";
    $jid = "";

    unless ( open QSTAT, "$command 2>/dev/null |" ) {
	debug("Command $command failed.")}
    
    while ( $line = <QSTAT>) {
	
	if ( $line =~ /^job_number:\s+(\d+)/) {
	    $jid=$1;
	    $lrms_jobs{$jid}{nodes} = [];
	    $lrms_jobs{$jid}{mem} = "";
	    $lrms_jobs{$jid}{cputime} = "";
	    $lrms_jobs{$jid}{walltime} = "";
	    next;
	}

	if ($line =~ /^hard resource_list/) {
	    ( $lrms_jobs{$jid}{reqcputime},
	      $lrms_jobs{$jid}{reqwalltime} ) =
		  req_limits($line);
	    next;
	}

	# Error reason nro 1

	if ($line =~ /^error reason\s*\d*:\s*(.*)/ ) {
	    if ( exists $lrms_jobs{$jid}{comment} ) {
		push @{$lrms_jobs{$jid}{comment}}, "LRMS: $1";
	    } else {
		$lrms_jobs{$jid}{comment} = [ "LRMS: $1" ];
	    }
	    next;
	}

	# Let's say it once again ;-)
	if ($line =~ /(job is in error state)/ ) {
	    if ( exists $lrms_jobs{$jid}{comment} ) {
		push @{ $lrms_jobs{$jid}{comment} }, "LRMS: $1";
	    } else {
		$lrms_jobs{$jid}{comment} = [ "LRMS: $1" ];
	    }
	}
	next;
    }

    return %lrms_jobs;

}

sub users_info($$@) {
    $config = shift;
    my ($qname) = shift;
    my ($accts) = shift;

    my (%lrms_users);

    # freecpus
    # queue length
    #
    # This is nearly impossible to implement generally for
    # complex system such as grid engine. Using simple 
    # estimate.

    if ( ! exists $lrms_queue{status} ) {
	%lrms_queue = queue_info($config,$qname);
    }

    foreach my $u ( @{$accts} ) {
        $lrms_users{$u}{freecpus} = $lrms_queue{status};
        $lrms_users{$u}{freecpus} .= ":$maxwalltime" if $maxwalltime;
	$lrms_users{$u}{queuelength} = "$lrms_queue{queued}";
    }
    return %lrms_users;
}
	      


1;
