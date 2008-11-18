package ARC1ClusterInfo;

# This InfoCollector combines the output of the other information collectors
# and prepares the GLUE2 information model of A-REX. The returned structure is
# meant to be converted to XML with XML::Simple.

use base InfoCollector;
use ARC1ClusterSchema;

use Storable;
use LogUtils;

use strict;

my $arc1_info_schema = ARC1ClusterSchema::arc1_info_schema();

our $log = LogUtils->getLogger(__PACKAGE__);


sub timenow(){
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = gmtime(time);
    return sprintf "%4d-%02d-%02dT%02d:%02d:%02d%1s", $year+1900, $mon+1, $mday,$hour,$min,$sec,"Z";
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

    # jobs in each GM state, by queue
    my %gmqueuecount;

    # grid jobs running in the lrms, by queue
    my %lrmsgridrunning;

    # grid jobs queued in the lrms, by queue
    my %lrmsgridqueued;

    # number of slots needed by all waiting jobs, per queue
    my %requestedslots;

    for my $job (values %{$gmjobs_info}) {
        my $qname = $job->{queue} || '';

        $log->warning("queue not defined") and next unless $qname;

        $gmtotalcount{totaljobs}++;
        $gmqueuecount{$qname}{totaljobs}++;

        # count grid jobs running and queued in LRMS for each queue

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
                    $gmqueuecount{$qname}{$state_name}++;
                    last STATES;
                }
            }
            # none of the %states matched this job
            $log->warning("Unexpected job status: $job->{status}");
        };


        next if $job->{status} eq 'DELETED';

        # if we got to this point, the job was not yet deleted
        $gmtotalcount{notdeleted}++;
        $gmqueuecount{$qname}{notdeleted}++;

        next if $job->{status} =~ /FINISHED/;
        next if $job->{status} =~ /FAILED/;
        next if $job->{status} =~ /KILLED/;

        # if we got to this point, the job has not yet finished
        $gmtotalcount{notfinished}++;
        $gmqueuecount{$qname}{notfinished}++;

        next if $job->{status} =~ /FINISHING/;
        next if $job->{status} =~ /CANCELING/;
        next if $job->{status} =~ /INLRMS/;

        # if we got to this point, the job whas not yet reached the LRMS
        $gmtotalcount{notsubmitted}++;
        $gmqueuecount{$qname}{notsubmitted}++;
        $requestedslots{$qname} += $job->{count} || 1;

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

        my $qname = $gmjobs_info->{$ID}{queue};

        # set the job_gridowner of the job (read from the job.id.local)
        # which is used as the key of the %gm_queued
        my $job_gridowner = $gmjobs_info->{$ID}{subject};

        # count the gm_queued jobs per grid users (SNs) and the total
        if ( grep /^$gmjobs_info->{$ID}{status}$/, @gmqueued_states ) {
            $gm_queued{$job_gridowner}++;
            $prelrmsqueued{$qname}++;
        }
        # count the GM PRE-LRMS pending jobs
        if ( grep /^$gmjobs_info->{$ID}{status}$/, @gmpendingprelrms_states ) {
           $pendingprelrms{$qname}++;
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

    # Locally Unique IDs
    my $csvID = 'csv0'; # ComputingService
    my $cepID = 'cep0'; # ComputingEndpoint
    my $cmgrID = 'cmgr0'; # ComputingManager
    my $cactID = 'cact0'; my @cactIDs; # ComputingActivity
    my $cshaID = 'csha0'; my @cshaIDs; # ComputingShare
    my $aenvID = 'aenv0'; my @aenvIDs; # ApplicationEnvironment
    my $xenvID = 'xenv0'; my @xenvIDs; # ExecutionEnvironment
    my $locID = 'loc0';   my @locIDs;  # Location
    my $conID = 'con0';   my @conIDs;  # Contact
    my $apolID = 'apol0'; my @apolIDs; # AccessPolicy
    my $mpolID = 'mpol0'; my @mpolIDs; # MappingPolicy

    my $csv = {};

    $csv->{CreationTime} = $creation_time;
    $csv->{Validity} = $validity_ttl;
    $csv->{BaseType} = 'Service';

    $csv->{ID} = [ $csvID++ ];

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

    $cep->{URL} = [ "https://$host_info->{hostname}:$config->{gm_port}/arex" ];
    $cep->{Technology} = [ 'webservice' ];
    $cep->{Interface} = [ 'OGSA-BES' ];
    #$cep->{InterfaceExtension} = '';
    $cep->{WSDL} = [ "https://$host_info->{hostname}:$config->{gm_port}/arex/?wsdl" ];
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

    for my $qname (keys %{$config->{queues}}) {

        my $qinfo = $lrms_info->{queues}{$qname};
        my $qconfig = { %$config, %{$config->{queues}{$qname}} };

        my $csha = {};

        push @{$csv->{ComputingShares}{ComputingShare}}, $csha;

        $csha->{CreationTime} = $creation_time;
        $csha->{Validity} = $validity_ttl;
        $csha->{BaseType} = 'Share';

        $csha->{LocalID} = [ $cshaID ]; push @cshaIDs, $cshaID++;

        $csha->{Name} = [ $qname ];
        $csha->{Description} = [ $qconfig->{comment} ] if $qconfig->{comment};
        $csha->{MappingQueue} = [ $qname ];

        # use limits from LRMS
        # TODO: convert backends to return values in seconds. Document new units in schema
        $csha->{MaxCPUTime} = [ $qinfo->{maxcputime} * 60 ] if defined $qinfo->{maxcputime};
        $csha->{MinCPUTime} = [ $qinfo->{mincputime} * 60 ] if defined $qinfo->{mincputime};
        $csha->{DefaultCPUTime} = [ $qinfo->{defaultcput} * 60 ] if defined $qinfo->{defaultcput};
        $csha->{MaxWallTime} =  [ $qinfo->{maxwalltime} * 60 ] if defined $qinfo->{maxwalltime};
        $csha->{MinWallTime} =  [ $qinfo->{minwalltime} * 60 ] if defined $qinfo->{minwalltime};
        $csha->{DefaultWallTime} = [ $qinfo->{defaultwallt} * 60 ] if defined $qinfo->{defaultwallt};

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
        $csha->{TotalJobs} = [ $gmqueuecount{$qname}{notfinished} || 0 ];

        $csha->{RunningJobs} = [ $lrmsgridrunning{$qname} || 0 ];
        $csha->{WaitingJobs} = [ $lrmsgridqueued{$qname} || 0 ];
        $csha->{LocalRunningJobs} = [ $qinfo->{running} - ($lrmsgridrunning{$qname} || 0) ]
            if defined $qinfo->{running}; 
        $csha->{LocalWaitingJobs} = [ $qinfo->{queued}  - ($lrmsgridqueued{$qname} || 0) ]
            if defined $qinfo->{queued}; 

	$csha->{StagingJobs} = [ ( $gmqueuecount{$qname}{perparing} || 0 )
                             + ( $gmqueuecount{$qname}{finishing} || 0 ) ];

        # OBS: suspending jobs is not yet supported
        $csha->{SuspendedJobs} = [ 0 ];

        $csha->{PreLRMSWaitingJobs} = [ $gmqueuecount{$qname}{notsubmitted} || 0 ];

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
        $csha->{FreeSlotsWithDuration} = [ 0 ] if $pendingprelrms{$qname};

        $csha->{UsedSlots} = [ $qinfo->{running} ];

        $csha->{RequestedSlots} = [ $requestedslots{$qname} || 0 ];

        # TODO: detect reservationpolicy in the lrms
        $csha->{ReservationPolicy} = [ $qinfo->{reservationpolicy} ]
            if $qinfo->{reservationpolicy};

        # Tag: skip it for now

    }


    # TODO: move after Shares are defined
    $cep->{Associations}{ComputingShareID} = [ @cshaIDs ];

    # TODO: add Jobs here
    $cep->{ComputingActivities}{ComputingActivity} = [ ];

    return $csv;
}


1;

