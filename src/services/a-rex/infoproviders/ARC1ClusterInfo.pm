package ARC1ClusterInfo;

# This InfoCollector combines the output of the other information collectors
# and prepares the GLUE2 information model of A-REX. The returned structure is
# meant to be converted to XML with XML::Simple.

use base InfoCollector;
use ARC1ClusterSchema;

use Storable;
use POSIX qw(ceil);

use Sysinfo qw(cpuinfo processid diskinfo diskspaces);
use LogUtils;

use strict;

my $arc1_info_schema = ARC1ClusterSchema::arc1_info_schema();

our $log = LogUtils->getLogger(__PACKAGE__);


sub timenow(){
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = gmtime(time);
    return sprintf "%4d-%02d-%02dT%02d:%02d:%02d%1s", $year+1900, $mon+1, $mday,$hour,$min,$sec,"Z";
}

# converts MDS-style time to ISO 8061 time ( 20081212151903Z --> 2008-12-12T15:19:03Z )
sub mdstoiso {
    return "$1-$2-$3T$4:$5:$6Z" if shift =~ /^(\d\d\d\d)(\d\d)(\d\d)(\d\d)(\d\d)(\d\d(?:\.\d+)?)Z$/;
    return undef;
}

# TODO: Stage-in and Stage-out are substates of what?
# OBS: Deleted is not in OGSA-BES
sub ogsa_state {
    my ($gm_state,$lrms_state) = @_;
    if      ($gm_state eq "ACCEPTED") {
        return "Pending:Accepted";
    } elsif ($gm_state eq "PREPARING") {
        return "Running:Stage-in";
    } elsif ($gm_state eq "SUBMIT") {
        return "Running:Submitting";
    } elsif ($gm_state eq "INLRMS") {
        if (not defined $lrms_state) {
            return "Running";
        } elsif ($lrms_state eq 'Q') {
            return "Running:Queuing";
        } elsif ($lrms_state eq 'R') {
            return "Running:Executing";
        } elsif ($lrms_state eq 'EXECUTED' or $lrms_state eq '') {
            return "Running:Executed";
        } elsif ($lrms_state eq 'S') {
            return "Running:Suspended";
        } else {
            return "Running:LRMSOther";
        }
    } elsif ($gm_state eq "FINISHING") {
        return "Running:Stage-out";
    } elsif ($gm_state eq "CANCELING") {
        return "Running:Cancelling";
    } elsif ($gm_state eq "KILLED") {
        return "Cancelled";
    } elsif ($gm_state eq "FAILED") {
        return "Failed";
    } elsif ($gm_state eq "FINISHED") {
        return "Finished";
    } elsif ($gm_state eq "DELETED") {
        return "Deleted";
    } else {
        return undef;
    }
}

############################################################################
# Combine info from all sources to prepare the final representation
############################################################################

# override InfoCollector base class methods

sub _get_options_schema {
    return {}; # too many inputs to name all
}
sub _get_results_schema {
    return $arc1_info_schema;
}

