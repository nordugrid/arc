package ARC1ClusterInfo;

use File::Basename;
use lib dirname($0);

# This InfoCollector gathers full information about the cluster, queues, users
# and jobs.  Prepares info modelled on the classic Nordugrid information schema
# (arc0).  Returned structure is meant to be converted to XML with XML::Simple.

use base InfoCollector;
use HostInfo;
use GMJobsInfo;
use LRMSInfo;
use ARC1ClusterSchema;

use Storable;
use LogUtils;

use strict;

my $arc0_options_schema = {
    lrms => '',              # name of the LRMS module
    gridmap => '',
    x509_user_cert => '',
    x509_cert_dir => '',
    sessiondir => '',
    cachedir => '*',
    ng_location => '',
    runtimedir => '*',
    processes => [ '' ],
    queues => {              # queue names are keys in this hash
        '*' => {
            name => '*'
        }
    }
};

my $arc0_info_schema = ARC1ClusterSchema::arc1_info_schema();

our $log = LogUtils->getLogger(__PACKAGE__);


# override InfoCollector base class methods

sub _get_options_schema {
    return $arc0_options_schema;
}
sub _get_results_schema {
    return $arc0_info_schema;
}

sub _collect($$) {
    my ($self,$config) = @_;

    # get all local users from grid-map. Sort unique
    my %saw = ();
    my $usermap = read_grid_mapfile($config->{gridmap});
    my @localusers = grep !$saw{$_}++, values %$usermap;

    my $host_info = get_host_info($config,\@localusers);

    my $gmjobs_info = get_gmjobs_info($config);

    my @jobids;
    for my $job (values %$gmjobs_info) {
        # Uncomment later! Query all jobs for now
        #next unless $job->{status} eq 'INLRMS';
        next unless $job->{localid};
        push @jobids, $job->{localid};
    }
    my $lrms_info = get_lrms_info($config,\@localusers,\@jobids);

    return get_cluster_info ($config, $usermap, $host_info, $gmjobs_info, $lrms_info);
}

############################################################################
# Call other InfoCollectors
############################################################################

sub get_host_info($$) {
    my ($config,$localusers) = @_;

    my $host_opts = { %$config, localusers => $localusers };

    return HostInfo->new()->get_info($host_opts);
}

sub get_gmjobs_info($) {
    my $config = shift;
    my $gmjobs_opts = { controldir => $config->{controldir} };
    return GMJobsInfo->new()->get_info($gmjobs_opts);
}

sub get_lrms_info($$$) {
    my ($config,$localusers,$jobids) = @_;

    # possibly any options from config are needed , so just clone it all
    my $options = Storable::dclone($config);
    $options->{jobs} = $jobids;

    my @queues = keys %{$options->{queues}};
    for my $queue ( @queues ) {
        $options->{queues}{$queue}{users} = $localusers;
    }
    return LRMSInfo->new()->get_info($options);
}

#### grid-mapfile hack #####

sub read_grid_mapfile($) {
    my $gridmapfile = shift;
    my $usermap = {};

    unless (open MAPFILE, "<$gridmapfile") {
        $log->warning("can't open gridmapfile at $gridmapfile");
        return;
    }
    while(my $line = <MAPFILE>) {
        chomp($line);
        if ( $line =~ m/\"([^\"]+)\"\s+(\S+)/ ) {
            $usermap->{$1} = $2;
        }
    }
    close MAPFILE;

    return $usermap;
}

sub timenow(){
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = gmtime(time);
    return sprintf "%4d-%02d-%02dT%02d:%02d:%02d%1s", $year+1900, $mon+1, $mday,$hour,$min,$sec,"Z";
}

############################################################################
# Combine info from all sources to prepare the final representation
############################################################################

