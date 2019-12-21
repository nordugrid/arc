package PBS;

######################################################################
# DISCLAIMER
######################################################################
# This module depends on ARC0mod.pm which is obsolete and deprecated
# starting from ARC 6.0
# Please DO NOT build new LRMS modules based on this one but follow
# the indications in
#                       LRMSInfo.pm
# instead.
######################################################################

use strict;
our @ISA = ('Exporter');

# Module implements these subroutines for the LRMS interface

our @EXPORT_OK = ('cluster_info',
      'queue_info',
      'jobs_info',
      'users_info',
      'nodes_info');

use LogUtils ( 'start_logging', 'error', 'warning', 'debug' );

##########################################
# Saved private variables
##########################################

our(%lrms_queue);
my (%user_jobs_running, %user_jobs_queued);

# the queue passed in the latest call to queue_info, jobs_info or users_info
my $currentqueue = undef;

# cache info returned by PBS commands
my $pbsnodes;
my $qstat_f;
my $qstat_fQ;

# PBS type and flavour
my $lrms_type = undef;
my $lrms_version = undef;

# Resets queue-specific global variables if
# the queue has changed since the last call
sub init_globals($) {
    my $qname = shift;
    if (not defined $currentqueue or $currentqueue ne $qname) {
        $currentqueue = $qname;
        %lrms_queue = ();
        %user_jobs_running = ();
        %user_jobs_queued = ();
    }
}

# get PBS type and version
sub get_pbs_version ($) {
    return unless not defined $lrms_type;

    # path to LRMS commands
    my ($config) = shift;
    my ($path) = $$config{pbs_bin_path};
    error("pbs_bin_path not defined, cannot continue. Exiting...") unless defined $path;

    # determine the flavour and version of PBS
    my $qmgr_string=`$path/qmgr -c "list server"`;
    if ( $? != 0 ) {
        warning("Can't run qmgr");
    }
    if ($qmgr_string =~ /pbs_version = \b(\D+)_(\d\S+)\b/) {
        $lrms_type = $1;
        $lrms_version = $2;
    } else {
        $qmgr_string =~ /pbs_version = \b(\d\S+)\b/;
        $lrms_type = "torque";
        $lrms_version = $1;
    }
}

##########################################
# Private subs
##########################################

sub read_pbsnodes ($) {

    return %$pbsnodes if $pbsnodes;

    #processing the pbsnodes output by using a hash of hashes
    # %hoh_pbsnodes (referrenced by $hashref)

    my ( $path ) = shift;
    my ( %hoh_pbsnodes);
    my ($nodeid,$node_var,$node_value);

    unless (open PBSNODESOUT,  "$path/pbsnodes -a 2>/dev/null |") {
        error("error in executing pbsnodes");
    }
    while (my $line= <PBSNODESOUT>) {
        if ($line =~ /^$/) {next};
        if ($line =~ /^([\w\-]+)/) {
            $nodeid= $1 ;
            next;
        }
        if ($line =~ / = /)  {
            ($node_var,$node_value) = split (/ = /, $line, 2);
            $node_var =~ s/\s+//g;
            chop $node_value;
        }
        $hoh_pbsnodes{$nodeid}{$node_var} = $node_value;
    }
    close PBSNODESOUT;
    $pbsnodes = \%hoh_pbsnodes;

    return %hoh_pbsnodes;
}

sub read_qstat_fQ ($) {
    # return already parsed value
    return %$qstat_fQ if $qstat_fQ;

    #processing the qstat -fQ output by using a hash of hashes
    my ( $path ) = shift;
    my ( %hoh_qstat );

    unless (open QSTATOUTPUT, "$path/qstat -Q -f 2>/dev/null |") {
        error("Error in executing qstat: $path/qstat -Q -f");
    }

    my $current_queue = undef;

    my ($qstat_var,$qstat_value) = ();
    while (my $line= <QSTATOUTPUT>) {
        chomp($line);
        if ($line =~ /^$/) {next};
        if ($line =~ /^Queue: ([\w\-.]+)$/) {
            $current_queue = $1;
            next;
        }
        if ( ! defined $current_queue ) {next};
        if ($line =~ m/ = /) {
            ($qstat_var,$qstat_value) = split("=", $line, 2);
            $qstat_var =~ s/^\s+|\s+$//g;
            $qstat_value =~ s/^\s+|\s+$//g;
            $hoh_qstat{$current_queue}{$qstat_var} = $qstat_value;
            next;
        }
        # older PBS versions has no '-1' support
        # a line starting with a tab is a continuation line
        if ( $line =~ m/^\t(.+)$/ ) {
            $qstat_value .= $1;
            $qstat_value =~ s/\s+$//g;
            $hoh_qstat{$current_queue}{$qstat_var} = $qstat_value;
        }
    }
    close QSTATOUTPUT;

    $qstat_fQ = \%hoh_qstat;
    return %hoh_qstat;
}

