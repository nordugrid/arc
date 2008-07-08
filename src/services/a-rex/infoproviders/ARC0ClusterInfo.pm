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

# override InfoCollector base class methods

sub _get_options_schema {
    return {}; # too many inputs to name all
}
sub _get_results_schema {
    return $arc0_info_schema;
}

sub _collect($$) {
    my ($self,$options) = @_; 
    my $config = $options->{config};
    my $usermap = $options->{usermap};
    my $host_info = $options->{host_info};
    my $gmjobs_info = $options->{gmjobs_info};
    my $lrms_info = $options->{lrms_info};

    my ($valid_from, $valid_to) = $config->{ttl} ? mds_valid($config->{ttl}) : ();

    # adotf means autodetect on the frontend
    $config->{architecture} = $host_info->{architecture} if $config->{architecture} eq 'adotf';
    $config->{nodecpu} = $host_info->{nodecpu} if $config->{nodecpu} eq 'adotf';

    # config overrides
    $host_info->{hostname} = $config->{hostname} if $config->{hostname};

    # count grid-manager jobs

    my %gmjobcount;
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

    my $c = {};

    $c->{'xmlns:n'} = "urn:nordugrid";
    $c->{'xmlns:M0'} = "urn:Mds";
    $c->{'xmlns:nc0'} = "urn:nordugrid-cluster";

    $c->{'nc0:name'} = [ $host_info->{hostname} ];
    $c->{'nc0:aliasname'} = [ $config->{cluster_alias} ] if $config->{cluster_alias};
    $c->{'nc0:comment'} = [ $config->{comment} ] if $config->{comment};
    $c->{'nc0:owner'} = [ split /\[separator\]/, $config->{cluster_owner} ] if $config->{cluster_owner};
    $c->{'nc0:acl'} = [ map "VO:$_", split /\[separator\]/, $config->{authorizedvo} ] if $config->{authorizedvo};
    $c->{'nc0:location'} = [ $config->{cluster_location} ] if $config->{cluster_location};
    $c->{'nc0:issuerca'} = [ $host_info->{issuerca} ];
    $c->{'nc0:issuerca-hash'} = [ $host_info->{issuerca_hash} ];
    $c->{'nc0:trustedca'} = $host_info->{trustedcas};
    $c->{'nc0:contactstring'} = [ "gsiftp://$host_info->{hostname}:$config->{gm_port}$config->{gm_mount_point}" ];
    $c->{'nc0:interactive-contactstring'} = [ split /\[separator\]/, $config->{interactive_contactstring} ]
        if $config->{interactive_contactstring};
    $c->{'nc0:support'} = [ split /\[separator\]/, $config->{clustersupport} ] if $config->{clustersupport};
    $c->{'nc0:lrms-type'} = [ $lrms_info->{cluster}{lrms_type} ];
    $c->{'nc0:lrms-version'} = [ $lrms_info->{cluster}{lrms_version} ];
    $c->{'nc0:lrms-config'} = [ $config->{lrmsconfig} ] if $config->{lrmsconfig};
    $c->{'nc0:architecture'} = [ $config->{architecture} ] if $config->{architecture};
    $c->{'nc0:opsys'} = [ split /\[separator\]/, $config->{opsys} ] if $config->{opsys};
    $c->{'nc0:benchmark'} = [ map {join ' @ ', split /\s+/,$_,2 } split /\[separator\]/, $config->{benchmark} ]
        if $config->{benchmark};
    $c->{'nc0:homogeneity'} = [ uc $config->{homogeneity} ] if $config->{homogeneity};
    $c->{'nc0:nodecpu'} = [ $config->{nodecpu} ] if $config->{nodecpu};
    $c->{'nc0:nodememory'} = [ $config->{nodememory} ] if $config->{nodememory};
    $c->{'nc0:nodeaccess'} = [ split /\[separator\]/, $config->{nodeaccess} ] if $config->{nodeaccess};
    $c->{'nc0:totalcpus'} = [ $lrms_info->{cluster}{totalcpus} ];
    $c->{'nc0:usedcpus'} = [ $lrms_info->{cluster}{usedcpus} ];
    $c->{'nc0:cpudistribution'} = [ $lrms_info->{cluster}{cpudistribution} ];
    $c->{'nc0:prelrmsqueued'} = [ ($gmjobcount{accepted} + $gmjobcount{preparing} + $gmjobcount{submit}) ];
    $c->{'nc0:totaljobs'} =
        [ ($gmjobcount{totaljobs} - $gmjobcount{finishing} - $gmjobcount{finished} - $gmjobcount{deleted}
        + $lrms_info->{cluster}{queuedcpus} + $lrms_info->{cluster}{usedcpus} - $gmjobcount{inlrms}) ];
    $c->{'nc0:localse'} = [ split /\[separator\]/, $config->{localse} ] if $config->{localse};
    $c->{'nc0:sessiondir-free'} = [ $host_info->{session_free} ];
    $c->{'nc0:sessiondir-total'} = [ $host_info->{session_total} ];
    $c->{'nc0:sessiondir-lifetime'} = [ int $1/60 ] if $config->{defaultttl} =~ /(\d+)/;
    $c->{'nc0:cache-free'} = [ $host_info->{cache_free} ];
    $c->{'nc0:cache-total'} = [ $host_info->{cache_total} ];
    $c->{'nc0:runtimeenvironment'} = $host_info->{runtimeenvironments};
    push @{$c->{'nc0:middleware'}}, "nordugrid-arc-$host_info->{ngversion}" if $host_info->{ngversion};
    push @{$c->{'nc0:middleware'}}, "globus-$host_info->{globusversion}" if $host_info->{globusversion};
    push @{$c->{'nc0:middleware'}}, grep {! /nordugrid/i and $host_info->{ngversion} }
        split /\[separator\]/, $config->{middleware}
        if $config->{middleware};
    $c->{'M0:validfrom'} = [ $valid_from ] if $valid_from;
    $c->{'M0:validto'} = [ $valid_to ] if $valid_to;

    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

    $c->{'nq0:name'} = [];

    for my $qname ( keys %{$lrms_info->{queues}} ) {

        my $q = {};
        push @{$c->{'nq0:name'}}, $q;

        # merge cluster wide and queue-specific options
        my $qconfig = { %$config, %{$config->{queues}{$qname}} };

        my $qinfo = $lrms_info->{queues}{$qname};

        # adotf means autodetect on the frontend
        $qconfig->{architecture} = $host_info->{architecture} if $qconfig->{architecture} eq 'adotf';
        $qconfig->{nodecpu} = $host_info->{nodecpu} if $qconfig->{nodecpu} eq 'adotf';

        $q->{'xmlns:nq0'} = "urn:nordugrid-queue";
        $q->{'name'} = $qname;
        $q->{'nq0:name'} = [ $qname ];
        
        if ($qconfig->{allownew} eq "no") {
            $q->{'nq0:status'} = [ 'inactive, grid-manager does not accept new jobs' ];
        } elsif (not $host_info->{processes}{'grid-manager'}) {
            $q->{'nq0:status'} = [ 'inactive, grid-manager is down' ];   
        } elsif (not $host_info->{processes}{'gridftpd'})  {
            $q->{'nq0:status'} = [ 'inactive, gridftpd is down' ];  
        } elsif ( $qinfo->{status} < 0 ) {
            $q->{'nq0:status'} = [ 'inactive, LRMS interface returns negative status' ];
        } else {
            $q->{'nq0:status'} = [ 'active' ];
        }
        $q->{'nq0:comment'} = [ $qconfig->{comment} ] if $qconfig->{comment};
        $q->{'nq0:schedulingpolicy'} = [ $qconfig->{scheduling_policy} ] if $qconfig->{scheduling_policy};
        $q->{'nq0:homogeneity'} = [ uc $qconfig->{homogeneity} ] if $qconfig->{homogeneity};
        $q->{'nq0:nodecpu'} = [ $qconfig->{nodecpu} ] if $qconfig->{nodecpu};
        $q->{'nq0:nodememory'} = [ $qconfig->{nodememory} ] if $qconfig->{nodememory};
        $q->{'nq0:architecture'} = [ $qconfig->{architecture} ] if $qconfig->{architecture};
        $q->{'nq0:opsys'} = [ split /\[separator\]/, $qconfig->{opsys} ];
        $q->{'nq0:benchmark'} = [ map {join ' @ ', split /\s+/,$_,2 } split /\[separator\]/, $qconfig->{benchmark} ]
            if $qconfig->{benchmark};
        $q->{'nq0:maxrunning'} = [ $qinfo->{maxrunning} ] if defined $qinfo->{maxrunning};
        $q->{'nq0:maxqueuable'}= [ $qinfo->{maxqueuable}] if defined $qinfo->{maxqueuable};
        $q->{'nq0:maxuserrun'} = [ $qinfo->{maxuserrun} ] if defined $qinfo->{maxuserrun};
        $q->{'nq0:maxcputime'} = [ $qinfo->{maxcputime} ] if defined $qinfo->{maxcputime};
        $q->{'nq0:mincputime'} = [ $qinfo->{mincputime} ] if defined $qinfo->{mincputime};
        $q->{'nq0:defaultcputime'} = [ $qinfo->{defaultcput} ] if defined $qinfo->{defaultcput};
        $q->{'nq0:maxwalltime'} =  [ $qinfo->{maxwalltime} ] if defined $qinfo->{maxwalltime};
        $q->{'nq0:minwalltime'} =  [ $qinfo->{minwalltime} ] if defined $qinfo->{minwalltime};
        $q->{'nq0:defaultwalltime'} = [ $qinfo->{defaultwallt} ] if defined $qinfo->{defaultwallt};
        $q->{'nq0:running'} = [ $qinfo->{running} ] if defined $qinfo->{running};
        $q->{'nq0:gridrunning'} = [ $gridrunning{$qname} || 0 ];   
        $q->{'nq0:gridqueued'} = [ $gridqueued{$qname} || 0 ];    
        $q->{'nq0:localqueued'} = [ ($qinfo->{queued} - ( $gridqueued{$qname} || 0 )) ];   
        $q->{'nq0:prelrmsqueued'} = [ $prelrmsqueued{$qname} || 0 ];
        if ($qconfig->{queue_node_string}) {
            $q->{'nq0:totalcpus'} = [ $qinfo->{totalcpus} ];
        } elsif ( $qconfig->{totalcpus} ) {
            $q->{'nq0:totalcpus'} = [ $qconfig->{totalcpus} ];
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
            next unless $gmjob->{queue} eq $qname;

            my $j = {};
            push @{$jg->{'nuj0:name'}}, $j;

            $j->{'name'} = $jobid;
            $j->{'xmlns:nuj0'} = "urn:nordugrid-job";
            $j->{'nuj0:name'} = [ $jobid ];
            $j->{'nuj0:globalid'} = [ $c->{'nc0:contactstring'}[0]."/$jobid" ];
            $j->{'nuj0:globalowner'} = [ $gmjob->{subject} ];
            $j->{'nuj0:jobname'} = [ $gmjob->{jobname} ] if $gmjob->{jobname};
            $j->{'nuj0:submissiontime'} = [ $gmjob->{starttime} ] if $gmjob->{starttime};
            $j->{'nuj0:execcluster'} = [ $host_info->{hostname} ];
            $j->{'nuj0:execqueue'} = [ $qname ];
            $j->{'nuj0:cpucount'} = [ $gmjob->{count} || 1 ];
            $j->{'nuj0:sessiondirerasetime'} = [ $gmjob->{cleanuptime} ] if $gmjob->{cleanuptime};
            $j->{'nuj0:stdin'}  = [ $gmjob->{stdin} ]  if $gmjob->{stdin};
            $j->{'nuj0:stdout'} = [ $gmjob->{stdout} ] if $gmjob->{stdout};
            $j->{'nuj0:stderr'} = [ $gmjob->{stderr} ] if $gmjob->{stderr};
            $j->{'nuj0:gmlog'}  = [ $gmjob->{gmlog} ]  if $gmjob->{gmlog};
            $j->{'nuj0:runtimeenvironment'} = $gmjob->{runtimeenvironments} if $gmjob->{runtimeenvironments};
            $j->{'nuj0:submissionui'} = [ $gmjob->{clientname} ];
            $j->{'nuj0:clientsoftware'} = [ $gmjob->{clientsoftware} ];
            $j->{'nuj0:proxyexpirationtime'} = [ $gmjob->{delegexpiretime} ];
            $j->{'nuj0:rerunable'} = [ $gmjob->{failedstate} ? $gmjob->{failedstate} : 'none' ]
                if $gmjob->{"status"} eq "FAILED";
            $j->{'nuj0:comment'} = [ $gmjob->{comment} ] if $gmjob->{comment};

            $j->{'nuj0:reqcputime'} = [ $gmjob->{reqcputime} ] if $gmjob->{reqcputime};
            $j->{'nuj0:reqwalltime'} = [ $gmjob->{reqwalltime} ] if $gmjob->{reqwalltime};

            if ($gmjob->{status} eq "INLRMS") {

                my $localid = $gmjob->{localid}
                    or $log->error("No local id for job $jobid") and next;
                my $lrmsjob = $lrms_info->{jobs}{$localid}
                    or $log->error("No local job for $jobid") and next;

                $j->{'nuj0:usedmem'}     = [ $lrmsjob->{mem} ]        if defined $lrmsjob->{mem};
                $j->{'nuj0:usedwalltime'}= [ $lrmsjob->{walltime} ]   if defined $lrmsjob->{walltime};
                $j->{'nuj0:usedcputime'} = [ $lrmsjob->{cputime} ]    if defined $lrmsjob->{cputime};
                $j->{'nuj0:reqwalltime'} = [ $lrmsjob->{reqwalltime}] if defined $lrmsjob->{reqwalltime};
                $j->{'nuj0:reqcputime'}  = [ $lrmsjob->{reqcputime} ] if defined $lrmsjob->{reqcputime};
                $j->{'nuj0:executionnodes'} = $lrmsjob->{nodes} if $lrmsjob->{nodes};

                # LRMS-dependent attributes taken from LRMS when the job
                # is in state 'INLRMS'

                #nuj0:status
                # take care of the GM latency, check if the job is in LRMS
                # according to both GM and LRMS, GM might think the job 
                # is still in LRMS while the job have already left LRMS              

                if ($lrmsjob->{status} and $lrmsjob->{status} ne 'EXECUTED') {
                    $j->{'nuj0:status'} = [ "INLRMS: $lrmsjob->{status}" ];
                } else {
                    $j->{'nuj0:status'} = [ 'EXECUTED' ];
                }
                push  @{$j->{'nuj0:comment'}}, @{$lrmsjob->{comment}} if $lrmsjob->{comment};
                $j->{'nuj0:queuerank'} = [ $lrmsjob->{rank} ] if $lrmsjob->{rank};

            } else {

                # LRMS-dependent attributes taken from GM when
                # the job has passed the 'INLRMS' state

                $j->{'nuj0:status'} = [ $gmjob->{status} ];
                $j->{'nuj0:usedwalltime'} = [ $gmjob->{WallTime} ] if defined $gmjob->{WallTime};
                $j->{'nuj0:usedcputime'} = [ $gmjob->{CpuTime} ] if defined $gmjob->{CpuTime};
                $j->{'nuj0:executionnodes'} = $gmjob->{nodenames} if $gmjob->{nodenames};
                $j->{'nuj0:usedmem'} = [ $gmjob->{UsedMem} ] if $gmjob->{UsedMem};
                $j->{'nuj0:completiontime'} = [ $gmjob->{completiontime} ] if $gmjob->{completiontime};
                $j->{'nuj0:errors'} = [ substr($gmjob->{errors},0,87) ] if $gmjob->{errors};
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
            next if exists $lrms_user->{acl_users}
                 and not grep { $_ eq $localid } @{$lrms_user->{acl_users}};

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

            # Don't advertise free slots while busy with staging
            $u->{'na0:freecpus'} = [ 0 ] if $pendingprelrms{$qname};

            $u->{'na0:queuelength'} = [ $gm_queued{$sn} + $lrms_user->{queuelength} ];
            $u->{'M0:validfrom'} = [ $valid_from ] if $valid_from;
            $u->{'M0:validto'} = [ $valid_to ] if $valid_to;
        }

    }

    return $c;
}


1;

