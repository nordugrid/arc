package LRMS;

# Interface to LRMS
#
# To include a new LRMS:
#
# 1. Each LRMS specific module needs to provide subroutines
#    cluster_info, queue_info, jobs_info, and users_info.
#
#    The interfaces are documented in this file. All variables
#    required in the interface should be defined in LRMS modules.
#    Returning empty variable "" is perfectly ok if variable does not apply
#    to a LRMS.
#    
# 2. References to subroutines defined in new LRMS modules are added
#    to the select_lrms subroutine in this module, and the module reference
#    itself, naturally.

use File::Basename;
use lib dirname($0);
use Exporter;
@ISA = ('Exporter');     # Inherit from Exporter
@EXPORT_OK = ( 'select_lrms',
	       'cluster_info',
	       'queue_info',
	       'jobs_info',
	       'users_info');
use LogUtils ( 'start_logging', 'error', 'warning', 'debug' ); 
use SGE;
use Fork;
use PBS;
use LL;
use LSF;
use Condor;

use strict;

our ( $lrms_name, $cluster_info, $queue_info, $jobs_info, $users_info ); 

sub select_lrms ($) {
    # %config read from arc.conf
    my ($config) = shift;

    my ($lrms_name) = $$config{lrms};

    if ( lc($lrms_name) eq "sge" ) {
	$cluster_info = \&SGE::cluster_info;
	$queue_info   = \&SGE::queue_info;
	$jobs_info    = \&SGE::jobs_info;
	$users_info   = \&SGE::users_info;
    } elsif ( lc($lrms_name) eq "fork" ) {
	$cluster_info = \&Fork::cluster_info;
	$queue_info   = \&Fork::queue_info;
	$jobs_info    = \&Fork::jobs_info;
	$users_info   = \&Fork::users_info;
    } elsif ( lc($lrms_name) eq "pbs" ) {
	$cluster_info = \&PBS::cluster_info;
	$queue_info   = \&PBS::queue_info;
	$jobs_info    = \&PBS::jobs_info;
	$users_info   = \&PBS::users_info;
    } elsif ( lc($lrms_name) eq "ll" ) {
	$cluster_info = \&LL::cluster_info;
	$queue_info   = \&LL::queue_info;
	$jobs_info    = \&LL::jobs_info;
	$users_info   = \&LL::users_info;
    } elsif ( lc($lrms_name) eq "lsf" ) {
	$cluster_info = \&LSF::cluster_info;
	$queue_info   = \&LSF::queue_info;
	$jobs_info    = \&LSF::jobs_info;
	$users_info   = \&LSF::users_info;
    } elsif ( lc($lrms_name) eq "condor" ) {
	$cluster_info = \&Condor::cluster_info;
	$queue_info   = \&Condor::queue_info;
	$jobs_info    = \&Condor::jobs_info;
	$users_info   = \&Condor::users_info;
    } else {
	error("LRMS $lrms_name not implemented.");  
    }
}


sub cluster_info ($) {
    # Path to LRMS commands
    my ($config) = shift;

    my (%lrms_cluster) = &$cluster_info($config);

    # lrms_type
    # lrms_version
    # totalcpus          total number of cpus in the system
    # queuedcpus         number of cpus requested in queueing jobs in LRMS
    # usedcpus           used cpus in the system
    # cpudistribution    cpu distribution string
    # queue              names of the LRMS queues

    my (@scalar_checklist) = ( 'lrms_type',
			       'lrms_version',
			       'totalcpus',
			       'queuedcpus',
			       'usedcpus',
			       'cpudistribution' );

    my (@array_checklist) = ( 'queue' );

    # Check that all got defined

    foreach my $k ( @scalar_checklist ) {
	unless ( defined $lrms_cluster{$k} ) {
	    debug("${lrms_name}::cluster_info failed to ".
		  "populate \$lrms_cluster{$k}.");
	    $lrms_cluster{$k} = "";
	}
    }

    foreach my $k ( @array_checklist ) {
	unless ( defined $lrms_cluster{$k} ) {
	    debug("${lrms_name}::cluster_info failed to ".
		  "populate \@{\$lrms_cluster{$k}}.");
	    @{ $lrms_cluster{$k} } = ( );
	}
    }
    
    # Check for extras ;-)
    
    foreach my $k (keys %lrms_cluster ) {
	if ( ! grep /$k/, (@scalar_checklist, @array_checklist) ) {
	    debug("\%lrms_cluster contains a key $k which is not defined".
		  "in the LRMS interface in Shared.pm")
	}
    }

    return %lrms_cluster;
}


