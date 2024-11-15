package ARC1ClusterInfo;

# This information collector combines the output of the other information collectors
# and prepares the GLUE2 information model of A-REX.

use Storable;
#use Data::Dumper;
use FileHandle;
use File::Temp;
use POSIX qw(ceil);

use strict;

use Sysinfo qw(cpuinfo processid diskinfo diskspaces);
use LogUtils;
use InfoChecker;

our $log = LogUtils->getLogger(__PACKAGE__);

# the time now in ISO 8061 format
sub timenow {
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = gmtime(time);
    return sprintf "%4d-%02d-%02dT%02d:%02d:%02d%1s", $year+1900, $mon+1, $mday,$hour,$min,$sec,"Z";
}

# converts MDS-style time to ISO 8061 time ( 20081212151903Z --> 2008-12-12T15:19:03Z )
sub mdstoiso {
    return "$1-$2-$3T$4:$5:$6Z" if shift =~ /^(\d\d\d\d)(\d\d)(\d\d)(\d\d)(\d\d)(\d\d(?:\.\d+)?)Z$/;
    return undef;
}

sub glue2bool {
    my $bool = shift;
    return 'undefined' unless defined $bool;
    return $bool ? "true" : "false";
}

# Given the controldir path, the jobid and a suffix
# returns a path to that job in the fragmented controldir.
sub control_path {
    my ($controldir, $jobid, $suffix) = @_;

    my ($a,$b,$c,$d) = unpack("A3A3A3A3", $jobid);
    my $path = "$controldir/jobs/$a/$b/$c/$d/$suffix";

    return $path;
}

sub local_state {
    ## Maps the gm_state added with detailed info of the lrms_state to local-state
    ## Added the local state terminal to gm_state KILLED - this will allow the job to be cleaned when calling arcclean
    #require Data::Dumper; import Data::Dumper qw(Dumper);

    # TODO: probably add $failure_state taken from somewhere
    my ($gm_state,$lrms_state,$failure_state) = @_;
    my $loc_state = {
    'State' => ''
    };
    if ($gm_state eq "ACCEPTED") {
        $loc_state->{State} = [ "accepted" ];
        return $loc_state;
    } elsif ($gm_state eq "PREPARING") {
        $loc_state->{State} = [ "preparing" ];
        return $loc_state;
    } elsif ($gm_state eq "SUBMIT") {
        $loc_state->{State} = [ "submit" ];
        return $loc_state;
    } elsif ($gm_state eq "INLRMS") {
        if (not defined $lrms_state) {
            $loc_state->{State} = [ "inlrms" ];
            return $loc_state;
        } elsif ($lrms_state eq 'Q') {
            $loc_state->{State} = [ "inlrms","q" ];
            return $loc_state;
        } elsif ($lrms_state eq 'R') {
            $loc_state->{State} = [ "inlrms","r" ];
            return $loc_state;
        } elsif ($lrms_state eq 'EXECUTED'
              or $lrms_state eq '') {
            $loc_state->{State} = [ "inlrms","e" ];
            return $loc_state;
        } elsif ($lrms_state eq 'S') {
            $loc_state->{State} = [ "inlrms","s" ];
            return $loc_state;
        } else {
            $loc_state->{State} = [ "running" ];
            return $loc_state;
        }
    } elsif ($gm_state eq "FINISHING") {
            $loc_state->{State} = [ "finishing" ];
            return $loc_state;
    } elsif ($gm_state eq "CANCELING") {
            $loc_state->{State} = [ "canceling" ];
            return $loc_state;
    } elsif ($gm_state eq "KILLED") {
	$loc_state->{State} = [ "killed" ];
        return $loc_state;
    } elsif ($gm_state eq "FAILED") {
	$loc_state->{State} = [ "failed" ];
	if (! defined($failure_state)) {
	    return $loc_state;
	} else {
            ## Not sure how these will be rendered - to-fix/to-check
	    if ($failure_state eq "ACCEPTED")  {                
		$loc_state->{State} = [ "accepted","validation-failure" ];
		return $loc_state;
	    } elsif ($failure_state eq "PREPARING") {
		$loc_state->{State} = [ "preparing","cancel","failure" ];
		return $loc_state;
	    } elsif ($failure_state eq "SUBMIT") {
		$loc_state->{State} = [ "submit","cancel","failure" ];
		return $loc_state;
	    } elsif ($failure_state eq "INLRMS") {
		if ( $lrms_state eq "R" ) {
		    $loc_state->{State} = [ "inlrms","cancel","app-failure" ];
		    return $loc_state;
		} else {
		    $loc_state->{State} = [ "inlrms","cancel","processing-failure" ];
		    return $loc_state;
		}                
	    } elsif ($failure_state eq "FINISHING") {
		$loc_state->{State} = [ "finishing","cancel","failure" ];
		return $loc_state;
	    } elsif ($failure_state eq "FINISHED") {
		$loc_state->{State} = [ "finished","failure"];
		return $loc_state;
	    } elsif ($failure_state eq "DELETED") {
		$loc_state->{State} = [ "deleted","failure" ];
		return $loc_state;
	    } elsif ($failure_state eq "CANCELING") {
		$loc_state->{State} = ["canceling","failure"];
		return $loc_state;
	    } else {
		return $loc_state;
	    }
	}
    } elsif ($gm_state eq "FINISHED") {
	$loc_state->{State} = [ "finished" ];
	return $loc_state;
    } elsif ($gm_state eq "DELETED") {
	$loc_state->{State} = [ "deleted" ];
	return $loc_state;
    } elsif ($gm_state) { # this is the "pending" case
        $loc_state->{State} = ["hold"];
        return $loc_state;
    } else {
        return $loc_state;
    }
}


# TODO: verify: this sub evaluates also failure states and changes
# rest attributes accordingly.
sub rest_state {
    my ($gm_state,$lrms_state,$failure_state) = @_;
    my $state = [ "None" ];

    my $is_pending = 0;
    if ($gm_state =~ /^PENDING:/) {
      $is_pending = 1;
      $gm_state = substr $gm_state, 8
    }

    #   REST State   A-REX State
    # * ACCEPTING    ACCEPTED
    # * ACCEPTED     PENDING:ACCEPTED
    # * PREPARING    PREPARING
    # * PREPARED     PENDING:PREPARING
    # * SUBMITTING   SUBMIT
    # * QUEUING      INLRMS + LRMS queued
    # * RUNNING      INLRMS + LRMS running
    # * HELD         INLRMS + LRMS on hold
    # * EXITINGLRMS  INLRMS + LRMS finished
    # * OTHER        INLRMS + LRMS other
    # * EXECUTED     PENDING:INLRMS
    # * FINISHING    FINISHING
    # * KILLING      CANCELLING
    # KILLING      PREPARING + DTR cancel | FINISHING + DTR cancel
    # * FINISHED     FINISHED + no errors & no cancel
    # * FAILED       FINISHED + errors
    # * KILLED       FINISHED + cancel
    # * WIPED        DELETED

    if ($gm_state eq "ACCEPTED") {
        if ( $is_pending ) {
            $state = [ "ACCEPTED" ];
        } else {
            $state = [ "ACCEPTING" ];
        }
    } elsif ($gm_state eq "PREPARING") {
        if ( $is_pending ) {
            $state = [ "PREPARED" ];
        } else {
            # KILLING
            $state = [ "PREPARING" ];
        }
    } elsif ($gm_state eq "SUBMIT") {
        $state = [ "SUBMITTING" ];
    } elsif ($gm_state eq "INLRMS") {
        if ( $is_pending ) {
            $state = [ "EXECUTED" ];
        } else {
            if (not defined $lrms_state) {
                $state = [ "OTHER" ];
            } elsif ($lrms_state eq 'Q') {
                $state = [ "QUEUING" ];
            } elsif ($lrms_state eq 'R') {
                $state = [ "RUNNING" ];
            } elsif ($lrms_state eq 'EXECUTED'
                  or $lrms_state eq '') {
                $state = [ "EXITINGLRMS" ];
            } elsif ($lrms_state eq 'S') {
                $state = [ "HELD" ];
            } else {
                $state = [ "OTHER" ];
            }
        }
    } elsif ($gm_state eq "FINISHING") {
        # KILLING
        $state = [ "FINISHING" ];
    } elsif ($gm_state eq "CANCELING") {
        $state = [ "KILLING" ];
    } elsif ($gm_state eq "KILLED") {
        $state = [ "KILLED" ];
    } elsif ($gm_state eq "FAILED") {
        $state = [ "FAILED" ];
    } elsif ($gm_state eq "FINISHED") {
        $state = [ "FINISHED" ];
    } elsif ($gm_state eq "DELETED") {
        $state = [ "WIPED" ];
    } elsif ($gm_state) { # this is the "pending" case
        $state = [ "None" ];
    } else {
        # No idea
    }

    return $state;
}

# input is an array with (state, lrms_state, failure_state)
sub glueState {
    my @ng_status = @_;

    return [ "UNDEFINEDVALUE" ] unless $ng_status[0];
    my $status = [ "nordugrid:".join(':',@ng_status) ];
    my $local_state = local_state(@ng_status);
    push @$status, "file:".@{$local_state->{State}}[0] if $local_state->{State};#try to fix so I have the full state here
    my $rest_state = rest_state(@ng_status); 
    push @$status, "arcrest:".$rest_state->[0] if $rest_state;

    return $status;
}

sub getGMStatus {
    my ($controldir, $ID) = @_;
    foreach my $gmjob_status ("$controldir/accepting/$ID.status", "$controldir/processing/$ID.status", "$controldir/finished/$ID.status") {
        unless (open (GMJOB_STATUS, "<$gmjob_status")) {
            next;
        } else {
            my ($first_line) = <GMJOB_STATUS>;
            close GMJOB_STATUS;
            unless ($first_line) {
                $log->verbose("Job $ID: cannot get status from file $gmjob_status : Skipping job");
                next;
            }
            chomp $first_line;
            return $first_line;
        }
    }
    return undef;
}

# Helper function that assists the GLUE2 XML renderer handle the 'splitjobs' option
#   $config       - the config hash
#   $jobid        - job id from GM
#   $gmjob        - a job hash ref as returned by GMJobsInfo
#   $xmlGenerator - a function ref that returns a string (the job's GLUE2 XML description)
# Returns undef on error, 0 if the XML file was already up to date, 1 if it was written
sub jobXmlFileWriter {
    my ($config, $jobid, $gmjob, $xmlGenerator) = @_;
    $log->debug("XML writer for $jobid.");
    # If this is defined, then it's a job managed by local A-REX.
    my $gmuser = $gmjob->{gmuser};
    # This below is to avoid any processing for remote grid managers, a removed feature
    return 0 unless defined $gmuser;
    my $controldir = $config->{arex}{controldir};
    $log->debug("XML writer in $controldir.");
    my $xml_file = control_path($controldir, $jobid, "xml");
    $log->debug("XML writer to $xml_file.");

    # Here goes simple optimisation - do not write new
    # XML if status has not changed while in "slow" states
    my $xml_time = (stat($xml_file))[9];
    my $status_time = $gmjob->{statusmodified};
    return 0 if defined $xml_time
                and defined $status_time
                and $status_time < $xml_time
                and $gmjob->{status} =~ /ACCEPTED|FINISHED|FAILED|KILLED|DELETED/;

    my $xmlstring = &$xmlGenerator();
    return undef unless defined $xmlstring;

    # tempfile croaks on error
    my $jobdir = control_path($controldir, $jobid, "");
    my ($fh, $tmpnam) = File::Temp::tempfile("xml.XXXXXXX", DIR => $jobdir);
    $log->debug("XML $tmpnam to $xml_file.");
    binmode $fh, ':encoding(utf8)';
    print $fh $xmlstring and close $fh
        or $log->warning("Error writing to temporary file $tmpnam: $!")
        and close $fh
        and unlink $tmpnam
        and return undef;
    rename $tmpnam, $xml_file
        or $log->warning("Error moving $tmpnam to $xml_file: $!")
        and unlink $tmpnam
        and return undef;


    # Avoid .xml files created after job is deleted
    # Check if status file exists
    if(not defined getGMStatus($controldir,$jobid)) {
      unlink $xml_file;
      return undef;
    }

    # Set timestamp to the time when the status file was read in.
    # This is because the status file might have been updated by the time the
    # XML file gets written. This step ensures that the XML will be updated on
    # the next run of the infoprovider.
    my $status_read = $gmjob->{statusread};
    return undef unless defined $status_read;
    utime(time(), $status_read, $xml_file)
        or $log->warning("Couldn't touch $xml_file: $!")
        and return undef;

    # *.xml file was updated
    return 1;
};

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