sub read_qstat_f ($) {
    # return already parsed value
    return %$qstat_f if $qstat_f;

    #processing the qstat -f output by using a hash of hashes
    my ( $path ) = shift;
    my ( %hoh_qstat );

    unless (open QSTATOUTPUT, "$path/qstat -f 2>/dev/null |") {
        error("Error in executing qstat: $path/qstat -f");
    }

    my $jobid = undef;

    my ($qstat_var,$qstat_value) = ();
    while (my $line= <QSTATOUTPUT>) {
        chomp($line);
        if ($line =~ /^$/) {next};
        if ($line =~ /^Job Id: (.+)$/) {
            $jobid = $1;
            next;
        }
        if ( ! defined $jobid ) {next};
        if ($line =~ m/ = /) {
            ($qstat_var,$qstat_value) = split("=", $line, 2);
            $qstat_var =~ s/^\s+|\s+$//g;
            $qstat_value =~ s/^\s+|\s+$//g;
            $hoh_qstat{$jobid}{$qstat_var} = $qstat_value;
        }
        # older PBS versions has no '-1' support
        # a line starting with a tab is a continuation line
        if ( $line =~ m/^\t(.+)$/ ) {
            $qstat_value .= $1;
            $qstat_value =~ s/\s+$//g;
            $hoh_qstat{$jobid}{$qstat_var} = $qstat_value;
        }
    }
    close QSTATOUTPUT;

    $qstat_f = \%hoh_qstat;
    return %hoh_qstat;
}

# Splits up the value of the exec_host string.
# Returns a list of node names, one for each used cpu.
# Should handle node specs of the form:
#   (1) node1/0+node0/1+node0/2+node2/1                   (torque)
#   (2) hosta/J1+hostb/J2*P       (according to the PBSPro manual)
#   (3) node1+node1+node2+node2
#   (4) altix:ssinodes=2:mem=7974912kb:ncpus=4  (found on the web)
#   (5) grid-wn0749.desy.de/2 Resource_List.neednodes=1:ppn=8 (reported by Andreas Gellrich from Desy HH)
sub split_hostlist {
    my ($exec_host_string) = @_;
    my @nodes;
    my $err;
    for my $nodespec (split '\+', $exec_host_string) {
        if ($nodespec =~ m{^([^/:]+)/\d+(?:\*(\d+))?$}) {  # cases (1) and (2)
            my ($nodename, $multiplier) = ($1, $2 || 1);
            push @nodes, $nodename for 1..$multiplier;
        } elsif ($nodespec =~ m{^([^/:]+)(?::(.+))?$}) {  # cases (3) and (4)
            my ($nodename, $resc) = ($1, $2 || '');
            my $multiplier = get_ncpus($resc);
            push @nodes, $nodename for 1..$multiplier;
        } elsif ($nodespec =~ m{^([^/]+)/\d+ Resource_List\.neednodes=(\d+):ppn=(\d+)?$}  ){   # case (5)
            my $nodename = $1;
            my $numnodes = $2 || 1;
            my $ppn = $3 || 1;
            my $multiplier = $ppn;   #Not sure if this is the correct multiplier. Is there an entry for each node? or should multiplier be numnodes*ppn?
            push @nodes, $nodename for 1..$multiplier;
        } else {
            $err = $nodespec;
        }
    }
    warning("failed counting nodes in expression: $exec_host_string") if $err;
    return @nodes;
}

# Deduces the number of requested cpus from the values of these job properties:
#   Resource_List.select (PBSPro 8+)
#   Resource_List.nodes
#   Resource_List.ncpus
sub set_cpucount {
    my ($job) = (@_);
    my $select = $job->{"Resource_List.select"};
    my $nodes = $job->{"Resource_List.nodes"};
    my $ncpus = $job->{"Resource_List.ncpus"};
    $job->{cpus} = count_usedcpus($select, $nodes, $ncpus);
    delete $job->{"Resource_List.select"};
    delete $job->{"Resource_List.nodes"};
    delete $job->{"Resource_List.ncpus"};
}

# Convert time from [DD:[HH:[MM:]]]SS to minutes
sub count_time {
    my $pbs_time = shift;
    # split and reverse PBS time to start from seconds, then drop seconds
    my @t = reverse split /:/, $pbs_time;
    my $minutes = 0;

    if ( ! defined $t[1] ) {
        # PBS seconds only case
        $minutes = int( $t[0] / 60 );
    } else {
        # drop seconds
        shift @t;
        $minutes = $t[0];
        $minutes += $t[1]*60 if defined $t[1];
        $minutes += $t[2]*60*24 if defined $t[2];
    }
    return $minutes;
}

sub count_usedcpus {
    my ($select, $nodes, $ncpus) = @_;
    return sum_over_chunks(\&cpus_in_select_chunk, $select) if defined $select;
    return $ncpus || 1 if not defined $nodes or $nodes eq '1';
    return sum_over_chunks(\&cpus_in_nodes_chunk, $nodes) if defined $nodes;
    return 1;
}

