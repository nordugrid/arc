package SGE;

use POSIX qw(ceil);
use File::Basename;
use lib dirname($0);
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

##########################################
# Private subs
##########################################

sub type_and_version () {

    my ($type, $version);
    my ($command) = "$path/qstat -help";

    unless ( open QSTAT, "$command 2>/dev/null |" ) {
	error("$command failed.");}
    my ($line) = <QSTAT>;
    ($type, $version) = split " ", $line;
    close QSTAT;
    unless ( ( $type =~ /GE/ ) and ( $version =~ /^6/ ) ) {
	warning("Unsupported LRMS type or version: $type $version.");
    }
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

    # Pending (queued) jobs

    $command = "$path/qstat -u '*' -s p";
    unless ( open QSTAT, "$command 2>/dev/null |" ) {
	error("$command failed.");}
    $queued = 0;
    while ( $line = <QSTAT> ) {
	if ( ! $line =~ /^\s*\d+\s+/ ) { next; }
	my ( @tmp ) = split " ", $line;
        if ( $tmp[-1] =~ /\:/ ) {
            $queued += $tmp[-2];
        } else {
            $queued += $tmp[-1];
        }
    }
    close QSTAT;
    return ($totalcpus, $distr, $usedslots, $queued, $queuetotalslots);
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

    # Count used/free CPUs and queued jobs in the cluster
    
    # Note: SGE has the concept of "slots", which roughly corresponds to
    # concept of "cpus" in ARC (PBS) LRMS interface.

    ( $lrms_cluster{totalcpus},
      $lrms_cluster{cpudistribution},
      $lrms_cluster{usedcpus},
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
	    $lrms_queue{status} = $a[3];
	    $lrms_queue{running} = $a[2];
	    $lrms_queue{totalcpus} = $a[4];
        }
    }
    close QSTAT;

    # settings in the config file override
    $lrms_queue{totalcpus} = $$config{totalcpus} if $$config{totalcpus};

    # Number of available (free) cpus can not be larger that
    # free cpus in the whole cluster
    my ($totalcpus, $distr, $usedslots, $queued, $queuetotalslots) = slots ( );
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
                $timelimit = 60*$a[0]+$a[1]+int($a[2]);
            } elsif (@a == 4) {
                $timelimit = 24*60*$a[0]+60*$a[1]+$a[2]+int($a[3]);
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
                $timelimit = 60*$a[0]+$a[1]+int($a[2]);
            } elsif (@a == 4) {
                $timelimit = 24*60*$a[0]+60*$a[1]+$a[2]+int($a[3]);
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
    # NOTE: We also violate documentation and report number
    #       or queueing slots instead of jobs

    $lrms_queue{queued} = $queued;

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
    my ($line);
    while ( $line = <QSTAT>) {
	# assume that all lines beginning with an integer are describing jobs
	if ( ! $line =~ /^\s*\d+\s+/ ) {next}
	my ($id, $prior, $name, $user, $state, $submitstart,
	    $at, $queue, $slots ) = split " ", $line;
	$total_user_jobs{$user}++;
	if (grep /^$id$/, @{$jids}) {
	    if ($state =~ /r/) {
                $lrms_jobs{$id}{status} = 'R';
            } else {
                $lrms_jobs{$id}{status} = 'S';
            }
	    my ($cluster_queue, $exec_host) = split '@', $queue;
	    $lrms_jobs{$id}{nodes} = [ $exec_host ]; # master node for parellel runs
	    $lrms_jobs{$id}{walltime} = "";
	    $lrms_jobs{$id}{rank} = "";
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
    my ($rank) = 0;
    while ( $line = <QSTAT> ) {
	# assume that all lines beginning with an integer are describing jobs
	if ( ( $line =~ /^job-ID/) || ( $line =~ /^-+/) ) {next}
	$rank++;
	my ($id, $prior, $name, $user, $state, $submitstart,
	    $at, $queue, $slots ) = split " ", $line;
	$total_user_jobs{$user}++;
	if (grep { $id == $_ } @$jids) {
	    $lrms_jobs{$id}{rank} = $rank;
	    if ( $state =~ /E/ ) {
		$lrms_jobs{$id}{status} = 'O';
	    } else { # Normally queued
		$lrms_jobs{$id}{status} = 'Q';
	    }
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

    # If qstat does not match, job has probably finished already
    # Querying accounting system is slow, so we skip it for now ;-)
    # That will be done by scan-sge-jobs.
    # Instead, just set empty values

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
        if ($max_u_jobs) {
            $total_user_jobs{$u} = 0 unless $total_user_jobs{$u};
            my $freecpus = $max_u_jobs - $total_user_jobs{$u};
            if ($lrms_queue{status} < $freecpus) {
                $freecpus = $lrms_queue{status};
            }
	    $lrms_users{$u}{freecpus} = $freecpus;
        } else {
	    $lrms_users{$u}{freecpus} = $lrms_queue{status};
        };
        $lrms_users{$u}{freecpus} .= ":$maxwalltime" if $maxwalltime;
	$lrms_users{$u}{queuelength} = "$lrms_queue{queued}";
    }
    return %lrms_users;
}
	      


1;
