package ARC1ClusterInfo;

# This information collector combines the output of the other information collectors
# and prepares the GLUE2 information model of A-REX. The returned structure is
# meant to be converted to XML with XML::Simple.

use Storable;
use POSIX qw(ceil);

use strict;

use Sysinfo qw(cpuinfo processid diskinfo diskspaces);
use LogUtils;
use InfoChecker;

use ARC1ClusterSchema;

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
sub bes_state {
    my ($gm_state,$lrms_state) = @_;
    if      ($gm_state eq "ACCEPTED") {
        return [ "Pending", "Accepted" ];
    } elsif ($gm_state eq "PREPARING") {
        return [ "Running", "Stage-in" ];
    } elsif ($gm_state eq "SUBMIT") {
        return [ "Running", "Submitting" ];
    } elsif ($gm_state eq "INLRMS") {
        if (not defined $lrms_state) {
            return [ "Running" ];
        } elsif ($lrms_state eq 'Q') {
            return [ "Running", "Queuing" ];
        } elsif ($lrms_state eq 'R') {
            return [ "Running", "Executing" ];
        } elsif ($lrms_state eq 'EXECUTED'
              or $lrms_state eq '') {
            return [ "Running", "Executed" ];
        } elsif ($lrms_state eq 'S') {
            return [ "Running", "Suspended" ];
        } else {
            return [ "Running", "LRMSOther" ];
        }
    } elsif ($gm_state eq "FINISHING") {
        return [ "Running", "Stage-out" ];
    } elsif ($gm_state eq "CANCELING") {
        return [ "Running", "Cancelling" ];
    } elsif ($gm_state eq "KILLED") {
        return [ "Cancelled" ];
    } elsif ($gm_state eq "FAILED") {
        return [ "Failed" ];
    } elsif ($gm_state eq "FINISHED") {
        return [ "Finished" ];
    } elsif ($gm_state eq "DELETED") {
        # Cannot map to BES state
        return [ ];
    } else {
        return [ ];
    }
}

sub glueState {
    my @ng_status = @_;
    return [ "UNDEFINEDVALUE" ] unless $ng_status[0];
    my $status = [ "nordugrid:".join(':',@ng_status) ];
    my $bes_state = bes_state(@ng_status);
    push @$status, "bes:".$bes_state->[0] if @$bes_state;
    return $status;
}

# Intersection of two arrays that completes in linear time.  The input arrays
# are the keys of the two hashes passed as reference.  The intersection array
# consists of the keys of the returned hash reference.
sub intersection {
    my ($a, $b) = @_;
    my (%union, %xor, %isec);
    for (keys %$a) { $union{$_} = 1; $xor{$_} = 1 if exists $b->{$_} }
    for (keys %$b) { $union{$_} = 1; $xor{$_} = 1 if exists $a->{$_} }
    for (keys %union) { $isec{$_} = 1 if exists $xor{$_} }
    return \%isec;
}


# processes NodeSelection options and returns the matching nodes.
sub selectnodes {
    my ($nodes, %nscfg) = @_;
    return undef unless %$nodes and %nscfg;

    my @allnodes = keys %$nodes;

    my %selected = ();
    if ($nscfg{Regex}) {
        for my $re (@{$nscfg{Regex}}) {
            map { $selected{$_} = 1 if /$re/ } @allnodes;
        }
    }
    if ($nscfg{Tag}) {
        for my $tag (@{$nscfg{Tag}}) {
            for my $node (@allnodes) {
                my $tags = $nodes->{$node}{tags};
                next unless $tags;
                map { $selected{$node} = 1 if $tag eq $_ } @$tags; 
            }
        }
    }
    if ($nscfg{Command}) {
        $log->warning("Not implemented: NodeSelection: Command");
    }

    delete $nscfg{Regex};
    delete $nscfg{Tag};
    delete $nscfg{Command};
    $log->warning("Unknown NodeSelection option: @{[keys %nscfg]}") if %nscfg;

    $selected{$_} = $nodes->{$_} for keys %selected;

    return \%selected;
}

# Sums up ExecutionEnvironments attributes from the LRMS plugin
sub xestats {
    my ($xenv, $nodes) = @_;
    return undef unless %$nodes;

    my %continuous = (vmem => 'VirtualMemorySize', pmem => 'MainMemorySize');
    my %discrete = (lcpus => 'LogicalCPUs', pcpus => 'PhysicalCPUs', sysname => 'OSFamily', machine => 'Platform');

    my (%minval, %maxval);
    my (%minnod, %maxnod);
    my %distrib;

    my %stats = (total => 0, free => 0, available => 0);

    for my $host (keys %$nodes) {
        my %node = %{$nodes->{$host}};
        $stats{total}++;
        $stats{free}++ if $node{isfree};
        $stats{available}++ if $node{isavailable};

        # Also agregate values across nodes, check consistency
        for my $prop (%discrete) {
            my $val = $node{$prop};
            next unless defined $val;
            push @{$distrib{$prop}{$val}}, $host;
        }
        for my $prop (keys %continuous) {
            my $val = $node{$prop};
            next unless defined $val;
            if (not defined $minval{$prop} or (defined $minval{$prop} and $minval{$prop} > $val)) {
                $minval{$prop} = $val;
                $minnod{$prop} = $host;
            }
            if (not defined $maxval{$prop} or (defined $maxval{$prop} and $maxval{$prop} < $val)) {
                $maxval{$prop} = $val;
                $maxnod{$prop} = $host;
            }
        }
    }

   my $homogeneous = 1;
    while (my ($prop, $opt) = each %discrete) {
        my $values = $distrib{$prop};
        next unless $values;
        if (scalar keys %$values > 1) {
            my $msg = "ExecutionEnvironment $xenv is inhomogeneous regarding $opt:";
            while (my ($val, $hosts) = each %$values) {
                my $first = pop @$hosts;
                my $remaining = @$hosts;
                $val = defined $val ? $val : 'undef';
                $msg .= " $val($first";
                $msg .= "+$remaining more" if $remaining;
                $msg .= ")";
            }
            $log->info($msg);
            $homogeneous = 0;
        } else {
            my ($val) = keys %$values;
            $stats{$prop} = $val;
        }
    }
    if ($maxval{pmem}) {
        my $rdev = 2 * ($maxval{pmem} - $minval{pmem}) / ($maxval{pmem} + $minval{pmem});
        if ($rdev > 0.1) {
            my $msg = "ExecutionEnvironment $xenv has variability larger than 10% regarding MainMemorySize:";
            $msg .= " Min=$minval{pmem}($minnod{pmem}),";
            $msg .= " Max=$maxval{pmem}($maxnod{pmem})";
            $log->info($msg);
            $homogeneous = 0;
        }
        $stats{pmem} = $minval{pmem};
    }
    if ($maxval{vmem}) {
        my $rdev = 2 * ($maxval{vmem} - $minval{vmem}) / ($maxval{vmem} + $minval{vmem});
        if ($rdev > 0.5) {
            my $msg = "ExecutionEnvironment $xenv has variability larger than 50% regarding VirtualMemorySize:";
            $msg .= " Min=$minval{vmem}($minnod{vmem}),";
            $msg .= " Max=$maxval{vmem}($maxnod{vmem})";
            $log->debug($msg);
        }
        $stats{vmem} = $minval{vmem};
    }
    $stats{homogeneous} = $homogeneous;

    return \%stats;
}