sub sum_over_chunks {
    my ($count_func, $string) = @_;
    my $totalcpus = 0;
    for my $chunk (split '\+', $string) {
        my $cpus = &$count_func($chunk);
        $totalcpus += $cpus;
    }
    return $totalcpus;
}

# counts cpus in chunk definitions of the forms found in Resource_List.nodes property
#   4
#   2:ppn=2
#   host1
#   host1:prop1:prop2
#   prop1:prop2:ppn=4
sub cpus_in_nodes_chunk {
    my ($chunk) = @_;
    my ($ncpus,$dummy) = split ':', $chunk;
    $ncpus = 1 if $ncpus =~ m/\D/; # chunk count ommited
    return $ncpus * get_ppn($chunk);
}

# counts cpus in chunk definitions of the forms found in Resource_List.select (PBSPro 8+):
#   4
#   2:ncpus=1
#   1:ncpus=4:mpiprocs=4:host=hostA
sub cpus_in_select_chunk {
    my ($chunk) = @_;
    return $1 if $chunk =~ m/^(\d+)$/;
    if ($chunk =~ m{^(\d+):(.*)$}) {
        my ($cpus, $resc) = ($1, $2);
        return $cpus * get_ncpus($resc);
    }
    return 0; # not a valid chunk
}

# extracts the value of ppn from a string like blah:ppn=N:blah
sub get_ppn {
    my ($resc) = @_;
    for my $res (split ':', $resc) {
        return $1 if $res =~ m /^ppn=(\d+)$/;
    }
    return 1;
}

# extracts the value of ncpus from a string like blah:ncpus=N:blah
sub get_ncpus {
    my ($resc) = @_;
    for my $res (split ':', $resc) {
        return $1 if $res =~ m /^ncpus=(\d+)$/;
    }
    return 1;
}

# gets information about each destination queue behind a
# routing queue and copies it into the routing queue data structure.
# at the moment it only copies data from the first queue
#
# input: $queue name of the current queue
#        $path to pbs binaries
#        $singledqueue that contains the only queue behind the routing one
#        %qstat{} for the current queue
# output: the %dqueue hash containing info about destination queues
#         in %lrms_queue fashion

sub process_dqueues($$%){
    my $qname = shift;
    my $path = shift;
    my (%qstat) = %{$_[0]};
    my $singledqueue;
    my %dqueues;

    # build DQs data structure
    my @dqnames;
    if (defined $qstat{'route_destinations'}) {
        @dqnames=split(",",$qstat{'route_destinations'});
        @dqueues{@dqnames}=undef;
        my (%hoh_qstatfQ) = read_qstat_fQ($path);
        foreach my $dqname ( keys %dqueues ) {
            debug("Processing queues behind routing queue. Current queue is $dqname");
            $dqueues{$dqname}=$hoh_qstatfQ{$dqname};
        }
        # debug($dqueues{'verylong'}{'resources_max.walltime'});
    }
    else {
        error("No route_destinations for routing queue $qname. Please check LRMS configuration.");
    }
    # take the first destination queue behind the RQ, copy its data to the RQ
    # this happens only if the RQ has no data defined on PBS
    # this should solve bug #859
    $singledqueue=shift(@dqnames);
    debug('Just one queue behind routing queue is currently supported: '.$singledqueue);
    my @attributes=(
        'max_running',
        'max_user_run',
        'max_queuable',
        'resources_max.cput',
        'resources_min.cput',
        'resources_default.cput',
        'resources_max.walltime',
        'resources_min.walltime',
        'resources_default.walltime',
        'state_count'
    );
    foreach my $rkey (@attributes) {
        # line to check queues under routing queue values. Undefined values generate crap in logs,
        # so is commented out.
        # debug('with key '.$rkey.' qstat returns '.$qstat{$rkey}.' and the dest. queue has '.$dqueues{$singledqueue}{$rkey} );
        if (!defined $qstat{$rkey}) {${$_[0]}{$rkey}=$dqueues{$singledqueue}{$rkey};};
    }
    return %dqueues;
}

############################################
# Public subs
#############################################