sub queue_info ($$) {
    # Path to LRMS commands
    my ($config) = shift;
    # Name of the queue to query
    my ($queue_name) = shift;
    
    my (%lrms_queue) = &$queue_info( $config, $queue_name );

    # status        available slots in the queue, negative number signals
    #               some kind of LRMS error state for the queue
    # maxrunning    queue limit for number of running jobs
    # maxqueuable   queue limit for number of queueing jobs
    # maxuserrun    queue limit for number of running jobs per user
    # maxcputime    queue limit for max cpu time for a job
    # mincputime    queue limit for min cpu time for a job
    # defaultcput   queue default for cputime
    # running       number of procs used by running jobs in the queue
    # queued        number of procs requested by queueing jobs in the queue
    # totalcpus     number of procs in the queue

    my (@scalar_checklist) = ( 'status',
			       'maxrunning',
			       'maxqueuable',
			       'maxuserrun',
			       'maxcputime',
			       'mincputime',
			       'defaultcput',
			       'maxwalltime',
			       'minwalltime',
			       'defaultwallt',
			       'running',
			       'queued',
			       'totalcpus');

    # Check that all got defined
    
    foreach my $k ( @scalar_checklist ) {
	unless ( defined $lrms_queue{$k} ) {
	    debug("${lrms_name}::queue_info failed to ".
		  "populate \$lrms_queue{$k}.");
	    $lrms_queue{$k} = "";
	}
    }

    # Check for extras ;-)

    foreach my $k (keys %lrms_queue ) {
	if ( ! grep /$k/, @scalar_checklist ) {
	    debug("\%lrms_queue contains a key $k which is not defined".
		  "in the LRMS interface in Shared.pm")
	}
    }

    return %lrms_queue;
}

sub jobs_info ($$@) {
    # Path to LRMS commands
    my ($config) = shift;
    # Name of the queue to query
    my ($queue_name) = shift;
    # LRMS job IDs from Grid Manager (jobs with "INLRMS" GM status)
    my ($lrms_jids) = shift;
    
    my (%lrms_jobs) = &$jobs_info($config, $queue_name, \@{$lrms_jids});

    # status        Status of the job: Running 'R', Queued'Q',
    #                                  Suspended 'S', Exiting 'E', Other 'O'
    # rank          Position in the queue
    # mem           Used (virtual) memory
    # walltime      Used walltime
    # cputime       Used cpu-time
    # reqwalltime   Walltime requested from LRMS
    # reqcputime    Cpu-time requested from LRMS
    # nodes         List of execution hosts. (one host in case of serial jobs)
    # comment       Comment about the job in LRMS, if any

    my (@scalar_checklist) = ( 'status',
			       'rank',
			       'mem',
			       'walltime',
			       'cputime',
			       'reqwalltime',
			       'reqcputime');

    my (@array_checklist) = ( 'nodes', 'comment' );

    # Check returned hash

    foreach my $jid ( @{$lrms_jids} ) {
	unless ( exists $lrms_jobs{$jid} ) {
	    debug("Data not acquired for job lrms id = $jid.");
	}
	
	# Check that all got defined
	
	foreach my $k ( @scalar_checklist ) {
	    unless ( exists $lrms_jobs{$jid}{$k} ) {
		debug("${lrms_name}::jobs_info failed to ".
		      "populate \$lrms_jobs{$jid}{$k}.");
		$lrms_jobs{$jid}{$k} = "";
	    }
	}

	foreach my $k ( @array_checklist ) {
	    unless ( exists $lrms_jobs{$jid}{$k} ) {
		debug("${lrms_name}::jobs_info failed to ".
		      "populate \@{\$lrms_jobs{$jid}{$k}}.");
		@{ $lrms_jobs{$jid}{$k} } = ( );
	    }
	}

	# Check for extras ;-)

	foreach my $k (keys %{$lrms_jobs{$jid}} ) {
	    if ( ! grep /$k/, ( @scalar_checklist, @array_checklist) ) {
		debug("\%lrms_jobs{$jid} contains a key $k which is not".
		      "defined in the LRMS interface in Shared.pm");
	    }
	}
    }
    
    return %lrms_jobs;
}


sub users_info ($$@) {
    # Path to LRMS commands
    my ($config) = shift;
    # Name of the queue to query
    my ($queue_name) = shift;
    # Unix user names mapped to grid users
    my ($user) = shift;
    
    my (%lrms_users) = &$users_info($config, $queue_name, \@{$user});

    # freecpus       free cpus available for the specified user
    # queuelength    estimated queue length for the specified user

    my (@scalar_checklist) = ( 'freecpus',
			       'queuelength');
    # Check returned hash

    foreach my $u ( @{$user} ) {
	unless ( exists $lrms_users{$u} ) {
	    debug("Data not acquired for user $user.");
	}
	
	# Check that all got defined
	
	foreach my $k ( @scalar_checklist ) {
	    unless ( exists $lrms_users{$u}{$k} ) {
		debug("${lrms_name}::users_info failed to ".
		      "populate \$lrms_users{$u}{$k}.");
	    }
	}

	# Check for extras ;-)

	foreach my $k (keys %{$lrms_users{$u}} ) {
	    if ( ! grep /$k/, @scalar_checklist ) {
		debug("\%lrms_users{$u} contains a key $k which is not".
		      "defined in the LRMS interface in Shared.pm");
	    }
	}
    }
    
    return %lrms_users;
}

1;