# Combine info about ExecutionEnvironments from config options and the LRMS plugin
sub xeinfos {
    my ($config, $nodes) = @_;
    my $infos = {};
    my %nodemap = ();
    my @xenvs = keys %{$config->{xenvs}};
    for my $xenv (@xenvs) {
        my $xecfg = $config->{xenvs}{$xenv};
        my $info = $infos->{$xenv} = {};
        my $nscfg = $xecfg->{NodeSelection};
        if (ref $nodes eq 'HASH') {
            my $selected;
            if (not $nscfg) {
                $log->info("NodeSelection configuration missing for ExecutionEnvironment $xenv, implicitly assigning all nodes into it")
                    unless keys %$nodes == 1 and @xenvs == 1;
                $selected = $nodes;
            } else {
                $selected = selectnodes($nodes, %$nscfg);
            }
            $nodemap{$xenv} = $selected;
            $log->debug("Nodes in ExecutionEnvironment $xenv: ".join ' ', keys %$selected);
            $log->info("No nodes matching NodeSelection for ExecutionEnvironment $xenv") unless %$selected;
            my $stats = xestats($xenv, $selected);
            if ($stats) {
                $info->{ntotal} = $stats->{total};
                $info->{nbusy} = $stats->{available} - $stats->{free};
                $info->{nunavailable} = $stats->{total} - $stats->{available};
                $info->{pmem} = $stats->{pmem} if $stats->{pmem};
                $info->{vmem} = $stats->{vmem} if $stats->{vmem};
                $info->{pcpus} = $stats->{pcpus} if $stats->{pcpus};
                $info->{lcpus} = $stats->{lcpus} if $stats->{lcpus};
                $info->{slots} = $stats->{slots} if $stats->{slots};
                $info->{sysname} = $stats->{sysname} if $stats->{sysname};
                $info->{machine} = $stats->{machine} if $stats->{machine};
            }
        } else {
            $log->info("The LRMS plugin has no support for NodeSelection options, ignoring them") if $nscfg;
        }
        $info->{pmem} = $xecfg->{MainMemorySize} if $xecfg->{MainMemorySize};
        $info->{vmem} = $xecfg->{VirtualMemorySize} if $xecfg->{VirtualMemorySize};
        $info->{pcpus} = $xecfg->{PhysicalCPUs} if $xecfg->{PhysicalCPUs};
        $info->{lcpus} = $xecfg->{LogicalCPUs} if $xecfg->{LogicalCPUs};
        $info->{sysname} = $xecfg->{OSFamily} if $xecfg->{OSFamily};
        $info->{machine} = $xecfg->{Platform} if $xecfg->{Platform};
    }
    # Check for overlap of nodes
    if (ref $nodes eq 'HASH') {
        for (my $i=0; $i<@xenvs; $i++) {
            my $nodes1 = $nodemap{$xenvs[$i]};
            next unless $nodes1;
            for (my $j=0; $j<$i; $j++) {
                my $nodes2 = $nodemap{$xenvs[$j]};
                next unless $nodes2;
                my $overlap = intersection($nodes1, $nodes2);
                $log->warning("Overlap detected between ExecutionEnvironments $xenvs[$i] and $xenvs[$j]. "
                             ."Use NodeSelection options to select correct nodes") if %$overlap;
            }
        }
    }
    return $infos;
}