sub cluster_info ($) {

    # Path to LRMS commands
    my ($config) = shift;
    my ($path) = $$config{pbs_bin_path};
    error("pbs_bin_path not defined, cannot continue. Exiting...") unless defined $path;

    # Return data structure %lrms_cluster{$keyword}
    #
    # All values should be defined, empty values "" are ok if field
    # does not apply to particular LRMS.

    my (%lrms_cluster);

    # flavour and version of PBS
    get_pbs_version($config);
    $lrms_cluster{lrms_type} = $lrms_type;
    $lrms_cluster{lrms_glue_type}=lc($lrms_type);
    $lrms_cluster{lrms_version} = $lrms_version;

    # PBS treats cputime limit for parallel/multi-cpu jobs as job-total
    $lrms_cluster{has_total_cputime_limit} = 1;

    # processing the pbsnodes output by using a hash of hashes %hoh_pbsnodes
    my ( %hoh_pbsnodes ) = read_pbsnodes( $path );

    error("The given flavour of PBS $lrms_cluster{lrms_type} is not supported")
        unless grep {$_ eq lc($lrms_cluster{lrms_type})}
            qw(openpbs spbs torque pbspro);

    $lrms_cluster{totalcpus} = 0;
    my ($number_of_running_jobs) = 0;
    $lrms_cluster{cpudistribution} = "";
    my (@cpudist) = 0;
    my %available_nodes = ();

    # loop over all available nodes
    foreach my $node (keys %hoh_pbsnodes) {
        # skip nodes that does not conform dedicated_node_string filter
        if ( exists $$config{dedicated_node_string} &&  $$config{dedicated_node_string} ne "") {
            next unless ( $hoh_pbsnodes{$node}{"properties"} =~
                m/^([^,]+,)*$$config{dedicated_node_string}(,[^,]+)*$/);
        }

        # add node to available_nodes hash
        $available_nodes{$node} = 1;

        # get node state and number of CPUs
        my ($nodestate) = $hoh_pbsnodes{$node}{"state"};

        my $nodecpus;
        if ($hoh_pbsnodes{$node}{'np'}) {
            $nodecpus = $hoh_pbsnodes{$node}{'np'};
        } elsif ($hoh_pbsnodes{$node}{'resources_available.ncpus'}) {
            $nodecpus = $hoh_pbsnodes{$node}{'resources_available.ncpus'};
        }

        next if ($nodestate=~/down/ or $nodestate=~/offline/);

        if ($nodestate=~/(?:,|^)busy/) {
            $lrms_cluster{totalcpus} += $nodecpus;
            $cpudist[$nodecpus] +=1;
            $number_of_running_jobs += $nodecpus;
            next;
        }

        $lrms_cluster{totalcpus} += $nodecpus;
        $cpudist[$nodecpus] += 1;

        if ($hoh_pbsnodes{$node}{"jobs"}){
            $number_of_running_jobs++;
            my ( @comma ) = ($hoh_pbsnodes{$node}{"jobs"}=~ /,/g);
            $number_of_running_jobs += @comma;
        }
    }

    # form LRMS cpudistribution string
    for (my $i=0; $i<=$#cpudist; $i++) {
        next unless ($cpudist[$i]);
        $lrms_cluster{cpudistribution} .= " ".$i."cpu:".$cpudist[$i];
    }

    # read the qstat -n information about all jobs
    # queued cpus, total number of cpus in all jobs
    $lrms_cluster{usedcpus} = 0;
    $lrms_cluster{queuedcpus} = 0;
    $lrms_cluster{queuedjobs} = 0;
    $lrms_cluster{runningjobs} = 0;

    my %qstat_jobs = read_qstat_f($path);

    for my $key (keys %qstat_jobs) {
        # usercpus (running jobs)
        if ( $qstat_jobs{$key}{job_state} =~ /R/) {
            $lrms_cluster{runningjobs}++;
            my @nodes = split_hostlist($qstat_jobs{$key}{exec_host});
            # filter using dedicated_node_string
            foreach my $node ( @nodes ) {
                next unless defined $available_nodes{$node};
                $lrms_cluster{usedcpus}++;
            }
        }
        #
        if ( $qstat_jobs{$key}{job_state} =~ /(W|T|Q)/) {
            $lrms_cluster{queuedjobs}++;
            $lrms_cluster{queuedcpus}+=count_usedcpus($qstat_jobs{$key}{"Resource_List.select"},
            $qstat_jobs{$key}{"Resource_List.nodes"},
            $qstat_jobs{$key}{"Resource_List.ncpus"});
        }
    }

    # Names of all LRMS queues
    @{$lrms_cluster{queue}} = ();

    my ( %hoh_qstat ) = read_qstat_fQ($path);
    for my $qkey (keys %hoh_qstat) {
        push @{$lrms_cluster{queue}}, $qkey;
    }

    return %lrms_cluster;
}

