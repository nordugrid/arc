package ARC0ClusterInfo;

use File::Basename;
use lib dirname($0);

# This InfoCollector gathers full information about the cluster, queues, users
# and jobs.  Prepares info modelled on the classic Nordugrid information schema
# (arc0).  Returned structure is meant to be converted to XML with XML::Simple.

use base InfoCollector;
use HostInfo;
use GMJobsInfo;
use LRMSInfo;
use ARC0ClusterSchema;

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

my $arc0_info_schema = ARC0ClusterSchema::arc0_info_schema();

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

    my %gmjobcount;
    for my $job (values %{$gmjobs_info}) {
        $gmjobcount{totaljobs}++;
        if ( $job->{status} =~ /DELETED/  ) { $gmjobcount{deleted}++  ; next; }
        $gmjobcount{notdeleted}++;
        if ( $job->{status} =~ /FINISHED/ ) { $gmjobcount{finished}++ ; next; }
        if ( $job->{status} =~ /FAILED/   ) { $gmjobcount{finished}++ ; next; }
        if ( $job->{status} =~ /KILLED/   ) { $gmjobcount{finished}++ ; next; }
        $gmjobcount{notfinished}++;
        if ( $job->{status} =~ /FINISHING/) { $gmjobcount{finishing}++; next; }
        if ( $job->{status} =~ /CANCELING/) { $gmjobcount{canceling}++; next; }
        if ( $job->{status} =~ /INLRMS/   ) { $gmjobcount{inlrms}++   ; next; }
        if ( $job->{status} =~ /SUBMIT/   ) { $gmjobcount{submit}++   ; next; }
        if ( $job->{status} =~ /PREPARING/) { $gmjobcount{preparing}++; next; }
        if ( $job->{status} =~ /ACCEPTED/ ) { $gmjobcount{accepted}++ ; next; }
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
    my $csID = 'cs0'; # ComputingService
    my $ceID = 'ce0'; # ComputingEndpoint
    my $cmID = 'cm0'; # ComputingManager
    my $caID = 'ca0'; # ComputingActivity
    my $shID = 'sh0'; # ComputingShare
    my $aeID = 'ae0'; # ApplicationEnvironment
    my $xeID = 'xe0'; # ExecutionEnvironment
    my $loID = 'lo0'; # Location
    my $coID = 'co0'; # Contact
    my $apID = 'ap0'; # AccessPolicy
    my $mpID = 'mp0'; # MappingPolicy

    my $services = {};

    my $cs = {};
    $services{ComputingService} = [ $cs ];

    $cs->{CreationTime} = $creation_time;
    $cs->{Validity} = $validity_ttl;
    $cs->{BaseType} = 'Service';

    $cs->{ID} = $csID++;
    $cs->{Name} = $config->{cluster_alias} if $config->{cluster_alias};
    $cs->{Capability} = 'executionmanagement.jobexecution';
    $cs->{Type} = 'org.nordugrid.arex';
    $cs->{QualityLevel} = 'pre-production';

    # StatusPage: new config option ?

    # OBS: assuming one share per queue
    my $nqueues = keys %{$options->{queues}};
    $cs->{Complexity} = "endpoint=1,share=$nqueues,resource=1";

    $cs->{TotalJobs} = $gmjobcount{notdeleted}; # OBS: Finished also counted
    $cs->{RunningJobs} += $_ for values $gridrunning;
    $cs->{WaitingJobs} += $_ for values %gridqueued;
    $cs->{StagingJobs} = $gmjobcount{preparing} + $gmjobcount{finishing};

    # Suspended not supported yet
    $cs->{SuspendedJobs} = 0;

    # should this include staging in jobs?
    $cs->{PreLRMSWaitingJobs} += $_ for values %prelrmsqueued;

    my $ce = {};
    $cs->{ComputingEndpoint} = [ $ce ];

    $ce->{CreationTime} = $creation_time;
    $ce->{Validity} = $validity_ttl;
    $ce->{BaseType} = 'Endpoint';

    $ce->{ID} = $ceID++;

    # Name ?

    $ce->{URL} = "https://$host_info->{hostname}:$config->{gm_port}/arex";
    $ce->{Technology} = [ 'webservice' ];
    $ce->{Interface} = [ 'OGSA-BES' ];
    #$ce->{InterfaceExtension} = '';
    $ce->{WSDL} = [ "https://$host_info->{hostname}:$config->{gm_port}/arex/?wsdl" ];
    # Wrong type, should be URI
    $ce->{SupportedProfile} = [ "WS-I 1.0", "HPC-BP" ];
    $ce->{Semantics} = [ "http://www.nordugrid.org/documents/arex.pdf" ];
    $ce->{Implementor} = [ "NorduGrid" ];
    $ce->{ImplementationName} = [ "ARC1" ];

    # TODO: use value from config.h
    $ce->{ImplementationVersion} = [ "0.9" ];

    # TODO: add config option
    $ce->{QualityLevel} = [ "testing" ];

    # TODO: health state moitoring
    $ce->{HealthState} = [ "ok" ];
    #$ce->{HealthStateInfo} = [ "" ];

    # TODO: when is it 'queueing' and 'closed'?
    $ce->{ServingState} = ($config->{allownew} eq "no") : 'draining' ? 'production';

    # StartTime: get it from hed

    $ce->{IssuerCA} = [ $host_info->{issuerca} ];
    $ce->{TrustedCA} = $host_info->{trustedcas};

    # TODO: Downtime

    $ce->{Staging} = 'staginginout';
    $ce->{JobDescription} = 'ogf:jsdl:1.0';

    return $services;
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

    my $cluster_info = ARC0ClusterInfo->new()->get_info($config);
    #print Dumper($cluster_info);

    require XML::Simple;
    my $xml = new XML::Simple(NoAttr => 0, ForceArray => 1, RootName => 'n:nordugrid', KeyAttr => ['name']);
    print $xml->XMLout($cluster_info);
}

#test;

1;