sub _collect($$) {
    my ($self,$options) = @_;
    my $config = $options->{config};
    my $usermap = $options->{usermap};
    my $host_info = $options->{host_info};
    my $gmjobs_info = $options->{gmjobs_info};
    my $lrms_info = $options->{lrms_info};

    my $creation_time = timenow();
    my $validity_ttl = $config->{ttl};

    # adotf means autodetect on the frontend
    $config->{architecture} = $host_info->{architecture}
        if defined($config->{architecture}) and $config->{architecture} eq 'adotf';
    $config->{nodecpu} = $host_info->{nodecpu}
        if defined($config->{nodecpu}) and $config->{nodecpu} eq 'adotf';

    # config overrides
    $host_info->{hostname} = $config->{hostname} if $config->{hostname};

    # # # # # # # # # # # # # # # # # # #
    # # # # # Job statistics  # # # # # #
    # # # # # # # # # # # # # # # # # # #

    # total jobs in each GM state
    my %gmtotalcount;

    # jobs in each GM state, by share
    my %gmsharecount;

    # grid jobs in each lrms sub-state (queued, running, suspended), by share
    my %inlrmsjobs;

    # grid jobs in each lrms sub-state (queued, running, suspended)
    my %inlrmsjobstotal;

    # slots needed by grid jobs in each lrms sub-state (queued, running, suspended), by share
    my %inlrmsslots;

    # number of slots needed by all waiting jobs, per share
    my %requestedslots;

    # Jobs waiting to be prepared by GM (indexed by share)
    my %pending;

    # Jobs waiting to be prepared by GM
    my $pendingtotal;

    # Jobs being prepared by GM (indexed by share)
    my %share_prepping;

    # Jobs being prepared by GM (indexed by grid owner)
    my %user_prepping;
    # $user_prepping{$_} = 0 for keys %$usermap;

    for my $jobid (keys %$gmjobs_info) {

        my $job = $gmjobs_info->{$jobid};
        my $gridowner = $gmjobs_info->{$jobid}{subject};
        my $qname = $job->{queue};

        # Temporary hack: If A-REX has not choosen a queue for the job, default to one.
        unless ($qname) {
            my $msg = "Queue not defined for job $jobid";
            if ($config->{defaultqueue}) {
                $log->info($msg.". Assuming default: ".$config->{defaultqueue});
                $qname = $job->{queue} = $config->{defaultqueue};
            } else {
                my @qnames = keys %{$config->{queues}};
                if (@qnames == 1) {
                    $log->info($msg.". Assuming: ".$qnames[0]);
                } else {
                    $log->error($msg." and no default queue is defined.");
                }
                $qname = $job->{queue} = $qnames[0];
            }
            $job->{share} = $job->{queue};
        }

        my $share = $job->{share} ||= '';
        # A share which is not defined in the configuration is invalid
        $share = '' unless exists $config->{shares}{$share};
        $job->{share} = $share;

        my $gmstatus = $job->{status};

        $gmtotalcount{totaljobs}++;
        $gmsharecount{$share}{totaljobs}++;

        # count GM states by category

        my %states = ( 'UNDEFINED'        => [0, 'undefined'],
                       'ACCEPTED'         => [1, 'accepted'],
                       'PENDING:ACCEPTED' => [1, 'accepted'],
                       'PREPARING'        => [2, 'preparing'],
                       'PENDING:PREPARING'=> [2, 'preparing'],
                       'SUBMIT'           => [2, 'preparing'],
                       'INLRMS'           => [3, 'inlrms'],
                       'PENDING:INLRMS'   => [4, 'finishing'],
                       'FINISHING'        => [4, 'finishing'],
                       'CANCELING'        => [4, 'finishing'],
                       'FAILED'           => [5, 'finished'],
                       'KILLED'           => [5, 'finished'],
                       'FINISHED'         => [5, 'finished'],
                       'DELETED'          => [6, 'deleted']   );

        unless ($states{$gmstatus}) {
            $log->warning("Unexpected job status for job $jobid: $gmstatus");
            $gmstatus = $job->{status} = 'UNDEFINED';
        }
        my ($age, $category) = @{$states{$gmstatus}};

        $gmtotalcount{$category}++;
        $gmsharecount{$share}{$category}++;

        if ($age < 6) {
            $gmtotalcount{notdeleted}++;
            $gmsharecount{$share}{notdeleted}++;
        } elsif ($age < 5) {
            $gmtotalcount{notfinished}++;
            $gmsharecount{$share}{notfinished}++;
        } elsif ($age < 3) {
            $gmtotalcount{notsubmitted}++;
            $gmsharecount{$share}{notsubmitted}++;
            $requestedslots{$share} += $job->{count} || 1;
            $share_prepping{$share}++;
            $user_prepping{$gridowner}++;
        } elsif ($age < 2) {
            $pending{$share}++;
            $pendingtotal++;
        }

        # count grid jobs running and queued in LRMS for each share

        if ($gmstatus eq 'INLRMS') {
            my $lrmsid = $job->{localid};
            my $lrmsjob = $lrms_info->{jobs}{$lrmsid};
            my $slots = $job->{count} || 1;

            if (defined $lrmsjob) {
                if ($lrmsjob->{status} ne 'EXECUTED') {
                    if ($lrmsjob->{status} eq 'R') {
                        $inlrmsjobstotal{running}++;
                        $inlrmsjobs{$share}{running}++;
                        $inlrmsslots{$share}{running} += $slots;
                    } elsif ($lrmsjob->{status} eq 'S') {
                        $inlrmsjobstotal{suspended}++;
                        $inlrmsjobs{$share}{suspended}++;
                        $inlrmsslots{$share}{suspended} += $slots;
                    } else {  # Consider other states 'queued'
                        $inlrmsjobstotal{queued}++;
                        $inlrmsjobs{$share}{queued}++;
                        $inlrmsslots{$share}{queued} += $slots;
                        $requestedslots{$share} += $slots;
                    }
                }
            } else {
                $log->warning("Info missing about lrms job $lrmsid");
            }
        }

    }

    # Infosys is run by A-REX: Always assume GM is up
    $host_info->{processes}{'grid-manager'} = 1;


    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
    # # # # # # # # # # build information tree  # # # # # # # # # #
    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

    my $arexhostport = "$host_info->{hostname}:$config->{gm_port}";

    # Global IDs
    my $csvID = "urn:grid:csv:$arexhostport"; # ComputingService
    my $cepID = "urn:grid:cep:$arexhostport"; # ComputingEndpoint
    my $cmgrID = "urn:grid:cmgr:$arexhostport"; # ComputingManager
    my %cactIDs; # ComputingActivity IDs

    # Locally Unique IDs
    my $cshaLID = 'csha0'; my %cshaLIDs; # ComputingShare
    my $xenvLID = 'xenv0'; my @xenvIDs; # ExecutionEnvironment
    my $locLID = 'loc0';   my @locLIDs;  # Location
    my $conLID = 'con0';   my @conLIDs;  # Contact
    my $apolLID = 'apol0'; my @apolLIDs; # AccessPolicy
    my $mpolLID = 'mpol0'; my @mpolLIDs; # MappingPolicy

    # Generate CompShare LocalIDs
    for my $share (keys %{$config->{shares}}) {
        $cshaLIDs{$share} = $cshaLID++;
    }

    # generate CompActivity Global IDs
    for my $jobid (keys %$gmjobs_info) {
        my $share = $gmjobs_info->{$jobid}{share};
        my $gridid = "https://$arexhostport/arex/$jobid";
        if ($cshaLIDs{$share}) {
            $cactIDs{$share}{$jobid} = $gridid;
        } else {
            $log->warning("Job $jobid belongs to invalid share ($share)");
        }
    }

    my $csv = {};

    $csv->{CreationTime} = $creation_time;
    $csv->{Validity} = $validity_ttl;
    $csv->{BaseType} = 'Service';

    $csv->{ID} = [ $csvID ];

    $csv->{Name} = [ $config->{cluster_alias} ] if $config->{cluster_alias};
    $csv->{Capability} = [ 'executionmanagement.jobexecution' ];
    $csv->{Type} = [ 'org.nordugrid.arex' ];

    # OBS: QualityLevel reflects the quality of the sotware
    # One of: development, testing, pre-production, production
    $csv->{QualityLevel} = [ 'development' ];

    # StatusPage: new config option ?

    my $nshares = keys %{$config->{shares}};
    $csv->{Complexity} = [ "endpoint=1,share=$nshares,resource=1" ];

    # OBS: Finished/failed/deleted jobs are not counted
    $csv->{TotalJobs} = [ $gmtotalcount{notfinished} || 0 ];

    $csv->{RunningJobs} = [ $inlrmsjobstotal{running} || 0 ];
    $csv->{SuspendedJobs} = [ $inlrmsjobstotal{suspended} || 0 ];
    $csv->{WaitingJobs} = [ $inlrmsjobstotal{queued} || 0 ];

    $csv->{StagingJobs} = [ ( $gmtotalcount{perparing} || 0 )
                         + ( $gmtotalcount{finishing} || 0 ) ];

    $csv->{PreLRMSWaitingJobs} = [ $pendingtotal || 0 ];

    $csv->{ComputingShares} = { ComputingShare => [] };

    $csv->{Associations} = {};


    # ComputingEndpoint

    my $cep = {};
    $csv->{ComputingEndpoint} = [ $cep ];

    $cep->{CreationTime} = $creation_time;
    $cep->{Validity} = $validity_ttl;
    $cep->{BaseType} = 'Endpoint';

    $cep->{ID} = [ $cepID ];

    # Name not necessary

    # OBS: ideally HED should be asked for the URL
    $cep->{URL} = [ "https://$arexhostport/arex" ];
    $cep->{Technology} = [ 'webservice' ];
    $cep->{Interface} = [ 'OGSA-BES' ];
    #$cep->{InterfaceExtension} = '';
    $cep->{WSDL} = [ "https://$arexhostport/arex/?wsdl" ];
    # Wrong type, should be URI
    $cep->{SupportedProfile} = [ "WS-I 1.0", "HPC-BP" ];
    $cep->{Semantics} = [ "http://www.nordugrid.org/documents/arex.pdf" ];
    $cep->{Implementor} = [ "NorduGrid" ];
    $cep->{ImplementationName} = [ "ARC1" ];

    # TODO: use value from config.h
    $cep->{ImplementationVersion} = [ "0.9" ];

    # TODO: add config option
    $cep->{QualityLevel} = [ "development" ];

    # TODO: a way to detect if LRMS is up
    $cep->{HealthState} = [ "ok" ];
    #$cep->{HealthStateInfo} = [ "" ];

    # TODO: when is it 'queueing' and 'closed'?
    if ( defined($config->{allownew}) and $config->{allownew} eq "no" ) {
        $cep->{ServingState} = [ 'draining' ];
    } else {
        $cep->{ServingState} = [ 'production' ];
    }

    # StartTime: get it from hed

    $cep->{IssuerCA} = [ $host_info->{issuerca} ];
    $cep->{TrustedCA} = $host_info->{trustedcas};

    # TODO: Downtime, is this necessary, and how should it work?

    $cep->{Staging} =  [ 'staginginout' ];
    $cep->{JobDescription} = [ 'ogf:jsdl:1.0' ];

    # TODO: AccessPolicy, needs studying the security configuration
    $cep->{AccessPolicy} = [];

    $cep->{TotalJobs} = [ $gmtotalcount{totaljobs} || 0 ];

    $cep->{RunningJobs} = [ $inlrmsjobstotal{running} || 0 ];
    $cep->{SuspendedJobs} = [ $inlrmsjobstotal{suspended} || 0 ];
    $cep->{WaitingJobs} = [ $inlrmsjobstotal{queued} || 0 ];

    $cep->{StagingJobs} = [ ( $gmtotalcount{perparing} || 0 )
                         + ( $gmtotalcount{finishing} || 0 ) ];

    $cep->{PreLRMSWaitingJobs} = [ $pendingtotal || 0 ];

    $cep->{Associations}{ComputingShareLocalID} = [ values %cshaLIDs ];
    $cep->{Associations}{ComputingActivityID} = [ map { values %{$_} } values %cactIDs ];


    # ComputingShares: multiple shares can share the same LRMS queue

    for my $share (keys %{$config->{shares}}) {

        my $qname = $config->{shares}{$share}{qname};

        my $qinfo = $lrms_info->{queues}{$qname};

        # Prepare flattened config hash for this share. Share options override queue options
        my $sconfig = { %$config, %{$config->{queues}{$qname}}, %{$config->{shares}{$share}} };
        delete $sconfig->{shares};
        delete $sconfig->{queues};

        # List of all shares submitting to the current queue, including the current share.
        my @siblingshares = grep { $qname if $qname eq $_->{qname} } values %{$config->{shares}};
        $sconfig->{siblingshares} = \@siblingshares;

        my $csha = {};

        push @{$csv->{ComputingShares}{ComputingShare}}, $csha;

        $csha->{CreationTime} = $creation_time;
        $csha->{Validity} = $validity_ttl;
        $csha->{BaseType} = 'Share';

        $csha->{LocalID} = [ $cshaLIDs{$share} ];

        $csha->{Name} = [ $share ];
        $csha->{Description} = [ $sconfig->{comment} ] if $sconfig->{comment};
        $csha->{MappingQueue} = [ $qname ];

        # use limits from LRMS
        $csha->{MaxCPUTime} = [ $qinfo->{maxcputime} ] if defined $qinfo->{maxcputime};
        $csha->{MinCPUTime} = [ $qinfo->{mincputime} ] if defined $qinfo->{mincputime};
        $csha->{DefaultCPUTime} = [ $qinfo->{defaultcput} ] if defined $qinfo->{defaultcput};
        $csha->{MaxWallTime} =  [ $qinfo->{maxwalltime} ] if defined $qinfo->{maxwalltime};
        # TODO: MaxMultiSlotWallTime replaces MaxTotalWallTime, but has different meaning. Check that it's used correctly
        $csha->{MaxMultiSlotWallTime} =  [ $qinfo->{maxwalltime} ] if defined $qinfo->{maxwalltime};
        $csha->{MinWallTime} =  [ $qinfo->{minwalltime} ] if defined $qinfo->{minwalltime};
        $csha->{DefaultWallTime} = [ $qinfo->{defaultwallt} ] if defined $qinfo->{defaultwallt};

        my ($maxtotal, $maxlrms) = split ' ', ($sconfig->{maxjobs} || '');

        # MaxWaitingJobs: use the maxjobs config option
        # OBS: An upper limit is not really enforced by A-REX.
        # OBS: Currently A-REX only cares about totals, not per share limits!
        $csha->{MaxTotalJobs} = [ $maxtotal ] if defined $maxtotal;

        # MaxWaitingJobs, MaxRunningJobs:
        my ($maxrunning, $maxwaiting);

        # use values from lrms if avaialble
        if (defined $qinfo->{maxrunning}) {
            $maxrunning = $qinfo->{maxrunning};
            if (defined $qinfo->{maxqueuable}) {
                $maxwaiting = $qinfo->{maxqueuable} - $maxrunning;
            }
        }

        # maxjobs config option sets upper limits
        if (defined $maxlrms) {
            $maxrunning = $maxlrms
                if not defined $maxrunning
                    or $maxrunning > $maxlrms;

            if (defined $maxrunning) {
                $maxwaiting = $maxlrms - $maxrunning
                    if not defined $maxwaiting
                        or $maxwaiting > $maxlrms - $maxrunning;
            }
        }
       
        $csha->{MaxRunningJobs} = [ $maxrunning ] if defined $maxrunning;
        $csha->{MaxWaitingJobs} = [ $maxwaiting ] if defined $maxwaiting;

        # MaxPreLRMSWaitingJobs: use GM's maxjobs option
        # OBS: Currently A-REX only cares about totals, not per share limits!
        # OBS: this formula is actually an upper limit on the sum of pre + post
        #      lrms jobs. A-REX does not have separate limit for pre lrms jobs
        $csha->{MaxPreLRMSWaitingJobs} = [ $maxtotal - $maxlrms ]
            if defined $maxtotal and defined $maxlrms;

        $csha->{MaxUserRunningJobs} = [ $qinfo->{maxuserrun} ]
            if defined $qinfo->{maxuserrun};

        # OBS: new config option. Default value is 1
        # TODO: new return value from LRMS infocollector
        # TODO: see how LRMSs can detect the correct value
        $csha->{MaxSlotsPerJob} = [ $sconfig->{maxslotsperjob} || $qinfo->{maxslotsperjob}  || 1 ];

        # MaxStageInStreams, MaxStageOutStreams
        # OBS: A-REX does not have separate limits for up and downloads.
        # OBS: A-REX only cares about totals, not per share limits!
        my ($maxstaging) = split ' ', ($sconfig->{maxload} || '');
        $csha->{MaxStageInStreams}  = [ $maxstaging ] if $maxstaging;
        $csha->{MaxStageOutStreams} = [ $maxstaging ] if $maxstaging;

        # TODO: new return value scheduling_policy from LRMS infocollector.
        my $shedpolicy = $lrms_info->{scheduling_policy} || undef;
        if ($sconfig->{scheduling_policy} and not $shedpolicy) {
            $shedpolicy = 'fifo' if lc($sconfig->{scheduling_policy}) eq 'fifo';
            $shedpolicy = 'fairshare' if lc($sconfig->{scheduling_policy}) eq 'maui';
        }

        # TODO: get it from ExecutionEnviromnets mapped to this share instead
	# GuaranteedVirtualMemory -- all nodes must be able to provide this
	# much memory per job. Some nodes might be able to afford more per job
	# (MaxVirtualMemory)
        $csha->{GuaranteedVirtualMemory} = [ $sconfig->{nodememory} ] if $sconfig->{nodememory};
        # MaxMainMemory -- usage not being tracked by most LRMSs

        # OBS: new config option (space measured in GB !?)
        # OBS: Disk usage of jobs is not being enforced,
	# This limit should correspond with the max local-scratch disk space on
	# clusters using local disks to run jobs.
        # TODO: implement check at job accept time in a-rex
        # TODO: implement check in job wrapper
        $csha->{MaxDiskSpace} = [ $sconfig->{maxdiskperjob} ] if $sconfig->{maxdiskperjob};

        # DefaultStorageService: Has no meaning for ARC

        # TODO: new return value from LRMS infocollector.
        # OBS: Should be ExtendedBoolean_t (one of 'True', 'False', 'Undefined')
        $csha->{Preemption} = [ $qinfo->{preemption} ] if $qinfo->{preemption};

        # ServingState: closed and queuing are not yet supported
        if (defined $sconfig->{allownew} and lc($sconfig->{allownew}) eq 'no') {
            $csha->{ServingState} = [ 'production' ];
        } else {
            $csha->{ServingState} = [ 'draining' ];
        }

        # Count local jobs
        my $localrunning = $qinfo->{running};
        my $localqueued = $qinfo->{queued};
        my $localsuspended = $qinfo->{suspended};

        # Substract grid jobs submitted belonging to shares that submit to the same lrms queue
        $localrunning -= $inlrmsjobs{$_}{running} || 0 for @{$sconfig->{siblingshares}};
        $localqueued -= $inlrmsjobs{$_}{queued} || 0 for @{$sconfig->{siblingshares}};
        $localsuspended -= $inlrmsjobs{$_}{suspended} || 0 for @{$sconfig->{siblingshares}};

        # OBS: Finished/failed/deleted jobs are not counted
        my $totaljobs = $gmsharecount{$share}{notfinished} || 0;
        $totaljobs += $localrunning + $localqueued + $localsuspended;
        $csha->{TotalJobs} = [ $totaljobs ];

        $csha->{RunningJobs} = [ $localrunning + ( $inlrmsjobs{$share}{running} || 0 ) ];
        $csha->{WaitingJobs} = [ $localqueued + ( $inlrmsjobs{$share}{queued} || 0 ) ];
        $csha->{SuspendedJobs} = [ $localsuspended + ( $inlrmsjobs{$share}{suspended} || 0 ) ];

        # TODO: backends to count suspended jobs

        $csha->{LocalRunningJobs} = [ $localrunning ];
        $csha->{LocalWaitingJobs} = [ $localqueued ];
        $csha->{LocalSuspendedJobs} = [ $localsuspended ];

        $csha->{StagingJobs} = [ ( $gmsharecount{$share}{perparing} || 0 )
                               + ( $gmsharecount{$share}{finishing} || 0 ) ];

        $csha->{PreLRMSWaitingJobs} = [ $gmsharecount{$share}{notsubmitted} || 0 ];

        # TODO: investigate if it's possible to get these estimates from maui/torque
        $csha->{EstimatedAverageWaitingTime} = [ $qinfo->{averagewaitingtime} ] if defined $qinfo->{averagewaitingtime};
        $csha->{EstimatedWorstWaitingTime} = [ $qinfo->{worstwaitingtime} ] if defined $qinfo->{worstwaitingtime};

        # TODO: implement $qinfo->{freeslots} in LRMS plugins

        my $freeslots = 0;
        if (defined $qinfo->{freeslots}) {
            $freeslots = $qinfo->{freeslots};
        } else {
            $freeslots = $qinfo->{totalcpus} - $qinfo->{running};
        }

        # Local users have individual restrictions
        # FreeSlots: find the maximum freecpus of any local user mapped in this
        # share and use that as an upper limit for $freeslots
        # FreeSlotsWithDuration: for each duration, find the maximum freecpus
        # of any local user mapped in this share
        # TODO: is this the correct way to do it?

        my %timeslots;

        for my $uid (keys %{$qinfo->{users}}) {
            my $uinfo = $qinfo->{users}{$uid};
            next unless defined $uinfo->{freecpus};

            for my $nfree ( keys %{$uinfo->{freecpus}} ) {
                my $seconds = 60 * $uinfo->{freecpus}{$nfree};

                if ($timeslots{$seconds}) {
                    $timeslots{$seconds} = $nfree > $timeslots{$seconds}
                                         ? $nfree : $timeslots{$seconds};
                } else {
                    $timeslots{$seconds} = $nfree;
                }
            }
        }

        # find maximum free slots regardless of duration
        my $maxuserslots = 0;
        for my $seconds ( keys %timeslots ) {
            my $nfree = $timeslots{$seconds};
            $maxuserslots = $nfree if $nfree > $maxuserslots;
        }
        $freeslots = $maxuserslots < $freeslots
                   ? $maxuserslots : $freeslots;

        $csha->{FreeSlots} = [ $freeslots ];

        my @durations;

        # sort descending by duration, keping 0 first (0 for unlimited)
        for my $seconds (sort { if ($a == 0) {1} elsif ($b == 0) {-1} else {$b <=> $a} } keys %timeslots) {
            my $nfree = $timeslots{$seconds} < $freeslots
                      ? $timeslots{$seconds} : $freeslots;
            unshift @durations, $seconds ? "$nfree:$seconds" : $nfree;
        }

        $csha->{FreeSlotsWithDuration} = [ join(" ", @durations) || 0 ];

        $csha->{UsedSlots} = [ $qinfo->{running} ];

        $csha->{RequestedSlots} = [ $requestedslots{$qname} || 0 ];

        # TODO: detect reservationpolicy in the lrms
        $csha->{ReservationPolicy} = [ $qinfo->{reservationpolicy} ]
            if $qinfo->{reservationpolicy};

        # Tag: skip it for now

        $csha->{Associations}{ExecutionEnvironmentLocalID} = [];
        $csha->{Associations}{ComputingEndpointID} = [ $cepID ];
        $csha->{Associations}{ComputingActivityID} = [ values %{$cactIDs{$share}} ];

    }


    # ComputingManager

    my $cmgr = {};
    $csv->{ComputingManager} = [ $cmgr ];

    $cmgr->{CreationTime} = $creation_time;
    $cmgr->{Validity} = $validity_ttl;
    $cmgr->{BaseType} = 'Activity';

    $cmgr->{ID} = [ $cmgrID ];
    my $cluster_info = $lrms_info->{cluster};

    # Name not needed

    $cmgr->{Type} = [ $cluster_info->{lrms_glue_type} ];
    $cmgr->{Version} = [ $cluster_info->{lrms_version} ];
    $cmgr->{Reservation} = [ "Undefined" ];
    $cmgr->{BulkSubmission} = [ "False" ];
    # OBS: most LRMSes don't differentiate between Physical and Logical CPUs.
    # Make it possible to override these values from the config.
    $cmgr->{TotalPhysicalCPUs} = [ $config->{totalcpusockets} || $cluster_info->{totalcpus} ];
    $cmgr->{TotalLogicalCPUs} = [ $config->{totalcputhreads} || $cluster_info->{totalcpus} ];

    # OBS: What's a slot? 
    $cmgr->{TotalSlots} = [ $cluster_info->{totalcpus} ];

    my $gridrunningslots = 0; $gridrunningslots += $_->{running} for values %inlrmsslots;
    my $localrunningslots = $cluster_info->{usedcpus} - $gridrunningslots;
    $cmgr->{SlotsUsedByLocalJobs} = [ $localrunningslots ];
    $cmgr->{SlotsUsedByGridJobs} = [ $gridrunningslots ];

    my $homogeneity = "True";
    $homogeneity = "False" if lc($config->{homogeneity}) eq "no";
    $cmgr->{Homogeneous} = [ $homogeneity ];

    # OBS: Can be 100megabitethernet, gigabitethernet, infiniband, myrinet or something else.
    $cmgr->{NetworkInfo} = [ $config->{networkinfo} ] if $config->{networkinfo};

    my $cpuistribution = $cluster_info->{cpudistribution};
    $cpuistribution =~ s/cpu:/:/g;
    $cmgr->{LogicalCPUDistribution} = [ $cpuistribution ];

    {
        my $sharedsession = "True";
        $sharedsession = "False" if lc($config->{shared_filesystem}) eq "no";
        $cmgr->{WorkingAreaShared} = [ $sharedsession ];

        my ($sessionlifetime) = (split ' ', $config->{defaultttl});
        $sessionlifetime ||= 7*24*60*60;
        $cmgr->{WorkingAreaLifeTime} = [ $sessionlifetime ];

        my $gigstotal = ceil($host_info->{session_total} / 1024);
        my $gigsfree = ceil($host_info->{session_free} / 1024);
        $cmgr->{WorkingAreaTotal} = [ $gigstotal ];
        $cmgr->{WorkingAreaFree} = [ $gigsfree ];

        # OBS: There is no special area for MPI jobs
        $cmgr->{WorkingAreaMPIShared} = [ $sharedsession ];
        $cmgr->{WorkingAreaMPITotal} = [ $gigstotal ];
        $cmgr->{WorkingAreaMPIFree} = [ $gigsfree ];
        $cmgr->{WorkingAreaMPILifeTime} = [ $sessionlifetime ];
    }
    {
        my $gigstotal = ceil($host_info->{cache_total} / 1024);
        my $gigsfree = ceil($host_info->{cache_free} / 1024);
        $cmgr->{CacheTotal} = [ $gigstotal ];
        $cmgr->{CacheFree} = [ $gigsfree ];
    }

    # Not publishing absolute paths
    #$cmgr->{TmpDir};
    #$cmgr->{ScratchDir};
    #$cmgr->{ApplicationDir};


    # Computing Activities

    for my $jobid (keys %$gmjobs_info) {

        my $gmjob = $gmjobs_info->{$jobid};

        my $exited= undef; # whether the job has already run; 

        my $cact = {};
        push @{$csv->{ComputingActivities}{ComputingActivity}}, $cact;

        $cact->{CreationTime} = $creation_time;
        $cact->{Validity} = $validity_ttl;
        $cact->{BaseType} = 'Activity';

        my $share = $gmjob->{share};
        my $gridid = $cactIDs{$share}{$jobid};

        $cact->{Type} = [ 'Computing' ];
        $cact->{ID} = [ $gridid ];
        $cact->{IDFromEndpoint} = [ $gridid ];
        $cact->{Name} = [ $gmjob->{jobname} ] if $gmjob->{jobname};
        # TODO: properly set either ogf:jsdl:1.0 or nordugrid:xrsl
        $cact->{JobDescription} = [ "ogf:jsdl:1.0" ];
        $cact->{State} = [ ogsa_state($gmjob->{status}) or 'UNDEFINEDVALUE' ];
        $cact->{RestartState} = [ ogsa_state($gmjob->{failedstate},'R') or 'UNDEFINEDVALUE' ] if $gmjob->{failedstate};
        $cact->{ExitCode} = [ $gmjob->{exitcode} ] if defined $gmjob->{exitcode};
        # TODO: modify scan-jobs to write it separately to .diag. All backends should do this.
        $cact->{ComputingManagerExitCode} = [ $gmjob->{lrmsexitcode} ] if $gmjob->{lrmsexitcode};
        $cact->{Error} = [ map { substr($_,0,255) } @{$gmjob->{errors}} ] if $gmjob->{errors};
        # TODO: VO info, like <UserDomain>ATLAS/Prod</UserDomain>; check whether this information is available to A-REX
        $cact->{Owner} = [ $gmjob->{subject} ];
        $cact->{LocalOwner} = [ $gmjob->{localowner} ] if $gmjob->{localowner};
        # OBS: Times are in seconds.
        $cact->{RequestedTotalWallTime} = [ $gmjob->{reqwalltime} ] if defined $gmjob->{reqwalltime};
        $cact->{RequestedTotalCPUTime} = [ $gmjob->{reqcputime} ] if defined $gmjob->{reqcputime};
        # OBS: Should include name and version. Exact format not specified
        $cact->{RequestedApplicationEnvironment} = $gmjob->{runtimeenvironments} if $gmjob->{runtimeenvironments};
        $cact->{RequestedSlots} = [ $gmjob->{count} ];
        $cact->{StdIn} = [ $gmjob->{stdin} ] if $gmjob->{stdin};
        $cact->{StdOut} = [ $gmjob->{stdout} ] if $gmjob->{stdout};
        $cact->{StdErr} = [ $gmjob->{stderr} ] if $gmjob->{stderr};
        $cact->{LogDir} = [ $gmjob->{gmlog} ] if $gmjob->{gmlog};
        $cact->{ExecutionNode} = $gmjob->{nodenames} if $gmjob->{nodenames};
        $cact->{Queue} = [ $gmjob->{queue} ] if $gmjob->{queue};
        # Times for finished jobs
        $cact->{UsedTotalWallTime} = [ $gmjob->{WallTime} * $gmjob->{count} ] if defined $gmjob->{WallTime};
        $cact->{UsedTotalCPUTime} = [ $gmjob->{CpuTime} ] if defined $gmjob->{CpuTime};
        $cact->{UsedMainMemory} = [ ceil($gmjob->{UsedMem}/1024) ] if defined $gmjob->{UsedMem};
        my $usbmissiontime = mdstoiso($gmjob->{starttime} || '');
        $cact->{SubmissionTime} = [ $usbmissiontime ] if $usbmissiontime;
        # TODO: change gm to save LRMSSubmissionTime
        $cact->{ComputingManagerSubmissionTime} = [ 'NotImplemented' ];
        # TODO: this should be queried in scan-job.
        $cact->{StartTime} = [ 'NotImplemented' ];
        # TODO: scan-job has to produce this
        $cact->{ComputingManagerEndTime} = [ 'NotImplemented' ];
        my $endtime = mdstoiso($gmjob->{completiontime} || '');
        $cact->{EndTime} = [ $endtime ] if $endtime;
        $cact->{WorkingAreaEraseTime} = [ $gmjob->{cleanuptime} ] if $gmjob->{cleanuptime};
        $cact->{ProxyExpirationTime} = [ $gmjob->{delegexpiretime} ] if $gmjob->{delegexpiretime};
        # OBS: address of client as seen by the server is used.
        my $dnschars = '-.A-Za-z0-9';  # RFC 1034,1035
        my ($external_address, $port, $clienthost) = $gmjob->{clientname} =~ /^([$dnschars]+)(?::(\d+))?(?:;(.+))?$/;
        $cact->{SubmissionHost} = [ $external_address ] if $external_address;
        $cact->{SubmissionClientName} = [ $gmjob->{clientsoftware} ] if $gmjob->{clientsoftware};

        # Computing Activity Associations

        # TODO: add link
        $cact->{Associations}{ExecutionEnvironmentID} = [];
        $cact->{Associations}{ComputingEndpointID} = [ $cepID ];
        
        $cact->{Associations}{ActivityID} = $gmjob->{activityid} if $gmjob->{activityid};

        my $shareid = $cshaLIDs{$share} || '';
        $cact->{Associations}{ComputingShareLocalID} = [ $shareid ] if $shareid;

        if ( $gmjob->{status} eq "INLRMS" ) {
            my $lrmsid = $gmjob->{localid};
            $log->warning("No local id for job $jobid") and next unless $lrmsid;
            $cact->{LocalIDFromManager} = [ $lrmsid ];

            my $lrmsjob = $lrms_info->{jobs}{$lrmsid};
            $log->warning("No local job for $jobid") and next unless $lrmsjob;

            $cact->{State} = [ ogsa_state("INLRMS", $lrmsjob->{status}) or 'UNDEFINEDVALUE' ];
            $cact->{WaitingPosition} = [ $lrmsjob->{rank} ] if defined $lrmsjob->{rank};
            $cact->{ExecutionNode} = $lrmsjob->{nodes} if $lrmsjob->{nodes};
            unshift @{$cact->{OtherMessages}}, $_ for @{$lrmsjob->{comment}};

            # Times for running jobs
            $cact->{UsedTotalWallTime} = [ $lrmsjob->{walltime} * $gmjob->{count} ] if defined $lrmsjob->{walltime};
            $cact->{UsedTotalCPUTime} = [ $lrmsjob->{cputime} ] if defined $lrmsjob->{cputime};
            $cact->{UsedMainMemory} = [ ceil($lrmsjob->{mem}/1024) ] if defined $lrmsjob->{mem};
        }

    }

    return $csv;
}

1;