sub queue_info ($$) {

    # Path to LRMS commands
    my ($config) = shift;
    my ($path) = $$config{pbs_bin_path};
    error("pbs_bin_path not defined, cannot continue. Exiting...") unless defined $path;

    # Name of the queue to query
    my ($qname) = shift;

    init_globals($qname);

    # The return data structure is %lrms_queue.
    # In this template it is defined as persistent module data structure,
    # because it is later used by jobs_info(), and we wish to avoid
    # re-construction of it. If it were not needed later, it would be defined
    # only in the scope of this subroutine, as %lrms_cluster previously.

    # Return data structure %lrms_queue{$keyword}
    #
    # All values should be defined, empty values "" are ok if field
    # does not apply to particular LRMS.

    # read the queue information for the queue entry from the qstat

    my (%hoh_qstat) = read_qstat_fQ($path);

    my (%qstat) = %{$hoh_qstat{$qname}};

    # this script contain a solution for a single queue behind the
    # routing one, the routing queue will inherit some of its
    # attributes.

    # this hash contains qstat records for queues - in this case just one
    my %dqueues;
    # this variable contains the single destination queue
    my $singledqueue;
    if ($qstat{queue_type} =~ /Route/) {
        %dqueues = process_dqueues($qname,$path,\%qstat);
        $singledqueue = ( keys %dqueues )[0];
    } else {
        undef %dqueues;
        undef $singledqueue;
    }

    # publish queue limits parameters
    # general limits (publish as is)
    my (%keywords);
    my (%keywords_all) = ( 'max_running' => 'maxrunning',
                           'max_user_run' => 'maxuserrun',
                           'max_queuable' => 'maxqueuable' );

    # TODO: MinSlots, etc.
    my (%keywords_torque) = ( 'resources_max.procct' => 'MaxSlotsPerJob' );

    my (%keywords_pbspro) = ( 'resources_max.ncpus' => 'MaxSlotsPerJob' );

    get_pbs_version($config);
    if ( $lrms_type eq lc "torque" ) {
        %keywords = (%keywords_all, %keywords_torque);
    } elsif ( $lrms_type eq lc "pbspro" ) {
        %keywords = (%keywords_all, %keywords_pbspro);
    } else {
        %keywords = %keywords_all;
    }

    foreach my $k (keys %keywords) {
        if (defined $qstat{$k} ) {
            $lrms_queue{$keywords{$k}} = $qstat{$k};
        } else {
            $lrms_queue{$keywords{$k}} = "";
        }
    }

    # queue time limits (convert to minutes)
    %keywords = ( 'resources_max.cput' => 'maxcputime',
                  'resources_min.cput' => 'mincputime',
                  'resources_default.cput' => 'defaultcput',
                  'resources_max.walltime' => 'maxwalltime',
                  'resources_min.walltime' => 'minwalltime',
                  'resources_default.walltime' => 'defaultwallt' );

    foreach my $k (keys %keywords) {
        if ( defined $qstat{$k} ) {
            $lrms_queue{$keywords{$k}} = (&count_time($qstat{$k})+($k eq 'resources_min.cput'?1:0));
        } else {
            $lrms_queue{$keywords{$k}} = "";
        }
    }

    # determine the queue status from the LRMS
    # Used to be set to 'active' if the queue can accept jobs
    # Now lists the number of available processors, "0" if no free
    # cpus. Negative number signals some error state of PBS
    # (reserved for future use).


    $lrms_queue{status} = -1;
    $lrms_queue{running} = 0;
    $lrms_queue{queued} = 0;
    $lrms_queue{totalcpus} = 0;
    if ( ($qstat{"enabled"} =~ /True/) and ($qstat{"started"} =~ /True/)) {
        # refresh routing queue records, in case something changed on the
        # destination queues
        if ($qstat{queue_type} =~ /Route/) {
            debug("CPUs calculation pass. Queues are scanned a second time. Current queue is: $qstat{queue_type}");
            %dqueues = process_dqueues($qname,$path,\%qstat);
            # this variable contains the single destination queue
            $singledqueue = ( keys %dqueues )[0];
        } else {
            undef %dqueues;
            undef $singledqueue;
        }

        # qstat does not return number of cpus, use pbsnodes instead.
        my ($torque_freecpus,$torque_totalcpus,$nodes_totalcpus,$nodes_freecpus)=(0,0,0,0);
        # processing the pbsnodes output by using a hash of hashes %hoh_pbsnodes
        my ( %hoh_pbsnodes ) = read_pbsnodes( $path );
        foreach my $node (keys %hoh_pbsnodes) {
            my $cpus;
            next if $hoh_pbsnodes{$node}{'state'} =~ m/offline/;
            next if $hoh_pbsnodes{$node}{'state'} =~ m/down/;

            if ($hoh_pbsnodes{$node}{'np'}) {
                $cpus = $hoh_pbsnodes{$node}{'np'};
            } elsif ($hoh_pbsnodes{$node}{'resources_available.ncpus'}) {
                $cpus = $hoh_pbsnodes{$node}{'resources_available.ncpus'};
            }
            $nodes_totalcpus+=$cpus;
            if ($hoh_pbsnodes{$node}{'state'} =~ m/free/){
                $nodes_freecpus+=$cpus;
            }

            # If pbsnodes have properties assigned to them
            # check if queuename or dedicated_node_string matches.
            # $singledqueue check has been added for routing queue support,
            # also the destination queue is checked to calculate totalcpus
            # also adds correct behaviour for queue_node_string
            if (
              ( ! defined $hoh_pbsnodes{$node}{'properties'}
              ) || (
                (
                    defined $qname &&
                    $hoh_pbsnodes{$node}{'properties'} =~ m/^([^,]+,)*$qname(,[^,]+)*$/
                ) || (
                    defined $$config{pbs_dedicated_node_string} &&
                    $hoh_pbsnodes{$node}{'properties'} =~ m/^([^,]+,)*$$config{pbs_dedicated_node_string}(,[^,]+)*$/
                ) || (
                    defined $$config{pbs_queue_node} &&
                    $hoh_pbsnodes{$node}{'properties'} =~ m/^([^,]+,)*$$config{pbs_queue_node}(,[^,]+)*$/
                ) || (
                    defined $singledqueue &&
                    $hoh_pbsnodes{$node}{'properties'} =~ m/^([^,]+,)*$singledqueue(,[^,]+)*$/
                )
              )
            ) {
                $torque_totalcpus+=$cpus;
                if ($hoh_pbsnodes{$node}{'state'} =~ m/free/){
                    $torque_freecpus+=$cpus;
                }
            }
        }

        if ($torque_totalcpus eq 0) {
            warning("Node properties are defined in PBS but nothing match the queue filters. Assigning counters for all nodes.");
            $torque_totalcpus = $nodes_totalcpus;
            $torque_freecpus = $nodes_freecpus;
        }

        $lrms_queue{totalcpus} = $torque_totalcpus;

        debug("Totalcpus for all queues are: $lrms_queue{totalcpus}");

        if(defined $$config{totalcpus}){
            if ($lrms_queue{totalcpus} eq "" or $$config{totalcpus} < $lrms_queue{totalcpus}) {
                $lrms_queue{totalcpus}=$$config{totalcpus};
            }
        }

        $lrms_queue{status} = $torque_freecpus;
        $lrms_queue{status}=0 if $lrms_queue{status} < 0;

        if ( $qstat{state_count} =~ m/.*Running:([0-9]*).*/ ){
            $lrms_queue{running}=$1;
        } else {
            $lrms_queue{running}=0;
        }

        # calculate running in case of a routing queue
        if ( $qstat{queue_type} =~ /Route/ ) {
           debug($dqueues{$singledqueue}{state_count});
           if ( $dqueues{$singledqueue}{state_count} =~ m/.*Running:([0-9]*).*/ ) {
                $lrms_queue{running}=$1;
           }
        }

        # the above gets the number of nodes not the number of cores in use. If multi core jobs are running, "running" will be underestimated.
        # Instead use totalcpus - freecpus (This might overrepresent running. because pbsnodes count whole nodes in use.)
        # CUS (2015-02-09)

        my $runningcores = $torque_totalcpus - $torque_freecpus ;
        $runningcores = 0 if $runningcores < 0;

        $lrms_queue{running} = $runningcores if $runningcores > $lrms_queue{running};

        if ($lrms_queue{totalcpus} eq 0) {
            warning("Can't determine number of cpus for queue $qname");
        }

        if ( $qstat{state_count} =~ m/.*Queued:([0-9]*).*/ ){
            $lrms_queue{queued}=$1;
        } else {
            $lrms_queue{queued}=0;
        }

        # fallback for defult values that are required for normal operation
        $lrms_queue{MaxSlotsPerJob} = $lrms_queue{totalcpus} if $lrms_queue{MaxSlotsPerJob} eq "";

        # calculate queued in case of a routing queue
        # queued jobs is the sum of jobs queued in the routing queue
        # plus jobs in the destination queue
        if ( $qstat{queue_type} =~ /Route/ ) {
           debug($dqueues{$singledqueue}{state_count});
           if ( $dqueues{$singledqueue}{state_count} =~ m/.*Queued:([0-9]*).*/ ) {
                $lrms_queue{queued}=$lrms_queue{queued}+$1;
           }
        }

    }

    return %lrms_queue;
}