# For each duration, find the largest available numer of slots of any user
# Input: the users hash returned by thr LRMS module.
sub max_userfreeslots {
    my ($users) = @_;
    my %timeslots;

    for my $uid (keys %$users) {
        my $uinfo = $users->{$uid};
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
    return %timeslots;
}


############################################################################
# Combine info from all sources to prepare the final representation
############################################################################

sub collect($) {
    my ($options) = @_;
    my ($checker, @messages);
    
    my $result = get_cluster_info($options);

    $checker = InfoChecker->new($arc1_info_schema);
    @messages = $checker->verify($result,1);
    $log->debug("SelfCheck: return value ComputingService->$_") foreach @messages;

    return $result;
}

sub get_cluster_info($) {
    my ($options) = @_;
    my $config = $options->{config};
    my $usermap = $options->{usermap};
    my $host_info = $options->{host_info};
    my $rte_info = $options->{rte_info};
    my $gmjobs_info = $options->{gmjobs_info};
    my $lrms_info = $options->{lrms_info};

    my $creation_time = timenow();
    my $validity_ttl = $config->{ttl};

    my @allxenvs = keys %{$config->{xenvs}};
    my @allshares = keys %{$config->{shares}};

    my $homogeneous = 1;
    $homogeneous = 0 if @allxenvs > 1;
    $homogeneous = 0 if @allshares > 1 and @allxenvs == 0;
    for my $xeconfig (values %{$config->{xenvs}}) {
        $homogeneous = 0 if defined $xeconfig->{Homogeneous}
                            and not $xeconfig->{Homogeneous};
    }

    my $xeinfos = xeinfos($config, $lrms_info->{nodes});

    # Figure out total number of CPUs
    my ($totalpcpus, $totallcpus) = (0,0);

    # First, try to sum up cpus from all ExecutionEnvironments
    for my $xeinfo (values %$xeinfos) {
        unless (exists $xeinfo->{ntotal} and $xeinfo->{pcpus}) { $totalpcpus = 0; last }
        $totalpcpus += $xeinfo->{ntotal}  *  $xeinfo->{pcpus};
    }
    for my $xeinfo (values %$xeinfos) {
        unless (exists $xeinfo->{ntotal} and $xeinfo->{lcpus}) { $totallcpus = 0; last }
        $totallcpus += $xeinfo->{ntotal}  *  $xeinfo->{lcpus};
    }
    #$log->debug("Cannot determine total number of physical CPUs in all ExecutionEnvironments") unless $totalpcpus;
    $log->debug("Cannot determine total number of logical CPUs in all ExecutionEnvironments") unless $totallcpus;

    # Next, use value returned by LRMS in case the the first try failed.
    # OBS: most LRMSes don't differentiate between Physical and Logical CPUs.
    $totalpcpus ||= $lrms_info->{cluster}{totalcpus};
    $totallcpus ||= $lrms_info->{cluster}{totalcpus};



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
        my $share = $job->{share};

        my $gmstatus = $job->{status} || '';

        $gmtotalcount{totaljobs}++;
        $gmsharecount{$share}{totaljobs}++;

        # count GM states by category

        my %states = ( 'UNDEFINED'        => [0, 'undefined'],
                       'ACCEPTING'        => [1, 'accepted'],
                       'ACCEPTED'         => [1, 'accepted'],
                       'PENDING:ACCEPTED' => [1, 'accepted'],
                       'PREPARING'        => [2, 'preparing'],
                       'PENDING:PREPARING'=> [2, 'preparing'],
                       'SUBMIT'           => [2, 'preparing'],
                       'SUBMITTING'       => [2, 'preparing'],
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
        }
        if ($age < 5) {
            $gmtotalcount{notfinished}++;
            $gmsharecount{$share}{notfinished}++;
        }
        if ($age < 3) {
            $gmtotalcount{notsubmitted}++;
            $gmsharecount{$share}{notsubmitted}++;
            $requestedslots{$share} += $job->{count} || 1;
            $share_prepping{$share}++;
            $user_prepping{$gridowner}++;
        }
        if ($age < 2) {
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
                    $inlrmsslots{$share}{running} ||= 0;
                    $inlrmsslots{$share}{suspended} ||= 0;
                    $inlrmsslots{$share}{queued} ||= 0;
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

    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
    # # # # # # # # # # build information tree  # # # # # # # # # #
    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

    my $admindomain = $config->{AdminDomain};
    my $servicename = $admindomain.":".$config->{service}{ClusterName};

    my $arexhostport = $config->{arexhostport};

    # Global IDs
    my $adID = "urn:ogf:AdminDomain:$admindomain"; # AdminDomain ID
    my $csvID = "urn:ogf:ComputingService:$servicename"; # ComputingService ID
    my $cmgrID = "urn:ogf:ComputingManager:$servicename"; # ComputingManager ID
    my $cepID = "urn:ogf:ComputingEndpoint:$arexhostport"; # ComputingEndpoint ID
    my $cactIDp = "urn:ogf:ComputingActivity:$arexhostport"; # ComputingActivity ID prefix
    my $cshaIDp = "urn:ogf:ComputingShare:$servicename"; # ComputingShare ID prefix
    my $xenvIDp = "urn:ogf:ExecutionEnvironment:$servicename"; # ExecutionEnvironment ID prefix
    my $aenvIDp = "urn:ogf:ApplicationEnvironment:$servicename"; # ApplicationEnvironment ID prefix
    my $apolID = "urn:ogf:AccessPolicy:$servicename"; # AccessPolicy ID prefix
    my $mpolIDp = "urn:ogf:MappingPolicy:$servicename"; # MappingPolicy ID prefix
    my %cactIDs; # ComputingActivity IDs
    my %cshaIDs; # ComputingShare IDs
    my %aenvIDs; # ApplicationEnvironment IDs
    my %xenvIDs; # ExecutionEnvironment IDs

    # Generate ComputingShare IDs
    for my $share (keys %{$config->{shares}}) {
        $cshaIDs{$share} = "$cshaIDp:$share";
    }

    # Generate ApplicationEnvironment IDs
    for my $rte (keys %$rte_info) {
        $aenvIDs{$rte} = "$aenvIDp:$rte";
    }

    # Generate ExecutionEnvironment IDs
    $xenvIDs{$_} = "$xenvIDp:$_" for @allxenvs;

    # generate ComputingActivity IDs
    for my $jobid (keys %$gmjobs_info) {
        my $share = $gmjobs_info->{$jobid}{share};
        $cactIDs{$share}{$jobid} = "$cactIDp:$jobid";
    }


    # ComputingService

    my $csv = {};

    $csv->{CreationTime} = $creation_time;
    $csv->{Validity} = $validity_ttl;
    $csv->{BaseType} = 'Service';

    $csv->{ID} = [ $csvID ];

    $csv->{Name} = [ $config->{service}{ClusterName} ] if $config->{service}{ClusterName};
    $csv->{OtherInfo} = $config->{service}{OtherInfo} if $config->{service}{OtherInfo};
    $csv->{Capability} = [ 'executionmanagement.jobexecution' ];
    $csv->{Type} = [ 'org.nordugrid.execution.arex' ];

    # OBS: QualityLevel reflects the quality of the sotware
    # One of: development, testing, pre-production, production
    $csv->{QualityLevel} = [ 'development' ];

    $csv->{StatusInfo} =  $config->{service}{StatusInfo} if $config->{service}{StatusInfo};

    my $nshares = keys %{$config->{shares}};
    $csv->{Complexity} = [ "endpoint=1,share=$nshares,resource=1" ];

    # OBS: Finished/failed/deleted jobs are not counted
    $csv->{TotalJobs} = [ $gmtotalcount{notfinished} || 0 ];

    $csv->{RunningJobs} = [ $inlrmsjobstotal{running} || 0 ];
    $csv->{SuspendedJobs} = [ $inlrmsjobstotal{suspended} || 0 ];
    $csv->{WaitingJobs} = [ $inlrmsjobstotal{queued} || 0 ];

    $csv->{StagingJobs} = [ ( $gmtotalcount{preparing} || 0 )
                         + ( $gmtotalcount{finishing} || 0 ) ];

    $csv->{PreLRMSWaitingJobs} = [ $pendingtotal || 0 ];

    if (my $lconfig = $config->{location}) {
        my $loc = $csv->{Location} = {};
        $loc->{ID} = [ "urn:ogf:Location:$servicename" ];
        for (qw(Name Address Place PostCode Country Latitude Longitude)) {
            $loc->{$_} = [ $lconfig->{$_} ] if $lconfig->{$_};
        }
    }
    if ($config->{contacts}) {
        for my $cconfig (@{$config->{contacts}}) {
            my $cont = {};
            push @{$csv->{Contact}}, $cont;
            my $detail = $cconfig->{Detail};
            $cont->{ID} = [ "urn:ogf:Contact:$servicename:$detail" ];
            for (qw(Name Detail Type)) {
                $cont->{$_} = [ $cconfig->{$_} ] if $cconfig->{$_};
            }
        }
    }

    $csv->{ComputingShares} = { ComputingShare => [] };


    # ComputingEndpoint

    my $cep = {};
    $csv->{ComputingEndpoint} = [ $cep ];

    $cep->{CreationTime} = $creation_time;
    $cep->{Validity} = $validity_ttl;
    $cep->{BaseType} = 'Endpoint';

    $cep->{ID} = [ $cepID ];

    # Name not necessary

    # OBS: ideally HED should be asked for the URL
    $cep->{URL} = [ $config->{endpoint} ];
    $cep->{Capability} = [ 'executionmanagement.jobexecution' ];
    $cep->{Technology} = [ 'webservice' ];
    $cep->{InterfaceName} = [ 'ogf.bes' ];
    $cep->{InterfaceVersion} = [ '1.0' ];
    # InterfaceExtension should return the same as BESExtension attribute of BES-Factory 
    #$cep->{InterfaceExtension} = [ 'http://www.nordugrid.org/schemas/a-rex' ];
    $cep->{WSDL} = [ $config->{endpoint}."/?wsdl" ];
    # Wrong type, should be URI
    $cep->{SupportedProfile} = [ "WS-I 1.0", "HPC-BP" ];
    $cep->{Semantics} = [ "http://www.nordugrid.org/documents/arex.pdf" ];
    $cep->{Implementor} = [ "NorduGrid" ];
    $cep->{ImplementationName} = [ "ARC" ];
    $cep->{ImplementationVersion} = [ $config->{arcversion} ];

    $cep->{QualityLevel} = [ "development" ];

    # TODO: a way to detect if LRMS is up
    $cep->{HealthState} = [ "ok" ];
    #$cep->{HealthStateInfo} = [ "" ];

    # OBS: when is it 'queueing' and 'closed'?
    # OBS: allownew no longer supported by arex?
    # OBS: This option is not even parsed by ConfigCentral
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
    $cep->{JobDescription} = [ 'ogf:jsdl:1.0', "nordugrid:xrsl" ];

    $cep->{TotalJobs} = [ $gmtotalcount{notfinished} || 0 ];

    $cep->{RunningJobs} = [ $inlrmsjobstotal{running} || 0 ];
    $cep->{SuspendedJobs} = [ $inlrmsjobstotal{suspended} || 0 ];
    $cep->{WaitingJobs} = [ $inlrmsjobstotal{queued} || 0 ];

    $cep->{StagingJobs} = [ ( $gmtotalcount{preparing} || 0 )
                         + ( $gmtotalcount{finishing} || 0 ) ];

    $cep->{PreLRMSWaitingJobs} = [ $pendingtotal || 0 ];

    $cep->{Associations}{ComputingShareID} = [ values %cshaIDs ];

    $cep->{ComputingActivities}{ComputingActivity} ||= [];


    # AccessPolicy: all unique VOs mapped to shares.

    my %allvos = ();
    $allvos{$_} = 1 for @{$config->{service}{AuthorizedVO}};
    for my $sconfig (values %{$config->{shares}} ) {
        next unless $sconfig->{AuthorizedVO};
        $allvos{$_} = 1 for @{$sconfig->{AuthorizedVO}};
    }
    $cep->{AccessPolicy} = [];
    if (%allvos) {
        my $apol = $cep->{AccessPolicy}[0] = {};
        $apol->{ID} = [ $apolID ];
        $apol->{Scheme} = [ "basic" ];
        $apol->{Rule} = [ map {"vo:$_"} keys %allvos ];
    } else {
        $log->info("No AuthorizedVO configured. No access policy will be published");
    }

    # ComputingShares: multiple shares can share the same LRMS queue

    for my $share (keys %{$config->{shares}}) {

        my $qinfo = $lrms_info->{queues}{$share};

        # Prepare flattened config hash for this share.
        my $sconfig = { %{$config->{service}}, %{$config->{shares}{$share}} };

        # List of all shares submitting to the current queue, including the current share.
        my $qname = $sconfig->{MappingQueue} || $share;

        if ($qname) {
            my $siblings = $sconfig->{siblingshares} = [];
            for my $sn (keys %{$config->{shares}}) {
                my $s = $config->{shares}{$sn};
                my $qn = $s->{MappingQueue} || $sn;
                push @$siblings, $sn if $qn eq $qname;
            }
        }

        my $csha = {};

        push @{$csv->{ComputingShares}{ComputingShare}}, $csha;

        $csha->{CreationTime} = $creation_time;
        $csha->{Validity} = $validity_ttl;
        $csha->{BaseType} = 'Share';

        $csha->{ID} = [ $cshaIDs{$share} ];

        $csha->{Name} = [ $share ];
        $csha->{Description} = [ $sconfig->{Description} ] if $sconfig->{Description};
        $csha->{MappingQueue} = [ $qname ] if $qname;

        # use limits from LRMS
        $csha->{MaxCPUTime} = [ $qinfo->{maxcputime} ] if defined $qinfo->{maxcputime};
        # TODO: implement in backends
        $csha->{MaxTotalCPUTime} = [ $qinfo->{maxtotalcputime} ] if defined $qinfo->{maxtotalcputime};
        $csha->{MinCPUTime} = [ $qinfo->{mincputime} ] if defined $qinfo->{mincputime};
        $csha->{DefaultCPUTime} = [ $qinfo->{defaultcput} ] if defined $qinfo->{defaultcput};
        $csha->{MaxWallTime} =  [ $qinfo->{maxwalltime} ] if defined $qinfo->{maxwalltime};
        # TODO: MaxMultiSlotWallTime replaces MaxTotalWallTime, but has different meaning. Check that it's used correctly
        #$csha->{MaxMultiSlotWallTime} =  [ $qinfo->{maxwalltime} ] if defined $qinfo->{maxwalltime};
        $csha->{MinWallTime} =  [ $qinfo->{minwalltime} ] if defined $qinfo->{minwalltime};
        $csha->{DefaultWallTime} = [ $qinfo->{defaultwallt} ] if defined $qinfo->{defaultwallt};

        my ($maxtotal, $maxlrms) = split ' ', ($config->{maxjobs} || '');
        $maxtotal = undef if defined $maxtotal and $maxtotal eq '-1';
        $maxlrms = undef if defined $maxlrms and $maxlrms eq '-1';

        # MaxWaitingJobs: use the maxjobs config option
        # OBS: An upper limit is not really enforced by A-REX.
        # OBS: Currently A-REX only cares about totals, not per share limits!
        $csha->{MaxTotalJobs} = [ $maxtotal ] if defined $maxtotal;

        # MaxWaitingJobs, MaxRunningJobs:
        my ($maxrunning, $maxwaiting);

        # use values from lrms if avaialble
        if (defined $qinfo->{maxrunning}) {
            $maxrunning = $qinfo->{maxrunning};
        }
        if (defined $qinfo->{maxqueuable}) {
            $maxwaiting = $qinfo->{maxqueuable};
        }

        # maxjobs config option sets upper limits
        if (defined $maxlrms) {
            $maxrunning = $maxlrms
                if not defined $maxrunning or $maxrunning > $maxlrms;
            $maxwaiting = $maxlrms
                if not defined $maxwaiting or $maxwaiting > $maxlrms;
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

        # TODO: new return value from LRMS infocollector
        # TODO: see how LRMSs can detect the correct value
        $csha->{MaxSlotsPerJob} = [ $sconfig->{MaxSlotsPerJob} || $qinfo->{MaxSlotsPerJob}  || 1 ];

        # MaxStageInStreams, MaxStageOutStreams
        # OBS: A-REX does not have separate limits for up and downloads.
        # OBS: A-REX only cares about totals, not per share limits!
        my ($maxloaders, $dummy, $maxthreads) = split ' ', ($config->{maxload} || '');
        $maxloaders = undef if defined $maxloaders and $maxloaders eq '-1';
        $maxthreads = undef if defined $maxthreads and $maxthreads eq '-1';
        if ($maxloaders) {
            # default is 5 (see MAX_DOWNLOADS defined in a-rex/grid-manager/loaders/downloader.cpp)
            $maxthreads = 5 unless defined $maxthreads;
            $csha->{MaxStageInStreams}  = [ $maxloaders * $maxthreads ];
            $csha->{MaxStageOutStreams} = [ $maxloaders * $maxthreads ];
        }

        # TODO: new return value schedpolicy from LRMS infocollector.
        my $schedpolicy = $lrms_info->{schedpolicy} || undef;
        if ($sconfig->{SchedulingPolicy} and not $schedpolicy) {
            $schedpolicy = 'fifo' if lc($sconfig->{SchedulingPolicy}) eq 'fifo';
            $schedpolicy = 'fairshare' if lc($sconfig->{SchedulingPolicy}) eq 'maui';
        }
        $csha->{SchedulingPolicy} = [ $schedpolicy ] if $schedpolicy;


        # GuaranteedVirtualMemory -- all nodes must be able to provide this
        # much memory per job. Some nodes might be able to afford more per job
        # (MaxVirtualMemory)
        # TODO: implement check at job accept time in a-rex
        # TODO: implement in LRMS plugin maxvmem and maxrss.
        $csha->{MaxVirtualMemory} = [ $sconfig->{MaxVirtualMemory} ] if $sconfig->{MaxVirtualMemory};
        # MaxMainMemory -- usage not being tracked by most LRMSs

        # OBS: new config option (space measured in GB !?)
        # OBS: Disk usage of jobs is not being enforced.
        # This limit should correspond with the max local-scratch disk space on
        # clusters using local disks to run jobs.
        # TODO: implement check at job accept time in a-rex
        # TODO: see if any lrms can support this. Implement check in job wrapper
        $csha->{MaxDiskSpace} = [ $sconfig->{DiskSpace} ] if $sconfig->{DiskSpace};

        # DefaultStorageService:

        # OBS: Should be ExtendedBoolean_t (one of 'true', 'false', 'undefined')
        $csha->{Preemption} = [ $qinfo->{Preemption} ] if $qinfo->{Preemption};

        # ServingState: closed and queuing are not yet supported
        if (defined $config->{allownew} and lc($config->{allownew}) eq 'no') {
            $csha->{ServingState} = [ 'draining' ];
        } else {
            $csha->{ServingState} = [ 'production' ];
        }

        # Count local jobs
        my $localrunning = $qinfo->{running};
        my $localqueued = $qinfo->{queued};
        my $localsuspended = $qinfo->{suspended} || 0;

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

        $csha->{StagingJobs} = [ ( $gmsharecount{$share}{preparing} || 0 )
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

        my @durations;
        my %timeslots = max_userfreeslots($qinfo->{users});

        if (%timeslots) {

            # find maximum free slots regardless of duration
            my $maxuserslots = 0;
            for my $seconds ( keys %timeslots ) {
                my $nfree = $timeslots{$seconds};
                $maxuserslots = $nfree if $nfree > $maxuserslots;
            }
            $freeslots = $maxuserslots < $freeslots
                       ? $maxuserslots : $freeslots;

            # sort descending by duration, keping 0 first (0 for unlimited)
            for my $seconds (sort { if ($a == 0) {1} elsif ($b == 0) {-1} else {$b <=> $a} } keys %timeslots) {
                my $nfree = $timeslots{$seconds} < $freeslots
                          ? $timeslots{$seconds} : $freeslots;
                unshift @durations, $seconds ? "$nfree:$seconds" : $nfree;
            }
        }

        $freeslots = 0 if $freeslots < 0;

        $csha->{FreeSlots} = [ $freeslots ];
        $csha->{FreeSlotsWithDuration} = [ join(" ", @durations) || 0 ];
        $csha->{UsedSlots} = [ $qinfo->{running} ];
        $csha->{RequestedSlots} = [ $requestedslots{$share} || 0 ];

        # TODO: detect reservationpolicy in the lrms
        $csha->{ReservationPolicy} = [ $qinfo->{reservationpolicy} ]
            if $qinfo->{reservationpolicy};

        # Tag: skip it for now

        my $xenvs = $sconfig->{ExecEnvName} || [];
        push @{$csha->{Associations}{ExecutionEnvironmentID}}, $xenvIDs{$_} for @$xenvs;

        $csha->{Associations}{ComputingEndpointID} = [ $cepID ];
        $csha->{Associations}{ComputingActivityID} = [ values %{$cactIDs{$share}} ];

        # MappingPolicy: VOs mapped to this share.
    
        my $vos = $sconfig->{AuthorizedVO};
        if ($vos) {
            my $mpol = $csha->{MappingPolicy}[0] = {};
            $mpol->{ID} = [ "$mpolIDp:$share" ];
            $mpol->{Scheme} = [ "basic" ];
            $mpol->{Rule} = [ map {"vo:$_"} @$vos ];
        }

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

    $cmgr->{ProductName} = [ $cluster_info->{lrms_glue_type} || lc $cluster_info->{lrms_type} ];
    $cmgr->{ProductVersion} = [ $cluster_info->{lrms_version} ];
    # $cmgr->{Reservation} = [ "undefined" ];
    $cmgr->{BulkSubmission} = [ "false" ];

    #$cmgr->{TotalPhysicalCPUs} = [ $totalpcpus ] if $totalpcpus;
    $cmgr->{TotalLogicalCPUs} = [ $totallcpus ] if $totallcpus;

    # OBS: Assuming 1 slot per CPU
    $cmgr->{TotalSlots} = [ $cluster_info->{totalcpus} ];

    my $gridrunningslots = 0; $gridrunningslots += $_->{running} for values %inlrmsslots;
    my $localrunningslots = $cluster_info->{usedcpus} - $gridrunningslots;
    $cmgr->{SlotsUsedByLocalJobs} = [ $localrunningslots ];
    $cmgr->{SlotsUsedByGridJobs} = [ $gridrunningslots ];

    $cmgr->{Homogeneous} = [ $homogeneous ? "true" : "false" ];

    # NetworkInfo of all ExecutionEnvironments
    my %netinfo = ();
    for my $xeconfig (values %{$config->{xenvs}}) {
        $netinfo{$xeconfig->{NetworkInfo}} = 1 if $xeconfig->{NetworkInfo};
    }
    $cmgr->{NetworkInfo} = [ keys %netinfo ] if %netinfo;

    # TODO: this could also be cross-checked with info from ExecEnvs
    my $cpuistribution = $cluster_info->{cpudistribution} || '';
    $cpuistribution =~ s/cpu:/:/g;
    $cmgr->{LogicalCPUDistribution} = [ $cpuistribution ] if $cpuistribution;

    {
        my $sharedsession = "true";
        $sharedsession = "false" if lc($config->{shared_filesystem}) eq "no"
                                 or lc($config->{shared_filesystem}) eq "false";
        $cmgr->{WorkingAreaShared} = [ $sharedsession ];
        $cmgr->{WorkingAreaGuaranteed} = [ "false" ];

        my ($sessionlifetime) = (split ' ', $config->{defaultttl});
        $sessionlifetime ||= 7*24*60*60;
        $cmgr->{WorkingAreaLifeTime} = [ $sessionlifetime ];

        my $gigstotal = ceil($host_info->{session_total} / 1024);
        my $gigsfree = ceil($host_info->{session_free} / 1024);
        $cmgr->{WorkingAreaTotal} = [ $gigstotal ];
        $cmgr->{WorkingAreaFree} = [ $gigsfree ];

        # OBS: There is no special area for MPI jobs, no need to advertize anything
        #$cmgr->{WorkingAreaMPIShared} = [ $sharedsession ];
        #$cmgr->{WorkingAreaMPITotal} = [ $gigstotal ];
        #$cmgr->{WorkingAreaMPIFree} = [ $gigsfree ];
        #$cmgr->{WorkingAreaMPILifeTime} = [ $sessionlifetime ];
    }
    {
        my $gigstotal = ceil($host_info->{cache_total} / 1024);
        my $gigsfree = ceil($host_info->{cache_free} / 1024);
        $cmgr->{CacheTotal} = [ $gigstotal ];
        $cmgr->{CacheFree} = [ $gigsfree ];
    }

    $cmgr->{Benchmark} = [];
    if ($config->{service}{Benchmark}) {
        for my $benchmark (@{$config->{service}{Benchmark}}) {
            my ($type, $value) = split " ", $benchmark;
            my $bench = {};
            $bench->{Type} = [ $type ];
            $bench->{Value} = [ $value ];
            $bench->{ID} = [ "urn:ogf:Benchmark:$servicename:$type" ];
            push @{$cmgr->{Benchmark}}, $bench;
        }
    }

    # Not publishing absolute paths
    #$cmgr->{TmpDir};
    #$cmgr->{ScratchDir};
    #$cmgr->{ApplicationDir};

    # ExecutionEnvironments

    for my $xenv (keys %{$config->{xenvs}}) {

        my $xeinfo = $xeinfos->{$xenv};

        # Prepare flattened config hash for this xenv.
        my $xeconfig = { %{$config->{service}}, %{$config->{xenvs}{$xenv}} };

        my $execenv = {};
        push @{$cmgr->{ExecutionEnvironments}{ExecutionEnvironment}}, $execenv;

        $execenv->{Name} = [ $xenv ];
        $execenv->{CreationTime} = $creation_time;
        $execenv->{Validity} = $validity_ttl;
        $execenv->{BaseType} = 'Resource';

        $execenv->{ID} = [ $xenvIDs{$xenv} ];

        my $machine = $xeinfo->{machine};
        if ($machine) {
            $machine =~ s/^x86_64/amd64/;
            $machine =~ s/^ia64/itanium/;
            $machine =~ s/^ppc/powerpc/;
        }
        my $sysname = $xeinfo->{sysname};
        if ($sysname) {
            $sysname =~ s/^Linux/linux/;
            $sysname =~ s/^Darwin/macosx/;
            $sysname =~ s/^SunOS/solaris/;
        } elsif ($xeconfig->{OpSys}) {
            $sysname = 'linux' if grep /linux/i, @{$xeconfig->{OpSys}};
        }

        $execenv->{Platform} = [ $machine ] if $machine;
        $execenv->{TotalInstances} = [ $xeinfo->{ntotal} ] if defined $xeinfo->{ntotal};
        $execenv->{UsedInstances} = [ $xeinfo->{nbusy} ] if defined $xeinfo->{nbusy};
        $execenv->{UnavailableInstances} = [ $xeinfo->{nunavailable} ] if defined $xeinfo->{nunavailable};
        $execenv->{VirtualMachine} = [ $xeconfig->{VirtualMachine} ] if defined $xeconfig->{VirtualMachine};

        $execenv->{PhysicalCPUs} = [ $xeinfo->{pcpus} ] if $xeinfo->{pcpus};
        $execenv->{LogicalCPUs} = [ $xeinfo->{lcpus} ] if $xeinfo->{lcpus};
        if ($xeinfo->{pcpus} and $xeinfo->{lcpus}) {
            my $cpum = ($xeinfo->{pcpus} > 1) ? 'multicpu' : 'singlecpu';
            my $corem = ($xeinfo->{lcpus} > $xeinfo->{pcpus}) ? 'multicore' : 'singlecore';
            $execenv->{CPUMultiplicity} = [ "$cpum-$corem" ];
        }
        $execenv->{CPUVendor} = [ $xeconfig->{CPUVendor} ] if $xeconfig->{CPUVendor};
        $execenv->{CPUModel} = [ $xeconfig->{CPUModel} ] if $xeconfig->{CPUModel};
        $execenv->{CPUVersion} = [ $xeconfig->{CPUVersion} ] if $xeconfig->{CPUVersion};
        $execenv->{CPUClockSpeed} = [ $xeconfig->{CPUClockSpeed} ] if $xeconfig->{CPUClockSpeed};
        $execenv->{CPUTimeScalingFactor} = [ $xeconfig->{CPUTimeScalingFactor} ] if $xeconfig->{CPUTimeScalingFactor};
        $execenv->{WallTimeScalingFactor} = [ $xeconfig->{WallTimeScalingFactor} ] if $xeconfig->{WallTimeScalingFactor};
        $execenv->{MainMemorySize} = [ $xeinfo->{pmem} ] if $xeinfo->{pmem};
        $execenv->{VirtualMemorySize} = [ $xeinfo->{vmem} ] if $xeinfo->{vmem};
        $execenv->{OSFamily} = [ $sysname ] if $sysname;
        $execenv->{OSName} = [ $xeconfig->{OSName} ] if $xeconfig->{OSName};
        $execenv->{OSVersion} = [ $xeconfig->{OSVersion} ] if $xeconfig->{OSVersion};
        $execenv->{ConnectivityIn} = [ $xeconfig->{ConnectivityIn} ? 'true' : 'false' ] if $xeconfig->{ConnectivityIn};
        $execenv->{ConnectivityOut} = [ $xeconfig->{ConnectivityOut} ? 'true' : 'false' ] if $xeconfig->{ConnectivityOut};
        $execenv->{NetworkInfo} = [ $xeconfig->{NetworkInfo} ] if $xeconfig->{NetworkInfo};

        my @missing;
        for (qw(Platform CPUVendor CPUModel CPUClockSpeed OSFamily OSName OSVersion)) {push @missing, $_ unless defined $execenv->{$_}}
        $log->info("Missing attributes for ExecutionEnvironment $xenv: ".join ", ", @missing) if @missing;

        $execenv->{Benchmark} = [];
        if ($xeconfig->{Benchmark}) {
            for my $benchmark (@{$xeconfig->{Benchmark}}) {
                my ($type, $value) = split " ", $benchmark;
                my $bench = {};
                $bench->{Type} = [ $type ];
                $bench->{Value} = [ $value ];
                $bench->{ID} = [ "urn:ogf:Benchmark:$servicename:$xenv:$type" ];
                push @{$execenv->{Benchmark}}, $bench;
            }
        }

        $execenv->{Associations} = {};
        for my $share (keys %{$config->{shares}}) {
            my $sconfig = $config->{shares}{$share};
            next unless $sconfig->{ExecEnvName};
            next unless grep { $xenv eq $_ } @{$sconfig->{ExecEnvName}};
            push @{$execenv->{Associations}{ComputingShareID}}, $cshaIDs{$share};
        }

    }


    # ApplicationEnvironments

    $cmgr->{ApplicationEnvironments}{ApplicationEnvironment} = [];

    for my $rte (sort keys %$rte_info) {

        my $appenv = {};
        push @{$cmgr->{ApplicationEnvironments}{ApplicationEnvironment}}, $appenv;

        # name and version is separated at the first dash (-) which is followed by a digit
        my ($name,$version) = ($rte, undef)
        ($name,$version) = ($1, $2) if $rte =~ m{^(.*?)(?:-([0-9].*))?$};

        $appenv->{AppName} = [ $name ];
        $appenv->{AppVersion} = [ $version ] if defined $version;
        $appenv->{ID} = [ $aenvIDs{$rte} ];
        $appenv->{State} = [ $rte_info->{$rte}{state} ] if $rte_info->{$rte}{state};
        $appenv->{Description} = [ $rte_info->{$rte}{description} ] if $rte_info->{$rte}{description};
        #$appenv->{ParallelSupport} = [ 'none' ];
    }

    # Computing Activities

    for my $jobid (keys %$gmjobs_info) {

        my $gmjob = $gmjobs_info->{$jobid};

        my $exited= undef; # whether the job has already run; 

        my $cact = {};
        push @{$cep->{ComputingActivities}{ComputingActivity}}, $cact;

        $cact->{CreationTime} = $creation_time;
        $cact->{Validity} = $validity_ttl;
        $cact->{BaseType} = 'Activity';

        my $share = $gmjob->{share};
        my $gridid = $config->{endpoint}."/$jobid";

        $cact->{Type} = [ 'single' ];
        $cact->{ID} = [ $cactIDs{$share}{$jobid} ];
        $cact->{IDFromEndpoint} = [ $gmjob->{globalid} ] if $gmjob->{globalid};
        $cact->{Name} = [ $gmjob->{jobname} ] if $gmjob->{jobname};
        # TODO: properly set either ogf:jsdl:1.0 or nordugrid:xrsl
        $cact->{JobDescription} = [ $gmjob->{description} eq 'xml' ? "ogf:jsdl:1.0" : "nordugrid:xrsl" ] if $gmjob->{description};
        $cact->{RestartState} = glueState($gmjob->{failedstate}) if $gmjob->{failedstate};
        $cact->{ExitCode} = [ $gmjob->{exitcode} ] if defined $gmjob->{exitcode};
        # TODO: modify scan-jobs to write it separately to .diag. All backends should do this.
        $cact->{ComputingManagerExitCode} = [ $gmjob->{lrmsexitcode} ] if $gmjob->{lrmsexitcode};
        $cact->{Error} = [ map { substr($_,0,255) } @{$gmjob->{errors}} ] if $gmjob->{errors};
        # TODO: VO info, like <UserDomain>ATLAS/Prod</UserDomain>; check whether this information is available to A-REX
        $cact->{Owner} = [ $gmjob->{subject} ] if $gmjob->{subject};
        $cact->{LocalOwner} = [ $gmjob->{localowner} ] if $gmjob->{localowner};
        # OBS: Times are in seconds.
        $cact->{RequestedTotalWallTime} = [ $gmjob->{reqwalltime} ] if defined $gmjob->{reqwalltime};
        $cact->{RequestedTotalCPUTime} = [ $gmjob->{reqcputime} ] if defined $gmjob->{reqcputime};
        # OBS: Should include name and version. Exact format not specified
        $cact->{RequestedApplicationEnvironment} = $gmjob->{runtimeenvironments} if $gmjob->{runtimeenvironments};
        $cact->{RequestedSlots} = [ $gmjob->{count} || 1 ];
        $cact->{StdIn} = [ $gmjob->{stdin} ] if $gmjob->{stdin};
        $cact->{StdOut} = [ $gmjob->{stdout} ] if $gmjob->{stdout};
        $cact->{StdErr} = [ $gmjob->{stderr} ] if $gmjob->{stderr};
        $cact->{LogDir} = [ $gmjob->{gmlog} ] if $gmjob->{gmlog};
        $cact->{ExecutionNode} = $gmjob->{nodenames} if $gmjob->{nodenames};
        $cact->{Queue} = [ $gmjob->{queue} ] if $gmjob->{queue};
        # Times for finished jobs
        $cact->{UsedTotalWallTime} = [ $gmjob->{WallTime} * ($gmjob->{count} || 1) ] if defined $gmjob->{WallTime};
        $cact->{UsedTotalCPUTime} = [ $gmjob->{CpuTime} ] if defined $gmjob->{CpuTime};
        $cact->{UsedMainMemory} = [ ceil($gmjob->{UsedMem}/1024) ] if defined $gmjob->{UsedMem};
        my $usbmissiontime = mdstoiso($gmjob->{starttime} || '');
        $cact->{SubmissionTime} = [ $usbmissiontime ] if $usbmissiontime;
        # TODO: change gm to save LRMSSubmissionTime
        #$cact->{ComputingManagerSubmissionTime} = [ 'NotImplemented' ];
        # TODO: this should be queried in scan-job.
        #$cact->{StartTime} = [ 'NotImplemented' ];
        # TODO: scan-job has to produce this
        #$cact->{ComputingManagerEndTime} = [ 'NotImplemented' ];
        my $endtime = mdstoiso($gmjob->{completiontime} || '');
        $cact->{EndTime} = [ $endtime ] if $endtime;
        $cact->{WorkingAreaEraseTime} = [ $gmjob->{cleanuptime} ] if $gmjob->{cleanuptime};
        $cact->{ProxyExpirationTime} = [ $gmjob->{delegexpiretime} ] if $gmjob->{delegexpiretime};
        if ($gmjob->{clientname}) {
            # OBS: address of client as seen by the server is used.
            my $dnschars = '-.A-Za-z0-9';  # RFC 1034,1035
            my ($external_address, $port, $clienthost) = $gmjob->{clientname} =~ /^([$dnschars]+)(?::(\d+))?(?:;(.+))?$/;
            $cact->{SubmissionHost} = [ $external_address ] if $external_address;
        }
        $cact->{SubmissionClientName} = [ $gmjob->{clientsoftware} ] if $gmjob->{clientsoftware};

        # Computing Activity Associations

        # TODO: add link
        $cact->{Associations}{ExecutionEnvironmentID} = [];
        $cact->{Associations}{ActivityID} = $gmjob->{activityid} if $gmjob->{activityid};
        $cact->{Associations}{ComputingShareID} = [ $cshaIDs{$share} || 'UNDEFINEDVALUE' ];

        if ( $gmjob->{status} eq "INLRMS" ) {
            my $lrmsid = $gmjob->{localid};
            $log->warning("No local id for job $jobid") and next unless $lrmsid;
            $cact->{LocalIDFromManager} = [ $lrmsid ];

            my $lrmsjob = $lrms_info->{jobs}{$lrmsid};
            $log->warning("No local job for $jobid") and next unless $lrmsjob;

            $cact->{State} = glueState("INLRMS", $lrmsjob->{status});
            $cact->{WaitingPosition} = [ $lrmsjob->{rank} ] if defined $lrmsjob->{rank};
            $cact->{ExecutionNode} = $lrmsjob->{nodes} if $lrmsjob->{nodes};
            unshift @{$cact->{OtherMessages}}, $_ for @{$lrmsjob->{comment}};

            # Times for running jobs
            $cact->{UsedTotalWallTime} = [ $lrmsjob->{walltime} * ($gmjob->{count} || 1) ] if defined $lrmsjob->{walltime};
            $cact->{UsedTotalCPUTime} = [ $lrmsjob->{cputime} ] if defined $lrmsjob->{cputime};
            $cact->{UsedMainMemory} = [ ceil($lrmsjob->{mem}/1024) ] if defined $lrmsjob->{mem};
        } else {
            $cact->{State} = glueState($gmjob->{status});
        }

    }

    return $csv;
}

1;

