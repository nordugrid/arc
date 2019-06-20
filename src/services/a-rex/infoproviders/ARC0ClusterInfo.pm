package ARC0ClusterInfo;

# This information collector combines the output of the other information collectors
# and prepares info modelled on the classic Nordugrid information schema (arc0).

use POSIX;
use Storable;

use strict;

use LogUtils;

our $log = LogUtils->getLogger(__PACKAGE__);

sub mds_date {
    my $seconds = shift;
    return strftime("%Y%m%d%H%M%SZ", gmtime($seconds));
}

# sub to pick a value in order: first value preferred to others
# can have as many parameters as one wants.
sub prioritizedvalues {
   my @values = @_;

   while (@values) {
      my $current = shift @values;
      return $current if (((defined $current) and ($current ne '')) or ((scalar @values) == 1));
  }

   # just in case all the above fails, return empty string
   $log->debug("No suitable value found in call to prioritizedvalues. Returning empty string");
   return undef;
}

############################################################################
# Combine info from all sources to prepare the final representation
############################################################################

sub collect($) {
    my ($data) = @_;

    my $config = $data->{config};
    my $usermap = $data->{usermap};
    my $host_info = $data->{host_info};
    my $rte_info = $data->{rte_info};
    my $gmjobs_info = $data->{gmjobs_info};
    my $lrms_info = $data->{lrms_info};
    my $nojobs = $data->{nojobs};

    my @allxenvs = keys %{$config->{xenvs}};
    my @allshares = keys %{$config->{shares}};

    # homogeneity of the cluster
    my $homogeneous; 
    if (defined $config->{service}{Homogeneous}) {
        $homogeneous = $config->{service}{Homogeneous};
    } else {
        # not homogeneous if there are multiple ExecEnvs
        $homogeneous = @allxenvs > 1 ? 0 : 1;
        # not homogeneous if one ExecEnv is not homogeneous
        for my $xeconfig (values %{$config->{xenvs}}) {
            $homogeneous = 0 if defined $xeconfig->{Homogeneous}
                                and not $xeconfig->{Homogeneous};
        }
    }

    # config overrides
    my $hostname = $config->{hostname} || $host_info->{hostname};

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
                next;
            }
            if ((defined $lrmsjob) and $lrmsjob->{status} ne 'EXECUTED') {
                if ($lrmsjob->{status} eq 'R' or $lrmsjob->{status} eq 'S') {
                    $gridrunning{$share} += $lrmsjob->{cpus};
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

    my @advertisedvos = (); 
    if ($config->{service}{AdvertisedVO}) {
        @advertisedvos = @{$config->{service}{AdvertisedVO}};
        # add VO: suffix to each advertised VO
        @advertisedvos = map { "VO:".$_ } @advertisedvos;
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

    # the earliest of hostcert and cacert enddates.
    my $credenddate;
    if ($host_info->{issuerca_enddate} and $host_info->{hostcert_enddate}) {
        $credenddate = ( $host_info->{hostcert_enddate} lt $host_info->{issuerca_enddate} )
                       ? $host_info->{hostcert_enddate}  : $host_info->{issuerca_enddate};
    }

    my $callcount = 0;

    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
    # # # # # # # # # # build information tree  # # # # # # # # # #
    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

    my $getCluster = sub {

        $callcount++;

        my $c = {};

        $c->{name} = $hostname;
        $c->{aliasname} = $config->{service}{ClusterAlias} if $config->{service}{ClusterAlias};
        $c->{comment} = $config->{service}{ClusterComment} if $config->{service}{ClusterComment};
        # added to help client to match GLUE2 services on the same machine
        $c->{comment} = $c->{comment} ? $c->{comment}."; GLUE2ServiceID=urn:ogf:ComputingService:$hostname:arex" : "GLUE2ServiceID=urn:ogf:ComputingService:$hostname:arex"; # GLUE2ComputingService ID
        $c->{owner} = $config->{service}{ClusterOwner} if $config->{service}{ClusterOwner};
        $c->{acl} = [ @advertisedvos ] if @advertisedvos;
        $c->{location} = $config->{location}{PostCode} if $config->{location}{PostCode};
        $c->{issuerca} = $host_info->{issuerca} if $host_info->{issuerca};
        $c->{'issuerca-hash'} = $host_info->{issuerca_hash} if $host_info->{issuerca_hash};
        $c->{credentialexpirationtime} = mds_date($credenddate) if $credenddate;
        $c->{trustedca} = $host_info->{trustedcas} if $host_info->{trustedcas};
        $c->{contactstring} = "gsiftp://$hostname:".$config->{gridftpd}{port}.$config->{gridftpd}{mountpoint} if ($config->{gridftpd}{enabled});
        $c->{'interactive-contactstring'} = $config->{service}{InteractiveContactstring}
            if $config->{service}{InteractiveContactstring};
        $c->{support} = [ @supportmails ] if @supportmails;
        $c->{'lrms-type'} = $lrms_info->{cluster}{lrms_type};
        $c->{'lrms-version'} = $lrms_info->{cluster}{lrms_version} if $lrms_info->{cluster}{lrms_version};
        $c->{'lrms-config'} = $config->{service}{lrmsconfig} if $config->{service}{lrmsconfig}; # orphan
        $c->{architecture} = $config->{service}{Platform} if $config->{service}{Platform};
        push @{$c->{opsys}}, @{$config->{service}{OpSys}} if $config->{service}{OpSys};
        push @{$c->{opsys}}, $config->{service}{OSName}.'-'.$config->{service}{OSVersion}
            if $config->{service}{OSName} and $config->{service}{OSVersion};
        $c->{benchmark} = [ map {join ' @ ', split /\s+/,$_,2 } @{$config->{service}{Benchmark}} ]
            if $config->{service}{Benchmark};
        $c->{nodecpu} = $config->{service}{CPUModel}." @ ".$config->{service}{CPUClockSpeed}." MHz"
            if $config->{service}{CPUModel} and $config->{service}{CPUClockSpeed};
        $c->{homogeneity} = $homogeneous ? 'TRUE' : 'FALSE';
        $c->{nodememory} = $config->{service}{MaxVirtualMemory} if ( $homogeneous && $config->{service}{MaxVirtualMemory} );
        $c->{nodeaccess} = 'inbound' if $inbound;
        $c->{nodeaccess} = 'outbound' if $outbound;
       if ($config->{service}{totalcpus}) {
           $c->{totalcpus} = $config->{service}{totalcpus};
       }
       else {
           $c->{totalcpus} = $lrms_info->{cluster}{totalcpus};
       }
        $c->{usedcpus} = $lrms_info->{cluster}{usedcpus};
        $c->{cpudistribution} = $lrms_info->{cluster}{cpudistribution};
        $c->{prelrmsqueued} = ($gmjobcount{accepted} + $gmjobcount{preparing} + $gmjobcount{submit});
        $c->{totaljobs} =
            ($gmjobcount{totaljobs} - $gmjobcount{finishing} - $gmjobcount{finished} - $gmjobcount{deleted}
            + $lrms_info->{cluster}{queuedcpus} + $lrms_info->{cluster}{usedcpus} - $gmjobcount{inlrms});
        $c->{localse} = $config->{service}{LocalSE} if $config->{service}{LocalSE};
        $c->{'sessiondir-free'} = $host_info->{session_free};
        $c->{'sessiondir-total'} = $host_info->{session_total};
        if ($config->{control}{'.'}{defaultttl}) {
            my ($sessionlifetime) = split ' ', $config->{control}{'.'}{defaultttl};
            $c->{'sessiondir-lifetime'} = int $sessionlifetime/60 if $sessionlifetime;
        }
        $c->{'cache-free'} = $host_info->{cache_free};
        $c->{'cache-total'} = $host_info->{cache_total};
        # get rid of case duplicates in rtes.
        my @rte_tmp = sort { "\L$a" cmp "\L$b"} (sort keys %$rte_info);
        my @rtes;
        foreach my $i (0 .. $#rte_tmp) {
			if ($i == 0) {
				push @rtes,$rte_tmp[$i];
			} elsif ( $i > 0 and $i < $#rte_tmp ) {
				if ( $rte_tmp[$i] =~ /^\Q$rtes[$#rtes]\E\z/i ) {
				next;
				} else {
					push @rtes,$rte_tmp[$i];
				}
			}
		}
        $c->{runtimeenvironment} = \@rtes;
        push @{$c->{middleware}}, "nordugrid-arc-".$config->{arcversion};
        push @{$c->{middleware}}, "globus-$host_info->{globusversion}" if $host_info->{globusversion};
        push @{$c->{middleware}}, @{$config->{service}{Middleware}} if $config->{service}{Middleware};

        # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

        my $getQueues = sub {

            return undef unless my ($share, $dummy) = each %{$config->{shares}};

            my $q = {};

            my $qinfo = $lrms_info->{queues}{$share};

            # merge cluster wide and queue-specific options
            my $sconfig = { %{$config->{service}}, %{$config->{shares}{$share}} };

            $sconfig->{ExecutionEnvironmentName} ||= [];
            my @nxenvs = @{$sconfig->{ExecutionEnvironmentName}};

            if (@nxenvs) {
                my $xeconfig = $config->{xenvs}{$nxenvs[0]};
                $log->info("The Nordugrid InfoSchema is not compatible with multiple ExecutionEnvironments per share") if @nxenvs > 1;

                $sconfig = { %$sconfig, %$xeconfig };
            }

            $q->{'name'} = $share;
            
            if ( defined $config->{GridftpdAllowNew} and $config->{GridftpdAllowNew} == 0 ) {
                $q->{status} = 'inactive, grid-manager does not accept new jobs';
            } elsif ( $host_info->{gm_alive} ne 'all' ) {
                if ($host_info->{gm_alive} eq 'some') {
                    $q->{status} = 'degraded, one or more grid-managers are down';
                } else {
                    $q->{status} = $config->{remotegmdirs} ? 'inactive, all grid managers are down'
                                                           : 'inactive, grid-manager is down';
                }
            } elsif (not $host_info->{processes}{'gridftpd'}) {
                $q->{status} = 'inactive, gridftpd is down';   
            } elsif (not $host_info->{hostcert_enddate} or not $host_info->{issuerca_enddate}) {
                $q->{status} = 'inactive, host credentials missing';
            } elsif ($host_info->{hostcert_expired} or $host_info->{issuerca_expired}) {
                $q->{status} = 'inactive, host credentials expired';
            } elsif ( $qinfo->{status} < 0 ) {
                $q->{status} = 'inactive, LRMS interface returns negative status';
            } else {
                $q->{status} = 'active';
            }
            $q->{comment}=$sconfig->{Description} if $sconfig->{Description};
            if ( defined $sconfig->{OtherInfo}) {
              my @sotherinfo = @{ $sconfig->{OtherInfo} };
              $q->{comment} = "$q->{comment}, OtherInfo: @sotherinfo";
            }
            $q->{schedulingpolicy} = $sconfig->{SchedulingPolicy} if $sconfig->{SchedulingPolicy};
            if (defined $sconfig->{Homogeneous}) {
                $q->{homogeneity} = $sconfig->{Homogeneous} ? 'TRUE' : 'FALSE';
            } else {
                $q->{homogeneity} = @nxenvs > 1 ? 'FALSE' : 'TRUE';
            }
            $q->{nodecpu} = $sconfig->{CPUModel}." @ ".$sconfig->{CPUClockSpeed}." MHz"
                if $sconfig->{CPUModel} and $sconfig->{CPUClockSpeed};
            $q->{nodememory} = $sconfig->{MaxVirtualMemory} if $sconfig->{MaxVirtualMemory};
            $q->{architecture} = $sconfig->{Platform}
                if $sconfig->{Platform};
            # override instead of merging
            if ($sconfig->{OSName} and $sconfig->{OSVersion}) {
                push @{$q->{opsys}}, $sconfig->{OSName}.'-'.$sconfig->{OSVersion}
            } else {
                push @{$q->{opsys}}, @{$sconfig->{OpSys}} if $sconfig->{OpSys};
            }
            $q->{benchmark} = [ map {join ' @ ', split /\s+/,$_,2 } @{$sconfig->{Benchmark}} ]
                if $sconfig->{Benchmark};
            $q->{maxrunning} = $qinfo->{maxrunning} if defined $qinfo->{maxrunning};
            $q->{maxqueuable}= $qinfo->{maxqueuable}if defined $qinfo->{maxqueuable};
            $q->{maxuserrun} = $qinfo->{maxuserrun} if defined $qinfo->{maxuserrun};
            $q->{maxcputime} = prioritizedvalues($sconfig->{maxcputime},$qinfo->{maxcputime});
            $q->{maxcputime} = defined $q->{maxcputime} ? int $q->{maxcputime}/60 : undef;
            $q->{mincputime} = int $qinfo->{mincputime}/60 if defined $qinfo->{mincputime};
            $q->{defaultcputime} = int $qinfo->{defaultcput}/60 if defined $qinfo->{defaultcput};
            $q->{maxwalltime} =  prioritizedvalues($sconfig->{maxwalltime},$qinfo->{maxwalltime});
            $q->{maxwalltime} = defined $q->{maxwalltime} ? int $q->{maxwalltime}/60 : undef;
            $q->{minwalltime} =  int $qinfo->{minwalltime}/60 if defined $qinfo->{minwalltime};
            $q->{defaultwalltime} = int $qinfo->{defaultwallt}/60 if defined $qinfo->{defaultwallt};
            $q->{running} = $qinfo->{running} if defined $qinfo->{running};
            $q->{gridrunning} = $gridrunning{$share} || 0;   
            $q->{gridqueued} = $gridqueued{$share} || 0;
            $q->{localqueued} = ($qinfo->{queued} - ( $gridqueued{$share} || 0 ));
            if ( $q->{localqueued} < 0 ) {
                $q->{localqueued} = 0;
            }  
            $q->{prelrmsqueued} = $prelrmsqueued{$share} || 0;
            if ( $sconfig->{totalcpus} ) {
                $q->{totalcpus} = $sconfig->{totalcpus}; # orphan
            } elsif ( $qinfo->{totalcpus} ) {
                $q->{totalcpus} = $qinfo->{totalcpus};
            }	

            keys %$gmjobs_info; # reset iterator of each()

            # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

            my $getJobs = sub {

                # find the next job that belongs to the current share
                my ($jobid, $gmjob);
                while (1) {
                    return undef unless ($jobid, $gmjob) = each %$gmjobs_info;
                    last if $gmjob->{share} eq $share;
                }

                my $j = {};

                $j->{name} = $jobid;
                $j->{globalid} = $c->{contactstring}."/$jobid";
                $j->{globalowner} = $gmjob->{subject} if $gmjob->{subject};
                $j->{jobname} = $gmjob->{jobname} if $gmjob->{jobname};
                $j->{submissiontime} = $gmjob->{starttime} if $gmjob->{starttime};
                $j->{execcluster} = $hostname if $hostname;
                $j->{execqueue} = [ $share ] if $share;
                $j->{cpucount} = [ $gmjob->{count} || 1 ];
                $j->{sessiondirerasetime} = [ $gmjob->{cleanuptime} ] if $gmjob->{cleanuptime};
                $j->{stdin}  = [ $gmjob->{stdin} ]  if $gmjob->{stdin};
                $j->{stdout} = [ $gmjob->{stdout} ] if $gmjob->{stdout};
                $j->{stderr} = [ $gmjob->{stderr} ] if $gmjob->{stderr};
                $j->{gmlog}  = [ $gmjob->{gmlog} ]  if $gmjob->{gmlog};
                $j->{runtimeenvironment} = $gmjob->{runtimeenvironments} if $gmjob->{runtimeenvironments};
                $j->{submissionui} = $gmjob->{clientname} if $gmjob->{clientname};
                $j->{clientsoftware} = $gmjob->{clientsoftware} if $gmjob->{clientsoftware};
                $j->{proxyexpirationtime} = $gmjob->{delegexpiretime} if $gmjob->{delegexpiretime};
                $j->{rerunable} = $gmjob->{failedstate} ? $gmjob->{failedstate} : 'none'
                    if $gmjob->{status} eq "FAILED";
                $j->{comment} = [ $gmjob->{comment} ] if $gmjob->{comment};
                # added to record which was the submission interface
                if ( $gmjob->{interface} ) {
                        my $submittedstring = 'SubmittedVia='.$gmjob->{interface};
                        push(@{$j->{comment}}, $submittedstring);
                };
                $j->{reqcputime} = int $gmjob->{reqcputime}/60 if $gmjob->{reqcputime};
                $j->{reqwalltime} = int $gmjob->{reqwalltime}/60 if $gmjob->{reqwalltime};

                if ($gmjob->{status} eq "INLRMS") {

                    my $localid = $gmjob->{localid}
                        or $log->warning("No local id for job $jobid") and next;
                    my $lrmsjob = $lrms_info->{jobs}{$localid}
                        or $log->warning("No local job for $jobid") and next;

                    $j->{usedmem}     =     $lrmsjob->{mem}            if defined $lrmsjob->{mem};
                    $j->{usedwalltime}= int $lrmsjob->{walltime}/60    if defined $lrmsjob->{walltime};
                    $j->{usedcputime} = int $lrmsjob->{cputime}/60     if defined $lrmsjob->{cputime};
                    $j->{reqwalltime} = int $lrmsjob->{reqwalltime}/60 if defined $lrmsjob->{reqwalltime};
                    $j->{reqcputime}  = int $lrmsjob->{reqcputime}/60  if defined $lrmsjob->{reqcputime};
                    $j->{executionnodes}  = $lrmsjob->{nodes} if $lrmsjob->{nodes};
                    if ($lrms_info->{cluster}{lrms_type} eq "boinc") {
                        # BOINC allocates a dynamic number of cores to jobs so set here what is actually used
                        # This is abusing the schema a bit since cpucount is really requested slots
                        $j->{cpucount}    = int $lrmsjob->{cpus}           if defined $lrmsjob->{cpus};
                    }

                    # LRMS-dependent attributes taken from LRMS when the job
                    # is in state 'INLRMS'

                    #nuj0:status
                    # take care of the GM latency, check if the job is in LRMS
                    # according to both GM and LRMS, GM might think the job 
                    # is still in LRMS while the job have already left LRMS              

                    if ($lrmsjob->{status} and $lrmsjob->{status} ne 'EXECUTED') {
                        $j->{status} = "INLRMS:$lrmsjob->{status}";
                    } else {
                        $j->{status} = 'EXECUTED';
                    }
                    push  @{$j->{comment}}, @{$lrmsjob->{comment}} if $lrmsjob->{comment};
                    $j->{queuerank} = $lrmsjob->{rank} if $lrmsjob->{rank};

                } else {

                    # LRMS-dependent attributes taken from GM when
                    # the job has passed the 'INLRMS' state

                    $j->{status} = $gmjob->{status};
                    $j->{usedwalltime} = int $gmjob->{WallTime}/60 if defined $gmjob->{WallTime};
                    $j->{usedcputime} = int $gmjob->{CpuTime}/60 if defined $gmjob->{CpuTime};
                    $j->{executionnodes} = $gmjob->{nodenames} if $gmjob->{nodenames};
                    $j->{usedmem} = $gmjob->{UsedMem} if $gmjob->{UsedMem};
                    $j->{completiontime} = $gmjob->{completiontime} if $gmjob->{completiontime};
                    $j->{errors} = join "; ", @{$gmjob->{errors}} if $gmjob->{errors};
                    $j->{exitcode} = $gmjob->{exitcode} if defined $gmjob->{exitcode};
                }

                return $j;
            };

            $q->{jobs} = $getJobs;

            # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

            my $usernumber = 0;

            keys %$usermap; # reset iterator of each()

            my $getUsers = sub {

                # find the next user that is authorized in this queue
                my ($sn, $localid, $lrms_user);
                while (1) {
                    return undef unless ($sn, $localid) = each %$usermap;
                    # skip users whose SNs need to be base64 encoded
                    if ($sn =~ /^[\s,:<]/ or $sn =~ /[\x0D\x0A\x00]/ or $sn =~ /[^\x00-\x7F]/) {
						$log->warning("While collecting info for queue $q->{'name'}: user with sn $sn will not be published due to characters that require base64 encoding. Skipping");
						next;
					}
                    $lrms_user = $qinfo->{users}{$localid};
                    last if not exists $qinfo->{acl_users};
                    last if grep { $_ eq $localid } @{$qinfo->{acl_users}};
                }

                my $u = {};

                ++$usernumber;
                my $space = $host_info->{localusers}{$localid};

                #name= CN from the SN  + unique number 
                my $cn = ($sn =~ m#/CN=([^/]+)(/Email)?#) ? $1 : $sn;

                $u->{name} = "${cn}...$usernumber";
                $u->{sn} = $sn;
                $u->{diskspace} = $space->{diskfree} if defined $space->{diskfree};

                my @freecpus;
                # sort by decreasing number of cpus
                for my $ncpu ( sort { $b <=> $a } keys %{$lrms_user->{freecpus}} ) {
                    my $minutes = $lrms_user->{freecpus}{$ncpu};
                    push @freecpus, $minutes ? "$ncpu:$minutes" : $ncpu;
                }
                $u->{freecpus} = join(" ", @freecpus) || 0;
                $u->{queuelength} = $gm_queued{$sn} + $lrms_user->{queuelength};

                return $u;
            };

            $q->{users} = $getUsers;

            return $q;
        };

        $c->{queues} = $getQueues;

        return $c;
    };

    return $getCluster;

}


1;