sub jobs_info ($$@) {

    # Path to LRMS commands
    my ($config) = shift;
    my ($path) = $$config{pbs_bin_path};
    error("pbs_bin_path not defined, cannot continue. Exiting...") unless defined $path;

    # Name of the queue to query
    my ($qname) = shift;

    init_globals($qname);

    # LRMS job IDs from Grid Manager (jobs with "INLRMS" GM status)
    my ($jids) = shift;

    # Return data structure %lrms_jobs{$lrms_local_job_id}{$keyword}
    #
    # All values should be defined, empty values "" are ok if field
    # does not apply to particular LRMS.

    my (%lrms_jobs);

    # Fill %lrms_jobs here (da implementation)

    # rank is treated separately as it does not have an entry in
    # qstat output, comment because it is an array, and mem
    # because "kB" needs to be stripped from the value
    my (%skeywords) = ('job_state' => 'status');

    my (%tkeywords) = ( 'resources_used.walltime' => 'walltime',
                        'resources_used.cput' => 'cputime',
                        'Resource_List.walltime' => 'reqwalltime',
                        'Resource_List.cputime' => 'reqcputime');

    my (%nkeywords) = ( 'Resource_List.select' => 1,
                        'Resource_List.nodes' => 1,
                        'Resource_List.ncpus' => 1);

    my ($alljids) = join ' ', @{$jids};
    my ($rank) = 0;
    my %job_owner;

    my $handle_attr = sub {
        my ($jid, $k, $v) = @_;

        if ( defined $skeywords{$k} ) {
            $lrms_jobs{$jid}{$skeywords{$k}} = $v;
            if($k eq "job_state") {
                if( $v eq "U" ) {
                    $lrms_jobs{$jid}{status} = "S";
                } elsif ( $v eq "C" ) {
                    $lrms_jobs{$jid}{status} = ""; # No status means job has completed
                } elsif ( $v ne "R" and $v ne "Q" and $v ne "S" and $v ne "E" ) {
                    $lrms_jobs{$jid}{status} = "O";
                }
            }
        } elsif ( defined $tkeywords{$k} ) {
            $lrms_jobs{$jid}{$tkeywords{$k}} = &count_time($v);
        } elsif ( defined $nkeywords{$k} ) {
            $lrms_jobs{$jid}{$k} = $v;
        } elsif ( $k eq 'exec_host' ) {
            my @nodes = split_hostlist($v);
            $lrms_jobs{$jid}{nodes} = \@nodes;
            #$lrms_jobs{$jid}{cpus} = scalar @nodes;
        } elsif ( $k eq 'comment' ) {
            $lrms_jobs{$jid}{comment} = []
                unless $lrms_jobs{$jid}{comment};
            push @{$lrms_jobs{$jid}{comment}}, "LRMS: $v";
        } elsif ($k eq 'resources_used.vmem') {
            $v =~ s/(\d+).*/$1/;
            $lrms_jobs{$jid}{mem} = $v;
        }

        if ( $k eq 'Job_Owner' ) {
            $v =~ /(\S+)@/;
            $job_owner{$jid} = $1;
        }
        if ( $k eq 'job_state' ) {
            if ($v eq 'R') {
                $lrms_jobs{$jid}{rank} = "";
            } elsif ($v eq 'C') {
                $lrms_jobs{$jid}{rank} = "";
            } else {
                $rank++;
                $lrms_jobs{$jid}{rank} = $rank;
                $jid=~/^(\d+).+/;
            }
            if ($v eq 'R' or 'E'){
                ++$user_jobs_running{$job_owner{$jid}};
            }
            if ($v eq 'Q'){
                ++$user_jobs_queued{$job_owner{$jid}};
            }
        }
    };

    my ( %hoh_qstatf ) = read_qstat_f($path);
    foreach my $pbsjid (keys %hoh_qstatf) {
        # only jobids known by A-REX are processed
        my $jid = undef;
        foreach my $j (@$jids) {
             if ( $pbsjid =~ /^$j$/ ) {
                 $jid = $j;
                 last;
             }
        }
        next unless defined $jid;
        # handle qstat attributes of the jobs
        foreach my $k (keys %{$hoh_qstatf{$jid}} ) {
            my $v = $hoh_qstatf{$jid}{$k};
            &$handle_attr($jid, $k, $v);
        }
        # count cpus for this jobs
        set_cpucount($lrms_jobs{$jid});
    }

    my (@scalarkeywords) = ('status', 'rank', 'mem', 'walltime', 'cputime',
                            'reqwalltime', 'reqcputime');

    foreach my $jid ( @$jids ) {
        foreach my $k ( @scalarkeywords ) {
            if ( ! defined $lrms_jobs{$jid}{$k} ) {
                $lrms_jobs{$jid}{$k} = "";
            }
        }
        $lrms_jobs{$jid}{comment} = [] unless $lrms_jobs{$jid}{comment};
        $lrms_jobs{$jid}{nodes} = [] unless $lrms_jobs{$jid}{nodes};
    }

    return %lrms_jobs;
}


