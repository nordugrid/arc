################################################################
#Template for implementing the information provider for an LRMS
#The full description of the interface is available in Nordugrid 
#technote 13: Arc back-ends interface guide.
################################################################
package Template; #change to LRMS name

@ISA = ('Exporter');

# Module implements these subroutines for the LRMS interface

@EXPORT_OK = ('cluster_info',
              'queue_info',
              'jobs_info',
              'users_info');

use LogUtils ( 'start_logging', 'error', 'warning', 'debug' ); 
use strict;

############################################
# Public subs
#############################################

sub cluster_info ($) {

    # config array
    my ($config) = shift;

    # Return data structure %lrms_cluster{$keyword}
    # should contain the keyvords listed in LRMS.pm namely:
    #
    # lrms_type          LRMS type (eg. LoadLeveler)
    # lrms_version       LRMS version
    # totalcpus          Total number of cpus in the system
    # queuedcpus         Number of cpus requested in queueing jobs in LRMS 
    # usedcpus           Used cpus in the system
    # cpudistribution    CPU distribution string
    #
    # All values should be defined, empty values "" are ok if field
    # does not apply to particular LRMS.
    my (%lrms_cluster);

    $lrms_cluster{lrms_type}="";
    $lrms_cluster{lrms_version}="";
    $lrms_cluster{totalcpus}="";
    $lrms_cluster{queuedcpus}="";
    $lrms_cluster{usedcpus}="";
    $lrms_cluster{cpudistribution}="";

    return %lrms_cluster;
}

sub queue_info ($$) {

    # config array
    my ($config) = shift;

    # Name of the queue to query
    my ($queue) = shift;

    # The return data structure is %lrms_queue.
    my (%lrms_queue);
    # Return data structure %lrms_queue{$keyword}
    # should contain the keyvords listed in LRMS.pm:
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

    $lrms_queue{status} = "";
    $lrms_queue{maxrunning} = "";
    $lrms_queue{maxqueuable} = "";
    $lrms_queue{maxuserrun} = "";
    $lrms_queue{maxcputime} = "";
    $lrms_queue{mincputime} = "";
    $lrms_queue{defaultcput} = "";
    $lrms_queue{maxwalltime} = "";
    $lrms_queue{minwalltime} = "";
    $lrms_queue{defaultwallt} = "";
    $lrms_queue{running} = "";
    $lrms_queue{queued}  = "";
    $lrms_queue{totalcpus} =  "";
    
    return %lrms_queue;
}

sub jobs_info ($$$) {

    # config array
    my ($config) = shift;
    # Name of the queue to query
    my ($queue) = shift;
    # LRMS job IDs from Grid Manager (jobs with "INLRMS" GM status)
    my ($lrms_ids) = @_;

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

    my (%lrms_jobs);

    foreach my $id (keys %jobinfo) {
        $lrms_jobs{$id}{status} = "O";
	$lrms_jobs{$id}{rank} = "";
	$lrms_jobs{$id}{mem} = -1;
	$lrms_jobs{$id}{walltime} ="";
	$lrms_jobs{$id}{cputime} = "";
	$lrms_jobs{$id}{reqwalltime} = "";
	$lrms_jobs{$id}{reqcputime} = "";
	$lrms_jobs{$id}{nodes} = [ ];
	$lrms_jobs{$id}{comment} = [ ];
    }

    return %lrms_jobs;
}

sub users_info($$@) {
    # config array
    my ($config) = shift;
    # name of queue to query
    my ($qname) = shift;
    # user accounts 
    my ($accts) = shift;

    my (%lrms_users);

    # freecpus for given account
    # queue length for given account
    #

    foreach my $u ( @{$accts} ) {
        $lrms_users{$u}{freecpus} = "";
        $lrms_users{$u}{queuelength} = "";
    }
    return %lrms_users;
}

1;
