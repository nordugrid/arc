package ARC1ClusterInfo;

# This InfoCollector combines the output of the other information collectors
# and prepares the GLUE2 information model of A-REX. The returned structure is
# meant to be converted to XML with XML::Simple.

use base InfoCollector;
use ARC1ClusterSchema;

use Storable;
use LogUtils;
use POSIX qw(ceil);

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

# TODO: Substates need discussing, documenting
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

    # grid jobs running in the lrms, by share
    my %lrmsgridrunning;

    # grid jobs queued in the lrms, by share
    # TODO: check correctness when there are multiple shares per queue
    my %lrmsgridqueued;

    # number of slots needed by all waiting jobs, per share
    my %requestedslots;

    for my $jobid (keys %$gmjobs_info) {

        my $job = $gmjobs_info->{$jobid};
        my $qname = $job->{queue};

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
                    $log->warning($msg." and no default queue is defined. Picked first queue: ".$qnames[0]);
                }
                $qname = $job->{queue} = $qnames[0];
            }
        }

        my $share = $job->{share} || $qname;

        $gmtotalcount{totaljobs}++;
        $gmsharecount{$share}{totaljobs}++;

        # count grid jobs running and queued in LRMS for each share

        if ($job->{status} eq 'INLRMS') {
            my $lrmsid = $job->{localid};
            my $lrmsjob = $lrms_info->{jobs}{$lrmsid};

            if (defined $lrmsjob and $lrmsjob->{status} ne 'EXECUTED') {
                if ($lrmsjob->{status} eq 'R' or $lrmsjob->{status} eq 'S') {
                    $lrmsgridrunning{$qname}++;
                } else {
                    $lrmsgridqueued{$qname}++;
                    $requestedslots{$qname} += $job->{count} || 1;
                }
            }
        }

        # count by GM state

        my @states = ( { ACCEPTED  => 'accepted'  },
                       { PREPARING => 'preparing' },
                       { SUBMIT    => 'submit'    },
                       { INLRMS    => 'inlrms'    },
                       { CANCELING => 'canceling' },
                       { FINISHING => 'finishing' },
                       { KILLED    => 'finished'  },
                       { FAILED    => 'finished'  },
                       { FINISHED  => 'finished'  },
                       { DELETED   => 'deleted'   } );

        STATES: {
            for my $state (@states) {
                my ($state_match, $state_name) = %$state;

                if ($job->{status} =~ /$state_match/) {
                    $gmtotalcount{$state_name}++;
                    $gmsharecount{$share}{$state_name}++;
                    last STATES;
                }
            }
            # none of the %states matched this job
            $log->warning("Unexpected job status: $job->{status}");
        };


        next if $job->{status} eq 'DELETED';

        # if we got to this point, the job was not yet deleted
        $gmtotalcount{notdeleted}++;
        $gmsharecount{$share}{notdeleted}++;

        next if $job->{status} =~ /FINISHED/;
        next if $job->{status} =~ /FAILED/;
        next if $job->{status} =~ /KILLED/;

        # if we got to this point, the job has not yet finished
        $gmtotalcount{notfinished}++;
        $gmsharecount{$share}{notfinished}++;

        next if $job->{status} =~ /FINISHING/;
        next if $job->{status} =~ /CANCELING/;
        next if $job->{status} =~ /INLRMS/;

        # if we got to this point, the job whas not yet reached the LRMS
        $gmtotalcount{notsubmitted}++;
        $gmsharecount{$share}{notsubmitted}++;
        $requestedslots{$share} += $job->{count} || 1;

        next if $job->{status} =~ /SUBMIT/;
        next if $job->{status} =~ /PREPARING/;
        next if $job->{status} =~ /ACCEPTED/;

    }

    my %prelrmsqueued;
    my %pendingprelrms;
    my %gm_queued;
    my @gmqueued_states = ("ACCEPTED","PENDING:ACCEPTED","PREPARING","PENDING:PREPARING","SUBMIT");
    my @gmpendingprelrms_states =("PENDING:ACCEPTED","PENDING:PREPARING" );

    for my $job_gridowner (keys %$usermap) {
        $gm_queued{$job_gridowner} = 0;
    }

    for my $ID (keys %{$gmjobs_info}) {

        my $share = $gmjobs_info->{$ID}{share};

        # set the job_gridowner of the job (read from the job.id.local)
        # which is used as the key of the %gm_queued
        my $job_gridowner = $gmjobs_info->{$ID}{subject};

        # count the gm_queued jobs per grid users (SNs) and the total
        if ( grep /^$gmjobs_info->{$ID}{status}$/, @gmqueued_states ) {
            $gm_queued{$job_gridowner}++;
            $prelrmsqueued{$share}++;
        }
        # count the GM PRE-LRMS pending jobs
        if ( grep /^$gmjobs_info->{$ID}{status}$/, @gmpendingprelrms_states ) {
           $pendingprelrms{$share}++;
        }
    }


    # Grid Manager job state mappings to Infosys job states

    my %map_always = ( 'ACCEPTED'          => 'ACCEPTING',
                       'PENDING:ACCEPTED'  => 'ACCEPTED',
                       'PENDING:PREPARING' => 'PREPARED',
                       'PENDING:INLRMS'    => 'EXECUTED',
                       'CANCELING'         => 'KILLING');

    my %map_if_gm_up = ( 'SUBMIT' => 'SUBMITTING');

    my %map_if_gm_down = ( 'PREPARING' => 'ACCEPTED',
                           'FINISHING' => 'EXECUTED',
                           'SUBMIT'    => 'PREPARED');

    # We're running A-REX: Always assume GM is up
    $host_info->{processes}{'grid-manager'} = 1;

    for my $job (values %$gmjobs_info) {

        if ( grep ( /^$job->{status}$/, keys %map_always ) ) {
            $job->{status} = $map_always{$job->{status}};
        }
        if ( grep ( /^$job->{status}$/, keys %map_if_gm_up ) and
             $host_info->{processes}{'grid-manager'} ) {
            $job->{status} = $map_if_gm_up{$job->{status}};
        }
        if ( grep ( /^$job->{status}$/, keys %map_if_gm_down ) and
             ! $host_info->{processes}{'grid-manager'} ) {
            $job->{status} = $map_if_gm_down{$job->{status}};
        }
    }


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

    my $csv = {};

    $csv->{CreationTime} = $creation_time;
    $csv->{Validity} = $validity_ttl;
    $csv->{BaseType} = 'Service';

    $csv->{ID} = [ $csvID ];

    $csv->{Name} = [ $config->{cluster_alias} ] if $config->{cluster_alias};
    $csv->{Capability} = [ 'executionmanagement.jobexecution' ];
    $csv->{Type} = [ 'org.nordugrid.arex' ];

    # QualityLevel: new config option ?
    $csv->{QualityLevel} = [ 'development' ];

    # StatusPage: new config option ?

    # OBS: assuming one share per queue
    my $nqueues = keys %{$config->{queues}};
    $csv->{Complexity} = [ "endpoint=1,share=$nqueues,resource=1" ];

    # OBS: Finished/failed/deleted jobs are not counted
    $csv->{TotalJobs} = [ $gmtotalcount{notfinished} || 0 ];

    my $nrun = 0; $nrun += $_ for values %lrmsgridrunning;
    my $nque = 0; $nque += $_ for values %lrmsgridqueued;
    $csv->{RunningJobs} = [ $nrun ];
    $csv->{WaitingJobs} = [ $nque ];

    $csv->{StagingJobs} = [ ( $gmtotalcount{perparing} || 0 )
                         + ( $gmtotalcount{finishing} || 0 ) ];

    # Suspended not supported yet
    $csv->{SuspendedJobs} = [ 0 ];

    # should this include staging in jobs?
    my $npreq = 0; $npreq += $_ for values %prelrmsqueued;
    $csv->{PreLRMSWaitingJobs} = [ $npreq ];


    my $cep = {};
    $csv->{ComputingEndpoint} = [ $cep ];

    $cep->{CreationTime} = $creation_time;
    $cep->{Validity} = $validity_ttl;
    $cep->{BaseType} = 'Endpoint';

    $cep->{ID} = [ $cepID ];

    # Name not necessary

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


    $csv->{ComputingShares} = { ComputingShare => [] };


    # ComputingShares: 1 share per LRMS queue

    for my $share (keys %{$config->{queues}}) {

        my $qname = $share; # This may change

        my $qinfo = $lrms_info->{queues}{$qname};
        my $qconfig = { %$config, %{$config->{queues}{$qname}} };

        my $csha = {};

        push @{$csv->{ComputingShares}{ComputingShare}}, $csha;

        $csha->{CreationTime} = $creation_time;
        $csha->{Validity} = $validity_ttl;
        $csha->{BaseType} = 'Share';

        $csha->{LocalID} = [ $cshaLID ]; $cshaLIDs{$qname} = $cshaLID++;

        $csha->{Name} = [ $share ];
        $csha->{Description} = [ $qconfig->{comment} ] if $qconfig->{comment};
        $csha->{MappingQueue} = [ $qname ];

        # use limits from LRMS
        $csha->{MaxCPUTime} = [ $qinfo->{maxcputime} ] if defined $qinfo->{maxcputime};
        $csha->{MinCPUTime} = [ $qinfo->{mincputime} ] if defined $qinfo->{mincputime};
        $csha->{DefaultCPUTime} = [ $qinfo->{defaultcput} ] if defined $qinfo->{defaultcput};
        $csha->{MaxWallTime} =  [ $qinfo->{maxwalltime} ] if defined $qinfo->{maxwalltime};
        $csha->{MinWallTime} =  [ $qinfo->{minwalltime} ] if defined $qinfo->{minwalltime};
        $csha->{DefaultWallTime} = [ $qinfo->{defaultwallt} ] if defined $qinfo->{defaultwallt};

        my ($maxtotal, $maxlrms) = split ' ', ($qconfig->{maxjobs} || '');

        # MaxWaitingJobs: use GM's maxjobs config option
        # OBS: GM only cares about totals, not per share limits!
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
        # OBS: GM only cares about totals, not per share limits!
    # OBS: this formula is actually an upper limit on the sum of pre + post
    #      lrms jobs. GM does not have separate limit for pre lrms jobs
    $csha->{MaxPreLRMSWaitingJobs} = [ $maxtotal - $maxlrms ]
            if defined $maxtotal and defined $maxlrms;

        $csha->{MaxUserRunningJobs} = [ $qinfo->{maxuserrun} ]
            if defined $qinfo->{maxuserrun};

        # OBS: new config option. Default value is 1
        # TODO: new return value from LRMS infocollector
        # TODO: see how LRMSs can detect the correct value
        $csha->{MaxSlotsPerJob} = [ $qconfig->{maxslotsperjob} || $qinfo->{maxslotsperjob}  || 1 ];

        # MaxStageInStreams, MaxStageOutStreams
        # OBS: GM does not have separate limits for up and downloads.
        # OBS: GM only cares about totals, not per share limits!
        my ($maxstaging) = split ' ', ($qconfig->{maxload} || '');
        $csha->{MaxStageInStreams}  = [ $maxstaging ] if defined $maxstaging;
        $csha->{MaxStageOutStreams} = [ $maxstaging ] if defined $maxstaging;

        # OBS: 'maui' is not a valid SchedulingPolicy
        $qconfig->{scheduling_policy} = 'fairshare'
            if defined($qconfig->{scheduling_policy})
                and lc($qconfig->{scheduling_policy}) eq 'maui';

        # TODO: new return value from LRMS infocollector.
        $csha->{SchedulingPolicy} = [ $qinfo->{schedpolicy} ] if $qinfo->{schedpolicy};
        $csha->{SchedulingPolicy} = [ lc $qconfig->{scheduling_policy} ] if $qconfig->{scheduling_policy};

        # TODO: get it from ExecutionEnviromnets mapped to this share instead
        $csha->{MaxMemory} = [ $qconfig->{nodememory} ] if $qconfig->{nodememory};

        # OBS: new config option (space measured in GB !?)
        # OBS: only informative, not enforced,
        # TODO: implement check at job accept time in a-rex
        $csha->{MaxDiskSpace} = [ $qconfig->{maxdiskperjob} ] if $qconfig->{maxdiskperjob};

        # DefaultStorageService: Has no meaning for ARC

        # TODO: new return value from LRMS infocollector.
        $csha->{Preemption} = [ $qinfo->{preemption} ] if $qinfo->{preemption};

        # ServingState: closed and queuing are not yet supported
        if (defined $qconfig->{allownew} and lc($qconfig->{allownew}) eq 'no') {
            $csha->{ServingState} = [ 'production' ];
        } else {
            $csha->{ServingState} = [ 'draining' ];
        }

        # OBS: Finished/failed/deleted jobs are not counted
        $csha->{TotalJobs} = [ $gmsharecount{$share}{notfinished} || 0 ];

        $csha->{RunningJobs} = [ $lrmsgridrunning{$qname} || 0 ];
        $csha->{WaitingJobs} = [ $lrmsgridqueued{$qname} || 0 ];
        $csha->{LocalRunningJobs} = [ $qinfo->{running} - ($lrmsgridrunning{$qname} || 0) ]
            if defined $qinfo->{running}; 
        $csha->{LocalWaitingJobs} = [ $qinfo->{queued}  - ($lrmsgridqueued{$qname} || 0) ]
            if defined $qinfo->{queued}; 

    $csha->{StagingJobs} = [ ( $gmsharecount{$share}{perparing} || 0 )
                             + ( $gmsharecount{$share}{finishing} || 0 ) ];

        # OBS: suspending jobs is not yet supported
        $csha->{SuspendedJobs} = [ 0 ];

        $csha->{PreLRMSWaitingJobs} = [ $gmsharecount{$share}{notsubmitted} || 0 ];

        # TODO: get these estimates from maui/torque
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

        # Don't advertise free slots while busy with staging
        $csha->{FreeSlotsWithDuration} = [ 0 ] if $pendingprelrms{$share};

        $csha->{UsedSlots} = [ $qinfo->{running} ];

        $csha->{RequestedSlots} = [ $requestedslots{$qname} || 0 ];

        # TODO: detect reservationpolicy in the lrms
        $csha->{ReservationPolicy} = [ $qinfo->{reservationpolicy} ]
            if $qinfo->{reservationpolicy};

        # Tag: skip it for now

    }

    # Computing Activities

    for my $jobid (keys %$gmjobs_info) {

        my $gmjob = $gmjobs_info->{$jobid};

        my $exited= undef; # whether the job has already run; 

        my $cact = {};
        push @{$csv->{ComputingActivities}{ComputingActivity}}, $cact;

        $cact->{CreationTime} = $creation_time;
        $cact->{Validity} = $validity_ttl;
        $cact->{BaseType} = 'Activity';

        my $gridid = "https://$arexhostport/arex/$jobid";
        $cactIDs{$jobid} = $gridid;

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

        my $share = $gmjob->{share};
        if ( $share and $cshaLIDs{$share} ) {
            $cact->{Associations}{ComputingShareLocalID} = [ $cshaLIDs{$share} ];
        } else {
            $log->warning("Job $jobid does not belong to a configured share (queue)");
        }

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


    $cep->{Associations}{ComputingShareLocalID} = [ values %cshaLIDs ];
    $cep->{Associations}{ComputingActivityID} = [ values %cactIDs ];

    return $csv;
}

1;