sub users_info($$@) {

    # Path to LRMS commands
    my ($config) = shift;
    my ($path) = $$config{pbs_bin_path};
    error("pbs_bin_path not defined, cannot continue. Exiting...") unless defined $path;

    # Name of the queue to query
    my ($qname) = shift;

    init_globals($qname);

    # Unix user names mapped to grid users
    my ($accts) = shift;

    # Return data structure %lrms_users{$unix_local_username}{$keyword}
    #
    # All values should be defined, empty values "" are ok if field
    # does not apply to particular LRMS.

    my (%lrms_users);

    # Check that users have access to the queue
    my ( %hoh_qstatfQ ) = read_qstat_fQ( $path );

    my $acl_user_enable = 0;
    my @acl_users;
    # added for routing queue support
    my @dqueues;
    my $singledqueue;
    my $isrouting;

    foreach my $k (keys %{$hoh_qstatfQ{$qname}}) {
        my $v = $hoh_qstatfQ{$qname}{$k};
        if ( $k eq "acl_user_enable" && $v eq "True") {
            $acl_user_enable = 1;
        }
        if ( $k eq "acl_users" ) {
            unless ( $v eq 'False' ) {
               # This condition is kept here in case the reason
               # for it being there in the first place was that some
               # version or flavour of PBS really has False as an alternative
               # to usernames to indicate the absence of user access control
               # A Corrallary: Dont name your users 'False' ...
                push @acl_users, split ',', $v;
            }
       }
       # added to support routing queues
       if ( !$acl_user_enable ) {
            if ($k eq "route_destinations" ) {
                @dqueues=split (',',$v);
                $singledqueue=shift(@dqueues);
                warning('Routing queue did not have acl information. Local user acl taken from destination queue: '.$singledqueue);
                $isrouting = 1;
            }
       }
    }

    # if the acl_user_enable is not defined in the RQ,
    # it could be defined in the destination queues.
    # we proceed same way as before but on the first
    # destination queue to propagate the info to the routing one
    if ($isrouting){
        debug("Getting acl from destination queue $singledqueue");
        # Check that users have access to the queue
        $acl_user_enable = 0;
        foreach my $k (keys %{$hoh_qstatfQ{$singledqueue}}) {
            my $v = $hoh_qstatfQ{$singledqueue}{$k};
            if ( $k eq "acl_user_enable" && $v eq "True") {
                $acl_user_enable = 1;
            }
            if ( $k eq "acl_users" ) {
                unless ( $v eq 'False' ) {
                    push @acl_users, split ',', $v;
                }
            }
       }
       debug(@acl_users);
    }

    # acl_users is only in effect when acl_user_enable is true
    if ($acl_user_enable) {
        foreach my $a ( @{$accts} ) {
            if (  grep { $a eq $_ } @acl_users ) {
              # The acl_users list has to be sent back to the caller.
              # This trick works because the config hash is passed by
              # reference.
              push @{$$config{acl_users}}, $a;
            }
            else {
                warning("Local user $a does not ".
                        "have access in queue $qname.");
            }
        }
    } else {
        delete $$config{acl_users};
    }

    # Uses saved module data structure %lrms_queue, which
    # exists if queue_info is called before
    if ( ! exists $lrms_queue{status} ) {
        %lrms_queue = queue_info( $config, $qname );
    }

    foreach my $u ( @{$accts} ) {
        $user_jobs_running{$u} = 0 unless $user_jobs_running{$u};
        if ($lrms_queue{maxuserrun} and ($lrms_queue{maxuserrun} - $user_jobs_running{$u}) < $lrms_queue{status} ) {
            $lrms_users{$u}{freecpus} = $lrms_queue{maxuserrun} - $user_jobs_running{$u};
        }
        else {
            $lrms_users{$u}{freecpus} = $lrms_queue{status};
        }
        $lrms_users{$u}{queuelength} = "$lrms_queue{queued}";
        if ($lrms_users{$u}{freecpus} < 0) {
            $lrms_users{$u}{freecpus} = 0;
        }
        if ($lrms_queue{maxcputime} and $lrms_users{$u}{freecpus} > 0) {
            $lrms_users{$u}{freecpus} .= ':'.$lrms_queue{maxcputime};
        }
    }
    return %lrms_users;
}