# union of two arrays using hashes. Returns an array.
sub union {
	my (@a, @b) = @_;
	my %union;
	foreach (@a) {$union{$_} = 1;}
	foreach (@b) {$union{$_} = 1;}
	return keys %union;
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
        $log->verbose("Not implemented: NodeSelection: Command");
    }

    delete $nscfg{Regex};
    delete $nscfg{Tag};
    delete $nscfg{Command};
    $log->verbose("Unknown NodeSelection option: @{[keys %nscfg]}") if %nscfg;

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
    my ($config, $nodes, $queues) = @_;
    my $infos = {};
    my %nodemap = ();
    my @xenvs = keys %{$config->{xenvs}};
    for my $xenv (@xenvs) {
        my $xecfg = $config->{xenvs}{$xenv};
        my $info = $infos->{$xenv} = {};
        my $nodelist = \@{$queues->{$xenv}{nodes}};
        my $nscfg = $xecfg->{NodeSelection} || { 'Regex' => $nodelist };
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
                $log->verbose("Overlap detected between ExecutionEnvironments $xenvs[$i] and $xenvs[$j]. "
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

# adds a prefix to a set of strings in an array.
# input: the prefix string, an array.
sub addprefix {
   my $prefix = shift @_;
   my @set = @_;
   my @prefixedset = @set;
   @prefixedset = map { $prefix.$_ } @prefixedset;
   return @prefixedset;	
}

# sub to pick a value in order: first value preferred to others
# can have as many parameters as one wants.
sub prioritizedvalues {
   my @values = @_;
   my $numelements = scalar @values;
   
   while (@values) {
      my $current = shift @values;
      return $current if (((defined $current) and ($current ne '')) or ( $numelements == 1));
  }

   # just in case all the above fails, log and return empty string
   $log->debug("No suitable value found in call to prioritizedvalues. Returning undefined");
   return undef;
}


############################################################################
# Combine info from all sources to prepare the final representation
############################################################################

sub collect($) {
    my ($data) = @_;

    # used for testing
#    print Dumper($data);

    my $config = $data->{config};
    my $usermap = $data->{usermap};
    my $host_info = $data->{host_info};
    my $rte_info = $data->{rte_info};
    my $gmjobs_info = $data->{gmjobs_info};
    my $lrms_info = $data->{lrms_info};
    my $nojobs = $data->{nojobs};

    my $creation_time = timenow();
    my $validity_ttl = $config->{infosys}{validity_ttl};
    my $hostname = $config->{hostname} || $host_info->{hostname};
    
    my $wsenabled =  (defined $config->{arex}{ws}) ? 1 : 0;
    my $restenabled = $config->{arex}{ws}{jobs}{enabled};
    
    my $wsendpoint = $config->{arex}{ws}{wsurl};

    my @allxenvs = keys %{$config->{xenvs}};
    my @allshares = keys %{$config->{shares}};
    ## NOTE: this might be moved to ConfigCentral, but Share is a glue only concept...
    # GLUE2 shares differ from the configuration one.
    # the one to one mapping from a share to a queue is too strong.
    # the following datastructure reshuffles queues into proper
    # GLUE2 shares based on advertisedvo
    # This may require rethinking of parsing the configuration...
    my $GLUE2shares = {};
    
    # If advertisedvo is present in arc.conf defined, 
    # generate one additional share for each VO.
    #
    # TODO: refactorize this to apply to cluster and queue VOs
    # with a single subroutine, even better do everything in ConfigCentral.pm if possible
    # what is needed to make it possible? new schema in configcentral for policies?
    #
    ## for each share(queue)
    for my $currentshare (@allshares) { 
       # always add a share with no mapping policy
       my $share_name = $currentshare;
       $GLUE2shares->{$share_name} = Storable::dclone($config->{shares}{$currentshare});
       # Create as many shares as the number of advertisedvo entries
       # in the [queue:queuename] block
       # if there is any VO generate new names
       if (defined $config->{shares}{$currentshare}{AdvertisedVO}) {
            my ($queueadvertvos) = $config->{shares}{$currentshare}{AdvertisedVO};
            for my $queueadvertvo (@{$queueadvertvos}) {
                # generate an additional share with such advertisedVO
                my $share_vo = $currentshare.'_'.$queueadvertvo;
                $GLUE2shares->{$share_vo} = Storable::dclone($config->{shares}{$currentshare});
                # add the queue from configuration as MappingQueue
                $GLUE2shares->{$share_vo}{MappingQueue} = $currentshare;
                # remove VOs from that share, substitute with default VO
                $GLUE2shares->{$share_vo}{AdvertisedVO} = $queueadvertvo;
                # Add supported policies 
                # ARC5 could use XML config elements for this, but now that configuration is gone, so just placing a default here.
                $GLUE2shares->{$share_vo}{MappingPolicies} = { 'BasicMappingPolicy' => ''};
            }
        } else {
       # create as many shares as the advertisedvo in the [infosys/cluster] block
       # iff advertisedvo not defined in queue block
            if (defined $config->{service}{AdvertisedVO}) { 
                my ($clusteradvertvos) = $config->{service}{AdvertisedVO};
                for my $clusteradvertvo (@{$clusteradvertvos}) {
                    # generate an additional share with such advertisedVO
                    my $share_vo = $currentshare.'_'.$clusteradvertvo;
                    $GLUE2shares->{$share_vo} = Storable::dclone($config->{shares}{$currentshare});
                    # add the queue from configuration as MappingQueue
                    $GLUE2shares->{$share_vo}{MappingQueue} = $currentshare;
                    # remove VOs from that share, substitute with default VO
                    $GLUE2shares->{$share_vo}{AdvertisedVO} = $clusteradvertvo; 
                    # ARC5 could use XML config elements for this, but now that configuration is gone, so just placing a default here.
                    $GLUE2shares->{$share_vo}{MappingPolicies} = { 'BasicMappingPolicy' => '' };
                }    
            }
        }
        # remove VO array from the datastructure of the share with the same name of the queue
        delete $GLUE2shares->{$share_name}{AdvertisedVO};
        undef $share_name;
    }
 
    ##replace @allshares with the newly created shares
    #@allshares = keys %{$GLUE2shares};

    my $homogeneous = 1;
    $homogeneous = 0 if @allxenvs > 1;
    $homogeneous = 0 if @allshares > 1 and @allxenvs == 0;
    for my $xeconfig (values %{$config->{xenvs}}) {
        $homogeneous = 0 if defined $xeconfig->{Homogeneous}
                            and not $xeconfig->{Homogeneous};
    }

    my $xeinfos = xeinfos($config, $lrms_info->{nodes}, $lrms_info->{queues});

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
    
    # Override totallcpus if defined in the cluster block
    $totallcpus = $config->{service}{totalcpus} if (defined $config->{service}{totalcpus});
    
    #$log->debug("Cannot determine total number of physical CPUs in all ExecutionEnvironments") unless $totalpcpus;
    $log->debug("Cannot determine total number of logical CPUs in all ExecutionEnvironments") unless $totallcpus;

    # Next, use value returned by LRMS in case the the first try failed.
    # OBS: most LRMSes don't differentiate between Physical and Logical CPUs.
    $totalpcpus ||= $lrms_info->{cluster}{totalcpus};
    $totallcpus ||= $lrms_info->{cluster}{totalcpus};

#    my @advertisedvos = (); 
#    if ($config->{service}{AdvertisedVO}) {
#        @advertisedvos = @{$config->{service}{AdvertisedVO}};
#        # add VO: suffix to each advertised VO
#        @advertisedvos = map { "vo:".$_ } @advertisedvos;
#    }

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
    
    # jobids divided per interface. This datastructure
    # is convenience way to fill jobs per endpoint
    # each endpoint its list of jobids
    my $jobs_by_endpoint = {};

    # corecount per internal AREX state
    my %state_slots;


    # fills most of the above hashes
    for my $jobid (keys %$gmjobs_info) {

        my $job = $gmjobs_info->{$jobid};
        my $gridowner = $gmjobs_info->{$jobid}{subject};
        my $share = $job->{share};
        # take only the first VO for now.
        # TODO: problem. A job gets assigned to the default
        # queue that is not assigned to that VO. How to solve?
        # This can only be solved with a better job<->vo mapping definition.
        # So it boils down to what to do when $job->{vomsvo} is not defined.
        my $vomsvo = $job->{vomsvo} if defined $job->{vomsvo};
        my $sharevomsvo = $share.'_'.$vomsvo if defined $vomsvo;

        my $gmstatus = $job->{status} || '';

        $gmtotalcount{totaljobs}++;
        $gmsharecount{$share}{totaljobs}++;
        # add info for VO dedicated shares
        $gmsharecount{$sharevomsvo}{totaljobs}++ if defined $vomsvo;

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
        $gmsharecount{$sharevomsvo}{$category}++ if defined $vomsvo;

        if ($age < 6) {
            $gmtotalcount{notdeleted}++;
            $gmsharecount{$share}{notdeleted}++;
            $gmsharecount{$sharevomsvo}{notdeleted}++ if defined $vomsvo;

        }
        if ($age < 5) {
            $gmtotalcount{notfinished}++;
            $gmsharecount{$share}{notfinished}++;
            $gmsharecount{$sharevomsvo}{notfinished}++ if defined $vomsvo;
        }
        if ($age < 3) {
            $gmtotalcount{notsubmitted}++;
            $gmsharecount{$share}{notsubmitted}++;
            $gmsharecount{$sharevomsvo}{notsubmitted}++ if defined $vomsvo;
            $requestedslots{$share} += $job->{count} || 1;
            $share_prepping{$share}++;
            if (defined $vomsvo) {
                $requestedslots{$sharevomsvo} += $job->{count} || 1;
                $share_prepping{$sharevomsvo}++;
            }
            # TODO: is this used anywhere?
            $user_prepping{$gridowner}++ if $gridowner;
        }
        if ($age < 2) {
            $pending{$share}++;
            $pending{$sharevomsvo}++ if defined $vomsvo;
            $pendingtotal++;
        }

        # count grid jobs running and queued in LRMS for each share

	
	my $slots = $job->{count} || 1;
	if ($gmstatus eq 'PREPARING'){

	    #how should initialization be done - fix
	    $state_slots{$share}{PREPARING} += $slots;
	}
	elsif ($gmstatus eq 'FINISHING'){
	    #my $slots = $job->{count} || 1;
	    $state_slots{$share}{FINISHING} += $slots;
	}
	elsif ($gmstatus eq 'INLRMS') {
            my $lrmsid = $job->{localid} || 'IDNOTFOUND';
            my $lrmsjob = $lrms_info->{jobs}{$lrmsid};
            #my $slots = $job->{count} || 1;

            if (defined $lrmsjob) {
                if ($lrmsjob->{status} ne 'EXECUTED') {
                    $inlrmsslots{$share}{running} ||= 0; 
                    $inlrmsslots{$share}{suspended} ||= 0;
                    $inlrmsslots{$share}{queued} ||= 0;
		    
		    $state_slots{$share}{"INLRMS:Q"} ||= 0;
		    $state_slots{$share}{"INLRMS:R"} ||= 0;
		    $state_slots{$share}{"INLRMS:O"} ||= 0;
		    $state_slots{$share}{"INLRMS:E"} ||= 0;
		    $state_slots{$share}{"INLRMS:S"} ||= 0;
		    
                    if (defined $vomsvo) {
                        $inlrmsslots{$sharevomsvo}{running} ||= 0; 
                        $inlrmsslots{$sharevomsvo}{suspended} ||= 0;
                        $inlrmsslots{$sharevomsvo}{queued} ||= 0;
                    }
                    if ($lrmsjob->{status} eq 'R') {
                        $inlrmsjobstotal{running}++;
                        $inlrmsjobs{$share}{running}++;
                        $inlrmsslots{$share}{running} += $slots;
			$state_slots{$share}{"INLRMS:R"} += $slots;
                        if (defined $vomsvo) {
                            $inlrmsjobs{$sharevomsvo}{running}++;
                            $inlrmsslots{$sharevomsvo}{running} += $slots;
                        }
                    } elsif ($lrmsjob->{status} eq 'S') {
                        $inlrmsjobstotal{suspended}++;
                        $inlrmsjobs{$share}{suspended}++;
                        $inlrmsslots{$share}{suspended} += $slots;
			$state_slots{$share}{"INLRMS:S"} += $slots;
                        if (defined $vomsvo) {
                            $inlrmsjobs{$sharevomsvo}{suspended}++;
                            $inlrmsslots{$sharevomsvo}{suspended} += $slots;
                        }
                    } elsif($lrmsjob->{status} eq 'Q'){
			$state_slots{$share}{"INLRMS:Q"} += $slots;
		    } elsif($lrmsjob->{status} eq 'E'){
			$state_slots{$share}{"INLRMS:E"} += $slots;
		    } elsif($lrmsjob->{status} eq 'O'){
			$state_slots{$share}{"INLRMS:O"} += $slots;
		    } else {  # Consider other states 'queued' for $inlrms*
                        $inlrmsjobstotal{queued}++;
                        $inlrmsjobs{$share}{queued}++;
                        $inlrmsslots{$share}{queued} += $slots;
                        $requestedslots{$share} += $slots;

                        if (defined $vomsvo) {
                            $inlrmsjobs{$sharevomsvo}{queued}++;
                            $inlrmsslots{$sharevomsvo}{queued} += $slots;
                            $requestedslots{$sharevomsvo} += $slots;
                        }
                    }
                }
            } else {
                $log->warning("Info missing about lrms job $lrmsid");
            }
        }
        
        # fills efficiently %jobs_by_endpoint, defaults to arcrest
        my $jobinterface = $job->{interface} || 'org.nordugrid.arcrest';
        
        $jobs_by_endpoint->{$jobinterface}{$jobid} = {};
	
    }
   
    my $admindomain = $config->{admindomain}{Name};
    my $lrmsname = $config->{lrms}{lrms};

    # Calculate endpoint URLs for A-REX and ARIS.
    # check what is enabled in configuration
    # also calculates static data that can be triggered by endpoints
    # such as known capabilities
    
    my $csvendpointsnum = 0;
    my $csvcapabilities = {};
    my $epscapabilities = {};

    # defaults now set in ConfigCentral
    my $ldaphostport = "ldap://$hostname:$config->{infosys}{ldap}{port}/" if ($config->{infosys}{ldap}{enabled});
    my $ldapngendpoint = '';
    my $ldapglue2endpoint = '';
   
    # data push/pull capabilities.
    # Is it still possible to scan datalib patch to search for .apd to fill these?
    $epscapabilities->{'common'} = [
                    'data.transfer.cepull.ftp',
                    'data.transfer.cepull.http',
                    'data.transfer.cepull.https',
                    'data.transfer.cepull.httpg',
                    'data.transfer.cepull.gridftp',
                    'data.transfer.cepull.srm',
                    'data.transfer.cepush.ftp',
                    'data.transfer.cepush.http',
                    'data.transfer.cepush.https',
                    'data.transfer.cepush.httpg',
                    'data.transfer.cepush.gridftp',
                    'data.transfer.cepush.srm',
                    'data.access.sessiondir.file',
                    'data.access.stageindir.file',
                    'data.access.stageoutdir.file'
                    ];
    
    ## Endpoints initialization.
    # checks for defined paths and enabled features, sets GLUE2 capabilities.
   
    # REST capabilities
    my $resthostport = '';
    if ($restenabled) {
        $resthostport = $config->{arexhostport};
        $csvendpointsnum = $csvendpointsnum + 1;
        $epscapabilities->{'org.nordugrid.arcrest'} = [
                                                'executionmanagement.jobcreation',
                                                'executionmanagement.jobdescription',
                                                'executionmanagement.jobmanagement',
                                                'information.discovery.resource',
                                                'information.discovery.job',
                                                'information.lookup.job',
                                                'security.delegation'
                                                ];
    }

    # for the org.nordugrid.internal submission endpoint (files created directly in the controldir)
    # Currently not advertised, so we will not count it
    #$csvendpointsnum = $csvendpointsnum + 1;
    $epscapabilities->{'org.nordugrid.internal'} = [
                                  'executionmanagement.jobcreation',
                                  'executionmanagement.jobexecution',
                                  'executionmanagement.jobmanagement',
                                  'executionmanagement.jobdescription',
                                  'information.discovery.resource',
                                  'information.discovery.job',
                                  'information.lookup.job',
                                  'security.delegation'
                                  ];  
    
    # ARIS LDAP endpoints
    
    # ldapng
    if ( $config->{infosys}{nordugrid}{enabled} ) {
        $csvendpointsnum++;
        $ldapngendpoint = $ldaphostport."Mds-Vo-Name=local,o=grid";
        $epscapabilities->{'org.nordugrid.ldapng'} = [ 'information.discovery.resource' ];
    }

    # ldapglue2
    if ( $config->{infosys}{glue2}{ldap}{enabled} ) {
        $csvendpointsnum++;
        $ldapglue2endpoint = $ldaphostport."o=glue";
        $epscapabilities->{'org.nordugrid.ldapglue2'} = [ 'information.discovery.resource' ];
    }

    # Calculcate service capabilities as a union, using hash power
    foreach my $key (keys %{$epscapabilities}) {
        foreach my $capability (@{$epscapabilities->{$key}}) {
            $csvcapabilities->{$capability} = '';
            }
    }

    # if all sessiondirs are in drain state, put the endpoints in
    # drain state too
    
    my $servingstate = 'draining';
    my ($sessiondirs) = ($config->{arex}{sessiondir});
    foreach my $sd (@$sessiondirs) {
        my @hasdrain = split(' ',$sd);
        if ($hasdrain[-1] ne 'drain') {
            $servingstate = 'production';
        }
    }

    # TODO: userdomain - maybe use mapping concepts. Not a prio.
    my $userdomain='';

    # Global IDs
    # ARC choices are as follows:
    # 
    my $adID = "urn:ad:$admindomain"; # AdminDomain ID
    my $udID = "urn:ud:$userdomain" ; # UserDomain ID;
    my $csvID = "urn:ogf:ComputingService:$hostname:arex"; # ComputingService ID
    my $cmgrID = "urn:ogf:ComputingManager:$hostname:$lrmsname"; # ComputingManager ID
    
    # Computing Endpoints IDs
    my $ARCRESTcepIDp;
    $ARCRESTcepIDp = "urn:ogf:ComputingEndpoint:$hostname:rest:$wsendpoint" if $restenabled; # ARCRESTComputingEndpoint ID
    my $NGLScepIDp = "urn:ogf:ComputingEndpoint:$hostname:ngls"; # NorduGridLocalSubmissionEndpoint ID
    # the following array is needed to publish in shares. Must be modified
    # if we support share-per-endpoint configurations.
    my @cepIDs = ();
    push(@cepIDs,$ARCRESTcepIDp) if ($restenabled);
    
    my $cactIDp = "urn:caid:$hostname"; # ComputingActivity ID prefix
    my $cshaIDp = "urn:ogf:ComputingShare:$hostname"; # ComputingShare ID prefix
    my $xenvIDp = "urn:ogf:ExecutionEnvironment:$hostname"; # ExecutionEnvironment ID prefix
    my $aenvIDp = "urn:ogf:ApplicationEnvironment:$hostname:rte"; # ApplicationEnvironment ID prefix
    # my $ahIDp = "urn:ogf:ApplicationHandle:$hostname:"; # ApplicationHandle ID prefix
    my $apolIDp = "urn:ogf:AccessPolicy:$hostname"; # AccessPolicy ID prefix
    my $mpolIDp = "urn:ogf:MappingPolicy:$hostname"; # MappingPolicy ID prefix
    my %cactIDs; # ComputingActivity IDs
    my %cshaIDs; # ComputingShare IDs
    my %aenvIDs; # ApplicationEnvironment IDs
    my %xenvIDs; # ExecutionEnvironment IDs
    my $tseID = "urn:ogf:ToStorageElement:$hostname:storageservice"; # ToStorageElement ID prefix
    
    my $ARISepIDp = "urn:ogf:Endpoint:$hostname"; # ARIS Endpoint ID kept for uniqueness

    # Generate ComputingShare IDs
    for my $share (keys %{$GLUE2shares}) {
        $cshaIDs{$share} = "$cshaIDp:$share";
    }

    # Generate ApplicationEnvironment IDs
    my $aecount = 0;
    for my $rte (keys %$rte_info) {
        $aenvIDs{$rte} = "$aenvIDp:$aecount";
        $aecount++;
    }

    # Generate ExecutionEnvironment IDs
    my $envcount = 0;
    $xenvIDs{$_} = "$xenvIDp:execenv".$envcount++ for @allxenvs;

    # generate ComputingActivity IDs
    unless ($nojobs) {
        for my $jobid (keys %$gmjobs_info) {
            my $share = $gmjobs_info->{$jobid}{share};
            my $interface = $gmjobs_info->{$jobid}{'interface'};
            $cactIDs{$share}{$jobid} = "$cactIDp:$interface:$jobid";
        }
    }

    # TODO: in a first attempt, accesspolicies were expected to be in the XML 
    # config. this is not yet the case, moreover it might not be possible to 
    # do that. So the following is commented out for now.
    # unless (@{$config->{accesspolicies}}) {
    #     $log->warning("No AccessPolicy configured");
    # }


    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
    # # # # # # # # # # build information tree  # # # # # # # # # #
    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

    my $callcount = 0;


    ### Authorized VOs: Policy stuff
    
    # calculate union of the advertisedvos in shares - a hash is used as a set
    # and add it to the cluster accepted advertisedvos
    
    # TODO: this code could be generalized and moved to configcentral.
    my @clusteradvertisedvos;
    if ($config->{service}{AdvertisedVO}) { @clusteradvertisedvos = @{$config->{service}{AdvertisedVO}}; }
    my $unionadvertisedvos;
    if (@clusteradvertisedvos) {
        foreach my $vo (@clusteradvertisedvos) {
            $unionadvertisedvos->{$vo}='';
        }
    }
    # add the per-queue advertisedvo if any
    my $shares = Storable::dclone($GLUE2shares);
    for my $share ( keys %$shares ) {
        if ($GLUE2shares->{$share}{AdvertisedVO}) {
            my (@tempvos) = $GLUE2shares->{$share}{AdvertisedVO} if ($GLUE2shares->{$share}{AdvertisedVO});
            foreach my $vo (@tempvos) {
                $unionadvertisedvos->{$vo}='';
            }
        }
    }   
    
    
    my @unionadvertisedvos;
    if ($unionadvertisedvos) {
        @unionadvertisedvos =  keys %$unionadvertisedvos ;
        @unionadvertisedvos = addprefix('vo:',@unionadvertisedvos);
        undef $unionadvertisedvos;
    }
    
    
    # AccessPolicies implementation. Can be called for each endpoint.
    # the basic policy value is taken from the service AdvertisedVO.
    # The logic is similar to the endpoints: first
    # all the policies subroutines are created, then stored in $accesspolicies,
    # then every endpoint passes custom values to the getAccessPolicies sub.
    
    my $accesspolicies = {};
       
    # Basic access policy: union of advertisedvos   
    my $getBasicAccessPolicy = sub {
         my $apol = {};
         my ($epID) = @_;
         $apol->{ID} = "$apolIDp:basic";
         $apol->{CreationTime} = $creation_time;
         $apol->{Validity} = $validity_ttl;
         $apol->{Scheme} = "basic";
         if (@unionadvertisedvos) { $apol->{Rule} = [ @unionadvertisedvos ]; };
         # $apol->{UserDomainID} = $apconf->{UserDomainID};
         $apol->{EndpointID} = $epID;
         return $apol;
    };
    
    $accesspolicies->{BasicAccessPolicy} = $getBasicAccessPolicy if (@unionadvertisedvos);
    
    ## more accesspolicies can go here.
    
    
    ## subroutines structure to return accesspolicies
    
    my $getAccessPolicies = sub {
       return undef unless my ($accesspolicy, $sub) = each %$accesspolicies; 
       my ($epID) = @_;
      return &{$sub}($epID);
     };

    # MappingPolicies implementation. Can be called for each ShareID.
    # the basic policy value is taken from the service AdvertisedVO.
    # The logic is similar to the endpoints: first
    # all the policies subroutines are created, stored in mappingpolicies,
    # then every endpoint passes custom values to the getMappingPolicies sub.
    
    my $mappingpolicies = {};
    
    # Basic mapping policy: it can only contain one vo.     
    
    my $getBasicMappingPolicy = sub {
        my ($shareID, $sharename) = @_;
        my $mpol = {};
        $mpol->{CreationTime} = $creation_time;
        $mpol->{Validity} = $validity_ttl;
        $mpol->{ID} = "$mpolIDp:basic:$GLUE2shares->{$sharename}{AdvertisedVO}";
        $mpol->{Scheme} = "basic";
        $mpol->{Rule} = [ "vo:$GLUE2shares->{$sharename}{AdvertisedVO}" ];
        # $mpol->{UserDomainID} = $apconf->{UserDomainID};
        $mpol->{ShareID} = $shareID;
        return $mpol;
    };
    
    $mappingpolicies->{'BasicMappingPolicy'} = $getBasicMappingPolicy;
    
    ## more accesspolicies can go here.
    
    
    ## subroutines structure to return MappingPolicies
    # MappingPolicies are processed by using the share name and the
    # GLUE2shares datastructure that contains the MappingPolicies applied to this
    # share.
        
    my $getMappingPolicies = sub {
	   my ($shareID, $sharename) = @_;
	   return undef unless my ($policy) = each %{$GLUE2shares->{$sharename}{MappingPolicies}};
       my $sub = $mappingpolicies->{$policy};
       return &{$sub}($shareID, $sharename);
    };

    # TODO: the above policies can be rewritten in an object oriented fashion
    # one single policy object that can be specialized
    # it's just about changing few strings
    # Only makes sense once we have other policies than Basic.

    # function that generates ComputingService data

    my $getComputingService = sub {

        $callcount++;

        my $csv = {};

        $csv->{CreationTime} = $creation_time;
        $csv->{Validity} = $validity_ttl;

        $csv->{ID} = $csvID;
  
        $csv->{Capability} = [keys %$csvcapabilities];
        
        $csv->{Name} = $config->{service}{ClusterName} if $config->{service}{ClusterName}; # scalar
        $csv->{OtherInfo} = $config->{service}{OtherInfo} if $config->{service}{OtherInfo}; # array
        $csv->{Type} = 'org.nordugrid.arex';

        # OBS: Service QualityLevel used to state the purpose of the service. Can be set by sysadmins.
        # One of: development, testing, pre-production, production
        $csv->{QualityLevel} = $config->{service}{QualityLevel};

        $csv->{StatusInfo} =  $config->{service}{StatusInfo} if $config->{service}{StatusInfo}; # array

        my $nshares = keys %{$GLUE2shares};

        $csv->{Complexity} = "endpoint=$csvendpointsnum,share=$nshares,resource=".(scalar @allxenvs);

        $csv->{AllJobs} = $gmtotalcount{totaljobs} || 0;
        # OBS: Finished/failed/deleted jobs are not counted
        $csv->{TotalJobs} = $gmtotalcount{notfinished} || 0;

        $csv->{RunningJobs} = $inlrmsjobstotal{running} || 0;
        $csv->{SuspendedJobs} = $inlrmsjobstotal{suspended} || 0;
        $csv->{WaitingJobs} = $inlrmsjobstotal{queued} || 0;
        $csv->{StagingJobs} = ( $gmtotalcount{preparing} || 0 )
                            + ( $gmtotalcount{finishing} || 0 );
        $csv->{PreLRMSWaitingJobs} = $pendingtotal || 0;

        # ComputingActivity sub. Will try to use as a general approach for each endpoint.
        my $getComputingActivities = sub {

          my ($interface) = @_;
          
          # $log->debug("interface is $interface");
          
          my $joblist = $jobs_by_endpoint->{$interface};

          return undef unless my ($jobid) = each %$joblist;
          
          my $gmjob = $gmjobs_info->{$jobid};

          my $exited = undef; # whether the job has already run; 

          my $cact = {};

          $cact->{CreationTime} = $creation_time;
          $cact->{Validity} = $validity_ttl;

          my $share = $gmjob->{share};

          $cact->{Type} = 'single';
          # TODO: this is currently not universal
          $cact->{ID} = $cactIDs{$share}{$jobid};
          $cact->{IDFromEndpoint} = "urn:idfe:$jobid" if $jobid;
          $cact->{Name} = $gmjob->{jobname} if $gmjob->{jobname};
          # Set job specification language based on description
          if ($gmjob->{description}) {
                if ($gmjob->{description} eq 'adl') { 
                    $cact->{JobDescription} = 'emies:adl';
                } else {
                    $cact->{JobDescription} = 'nordugrid:xrsl';
                }
          } else {
             $cact->{JobDescription} = 'UNDEFINEDVALUE';
          }
          # TODO: understand this below
          $cact->{RestartState} = glueState($gmjob->{failedstate}) if $gmjob->{failedstate};
          $cact->{ExitCode} = $gmjob->{exitcode} if defined $gmjob->{exitcode};
          # TODO: modify scan-jobs to write it separately to .diag. All backends should do this.
          $cact->{ComputingManagerExitCode} = $gmjob->{lrmsexitcode} if $gmjob->{lrmsexitcode};
          $cact->{Error} = [ @{$gmjob->{errors}} ] if $gmjob->{errors};
          # TODO: VO info, like <UserDomain>ATLAS/Prod</UserDomain>; check whether this information is available to A-REX
          $cact->{Owner} = $gmjob->{subject} if $gmjob->{subject};
          $cact->{LocalOwner} = $gmjob->{localowner} if $gmjob->{localowner};
          # OBS: Times are in seconds.
          $cact->{RequestedTotalWallTime} = $gmjob->{reqwalltime} * ($gmjob->{count} || 1) if defined $gmjob->{reqwalltime};
          $cact->{RequestedTotalCPUTime} = $gmjob->{reqcputime} if defined $gmjob->{reqcputime};
          # OBS: Should include name and version. Exact format not specified
          $cact->{RequestedApplicationEnvironment} = $gmjob->{runtimeenvironments} if $gmjob->{runtimeenvironments};
          $cact->{RequestedSlots} = $gmjob->{count} || 1;
          $cact->{StdIn} = $gmjob->{stdin} if $gmjob->{stdin};
          $cact->{StdOut} = $gmjob->{stdout} if $gmjob->{stdout};
          $cact->{StdErr} = $gmjob->{stderr} if $gmjob->{stderr};
          $cact->{LogDir} = $gmjob->{gmlog} if $gmjob->{gmlog};
          $cact->{ExecutionNode} = $gmjob->{nodenames} if $gmjob->{nodenames};
          $cact->{Queue} = $gmjob->{queue} if $gmjob->{queue};
          # Times for finished jobs
          $cact->{UsedTotalWallTime} = $gmjob->{WallTime} * ($gmjob->{count} || 1) if defined $gmjob->{WallTime};
          $cact->{UsedTotalCPUTime} = $gmjob->{CpuTime} if defined $gmjob->{CpuTime};
          $cact->{UsedMainMemory} = ceil($gmjob->{UsedMem}/1024) if defined $gmjob->{UsedMem};
          # Submission Time to AREX
          $cact->{SubmissionTime} = mdstoiso($gmjob->{starttime}) if $gmjob->{starttime};
          # TODO: change gm to save LRMSSubmissionTime - maybe take from accounting?
          #$cact->{ComputingManagerSubmissionTime} = 'NotImplemented';
          # Start time in LRMS
          $cact->{StartTime} = mdstoiso($gmjob->{LRMSStartTime}) if $gmjob->{LRMSStartTime};
          $cact->{ComputingManagerEndTime} = mdstoiso($gmjob->{LRMSEndTime}) if $gmjob->{LRMSEndTime};
          $cact->{EndTime} = mdstoiso($gmjob->{completiontime}) if $gmjob->{completiontime};
          $cact->{WorkingAreaEraseTime} = mdstoiso($gmjob->{cleanuptime}) if $gmjob->{cleanuptime};
          $cact->{ProxyExpirationTime} = mdstoiso($gmjob->{delegexpiretime}) if $gmjob->{delegexpiretime};
          if ($gmjob->{clientname}) {
              # OBS: address of client as seen by the server is used.
              my $dnschars = '-.A-Za-z0-9';  # RFC 1034,1035
              my ($external_address, $port, $clienthost) = $gmjob->{clientname} =~ /^([$dnschars]+)(?::(\d+))?(?:;(.+))?$/;
              $cact->{SubmissionHost} = $external_address if $external_address;
          }
          # TODO: this in not fetched by GMJobsInfo at all. .local does not contain name.
          #$cact->{SubmissionClientName} = $gmjob->{clientsoftware} if $gmjob->{clientsoftware};

          # Added for the client to know what was the original interface the job was submitted
          $cact->{OtherInfo} = ["SubmittedVia=$interface"];

          # Computing Activity Associations

          # TODO: add link, is this even possible? needs share where the job is running.
          #$cact->{ExecutionEnvironmentID} = ;
          $cact->{ActivityID} = $gmjob->{activityid} if $gmjob->{activityid};
          $cact->{ComputingShareID} = $cshaIDs{$share} || 'UNDEFINEDVALUE';

          if ( $gmjob->{status} eq "INLRMS" ) {
              my $lrmsid = $gmjob->{localid};
              if (not $lrmsid) {
              $log->warning("No local id for job $jobid") if $callcount == 1;
              next;
              }
              $cact->{LocalIDFromManager} = $lrmsid;

              my $lrmsjob = $lrms_info->{jobs}{$lrmsid};
              if (not $lrmsjob) {
              $log->warning("No local job for $jobid") if $callcount == 1;
              next;
              }
              $cact->{State} = $gmjob->{failedstate} ? glueState("INLRMS", $lrmsjob->{status}, $gmjob->{failedstate}) : glueState("INLRMS", $lrmsjob->{status});
              $cact->{WaitingPosition} = $lrmsjob->{rank} if defined $lrmsjob->{rank};
              $cact->{ExecutionNode} = $lrmsjob->{nodes} if $lrmsjob->{nodes};
              unshift @{$cact->{OtherMessages}}, $_ for @{$lrmsjob->{comment}};

              # Times for running jobs
              $cact->{UsedTotalWallTime} = $lrmsjob->{walltime} * ($gmjob->{count} || 1) if defined $lrmsjob->{walltime};
              $cact->{UsedTotalCPUTime} = $lrmsjob->{cputime} if defined $lrmsjob->{cputime};
              $cact->{UsedMainMemory} = ceil($lrmsjob->{mem}/1024) if defined $lrmsjob->{mem};
          } else {
              $cact->{State} = $gmjob->{failedstate} ? glueState($gmjob->{status},'',$gmjob->{failedstate}) : glueState($gmjob->{status});
          }
            
          # TODO: UserDomain association, how to calculate it?
          
          
          $cact->{jobXmlFileWriter} = sub { jobXmlFileWriter($config, $jobid, $gmjob, @_) };

          return $cact;
        };

        # Computing Endpoints ########
          
        # Here comes a list of endpoints we support.
        # TODO: verify: REST - org.nordugrid.arcrest
        # LDAP endpoints one per schema

        # these will contain only endpoints with URLs defined
        # Simple endpoints will be rendered as computingEndpoints
        # as GLUE2 does not admin simple Endpoints within a ComputingService.
        my $arexceps = {}; # arex computing endpoints.
        # my $arexeps = {}; # arex plain endpoints (i.e. former aris endpoints)

        # A-REX ComputingEndpoints
        
        # TODO: review that the content is consistent with GLUE2
        # ARCREST 

        my $getARCRESTComputingEndpoint = sub {

            # don't publish if no endpoint URL
            return undef unless $restenabled;

            my $cep = {};

            $cep->{CreationTime} = $creation_time;
            $cep->{Validity} = $validity_ttl;

            $cep->{ID} = "$ARCRESTcepIDp";

            $cep->{Name} = "ARC REST";

            # OBS: ideally HED should be asked for the URL
            $cep->{URL} = $wsendpoint;
            $cep->{Capability} = $epscapabilities->{'org.nordugrid.arcrest'};
            $cep->{Technology} = 'rest';
            $cep->{InterfaceName} = 'org.nordugrid.arcrest';
            # REST interface versions that we currently support:
            #$cep->{InterfaceVersion} = [ '1.1', '1.0'  ];
            # Due to a bug in LDAP only the first entry will be shown or the Endpoint will not be published.
            # I could change the LDAP code, but better to be consistent between the two renderings instead.
            # The LDAP issue is related to the GLUE2 schema (SingleValue instead of MultiValue) and cannot be corrected without hassle.
            # DO NOT CHANGE THIS BELOW or the REST endpoint will not be published in LDAP.
            $cep->{InterfaceVersion} = [ '1.1' ];
            #$cep->{SupportedProfile} = [ "http://www.ws-i.org/Profiles/BasicProfile-1.0.html",  # WS-I 1.0
            #            "http://schemas.ogf.org/hpcp/2007/01/bp"               # HPC-BP
            #              ];
            $cep->{Semantics} = [ "https://www.nordugrid.org/arc/arc7/tech/rest/rest.html" ];
            $cep->{Implementor} = "NorduGrid";
            $cep->{ImplementationName} = "nordugrid-arc";
            $cep->{ImplementationVersion} = $config->{arcversion};

            $cep->{QualityLevel} = "production";

            my %healthissues;

            if ($config->{x509_host_cert}) {
                if (     $host_info->{hostcert_expired}) {
                    push @{$healthissues{critical}}, "Host credentials expired";
                } elsif ($host_info->{issuerca_expired}) {
                    push @{$healthissues{critical}}, "Host CA credentials expired";
                } elsif (not $host_info->{hostcert_enddate}) {
                    push @{$healthissues{critical}}, "Host credentials missing";
	        } elsif (not $host_info->{issuerca_enddate}) {
                    push @{$healthissues{warning}}, "Host CA credentials missing";
                } else {
                    if (     $host_info->{hostcert_enddate} - time < 48*3600) {
                        push @{$healthissues{warning}}, "Host credentials will expire soon";
                    } elsif ($host_info->{issuerca_enddate} - time < 48*3600) {
                        push @{$healthissues{warning}}, "Host CA credentials will expire soon";
                    }
                }
            }

            # check health status by using port probe in hostinfo
            my $arexport = $config->{arex}{port};
            if (defined $host_info->{ports}{arched}{$arexport} and @{$host_info->{ports}{arched}{$arexport}}[0] ne 'ok') {
                push @{$healthissues{@{$host_info->{ports}{arched}{$arexport}}[0]}} , @{$host_info->{ports}{arched}{$arexport}}[1];
            }

            if (%healthissues) {
            my @infos;
            for my $level (qw(critical warning other unknown)) {
                next unless $healthissues{$level};
                $cep->{HealthState} ||= $level;
                push @infos, @{$healthissues{$level}};
            }
            $cep->{HealthStateInfo} = join "; ", @infos;
            } else {
            $cep->{HealthState} = 'ok';
            }

            $cep->{ServingState} = $servingstate;

            # TODO: StartTime: get it from hed or from Sysinfo.pm processes

            $cep->{IssuerCA} = $host_info->{issuerca}; # scalar
            $cep->{TrustedCA} = $host_info->{trustedcas}; # array

            # TODO: Downtime, is this necessary, and how should it work?

            $cep->{Staging} =  'staginginout';
            $cep->{JobDescription} = [ 'nordugrid:xrsl', 'emies:adl' ];

            $cep->{TotalJobs} = $gmtotalcount{notfinished} || 0;

            $cep->{RunningJobs} = $inlrmsjobstotal{running} || 0;
            $cep->{SuspendedJobs} = $inlrmsjobstotal{suspended} || 0;
            $cep->{WaitingJobs} = $inlrmsjobstotal{queued} || 0;

            $cep->{StagingJobs} = ( $gmtotalcount{preparing} || 0 )
                    + ( $gmtotalcount{finishing} || 0 );

            $cep->{PreLRMSWaitingJobs} = $pendingtotal || 0;

            $cep->{AccessPolicies} = sub { &{$getAccessPolicies}($cep->{ID}) };
            
            # No OtherInfo at the moment, but should be an array
            #$cep->{OtherInfo} = [] # array

            # ComputingActivities
            if ($nojobs) {
              $cep->{ComputingActivities} = undef;
            } else {
              # this complicated thing here creates a specialized getComputingActivities
              # version of sub with a builtin parameter!
              $cep->{ComputingActivities} = sub { &{$getComputingActivities}('org.nordugrid.arcrest'); };
            }

            # Associations

            $cep->{ComputingShareID} = [ values %cshaIDs ];
            $cep->{ComputingServiceID} = $csvID;

            return $cep;
        };

        # don't publish if no arex/ws/jobs configured
        $arexceps->{ARCRESTComputingEndpoint} = $getARCRESTComputingEndpoint if ($restenabled);

        #
        ## NorduGrid local submission
        #
        my $getNorduGridLocalSubmissionEndpoint = sub {

            # don't publish if no endpoint URL
            #return undef unless $emiesenabled;
            # To-decide: should really the local submission plugin be present in info.xml? It is not useful for the outside world.  
            my $cep = {};

            # Collected information not to be published
            $cep->{NOPUBLISH} = 1;
            $cep->{CreationTime} = $creation_time;
            $cep->{Validity} = $validity_ttl;

            $cep->{ID} = "$NGLScepIDp";

            $cep->{Name} = "ARC CE Local Submission";

            # OBS: ideally HED should be asked for the URL
            $cep->{URL} = $wsendpoint;
            $cep->{Technology} = 'direct';
            $cep->{InterfaceName} = 'org.nordugrid.internal';
            $cep->{InterfaceVersion} = [ '1.0' ];
            $cep->{Capability} = [ @{$epscapabilities->{'org.nordugrid.internal'}}, @{$epscapabilities->{'common'}} ]; 	    
            $cep->{Implementor} = "NorduGrid";
            $cep->{ImplementationName} = "nordugrid-arc";
            $cep->{ImplementationVersion} = $config->{arcversion};

            $cep->{QualityLevel} = "testing";

                       
            my %healthissues;
            # Host certificate not required for INTERNAL submission interface.
            if ( $host_info->{gm_alive} ne 'all' ) {
            if ($host_info->{gm_alive} eq 'some') {
                push @{$healthissues{warning}}, 'One or more grid managers are down';
            } else {
                push @{$healthissues{critical}},
                  $config->{remotegmdirs} ? 'All grid managers are down'
                              : 'Grid manager is down';
            }
            }

            if (%healthissues) {
            my @infos;
            for my $level (qw(critical warning other)) {
                next unless $healthissues{$level};
                $cep->{HealthState} ||= $level;
                push @infos, @{$healthissues{$level}};
            }
            $cep->{HealthStateInfo} = join "; ", @infos;
            } else {
            $cep->{HealthState} = 'ok';
            }

            $cep->{ServingState} = $servingstate;

            # StartTime: get it from hed

            $cep->{IssuerCA} = $host_info->{issuerca}; # scalar
            $cep->{TrustedCA} = $host_info->{trustedcas}; # array

            # TODO: Downtime, is this necessary, and how should it work?

            $cep->{Staging} =  'staginginout';
            $cep->{JobDescription} = [ 'nordugrid:xrsl', 'emies:adl' ];

            $cep->{TotalJobs} = $gmtotalcount{notfinished} || 0;

            $cep->{RunningJobs} = $inlrmsjobstotal{running} || 0;
            $cep->{SuspendedJobs} = $inlrmsjobstotal{suspended} || 0;
            $cep->{WaitingJobs} = $inlrmsjobstotal{queued} || 0;

            $cep->{StagingJobs} = ( $gmtotalcount{preparing} || 0 )
                    + ( $gmtotalcount{finishing} || 0 );

            $cep->{PreLRMSWaitingJobs} = $pendingtotal || 0;

            $cep->{AccessPolicies} = sub { &{$getAccessPolicies}($cep->{ID}) };
            

            # ComputingActivities
            if ($nojobs) {
              $cep->{ComputingActivities} = undef;
            } else {
              # this complicated thing here creates a specialized getComputingActivities
              # version of sub with a builtin parameter!
              #TODO: change interfacename for jobs?
              $cep->{ComputingActivities} = sub { &{$getComputingActivities}('org.nordugrid.internal'); };
            }

            # Associations

            $cep->{ComputingShareID} = [ values %cshaIDs ];
            $cep->{ComputingServiceID} = $csvID;

            return $cep;
        };

        $arexceps->{NorduGridLocalSubmissionEndpoint} = $getNorduGridLocalSubmissionEndpoint;

        ### ARIS endpoints are now part of the A-REX service. 
        # TODO: verify: change ComputingService code in printers to scan for Endpoints - this might be no longer relevant - check live

        my $getArisLdapNGEndpoint = sub {

            my $ep = {};

            $ep->{CreationTime} = $creation_time;
            $ep->{Validity} = $validity_ttl;

            $ep->{Name} = "ARC CE ARIS LDAP NorduGrid Schema Local Information System";

            $ep->{URL} = $ldapngendpoint;
            $ep->{ID} = "$ARISepIDp:ldapng:$config->{infosys}{ldap}{port}";
            $ep->{Capability} = $epscapabilities->{'org.nordugrid.ldapng'};
            $ep->{Technology} = 'ldap';
            $ep->{InterfaceName} = 'org.nordugrid.ldapng';
            $ep->{InterfaceVersion} = [ '1.0' ];
            # Wrong type, should be URI
            #$ep->{SupportedProfile} = [ "http://www.ws-i.org/Profiles/BasicProfile-1.0.html",  # WS-I 1.0
            #            "http://schemas.ogf.org/hpcp/2007/01/bp"               # HPC-BP
            #              ];
            $ep->{Semantics} = [ "http://www.nordugrid.org/documents/arc_infosys.pdf" ];
            $ep->{Implementor} = "NorduGrid";
            $ep->{ImplementationName} = "nordugrid-arc";
            $ep->{ImplementationVersion} = $config->{arcversion};

            $ep->{QualityLevel} = "production";

            my %healthissues;

            # check health status by using port probe in hostinfo
            my $ldapport = $config->{infosys}{ldap}{port} if defined $config->{infosys}{ldap}{port};
            if (defined $host_info->{ports}{slapd}{$ldapport} and @{$host_info->{ports}{slapd}{$ldapport}}[0] ne 'ok') {
                push @{$healthissues{@{$host_info->{ports}{slapd}{$ldapport}}[0]}} , @{$host_info->{ports}{slapd}{$ldapport}}[1];
            }

            if (%healthissues) {
            my @infos;
            for my $level (qw(critical warning other)) {
                next unless $healthissues{$level};
                $ep->{HealthState} ||= $level;
                push @infos, @{$healthissues{$level}};
            }
            $ep->{HealthStateInfo} = join "; ", @infos;
            } else {
            $ep->{HealthState} = 'ok';
            }

            $ep->{ServingState} = 'production';

            # TODO: StartTime: get it from hed?

            # TODO: Downtime, is this necessary, and how should it work?

            # AccessPolicies
            $ep->{AccessPolicies} = sub { &{$getAccessPolicies}($ep->{ID}) };
            
            $ep->{OtherInfo} = $host_info->{EMIversion} if ($host_info->{EMIversion}); # array
                   
            # Associations

            $ep->{ComputingServiceID} = $csvID;

            return $ep;
        };
        
        $arexceps->{LDAPNGEndpoint} = $getArisLdapNGEndpoint if $ldapngendpoint ne '';

        my $getArisLdapGlue2Endpoint = sub {

            my $ep = {};

            $ep->{CreationTime} = $creation_time;
            $ep->{Validity} = $validity_ttl;

            $ep->{Name} = "ARC CE ARIS LDAP GLUE2 Schema Local Information System";

            $ep->{URL} = $ldapglue2endpoint;
            $ep->{ID} = "$ARISepIDp:ldapglue2:$config->{infosys}{ldap}{port}";
            $ep->{Capability} = $epscapabilities->{'org.nordugrid.ldapglue2'};
            $ep->{Technology} = 'ldap';
            $ep->{InterfaceName} = 'org.nordugrid.ldapglue2';
            $ep->{InterfaceVersion} = [ '1.0' ];
            # Wrong type, should be URI
            #$ep->{SupportedProfile} = [ "http://www.ws-i.org/Profiles/BasicProfile-1.0.html",  # WS-I 1.0
            #            "http://schemas.ogf.org/hpcp/2007/01/bp"               # HPC-BP
            #              ];
            $ep->{Semantics} = [ "http://www.nordugrid.org/documents/arc_infosys.pdf" ];
            $ep->{Implementor} = "NorduGrid";
            $ep->{ImplementationName} = "nordugrid-arc";
            $ep->{ImplementationVersion} = $config->{arcversion};

            $ep->{QualityLevel} = "production";

            # How to calculate health for this interface?
            my %healthissues;

            # check health status by using port probe in hostinfo
            my $ldapport = $config->{infosys}{ldap}{port} if defined $config->{infosys}{ldap}{port};
            if (defined $host_info->{ports}{slapd}{$ldapport} and @{$host_info->{ports}{slapd}{$ldapport}}[0] ne 'ok') {
                push @{$healthissues{@{$host_info->{ports}{slapd}{$ldapport}}[0]}}, @{$host_info->{ports}{slapd}{$ldapport}}[1];
            }

            if (%healthissues) {
            my @infos;
            for my $level (qw(critical warning other)) {
                next unless $healthissues{$level};
                $ep->{HealthState} ||= $level;
                push @infos, @{$healthissues{$level}};
            }
            $ep->{HealthStateInfo} = join "; ", @infos;
            } else {
            $ep->{HealthState} = 'ok';
            }

            $ep->{ServingState} = 'production';

            # TODO: StartTime: get it from hed?

            # TODO: Downtime, is this necessary, and how should it work?

            # AccessPolicies
            $ep->{AccessPolicies} = sub { &{$getAccessPolicies}($ep->{ID}) };
            
            $ep->{OtherInfo} = $host_info->{EMIversion} if ($host_info->{EMIversion}); # array
                   
            # Associations

            $ep->{ComputingServiceID} = $csvID;

            return $ep;
        };
        
        $arexceps->{LDAPGLUE2Endpoint} = $getArisLdapGlue2Endpoint if $ldapglue2endpoint ne '';

        
        # Collect endpoints in the datastructure

        # return ComputingEndpoints in sequence
        my $getComputingEndpoints = sub {
          return undef unless my ($cep, $sub) = each %$arexceps;
          return &$sub;
        };

        $csv->{ComputingEndpoints} = $getComputingEndpoints;
        

        # ComputingShares: multiple shares can share the same LRMS queue

        my @shares = keys %{$GLUE2shares};

        my $getComputingShares = sub {

        return undef unless my ($share, $dummy) = each %{$GLUE2shares};

        # Prepare flattened config hash for this share.
        my $sconfig = { %{$config->{service}}, %{$GLUE2shares->{$share}} };

        # List of all shares submitting to the current queue, including the current share.
        my $qname = $sconfig->{MappingQueue} || $share;
        
        # get lrms stats from the actual queues, not share names as they might not match
        my $qinfo = $lrms_info->{queues}{$qname};

        # siblings for the main queue are just itself
        if ($qname ne $share) {
            my $siblings = $sconfig->{siblingshares} = [];
            # Do NOT use 'keys %{$GLUE2shares}' here because it would
            # reset the iterator of 'each' and cause this function to
            # always return the same result
            for my $sn (@shares) {
               my $s = $GLUE2shares->{$sn};
               my $qn = $s->{MappingQueue} || $sn;
               # main queue should not be among the siblings
               push @$siblings, $sn if (($qn eq $qname) && ($sn ne $qname));
            }
        } else {
            my $siblings = $sconfig->{siblingshares} = [$qname];
        }
        
        my $csha = {};

        $csha->{CreationTime} = $creation_time;
        $csha->{Validity} = $validity_ttl;

        $csha->{ID} = $cshaIDs{$share};

        $csha->{Name} = $share;
        $csha->{Description} = $sconfig->{Description} if $sconfig->{Description};
        $csha->{MappingQueue} = $qname if $qname;

        # use limits from LRMS
        $csha->{MaxCPUTime} = prioritizedvalues($sconfig->{maxcputime},$qinfo->{maxcputime});
        # TODO: implement in backends - has this been done?
        $csha->{MaxTotalCPUTime} = $qinfo->{maxtotalcputime} if defined $qinfo->{maxtotalcputime};
        $csha->{MinCPUTime} = prioritizedvalues($sconfig->{mincputime},$qinfo->{mincputime});
        $csha->{DefaultCPUTime} = $qinfo->{defaultcput} if defined $qinfo->{defaultcput};
        $csha->{MaxWallTime} =  prioritizedvalues($sconfig->{maxwalltime},$qinfo->{maxwalltime});
        # TODO: MaxMultiSlotWallTime replaces MaxTotalWallTime, but has different meaning. Check that it's used correctly
        #$csha->{MaxMultiSlotWallTime} = $qinfo->{maxwalltime} if defined $qinfo->{maxwalltime};
        $csha->{MinWallTime} =  prioritizedvalues($sconfig->{minwalltime},$qinfo->{minwalltime});
        $csha->{DefaultWallTime} = $qinfo->{defaultwallt} if defined $qinfo->{defaultwallt};

        my ($maxtotal, $maxlrms) = split ' ', ($config->{maxjobs} || '');
        $maxtotal = undef if defined $maxtotal and $maxtotal eq '-1';
        $maxlrms = undef if defined $maxlrms and $maxlrms eq '-1';

        # MaxWaitingJobs: use the maxjobs config option
        # OBS: An upper limit is not really enforced by A-REX.
        # OBS: Currently A-REX only cares about totals, not per share limits!
        $csha->{MaxTotalJobs} = $maxtotal if defined $maxtotal;

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

        $csha->{MaxRunningJobs} = $maxrunning if defined $maxrunning;
        $csha->{MaxWaitingJobs} = $maxwaiting if defined $maxwaiting;

        # MaxPreLRMSWaitingJobs: use GM's maxjobs option
        # OBS: Currently A-REX only cares about totals, not per share limits!
        # OBS: this formula is actually an upper limit on the sum of pre + post
        #      lrms jobs. A-REX does not have separate limit for pre lrms jobs
        $csha->{MaxPreLRMSWaitingJobs} = $maxtotal - $maxlrms
            if defined $maxtotal and defined $maxlrms;

        $csha->{MaxUserRunningJobs} = $qinfo->{maxuserrun}
            if defined $qinfo->{maxuserrun};

        # TODO: eventually new return value from LRMS infocollector
        # not published if not in arc.conf or returned by infocollectors
        if ($sconfig->{MaxSlotsPerJob} || $qinfo->{MaxSlotsPerJob}) {
            $csha->{MaxSlotsPerJob} = $sconfig->{MaxSlotsPerJob} || $qinfo->{MaxSlotsPerJob};
        }

	
	# Slots per share and state
	my @slot_entries;
	foreach my $state (keys %{$state_slots{$share}}) {
	    my $value = $state_slots{$share}{$state};
	    if($value){
		push @slot_entries, "$state\=$value";
	    }
	}
	if(@slot_entries){
	    $csha->{OtherInfo} = \@slot_entries;
	}
	

        # MaxStageInStreams, MaxStageOutStreams
        # OBS: A-REX does not have separate limits for up and downloads.
        # OBS: A-REX only cares about totals, not per share limits!
        my ($maxloaders, $maxemergency, $maxthreads) = split ' ', ($config->{maxload} || '');
        $maxloaders = undef if defined $maxloaders and $maxloaders eq '-1';
        $maxthreads = undef if defined $maxthreads and $maxthreads eq '-1';
        if ($maxloaders) {
            # default is 5 (see MAX_DOWNLOADS defined in a-rex/grid-manager/loaders/downloader.cpp)
            $maxthreads = 5 unless defined $maxthreads;
            $csha->{MaxStageInStreams}  = $maxloaders * $maxthreads;
            $csha->{MaxStageOutStreams} = $maxloaders * $maxthreads;
        }

        # TODO: new return value schedpolicy from LRMS infocollector.
        my $schedpolicy = $lrms_info->{schedpolicy} || undef;
        if ($sconfig->{SchedulingPolicy} and not $schedpolicy) {
            $schedpolicy = 'fifo' if lc($sconfig->{SchedulingPolicy}) eq 'fifo';
        }
        $csha->{SchedulingPolicy} = $schedpolicy if $schedpolicy;


        # GuaranteedVirtualMemory -- all nodes must be able to provide this
        # much memory per job. Some nodes might be able to afford more per job
        # (MaxVirtualMemory)
        # TODO: implement check at job accept time in a-rex
        # TODO: implement in LRMS plugin maxvmem and maxrss.
        $csha->{MaxVirtualMemory} = $sconfig->{MaxVirtualMemory} if $sconfig->{MaxVirtualMemory};
        # MaxMainMemory -- usage not being tracked by most LRMSs

        # OBS: new config option (space measured in GB !?)
        # OBS: Disk usage of jobs is not being enforced.
        # This limit should correspond with the max local-scratch disk space on
        # clusters using local disks to run jobs.
        # TODO: implement check at job accept time in a-rex
        # TODO: see if any lrms can support this. Implement check in job wrapper
        $csha->{MaxDiskSpace} = $sconfig->{DiskSpace} if $sconfig->{DiskSpace};

        # DefaultStorageService:

        # OBS: Should be ExtendedBoolean_t (one of 'true', 'false', 'undefined')
        $csha->{Preemption} = glue2bool($qinfo->{Preemption}) if defined $qinfo->{Preemption};

        # ServingState: closed and queuing are not yet supported
        # OBS: this serving state should come from LRMS.
        $csha->{ServingState} = 'production';


	# Slots per share and state
	my @slot_entries;
	foreach my $state (keys %{$state_slots{$share}}) {
	    my $value = $state_slots{$share}{$state};
	    push @slot_entries, "$state\=$value";
	}
	$csha->{OtherInfo} = join(',',@slot_entries);

	
        # We can't guess which local job belongs to a certain VO, hence
        # we set LocalRunning/Waiting/Suspended to zero for shares related to
        # a VO. 
        # The global share that represents the queue has also jobs not
        # managed by the ARC CE as it was in previous versions of ARC
        my $localrunning = ($qname eq $share) ? $qinfo->{running} : 0;
        my $localqueued = ($qname eq $share) ? $qinfo->{queued} : 0;
        my $localsuspended = ($qname eq $share) ? $qinfo->{suspended}||0 : 0;

        # TODO: [negative] This should avoid taking as local jobs
        # also those submitted without any VO
        # local jobs are per queue and not per share.
        $localrunning -= $inlrmsjobs{$qname}{running} || 0;
        if ( $localrunning < 0 ) {
             $localrunning = 0;
        }
        $localqueued -= $inlrmsjobs{$qname}{queued} || 0;
        if ( $localqueued < 0 ) {
             $localqueued = 0;
        }
        $localsuspended -= $inlrmsjobs{$qname}{suspended} || 0;
        if ( $localsuspended < 0 ) {
             $localsuspended = 0;
        }

        # OBS: Finished/failed/deleted jobs are not counted
        my $totaljobs = $gmsharecount{$share}{notfinished} || 0;
        $totaljobs += $localrunning + $localqueued + $localsuspended;
        $csha->{TotalJobs} = $totaljobs;

        $csha->{RunningJobs} = $localrunning + ( $inlrmsjobs{$share}{running} || 0 );
        $csha->{WaitingJobs} = $localqueued + ( $inlrmsjobs{$share}{queued} || 0 );
        $csha->{SuspendedJobs} = $localsuspended + ( $inlrmsjobs{$share}{suspended} || 0 );

        # TODO: backends to count suspended jobs
        
        # fix localrunning when displaying the values if negative
        if ( $localrunning < 0 ) {
             $localrunning = 0;
        }
        $csha->{LocalRunningJobs} = $localrunning;
        $csha->{LocalWaitingJobs} = $localqueued;
        $csha->{LocalSuspendedJobs} = $localsuspended;

        $csha->{StagingJobs} = ( $gmsharecount{$share}{preparing} || 0 )
                      + ( $gmsharecount{$share}{finishing} || 0 );

        $csha->{PreLRMSWaitingJobs} = $gmsharecount{$share}{notsubmitted} || 0;

        # TODO: investigate if it's possible to get these estimates from maui/torque
        $csha->{EstimatedAverageWaitingTime} = $qinfo->{averagewaitingtime} if defined $qinfo->{averagewaitingtime};
        $csha->{EstimatedWorstWaitingTime} = $qinfo->{worstwaitingtime} if defined $qinfo->{worstwaitingtime};

        # TODO: implement $qinfo->{freeslots} in LRMS plugins

        my $freeslots = 0;
        if (defined $qinfo->{freeslots}) {
            $freeslots = $qinfo->{freeslots};
        } else {
            # TODO: to be removed after patch testing. Uncomment to check values
            # $log->debug("share name: $share, qname: $qname, totalcpus is $qinfo->{totalcpus}, running is $qinfo->{running}, ".Dumper($qinfo));
            # TODO: still problems with this one, can be negative! Cpus are not enough. Cores must be counted, or logical cpus
            
            # in order, override with config values for queue or cluster or lrms module
            my $queuetotalcpus = $config->{shares}{$qname}{totalcpus} if (defined $config->{shares}{$qname}{totalcpus});
            $queuetotalcpus ||= (defined $config->{service}{totalcpus}) ? $config->{service}{totalcpus} : $qinfo->{totalcpus};
            $freeslots = $queuetotalcpus - $qinfo->{running};
        }

        # This should not be needed, but the above case may trigger it
        $freeslots = 0 if $freeslots < 0;

        # Local users have individual restrictions
        # FreeSlots: find the maximum freecpus of any local user mapped in this
        # share and use that as an upper limit for $freeslots
        # FreeSlotsWithDuration: for each duration, find the maximum freecpus
        # of any local user mapped in this share
        # TODO: is this the correct way to do it?
        # TODO: currently shows negative numbers, check why
        # TODO: [negative] If more slots than the available are overbooked the number is negative
        # for example fork with parallel multicore, so duration should be set to 0
        # OBS: this should be a string. extract the numeric part(s) and compare. Prevent type conversion.
        my @durations;
        # TODO: check the contents of this var
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

        # This should be 0 if the queue is full, check the zeroing above?
        $csha->{FreeSlots} = $freeslots;
        my $freeslotswithduration = join(" ", @durations);
        # fallback to same freeslots if @durations is empty
        if ( $freeslotswithduration eq "") {
            $freeslotswithduration = $freeslots;
        }
        $csha->{FreeSlotsWithDuration} = $freeslotswithduration;

        $csha->{UsedSlots} = $inlrmsslots{$share}{running};
        $csha->{RequestedSlots} = $requestedslots{$share} || 0;

        # TODO: detect reservationpolicy in the lrms
        $csha->{ReservationPolicy} = $qinfo->{reservationpolicy} if $qinfo->{reservationpolicy};

        # Florido's Mapping Policies 
        
        $csha->{MappingPolicies} = sub { &{$getMappingPolicies}($csha->{ID},$csha->{Name})};
 
        # Tag: skip it for now

        # Associations

        my $xenvs = $sconfig->{ExecutionEnvironmentName} || [];
        push @{$csha->{ExecutionEnvironmentID}}, $xenvIDs{$_} for @$xenvs;

        ## check this association below. Which endpoint?
        $csha->{ComputingEndpointID} = \@cepIDs;
        $csha->{ServiceID} = $csvID;
        $csha->{ComputingServiceID} = $csvID;

        return $csha;
        };

        $csv->{ComputingShares} = $getComputingShares;


        # ComputingManager

        my $getComputingManager = sub {

            my $cmgr = {};

            $cmgr->{CreationTime} = $creation_time;
            $cmgr->{Validity} = $validity_ttl;

            $cmgr->{ID} = $cmgrID;
            my $cluster_info = $lrms_info->{cluster}; # array

            # Name not needed

            $cmgr->{ProductName} = $cluster_info->{lrms_glue_type} || lc $cluster_info->{lrms_type};
            $cmgr->{ProductVersion} = $cluster_info->{lrms_version};
            # $cmgr->{Reservation} = "undefined";
            $cmgr->{BulkSubmission} = "false";

            #$cmgr->{TotalPhysicalCPUs} = $totalpcpus if $totalpcpus;
            $cmgr->{TotalLogicalCPUs} = $totallcpus if $totallcpus;

            # OBS: Assuming 1 slot per CPU
            # TODO: slots should be cores?
            $cmgr->{TotalSlots} = (defined $config->{service}{totalcpus}) ? $config->{service}{totalcpus} : $cluster_info->{totalcpus};


            # This number can be more than totalslots in case more
            # than the published cores can be used -- happens with fork
            my @queuenames = keys %{$lrms_info->{queues}};
            my $gridrunningslots = 0; 
            for my $qname (@queuenames) {
               $gridrunningslots += $inlrmsslots{$qname}{running} if defined $inlrmsslots{$qname}{running};
            }
            my $localrunningslots = $cluster_info->{usedcpus} - $gridrunningslots;
            $cmgr->{SlotsUsedByLocalJobs} = ($localrunningslots < 0) ? 0 : $localrunningslots;
            $cmgr->{SlotsUsedByGridJobs} = $gridrunningslots;

            $cmgr->{Homogeneous} = $homogeneous ? "true" : "false";

            # NetworkInfo of all ExecutionEnvironments
            my %netinfo = ();
            for my $xeconfig (values %{$config->{xenvs}}) {
                $netinfo{$xeconfig->{NetworkInfo}} = 1 if $xeconfig->{NetworkInfo};
            }
            $cmgr->{NetworkInfo} = [ keys %netinfo ] if %netinfo;

            # TODO: this could also be cross-checked with info from ExecEnvs
            my $cpuistribution = $cluster_info->{cpudistribution} || '';
            $cpuistribution =~ s/cpu:/:/g;
            $cmgr->{LogicalCPUDistribution} = $cpuistribution if $cpuistribution;

            if (defined $host_info->{session_total}) {
                my $sharedsession = "true";
                $sharedsession = "false" if lc($config->{arex}{shared_filesystem}) eq "no"
                                         or lc($config->{arex}{shared_filesystem}) eq "false";
                $cmgr->{WorkingAreaShared} = $sharedsession;
                $cmgr->{WorkingAreaGuaranteed} = "false";

                my $gigstotal = ceil($host_info->{session_total} / 1024);
                my $gigsfree = ceil($host_info->{session_free} / 1024);
                $cmgr->{WorkingAreaTotal} = $gigstotal;
                $cmgr->{WorkingAreaFree} = $gigsfree;

                # OBS: There is no special area for MPI jobs, no need to advertize anything
                #$cmgr->{WorkingAreaMPIShared} = $sharedsession;
                #$cmgr->{WorkingAreaMPITotal} = $gigstotal;
                #$cmgr->{WorkingAreaMPIFree} = $gigsfree;
                #$cmgr->{WorkingAreaMPILifeTime} = $sessionlifetime;
            }
            
            my ($sessionlifetime) = (split ' ', $config->{arex}{defaultttl});
            $sessionlifetime ||= 7*24*60*60;
            $cmgr->{WorkingAreaLifeTime} = $sessionlifetime;
            
            if (defined $host_info->{cache_total}) {
                my $gigstotal = ceil($host_info->{cache_total} / 1024);
                my $gigsfree = ceil($host_info->{cache_free} / 1024);
                $cmgr->{CacheTotal} = $gigstotal;
                $cmgr->{CacheFree} = $gigsfree;
            }

            if ($config->{service}{Benchmark}) {
                my @bconfs = @{$config->{service}{Benchmark}};
                $cmgr->{Benchmarks} = sub {
                    return undef unless @bconfs;
                    my ($type, $value) = split " ", shift @bconfs;
                    my $bench = {};
                    $bench->{Type} = $type;
                    $bench->{Value} = $value;
                    $bench->{ID} = "urn:ogf:Benchmark:$hostname:$lrmsname:$type";
                    return $bench;
                };
            }

            # Not publishing absolute paths
            #$cmgr->{TmpDir};
            #$cmgr->{ScratchDir};
            #$cmgr->{ApplicationDir};

            # ExecutionEnvironments

            my $getExecutionEnvironments = sub {

                return undef unless my ($xenv, $dummy) = each %{$config->{xenvs}};

                my $xeinfo = $xeinfos->{$xenv};

                # Prepare flattened config hash for this xenv.
                my $xeconfig = { %{$config->{service}}, %{$config->{xenvs}{$xenv}} };

                my $execenv = {};

                my $execenvName = $1 if ( $xenvIDs{$xenv} =~ /(?:.*)\:(.*)$/ );
                                
                # $execenv->{Name} = $xenv;
                $execenv->{Name} = $execenvName;
                $execenv->{CreationTime} = $creation_time;
                $execenv->{Validity} = $validity_ttl;

                $execenv->{ID} = $xenvIDs{$xenv};

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

                $execenv->{Platform} = $machine ? $machine : 'UNDEFINEDVALUE'; # placeholder value
                $execenv->{TotalInstances} = $xeinfo->{ntotal} if defined $xeinfo->{ntotal};
                $execenv->{UsedInstances} = $xeinfo->{nbusy} if defined $xeinfo->{nbusy};
                $execenv->{UnavailableInstances} = $xeinfo->{nunavailable} if defined $xeinfo->{nunavailable};
                $execenv->{VirtualMachine} = glue2bool($xeconfig->{VirtualMachine}) if defined $xeconfig->{VirtualMachine};

                $execenv->{PhysicalCPUs} = $xeinfo->{pcpus} if $xeinfo->{pcpus};
                $execenv->{LogicalCPUs} = $xeinfo->{lcpus} if $xeinfo->{lcpus};
                if ($xeinfo->{pcpus} and $xeinfo->{lcpus}) {
                    my $cpum = ($xeinfo->{pcpus} > 1) ? 'multicpu' : 'singlecpu';
                    my $corem = ($xeinfo->{lcpus} > $xeinfo->{pcpus}) ? 'multicore' : 'singlecore';
                    $execenv->{CPUMultiplicity} = "$cpum-$corem";
                }
                $execenv->{CPUVendor} = $xeconfig->{CPUVendor} if $xeconfig->{CPUVendor};
                $execenv->{CPUModel} = $xeconfig->{CPUModel} if $xeconfig->{CPUModel};
                $execenv->{CPUVersion} = $xeconfig->{CPUVersion} if $xeconfig->{CPUVersion};
                $execenv->{CPUClockSpeed} = $xeconfig->{CPUClockSpeed} if $xeconfig->{CPUClockSpeed};
                $execenv->{CPUTimeScalingFactor} = $xeconfig->{CPUTimeScalingFactor} if $xeconfig->{CPUTimeScalingFactor};
                $execenv->{WallTimeScalingFactor} = $xeconfig->{WallTimeScalingFactor} if $xeconfig->{WallTimeScalingFactor};
                $execenv->{MainMemorySize} = $xeinfo->{pmem} || "999999999";  # placeholder value
                $execenv->{VirtualMemorySize} = $xeinfo->{vmem} if $xeinfo->{vmem};
                $execenv->{OSFamily} = $sysname || 'UNDEFINEDVALUE'; # placeholder value
                $execenv->{OSName} = $xeconfig->{OSName} if $xeconfig->{OSName};
                $execenv->{OSVersion} = $xeconfig->{OSVersion} if $xeconfig->{OSVersion};
                # if Connectivity* not specified, assume false.
                # this has been change due to this value to be mandatory in the LDAP schema.
                $execenv->{ConnectivityIn} = glue2bool($xeconfig->{ConnectivityIn}) || 'FALSE'; # placeholder value
                $execenv->{ConnectivityOut} = glue2bool($xeconfig->{ConnectivityOut}) || 'FALSE'; # placeholder value
                $execenv->{NetworkInfo} = [ $xeconfig->{NetworkInfo} ] if $xeconfig->{NetworkInfo};

                if ($callcount == 1) {
                    $log->info("MainMemorySize not set for ExecutionEnvironment $xenv, will default to 9999999") unless $xeinfo->{pmem};
                    $log->info("OSFamily not set for ExecutionEnvironment $xenv") unless $sysname;
                    $log->info("ConnectivityIn not set for ExecutionEnvironment $xenv, will default to undefined")
                        unless defined $xeconfig->{ConnectivityIn};
                    $log->info("ConnectivityOut not set for ExecutionEnvironment $xenv, will default to undefined")
                        unless defined $xeconfig->{ConnectivityOut};

                    my @missing;
                    for (qw(Platform CPUVendor CPUModel CPUClockSpeed OSFamily OSName OSVersion)) {
                        push @missing, $_ unless defined $execenv->{$_};
                    }
                    $log->info("Missing attributes for ExecutionEnvironment $xenv: ".join ", ", @missing) if @missing;
                }

                if ($xeconfig->{Benchmark}) {
                    my @bconfs = @{$xeconfig->{Benchmark}};
                    $execenv->{Benchmarks} = sub {
                        return undef unless @bconfs;
                        my ($type, $value) = split " ", shift @bconfs;
                        my $bench = {};
                        $bench->{Type} = $type;
                        $bench->{Value} = $value;
                        $bench->{ID} = "urn:ogf:Benchmark:$hostname:$execenvName:$type";
                        return $bench;
                    };
                }

                # Associations

                for my $share (keys %{$GLUE2shares}) {
                    my $sconfig = $GLUE2shares->{$share};
                    next unless $sconfig->{ExecutionEnvironmentName};
                    next unless grep { $xenv eq $_ } @{$sconfig->{ExecutionEnvironmentName}};
                    push @{$execenv->{ComputingShareID}}, $cshaIDs{$share};
                }

                $execenv->{ManagerID} = $cmgrID;
                $execenv->{ComputingManagerID} = $cmgrID;

                return $execenv;
            };

            $cmgr->{ExecutionEnvironments} = $getExecutionEnvironments;

            # ApplicationEnvironments

            my $getApplicationEnvironments = sub {

                return undef unless my ($rte, $rinfo) = each %$rte_info;

                my $appenv = {};

                # name and version is separated at the first dash (-) which is followed by a digit
                my ($name,$version) = ($rte, undef);
                ($name,$version) = ($1, $2) if $rte =~ m{^(.*?)-([0-9].*)$};

                $appenv->{AppName} = $name;
                $appenv->{AppVersion} = $version if defined $version;
                $appenv->{ID} = $aenvIDs{$rte};
                $appenv->{State} = $rinfo->{state} if $rinfo->{state};
                $appenv->{Description} = $rinfo->{description} if $rinfo->{description};
                #$appenv->{ParallelSupport} = 'none';
                
                # Associations
                $appenv->{ComputingManagerID} = $cmgrID;

                return $appenv;
            };

            $cmgr->{ApplicationEnvironments} = $getApplicationEnvironments;

            # Associations

            $cmgr->{ServiceID} = $csvID;
            $cmgr->{ComputingServiceID} = $csvID;

            return $cmgr;
        };

        $csv->{ComputingManager} = $getComputingManager;

        # Location and Contacts
        if (my $lconfig = $config->{location}) {
            my $count = 1;
            $csv->{Location} = sub {
                return undef if $count-- == 0;
                my $loc = {};
                $loc->{ID} = "urn:ogf:Location:$hostname:ComputingService:arex";
                for (qw(Name Address Place PostCode Country Latitude Longitude)) {
                    $loc->{$_} = $lconfig->{$_} if defined $lconfig->{$_};
                }
                $loc->{ServiceForeignKey} = $csvID;
                return $loc;
            }
        }
        if (my $cconfs = $config->{contacts}) {
            my $i = 0;
            $csv->{Contacts} = sub {
                return undef unless $i < scalar(@$cconfs);
                my $cconfig = $cconfs->[$i];
                #my $detail = $cconfig->{Detail};
                my $cont = {};
                $cont->{ID} = "urn:ogf:Contact:$hostname:ComputingService:arex:con$i";
                for (qw(Name Detail Type)) {
                    $cont->{$_} = $cconfig->{$_} if $cconfig->{$_};
                }
                $cont->{ServiceForeignKey} = $csvID;
                $i++;
                return $cont;
            };
        }
        
        # Associations

        $csv->{AdminDomainID} = $adID;
        $csv->{ServiceID} = $csvID;

        return $csv;
    };

    my $getAdminDomain = sub {
        my $dom = { ID => $adID,
                    Name => $config->{admindomain}{Name},
                    OtherInfo => $config->{admindomain}{OtherInfo},
                    Description => $config->{admindomain}{Description},
                    WWW => $config->{admindomain}{WWW},
                    Owner => $config->{admindomain}{Owner},
                    CreationTime => $creation_time,
                    Validity => $validity_ttl
                  };
        $dom->{Distributed} = glue2bool($config->{admindomain}{Distributed});
        
        
        # TODO: Location and Contact for AdminDomain goes here.
        # Contacts can be multiple, don't know how to handle this
        # in configfile.
        # TODO: remember to sync ForeignKeys
        #  Disabled for now, as it would only cause trouble.
#         if (my $lconfig = $config->{location}) {
#             my $count = 1;
#             $dom->{Location} = sub {
#                 return undef if $count-- == 0;
#                 my $loc = {};
#                 $loc->{ID} = "urn:ogf:Location:$hostname:AdminDomain:$admindomain";
#                 for (qw(Name Address Place PostCode Country Latitude Longitude)) {
#                     $loc->{$_} = $lconfig->{$_} if defined $lconfig->{$_};
#                 }
#                 return $loc;
#             }
#         }
#         if (my $cconfs = $config->{contacts}) {
#             my $i = 0;
#             $dom->{Contacts} = sub {
#                 return undef unless $i < scalar(@$cconfs);
#                 my $cconfig = $cconfs->[$i++];
#                 #my $detail = $cconfig->{Detail};
#                 my $cont = {};
#                 $cont->{ID} = "urn:ogf:Contact:$hostname:AdminDomain:$admindomain:$i";
#                 for (qw(Name Detail Type)) {
#                     $cont->{$_} = $cconfig->{$_} if $cconfig->{$_};
#                 }
#                 return $cont;
#             };
#         }


        return $dom;
    };

    # Other Services

    my $othersv = {};

	#EndPoint here


    # aggregates services

    my $getServices = sub {
    
        return undef unless my ($service, $sub) = each %$othersv;
        # returns the hash for Entries. Odd, must understand this behaviour
        return &$sub;
             
    };
    
    # TODO: UserDomain
    
    my $getUserDomain = sub {

        my $ud = {};

        $ud->{CreationTime} = $creation_time;
        $ud->{Validity} = $validity_ttl;

        $ud->{ID} = $udID;

        $ud->{Name} = "";
        $ud->{OtherInfo} = $config->{service}{OtherInfo} if $config->{service}{OtherInfo}; # array
        $ud->{Description} = '';

        # Number of hops to reach the root
        $ud->{Level} = 0;
        # Endpoint of some service, such as VOMS server
        $ud->{UserManager} = 'http://voms.nordugrid.org';
        # List of users
        $ud->{Member} = [ 'users here' ];
        
        # TODO: Calculate Policies, ContactID and LocationID
        
        # Associations
        $ud->{UserDomainID} = $udID;
        
        return $ud;

    };

    # TODO: ToStorageElement

    my $getToStorageElement = sub {
      
	my $tse = {};

	$tse->{CreationTime} = $creation_time;
	$tse->{Validity} = $validity_ttl;

	$tse->{ID} = $tseID;

	$tse->{Name} = "";
	$tse->{OtherInfo} = ''; # array
	
	# Local path on the machine to access storage, for example a NFS share
	$tse->{LocalPath} = 'String';
  
	# Remote path in the Storage Service associated with the local path above
	$tse->{RemotePath} = 'String';
	
	# Associations
	$tse->{ComputingService} = $csvID;
	$tse->{StorageService} = '';
    };


    # returns the two branches for =grid and =services GroupName.
    # It's not optimal but it doesn't break recursion
    my $GLUE2InfoTreeRoot = sub {
        my $treeroot = { 
                        AdminDomain => $getAdminDomain,
                        UserDomain => $getUserDomain,
                        ComputingService => $getComputingService,
                        Services => $getServices
                       };
        return $treeroot;
    };

    return $GLUE2InfoTreeRoot;

}

1;
