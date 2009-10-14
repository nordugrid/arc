package ARC0ClusterInfo;

# This information collector combines the output of the other information collectors
# and prepares info modelled on the classic Nordugrid information schema
# (arc0).  Returned structure is meant to be converted to XML with XML::Simple.

use Storable;

use strict;

use LogUtils;
use InfoChecker;

use ARC0ClusterSchema;

my $arc0_info_schema = ARC0ClusterSchema::arc0_info_schema();

our $log = LogUtils->getLogger(__PACKAGE__);


sub mds_valid($){
    my $ttl = shift;
    my $seconds = time;
    my ($from, $to);
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = gmtime($seconds);
    $from = sprintf "%4d%02d%02d%02d%02d%02d%1s", $year+1900, $mon+1, $mday,$hour,$min,$sec,"Z";
    ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = gmtime ($seconds+$ttl);
    $to = sprintf "%4d%02d%02d%02d%02d%02d%1s", $year+1900,$mon+1,$mday,$hour,$min,$sec,"Z";
    return $from, $to;
}

############################################################################
# Combine info from all sources to prepare the final representation
############################################################################

sub collect($) {
    my ($options) = @_;
    my ($checker, @messages);

    my $result = get_cluster_info($options);

    $checker = InfoChecker->new($arc0_info_schema);
    @messages = $checker->verify($result,1);
    $log->debug("SelfCheck: return value nordugrid->$_") foreach @messages;

    return $result;
}