sub nodes_info($) {

    my $config = shift;
    my $path = $config->{pbs_bin_path};

    my %hoh_pbsnodes = read_pbsnodes($path);

    my %nodes;
    for my $host (keys %hoh_pbsnodes) {
        my ($isfree, $isavailable) = (0,0);
        $isfree = 1 if $hoh_pbsnodes{$host}{state} =~ /free/;
        $isavailable = 1 unless $hoh_pbsnodes{$host}{state} =~ /down|offline|unknown/;
        $nodes{$host} = {isfree => $isfree, isavailable => $isavailable};
        my $props = $hoh_pbsnodes{$host}{properties};
        $nodes{$host}{tags} = [ split /,\s*/, $props ] if $props;
        my $np = $hoh_pbsnodes{$host}{np};
        $nodes{$host}{slots} = int $np if $np;
        my $status = $hoh_pbsnodes{$host}{status};
        if ($status) {
            for my $token (split ',', $status) {
                my ($opt, $val) = split '=', $token, 2;
                next unless defined $val;
                if ($opt eq 'totmem') {
                    $nodes{$host}{vmem} = int($1/1024) if $val =~ m/^(\d+)kb/;
                } elsif ($opt eq 'physmem') {
                    $nodes{$host}{pmem} = int($1/1024) if $val =~ m/^(\d+)kb/;
                } elsif ($opt eq 'ncpus') {
                    $nodes{$host}{lcpus} = int $val;
                } elsif ($opt eq 'uname') {
                    my @uname = split ' ', $val;
                    $nodes{$host}{sysname} = $uname[0];
                    $nodes{$host}{release} = $uname[2] if @uname > 2;
                    $nodes{$host}{machine} = $uname[-1] if $uname[-1];
                }
            }
        }
    }
    return %nodes;
}


1;
