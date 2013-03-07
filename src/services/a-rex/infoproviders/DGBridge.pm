package DGBridge;

use strict;
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


############################################
# Public subs
#############################################

sub cluster_info ($) {
    my ($config) = shift;

    my (%lrms_cluster);

    $lrms_cluster{lrms_type} = "DGBridge";
    $lrms_cluster{lrms_version} = "1.8.1";

    # only enforcing per-process cputime limit
    $lrms_cluster{has_total_cputime_limit} = 0;

    $lrms_cluster{totalcpus} = 1000;

    # Since fork is a single machine backend all there will only be one machine available
    $lrms_cluster{cpudistribution} = $lrms_cluster{totalcpus}."cpu:1";

    # usedcpus on a fork machine is determined from the 1min cpu
    # loadaverage and cannot be larger than the totalcpus

    $lrms_cluster{usedcpus} = 0;

    $lrms_cluster{runningjobs} = 0;

    # no LRMS queuing jobs on a fork machine, fork has no queueing ability
    $lrms_cluster{queuedcpus} = 0;
    $lrms_cluster{queuedjobs} = 0;
    $lrms_cluster{queue} = [ ];
    return %lrms_cluster;
}

sub queue_info ($$) {
    my ($config) = shift;
    my ($qname) = shift;

    init_globals($qname);

    if (defined $running) {
        # job_info was already called, we know exactly how many grid jobs
        # are running
        $lrms_queue{running} = $running;

    } else {
        $lrms_queue{running}= 0;
    }

    $lrms_queue{totalcpus} =1000;

    $lrms_queue{status} = $lrms_queue{totalcpus}-$lrms_queue{running};

    # reserve negative numbers for error states
    $lrms_queue{status} = 0;
    

    $lrms_queue{maxrunning} = "10000";
    $lrms_queue{maxuserrun} = "10000";
    $lrms_queue{maxqueuable} = ""; #unlimited
    $lrms_queue{maxcputime} = "";
    $lrms_queue{queued} = 0;
    $lrms_queue{mincputime} = "";
    $lrms_queue{defaultcput} = "";
    $lrms_queue{minwalltime} = "";
    $lrms_queue{defaultwallt} = "";
    $lrms_queue{maxwalltime} = $lrms_queue{maxcputime};
    $lrms_queue{MaxSlotsPerJob} = 1;

    return %lrms_queue;
}

sub getDGstate($$) {
  my ($jid) = shift;
  my ($ep) = shift;
  my ($state);  
  my ($cmdl) = "wsclient -e $ep -m status -j $jid |";

  unless (open DGSTATUSOUT, $cmdl) {
    error("Error in executing wsclient");
  }
  $state="WSError";

  while (<DGSTATUSOUT>) {
    unless (/^$jid/) {
      next;
    }
    chomp;
    my ($vid,$val) = split' ',$_,2;
    $state = $val;
  }

  close DGSTATUSOUT;
  return $state;
}

sub jobs_info ($$@) {
    my ($config) = shift;
    my ($qname) = shift;
    my ($jids) = shift;

    init_globals($qname);

    my (%lrms_jobs);
    foreach my $id (@$jids){
        
        $lrms_jobs{$id}{nodes} = [ ];
        # get real endpoint
        my ($endp, $bid) = split'\|',$id,2;
        $endp =~ s/^\"//;
        $bid =~ s/\"$//;
        my $dgstate = getDGstate($bid, $endp);
        #states possible
 	#Init
	#Running
	#Unknown
	#Finished
	#Error
	#TempFailed
        $lrms_jobs{$id}{status} = 'O'; # job is ?
	if ($dgstate eq "Init") {
          $lrms_jobs{$id}{status} = 'Q'; # job is preparing
        }
        if ($dgstate eq "Running") {
          $lrms_jobs{$id}{status} = 'R'; # job is running
          ++$running;
        }
        if ( 
		($dgstate eq "Finished") ||
		($dgstate eq "Error") 
           ) {
          $lrms_jobs{$id}{status} = 'E'; # job is EXECUTED
        }
        if ( ($dgstate eq "TempFailed") ||
             ($dgstate eq "WSError")
           ) {
          $lrms_jobs{$id}{status} = 'S'; # job is temporarily failed
        }
        if ($dgstate eq "Unknown") {
          $lrms_jobs{$id}{status} = 'O'; # job is temporarily failed
        }
        
        $lrms_jobs{$id}{comment} = [ "LRMS: $dgstate" ];

        $lrms_jobs{$id}{mem} = '';
        $lrms_jobs{$id}{walltime} = '';
        $lrms_jobs{$id}{cputime} = '';
	$lrms_jobs{$id}{reqwalltime} = "";
	$lrms_jobs{$id}{reqcputime} = "";   
        $lrms_jobs{$id}{rank} = -1;
	#DGBridge backend does not support parallel jobs
	$lrms_jobs{$id}{cpus} = 1; 
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