sub get_cluster_info($) {
    my ($options) = @_; 
    my $config = $options->{config};
    my $usermap = $options->{usermap};
    my $host_info = $options->{host_info};
    my $gmjobs_info = $options->{gmjobs_info};
    my $lrms_info = $options->{lrms_info};

    my $ttl = $config->{ttl};
    my ($valid_from, $valid_to) = $ttl ? mds_valid($ttl) : ();

    my @allxenvs = keys %{$config->{xenvs}};
    my @allshares = keys %{$config->{shares}};

    my $homogeneous = 1; 
    $homogeneous = 0 if @allxenvs > 1;
    $homogeneous = 0 if @allshares > 1 and @allxenvs == 0;
    for my $xeconfig (values %{$config->{xenvs}}) {
        $homogeneous = 0 if defined $xeconfig->{Homogeneous}
                            and not $xeconfig->{Homogeneous};
    }

    # config overrides
    $host_info->{hostname} = $config->{hostname} if $config->{hostname};

    # count grid-manager jobs

    my %gmjobcount = (totaljobs => 0,
                      accepted => 0, preparing => 0, submit => 0, inlrms => 0,
                      canceling => 0, finishing => 0, finished => 0, deleted => 0);
    for my $job (values %{$gmjobs_info}) {
        $gmjobcount{totaljobs}++;
        if ( $job->{status} =~ /ACCEPTED/ ) { $gmjobcount{accepted}++ ; next; }
        if ( $job->{status} =~ /PREPARING/) { $gmjobcount{preparing}++; next; }
        if ( $job->{status} =~ /SUBMIT/   ) { $gmjobcount{submit}++   ; next; }
        if ( $job->{status} =~ /INLRMS/   ) { $gmjobcount{inlrms}++   ; next; }
        if ( $job->{status} =~ /CANCELING/) { $gmjobcount{canceling}++; next; }
        if ( $job->{status} =~ /FINISHING/) { $gmjobcount{finishing}++; next; }
        if ( $job->{status} =~ /FINISHED/ ) { $gmjobcount{finished}++ ; next; }
        if ( $job->{status} =~ /FAILED/   ) { $gmjobcount{finished}++ ; next; }
        if ( $job->{status} =~ /KILLED/   ) { $gmjobcount{finished}++ ; next; }
        if ( $job->{status} =~ /DELETED/  ) { $gmjobcount{deleted}++  ; next; }
        $log->warning("Unexpected job status: $job->{status}");
    }

    # count grid jobs running and queued in LRMS for each queue

    my %gridrunning;
    my %gridqueued;
    for my $jobid (keys %{$gmjobs_info}) {
        my $job = $gmjobs_info->{$jobid};
        my $share = $job->{share};

        if ($job->{status} eq 'INLRMS') {
            my $lrmsid = $job->{localid};
            unless (defined $lrmsid) {
                $log->warning("localid missing for INLRMS job $jobid");
                next;
            }
            my $lrmsjob = $lrms_info->{jobs}{$lrmsid};
            unless ((defined $lrmsjob) and $lrmsjob->{status}) {
                $log->warning("LRMS plugin returned no status for job $jobid (lrmsid: $lrmsid)");
                die;
            }
            if ((defined $lrmsjob) and $lrmsjob->{status} ne 'EXECUTED') {
                if ($lrmsjob->{status} eq 'R' or $lrmsjob->{status} eq 'S') {
                    $gridrunning{$share}++;
                } else {
                    $gridqueued{$share}++;
                }
            }
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

    # Infosys is run by A-REX: Always assume GM is up
    $host_info->{processes}{'grid-manager'} = 1;

    for my $job (values %$gmjobs_info) {

        $job->{status} = $map_always{$job->{status}}
            if grep { $job->{status} eq $_ } keys %map_always;

        if ($host_info->{processes}{'grid-manager'}) {
            $job->{status} = $map_if_gm_up{$job->{status}}
                if grep { $job->{status} eq $_ } keys %map_if_gm_up;
        } else {
            $job->{status} = $map_if_gm_down{$job->{status}}
                if grep { $job->{status} eq $_ } keys %map_if_gm_down;
        }
    }

    my @supportmails;
    if ($config->{contacts}) {
        for (@{$config->{contacts}}) {
            push @supportmails, $1 if $_->{Detail}  =~ m/^mailto:(.*)/;
        }
    }
    my %authorizedvos;
    $authorizedvos{"VO:$_"} = 1 for @{$config->{service}{AuthorizedVO}};
    for my $sconfig (values %{$config->{shares}}) {
        next unless $sconfig->{AuthorizedVO};
        $authorizedvos{"VO:$_"} = 1 for @{$sconfig->{AuthorizedVO}};
    }

	# Assume no connectivity unles explicitly configured otherwise on each
	# ExecutionEnvironment
    my ($inbound, $outbound) = (1,1);
    for my $xeconfig (values %{$config->{xenvs}}) {
        $inbound = 0 unless ($xeconfig->{connectivityIn} || 'false') eq 'true';
        $outbound = 0 unless ($xeconfig->{connectivityOut} || 'false') eq 'true';
    }
    $inbound = 1 if ($config->{service}{connectivityIn} || 'false') eq 'true';
    $outbound = 1 if ($config->{service}{connectivityOut} || 'false') eq 'true';

    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
    # # # # # # # # # # build information tree  # # # # # # # # # #
    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

    my $c = {};

    $c->{'xmlns:n'} = "urn:nordugrid";
    $c->{'xmlns:M0'} = "urn:Mds";
    $c->{'xmlns:nc0'} = "urn:nordugrid-cluster";

    $c->{'nc0:name'} = [ $host_info->{hostname} ];
    $c->{'nc0:aliasname'} = [ $config->{service}{Name} ] if $config->{service}{Name};
    $c->{'nc0:comment'} = $config->{service}{OtherInfo} if $config->{service}{OtherInfo};
    $c->{'nc0:owner'} = $config->{service}{ClusterOwner} if $config->{service}{ClusterOwner};
    $c->{'nc0:acl'} = [ keys %authorizedvos ] if %authorizedvos;
    $c->{'nc0:location'} = [ $config->{location}{PostCode} ] if $config->{location}{PostCode};
    $c->{'nc0:issuerca'} = [ $host_info->{issuerca} ] if $host_info->{issuerca};
    $c->{'nc0:issuerca-hash'} = [ $host_info->{issuerca_hash} ] if $host_info->{issuerca_hash};
    $c->{'nc0:trustedca'} = $host_info->{trustedcas} if $host_info->{trustedcas};
    $c->{'nc0:contactstring'} = [ "gsiftp://$host_info->{hostname}:$config->{gm_port}$config->{gm_mount_point}" ]
        if $config->{gm_port} and $config->{gm_mount_point};
    $c->{'nc0:interactive-contactstring'} = [ $config->{service}{InteractiveContactstring} ]
        if $config->{service}{InteractiveContactstring};
    $c->{'nc0:support'} = [ $supportmails[0] ] if @supportmails;
    $c->{'nc0:lrms-type'} = [ $lrms_info->{cluster}{lrms_type} ];
    $c->{'nc0:lrms-version'} = [ $lrms_info->{cluster}{lrms_version} ] if $lrms_info->{cluster}{lrms_version};
    $c->{'nc0:lrms-config'} = [ $config->{service}{lrmsconfig} ] if $config->{service}{lrmsconfig}; # orphan
    $c->{'nc0:architecture'} = [ $config->{service}{Platform} ] if $homogeneous and $config->{service}{Platform};
    push @{$c->{'nc0:opsys'}}, $config->{service}{OSName}.'-'.$config->{service}{OSVersion}
        if $config->{service}{OSName} and $config->{service}{OSVersion};
    push @{$c->{'nc0:opsys'}}, @{$config->{service}{OpSys}} if $config->{service}{OpSys};
    $c->{'nc0:benchmark'} = [ map {join ' @ ', split /\s+/,$_,2 } @{$config->{service}{Benchmark}} ]
        if $config->{service}{Benchmark};
    $c->{'nc0:nodecpu'} = [ $config->{service}{CPUModel}." @ ".$config->{service}{CPUClockSpeed}." MHz" ]
        if $config->{service}{CPUModel} and $config->{service}{CPUClockSpeed};
    $c->{'nc0:homogeneity'} = [ $homogeneous ? 'False' : 'True' ];
    $c->{'nc0:nodeaccess'} = [ 'inbound' ] if $inbound;
    $c->{'nc0:nodeaccess'} = [ 'outbound' ] if $outbound;
    $c->{'nc0:totalcpus'} = [ $lrms_info->{cluster}{totalcpus} ];
    $c->{'nc0:usedcpus'} = [ $lrms_info->{cluster}{usedcpus} ];
    $c->{'nc0:cpudistribution'} = [ $lrms_info->{cluster}{cpudistribution} ];
    $c->{'nc0:prelrmsqueued'} = [ ($gmjobcount{accepted} + $gmjobcount{preparing} + $gmjobcount{submit}) ];
    $c->{'nc0:totaljobs'} =
        [ ($gmjobcount{totaljobs} - $gmjobcount{finishing} - $gmjobcount{finished} - $gmjobcount{deleted}
        + $lrms_info->{cluster}{queuedcpus} + $lrms_info->{cluster}{usedcpus} - $gmjobcount{inlrms}) ];
    $c->{'nc0:localse'} = $config->{service}{LocalSE} if $config->{service}{LocalSE};
    $c->{'nc0:sessiondir-free'} = [ $host_info->{session_free} ];
    $c->{'nc0:sessiondir-total'} = [ $host_info->{session_total} ];
    if ($config->{defaultttl}) {
        my ($sessionlifetime) = split ' ', $config->{defaultttl};
        $c->{'nc0:sessiondir-lifetime'} = [ int $sessionlifetime/60 ] if $sessionlifetime;
    }
    $c->{'nc0:cache-free'} = [ $host_info->{cache_free} ];
    $c->{'nc0:cache-total'} = [ $host_info->{cache_total} ];
    $c->{'nc0:runtimeenvironment'} = $host_info->{runtimeenvironments};
    push @{$c->{'nc0:middleware'}}, "ARC-".$config->{arcversion};
    push @{$c->{'nc0:middleware'}}, "globus-$host_info->{globusversion}" if $host_info->{globusversion};
    push @{$c->{'nc0:middleware'}}, @{$config->{service}{Middleware}} if $config->{service}{Middleware};
    $c->{'M0:validfrom'} = [ $valid_from ] if $valid_from;
    $c->{'M0:validto'} = [ $valid_to ] if $valid_to;

    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

    $c->{'nq0:name'} = [];

    for my $share ( keys %{$config->{shares}} ) {

        my $q = {};
        push @{$c->{'nq0:name'}}, $q;

        my $qinfo = $lrms_info->{queues}{$share};

        # merge cluster wide and queue-specific options
        my $sconfig = { %{$config->{service}}, %{$config->{shares}{$share}} };

        $sconfig->{ExecEnvName} ||= [];
        my @nxenvs = @{$sconfig->{ExecEnvName}};

        if (@nxenvs) {
            my $xeconfig = $config->{xenvs}{$nxenvs[0]};
            $log->info("The Nordugrid InfoSchema is not compatible with multiple ExecutionEnvironments per share") if @nxenvs > 1;

            $sconfig = { %$sconfig, %$xeconfig };
        }

        $q->{'xmlns:nq0'} = "urn:nordugrid-queue";
        $q->{'name'} = $share;
        $q->{'nq0:name'} = [ $share ];
        
        if ( defined($config->{allownew}) and $config->{allownew} eq "no" ) {
            $q->{'nq0:status'} = [ 'inactive, grid-manager does not accept new jobs' ];
        } elsif (not $host_info->{processes}{'grid-manager'}) {
            $q->{'nq0:status'} = [ 'inactive, grid-manager is down' ];   
        } elsif ( $qinfo->{status} < 0 ) {
            $q->{'nq0:status'} = [ 'inactive, LRMS interface returns negative status' ];
        } else {
            $q->{'nq0:status'} = [ 'active' ];
        }
        $q->{'nq0:comment'} = [ $sconfig->{Description} ] if $sconfig->{Description};
        $q->{'nq0:comment'} = $sconfig->{OtherInfo} if $sconfig->{OtherInfo};
        $q->{'nq0:schedulingpolicy'} = [ $sconfig->{SchedulingPolicy} ] if $sconfig->{SchedulingPolicy};
        $q->{'nq0:homogeneity'} = [ @nxenvs > 1 ? 'False' : 'True' ];
        $q->{'nq0:nodecpu'} = [ $sconfig->{CPUModel}." @ ".$sconfig->{CPUClockSpeed}." MHz" ]
            if $sconfig->{CPUModel} and $sconfig->{CPUClockSpeed};
        $q->{'nq0:nodememory'} = [ $sconfig->{MaxVirtualMemory} ] if $sconfig->{MaxVirtualMemory};
        $q->{'nq0:architecture'} = [ $sconfig->{Platform} ]
            if $sconfig->{Platform};
        push @{$q->{'nq0:opsys'}}, $sconfig->{OSName}.'-'.$sconfig->{OSVersion}
            if $sconfig->{OSName} and $sconfig->{OSVersion};
        push @{$q->{'nq0:opsys'}}, @{$sconfig->{OpSys}} if $sconfig->{OpSys};
        $q->{'nq0:benchmark'} = [ map {join ' @ ', split /\s+/,$_,2 } @{$sconfig->{Benchmark}} ]
            if $sconfig->{Benchmark};
        $q->{'nq0:maxrunning'} = [ $qinfo->{maxrunning} ] if defined $qinfo->{maxrunning};
        $q->{'nq0:maxqueuable'}= [ $qinfo->{maxqueuable}] if defined $qinfo->{maxqueuable};
        $q->{'nq0:maxuserrun'} = [ $qinfo->{maxuserrun} ] if defined $qinfo->{maxuserrun};
        $q->{'nq0:maxcputime'} = [ int $qinfo->{maxcputime}/60 ] if defined $qinfo->{maxcputime};
        $q->{'nq0:mincputime'} = [ int $qinfo->{mincputime}/60 ] if defined $qinfo->{mincputime};
        $q->{'nq0:defaultcputime'} = [ int $qinfo->{defaultcput}/60 ] if defined $qinfo->{defaultcput};
        $q->{'nq0:maxwalltime'} =  [ int $qinfo->{maxwalltime}/60 ] if defined $qinfo->{maxwalltime};
        $q->{'nq0:minwalltime'} =  [ int $qinfo->{minwalltime}/60 ] if defined $qinfo->{minwalltime};
        $q->{'nq0:defaultwalltime'} = [ int $qinfo->{defaultwallt}/60 ] if defined $qinfo->{defaultwallt};
        $q->{'nq0:running'} = [ $qinfo->{running} ] if defined $qinfo->{running};
        $q->{'nq0:gridrunning'} = [ $gridrunning{$share} || 0 ];   
        $q->{'nq0:gridqueued'} = [ $gridqueued{$share} || 0 ];    
        $q->{'nq0:localqueued'} = [ ($qinfo->{queued} - ( $gridqueued{$share} || 0 )) ];   
        $q->{'nq0:prelrmsqueued'} = [ $prelrmsqueued{$share} || 0 ];
        if ( $sconfig->{totalcpus} ) {
            $q->{'nq0:totalcpus'} = [ $sconfig->{totalcpus} ]; # orphan
        } elsif ( $qinfo->{totalcpus} ) {
            $q->{'nq0:totalcpus'} = [ $qinfo->{totalcpus} ];      
        }	
        $q->{'M0:validfrom'} = [ $valid_from ] if $valid_from;
        $q->{'M0:validto'} = [ $valid_to ] if $valid_to;

        # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

        my $jg = {};
        $q->{'nig0:jobs'} = $jg;

        $jg->{'name'} = "jobs";
        $jg->{'xmlns:nig0'} = "urn:nordugrid-info-group";
        $jg->{'nig0:name'} = [ 'jobs' ];
        $jg->{'M0:validfrom'} = [ $valid_from ] if $valid_from;
        $jg->{'M0:validto'} = [ $valid_to ] if $valid_to;

        # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

        $jg->{'nuj0:name'} = [];

        for my $jobid ( keys %$gmjobs_info ) {

            my $gmjob = $gmjobs_info->{$jobid};

            # only handle jobs in this queue
            next unless $gmjob->{share} eq $share;

            my $j = {};
            push @{$jg->{'nuj0:name'}}, $j;

            $j->{'name'} = $jobid;
            $j->{'xmlns:nuj0'} = "urn:nordugrid-job";
            $j->{'nuj0:name'} = [ $jobid ];
            $j->{'nuj0:globalid'} = [ $c->{'nc0:contactstring'}[0]."/$jobid" ];
            $j->{'nuj0:globalowner'} = [ $gmjob->{subject} ] if $gmjob->{subject};
            $j->{'nuj0:jobname'} = [ $gmjob->{jobname} ] if $gmjob->{jobname};
            $j->{'nuj0:submissiontime'} = [ $gmjob->{starttime} ] if $gmjob->{starttime};
            $j->{'nuj0:execcluster'} = [ $host_info->{hostname} ] if $host_info->{hostname};
            $j->{'nuj0:execqueue'} = [ $share ] if $share;
            $j->{'nuj0:cpucount'} = [ $gmjob->{count} || 1 ];
            $j->{'nuj0:sessiondirerasetime'} = [ $gmjob->{cleanuptime} ] if $gmjob->{cleanuptime};
            $j->{'nuj0:stdin'}  = [ $gmjob->{stdin} ]  if $gmjob->{stdin};
            $j->{'nuj0:stdout'} = [ $gmjob->{stdout} ] if $gmjob->{stdout};
            $j->{'nuj0:stderr'} = [ $gmjob->{stderr} ] if $gmjob->{stderr};
            $j->{'nuj0:gmlog'}  = [ $gmjob->{gmlog} ]  if $gmjob->{gmlog};
            $j->{'nuj0:runtimeenvironment'} = $gmjob->{runtimeenvironments} if $gmjob->{runtimeenvironments};
            $j->{'nuj0:submissionui'} = [ $gmjob->{clientname} ] if $gmjob->{clientname};
            $j->{'nuj0:clientsoftware'} = [ $gmjob->{clientsoftware} ] if $gmjob->{clientsoftware};
            $j->{'nuj0:proxyexpirationtime'} = [ $gmjob->{delegexpiretime} ] if $gmjob->{delegexpiretime};
            $j->{'nuj0:rerunable'} = [ $gmjob->{failedstate} ? $gmjob->{failedstate} : 'none' ]
                if $gmjob->{"status"} eq "FAILED";
            $j->{'nuj0:comment'} = [ $gmjob->{comment} ] if $gmjob->{comment};

            $j->{'nuj0:reqcputime'} = [ int $gmjob->{reqcputime}/60 ] if $gmjob->{reqcputime};
            $j->{'nuj0:reqwalltime'} = [ int $gmjob->{reqwalltime}/60 ] if $gmjob->{reqwalltime};

            if ($gmjob->{status} eq "INLRMS") {

                my $localid = $gmjob->{localid}
                    or $log->warning("No local id for job $jobid") and next;
                my $lrmsjob = $lrms_info->{jobs}{$localid}
                    or $log->warning("No local job for $jobid") and next;

                $j->{'nuj0:usedmem'}     = [ $lrmsjob->{mem} ]        if defined $lrmsjob->{mem};
                $j->{'nuj0:usedwalltime'}= [ int $lrmsjob->{walltime}/60 ]   if defined $lrmsjob->{walltime};
                $j->{'nuj0:usedcputime'} = [ int $lrmsjob->{cputime}/60 ]    if defined $lrmsjob->{cputime};
                $j->{'nuj0:reqwalltime'} = [ int $lrmsjob->{reqwalltime}/60] if defined $lrmsjob->{reqwalltime};
                $j->{'nuj0:reqcputime'}  = [ int $lrmsjob->{reqcputime}/60 ] if defined $lrmsjob->{reqcputime};
                $j->{'nuj0:executionnodes'} = $lrmsjob->{nodes} if $lrmsjob->{nodes};

                # LRMS-dependent attributes taken from LRMS when the job
                # is in state 'INLRMS'

                #nuj0:status
                # take care of the GM latency, check if the job is in LRMS
                # according to both GM and LRMS, GM might think the job 
                # is still in LRMS while the job have already left LRMS              

                if ($lrmsjob->{status} and $lrmsjob->{status} ne 'EXECUTED') {
                    $j->{'nuj0:status'} = [ "INLRMS:$lrmsjob->{status}" ];
                } else {
                    $j->{'nuj0:status'} = [ 'EXECUTED' ];
                }
                push  @{$j->{'nuj0:comment'}}, @{$lrmsjob->{comment}} if $lrmsjob->{comment};
                $j->{'nuj0:queuerank'} = [ $lrmsjob->{rank} ] if $lrmsjob->{rank};

            } else {

                # LRMS-dependent attributes taken from GM when
                # the job has passed the 'INLRMS' state

                $j->{'nuj0:status'} = [ $gmjob->{status} ];
                $j->{'nuj0:usedwalltime'} = [ int $gmjob->{WallTime}/60 ] if defined $gmjob->{WallTime};
                $j->{'nuj0:usedcputime'} = [ int $gmjob->{CpuTime}/60 ] if defined $gmjob->{CpuTime};
                $j->{'nuj0:executionnodes'} = $gmjob->{nodenames} if $gmjob->{nodenames};
                $j->{'nuj0:usedmem'} = [ $gmjob->{UsedMem} ] if $gmjob->{UsedMem};
                $j->{'nuj0:completiontime'} = [ $gmjob->{completiontime} ] if $gmjob->{completiontime};
                $j->{'nuj0:errors'} = [ map { substr($_,0,87) } @{$gmjob->{errors}} ] if $gmjob->{errors};
                $j->{'nuj0:exitcode'} = [ $gmjob->{exitcode} ] if defined $gmjob->{exitcode};
            }

            $j->{'M0:validfrom'} = [ $valid_from ] if $valid_from;
            $j->{'M0:validto'} = [ $valid_to ] if $valid_to;
        }

        # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

        my $ug = {};
        $q->{'nig0:users'} = $ug;

        $ug->{'name'} = "users";
        $ug->{'xmlns:nig0'} = "urn:nordugrid-info-group";
        $ug->{'nig0:name'} = [ 'users' ];
        $ug->{'M0:validfrom'} = [ $valid_from ] if $valid_from;
        $ug->{'M0:validto'} = [ $valid_to ] if $valid_to;

        # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

        $ug->{'na0:name'} = [];

        my $usernumber = 0;

        for my $sn ( keys %$usermap ) {

            my $localid = $usermap->{$sn};
            my $lrms_user = $qinfo->{users}{$localid};

            # skip users that are not authorized in this queue
            next if exists $qinfo->{acl_users}
                 and not grep { $_ eq $localid } @{$qinfo->{acl_users}};

            my $u = {};
            push @{$ug->{'na0:name'}}, $u;

            ++$usernumber;
            my $space = $host_info->{localusers}{$localid};

            #na0:name= CN from the SN  + unique number 
            my $cn = ($sn =~ m#/CN=([^/]+)(/Email)?#) ? $1 : $sn;

            $u->{'name'} = "${cn}...$usernumber";
            $u->{'xmlns:na0'} = "urn:nordugrid-authuser";
            $u->{'na0:name'} = [ "${cn}...$usernumber" ];
            $u->{'na0:sn'} = [ $sn ];
            $u->{'na0:diskspace'} = [ $space->{diskfree} ] if defined $space->{diskfree};

            my @freecpus;
            # sort by decreasing number of cpus
            for my $ncpu ( sort { $b <=> $a } keys %{$lrms_user->{freecpus}} ) {
                my $minutes = $lrms_user->{freecpus}{$ncpu};
                push @freecpus, $minutes ? "$ncpu:$minutes" : $ncpu;
            }
            $u->{'na0:freecpus'} = [ join(" ", @freecpus) || 0 ];

            $u->{'na0:queuelength'} = [ $gm_queued{$sn} + $lrms_user->{queuelength} ];
            $u->{'M0:validfrom'} = [ $valid_from ] if $valid_from;
            $u->{'M0:validto'} = [ $valid_to ] if $valid_to;
        }

    }

    return $c;
}


1;