sub get_cluster_info($$$$) {
    my ($config,$usermap,$host_info,$gmjobs_info,$lrms_info) = @_;

    my $creation_time = timenow();
    my $validity_ttl = $config->{ttl};

    # adotf means autodetect on the frontend
    $config->{architecture} = $host_info->{architecture} if $config->{architecture} eq 'adotf';
    $config->{nodecpu} = $host_info->{nodecpu} if $config->{nodecpu} eq 'adotf';
    $config->{opsys} = $host_info->{opsys} if $config->{opsys} eq 'adotf';

    # config overrides
    $host_info->{hostname} = $config->{hostname} if $config->{hostname};

    # count grid-manager jobs

    my %gmtotalcount;
    my %gmqueuecount;

    JOBLOOP:
    for my $job (values %{$gmjobs_info}) {
        my $qname = $job->{queue} || '';

        $gmtotalcount{totaljobs}++;
        $gmqueuecount{$qname}{totaljobs}++ if $qname;

        # order matters
        my %states = ( DELETED   => 'deleted',
                       FINISHED  => 'finished',
                       FAILED    => 'finished',
                       KILLED    => 'finished',
                       FINISHING => 'finishing',
                       CANCELING => 'canceling',
                       INLRMS    => 'inlrms',
                       SUBMIT    => 'submit',
                       PREPARING => 'preparing',
                       ACCEPTED  => 'accepted');

        # loop GM states, from later to earlier
        for my $state_match (keys %states) {
            my $state_name = $states{$state_match};

            if ($job->{status} =~ /$state_match/) {
                $gmtotalcount{$state_name}++;
                $gmqueuecount{$qname}{$state_name}++ if $qname;
                next JOBLOOP;
            }

            next JOBLOOP if $state_match eq 'DELETED';

            # if we got to this point, the job was not yet deleted
            $gmtotalcount{notdeleted}++;
            $gmqueuecount{$qname}{notdeleted}++ if $qname;

            next JOBLOOP if $state_match eq 'FINISHING';

            # if we got to this point, the job has not yet finished
            $gmtotalcount{notfinished}++;
            $gmqueuecount{$qname}{notfinished}++ if $qname;

            next JOBLOOP if $state_match eq 'INLRMS';

            # if we got to this point, the job whas not yet reached the LRMS
            $gmtotalcount{notsubmitted}++;
            $gmqueuecount{$qname}{notsubmitted}++ if $qname;
        }

        # none of the %states matched this job
        $log->error("Unexpected job status: $job->{status}");
    }

    # count grid jobs running and queued in LRMS for each queue

    my %gridrunning;
    my %gridqueued;
    my $lrmsjobs = $lrms_info->{jobs};
    my %local2gm;
    $local2gm{$gmjobs_info->{$_}{localid}} = $_ for keys %$gmjobs_info;
    foreach my $jid (keys %$lrmsjobs) {
        my $gridid = $local2gm{$jid};
        next unless $gridid; # not a grid job
        my $status = $lrmsjobs->{$jid}{status};
        next unless $status; # probably executed
        next if $status eq 'EXECUTED';
        my $qname = $gmjobs_info->{$gridid}{queue};
        if ($status eq 'R' || $status eq 'S') {
            $gridrunning{$qname}++;
        } else {
            $gridqueued{$qname}++;
        }
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

    my $nrun = 0; $nrun += $_ for values %gridrunning;
    my $nque = 0; $nque += $_ for values %gridqueued;
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

    $cep->{ID} = [ $cepID++ ];

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

    # TODO: health state moitoring
    $cep->{HealthState} = [ "ok" ];
    #$cep->{HealthStateInfo} = [ "" ];

    # TODO: when is it 'queueing' and 'closed'?
    $cep->{ServingState} = [ ($config->{allownew} eq "no") ? 'draining' : 'production' ];

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
        $csha->{MaxTotalJobs} = $maxtotal if defined $maxtotal;

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
            if lc($qconfig->{scheduling_policy}) eq 'maui';

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
        $csha->{TotalJobs} = [ $gmtotalcount{notfinished} ];

        $csha->{RunningJobs} = [ $gridrunning{$qname} || 0 ];
        $csha->{WaitingJobs} = [ $gridqueued{$qname} || 0 ];
        $csha->{LocalRunningJobs} = [ $qinfo->{running} - $csha->{RunningJobs} ]
            if defined $qinfo->{running}; 
        $csha->{LocalWaitingJobs} = [ $qinfo->{queued}  - $csha->{WaitingJobs} ]
            if defined $qinfo->{queued}; 

	$csha->{StagingJobs} = [ ( $gmqueuecount{$qname}{perparing} || 0 )
                             + ( $gmqueuecount{$qname}{finishing} || 0 ) ];

        # OBS: suspending jobs is not yet supported
        $csha->{SuspendedJobs} = [ 0 ];

        $csha->{PreLRMSWaitingJobs} = [ $gmqueuecount{$qname}{notsubmitted} ];

        # TODO: get these estimates from maui/torque
        $csha->{EstimatedAverageWaitingTime} = [ $qinfo->{averagewaitingtime} ] if defined $qinfo->{averagewaitingtime};
        $csha->{EstimatedWorstWaitingTime} = [ $qinfo->{worstwaitingtime} ] if defined $qinfo->{worstwaitingtime};

        # $csha->{FreeSlots} =;

    }


    # TODO: move after Shares are defined
    $cep->{Associations}{ComputingShareID} = [ @cshaIDs ];

    # TODO: add Jobs here
    $cep->{ComputingActivities}{ComputingActivity} = [ ];

    return $csv;
}


#### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ####

my $config = {
          'lrms' => 'sge',
          #'lrms' => 'fork',
          'pbs_log_path' => '/var/spool/pbs/server_logs',
          'pbs_bin_path' => '/opt/pbs',
          'sge_cell' => 'cello',
          'sge_root' => '/opt/n1ge6',
          'sge_bin_path' => '/opt/n1ge6/bin/lx24-x86',
          'll_bin_path' => '/opt/ibmll/LoadL/full/bin',
          'lsf_profile_path' => '/usr/share/lsf/conf',

          'hostname' => 'squark.uio.no',
          'opsys' => 'Linux-2.4.21-mypatch[separator]glibc-2.3.1[separator]Redhat-7.2',
          'architecture' => 'adotf',
          'nodememory' => '512',
          'middleware' => 'Cheese and Salad[separator]nordugrid-old-and-busted',

          'ng_location' => '/opt/nordugrid',
          'gridmap' => '/etc/grid-security/grid-mapfile',
          'controldir' => '/tmp/arc1/control',
          'sessiondir' => '/tmp/arc1/session',
          'runtimedir' => '/home/grid/runtime',
          'cachedir' => '/home/grid/cache',
          'cachesize' => '10000000000 8000000000',
          'authorizedvo' => 'developer.nordugrid.org',
          'x509_user_cert' => '/etc/grid-security/hostcert.pem',
          'x509_user_key' => '/etc/grid-security/hostkey.pem',
          'x509_cert_dir' => '/scratch/adrianta/arc1/etc/grid-security/certificates',

          'processes' => [ 'gridftpd', 'grid-manager', 'arched' ],
          'nodeaccess' => 'inbound[separator]outbound',
          'homogeneity' => 'TRUE',
          'gm_mount_point' => '/jobs',
          'gm_port' => '2811',
          'defaultttl' => '259200',
          'ttl' => '600',

          'mail' => 'v.a.taga@fys.uio.no',
          'cluster_alias' => 'Big Blue Cluster in Nowhere',
          'clustersupport' => 'v.a.taga@fys.uio.no',
          'cluster_owner' => 'University of Oslo',
          'cluster_location' => 'NO-xxxx',
          'comment' => 'Adrian\'s test bed cluster',
          'benchmark' => 'SPECFP2000 333[separator]BOGOMIPS 1000',

          'queues' => {
                        'all.q' => {
                                     'name' => 'all.q',
                                     'sge_jobopts' => '-r yes',
                                     'nodecpu' => 'adotf',
                                     'opsys' => 'Mandrake 7.0',
                                     'architecture' => 'adotf',
                                     'nodememory' => '512',
                                     'comment' => 'Dedicated queue for ATLAS users'
                                   }
                      },
        };

sub test {
    require Data::Dumper; import Data::Dumper qw(Dumper);
    LogUtils->getLogger()->level($LogUtils::DEBUG);

    my $cluster_info = ARC1ClusterInfo->new()->get_info($config);
    #print Dumper($cluster_info);

    require XML::Simple;
    my $xml = new XML::Simple(NoAttr => 0, ForceArray => 1, RootName => 'ComputingService', KeyAttr => ['name']);
    print $xml->XMLout($cluster_info);
}

test;

1;

