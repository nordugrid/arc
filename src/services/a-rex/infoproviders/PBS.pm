package PBS;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw(get_lrms_info get_lrms_options_schema);

use LogUtils;

use strict;

our $options;
our $lrms_info = {};
our %user_jobs_running;
our %user_jobs_queued;

our $log = LogUtils->getLogger(__PACKAGE__);

############################################
# Public interface
#############################################

sub get_lrms_options_schema {
    return {
            'pbs_bin_path'   => '',
            'pbs_log_path'   => '*',            # only used in scan-pbs-jobs
            'pbs_scheduler'  => '*',
            'maui_bin_path'  => '*',
            'dedicated_node_string' => '*',
            'queues' => {
                '*' => {
                    'users'       => [ '' ],
                    'queue_node_string' => '*'  # only used in submit-pbs-job
                }
            },
            'jobs' => [ '' ]
        };
}


sub get_lrms_info($) {

    $options = shift;

    cluster_info();

    my %queues = %{$options->{queues}};
    for my $qname ( keys %queues ) {
        queue_info($qname);
    }

    my $jids = $options->{jobs};
    jobs_info($jids);

    for my $qname ( keys %queues ) {
        my $users = $queues{$qname}{users};
        users_info($qname,$users);
    }

    return $lrms_info;
}


##########################################
# Private subs
##########################################

sub read_pbsnodes ($) {

    #processing the pbsnodes output by using a hash of hashes
    # %hoh_pbsnodes (referrenced by $hashref)

    my ( $path ) = shift;
    my ( %hoh_pbsnodes);
    my ($nodeid,$node_var,$node_value);

    unless (open PBSNODESOUT,  "$path/pbsnodes -a |") {
	$log->error("error in executing pbsnodes") and die;
    }
    while (my $line= <PBSNODESOUT>) {
	if ($line =~ /^$/) {next};
	if ($line =~ /^([\w\-]+)/) {
   	    $nodeid= $1 ;
   	    next;
	}
	if ($line =~ / = /)  {
	    ($node_var,$node_value) = split (/ = /, $line);
	    $node_var =~ s/\s+//g;
	    chop $node_value;
	}
	$hoh_pbsnodes{$nodeid}{$node_var} = $node_value;
    }
    close PBSNODESOUT;
    return %hoh_pbsnodes;
}


sub cluster_info () {

    my $lrms_cluster = {};

    # add this cluster to the info tree
    $lrms_info->{cluster} = $lrms_cluster;

    my $path = $options->{pbs_bin_path};

    #determine the flavour and version of PBS"
    my $qmgr_string=`$path/qmgr -c "list server"`;
    if ( $? != 0 ) {
	$log->error("Can't run qmgr");
    }
    if ($qmgr_string =~ /pbs_version = \b(\D+)_(\d\S+)\b/) {
      $lrms_cluster->{lrms_type} = $1;
      $lrms_cluster->{lrms_version} = $2;
    }
    else {
	$qmgr_string =~ /pbs_version = \b(\d\S+)\b/;
	$lrms_cluster->{lrms_type}="torque";
	$lrms_cluster->{lrms_version}=$1;
    }

    # processing the pbsnodes output by using a hash of hashes %hoh_pbsnodes
    my %hoh_pbsnodes = read_pbsnodes( $path );

    my $npstring;
    my %np_str =   ( 'openpbs' => 'np',
		     'spbs' => 'np',
		     'torque' => 'np',
		     'pbspro' => 'np');

    if ( ! exists $np_str{lc($lrms_cluster->{lrms_type})}) {
	$log->error("Unsupported PBS flavour $lrms_cluster->{lrms_type}") and die;
    } else {
	$npstring = $np_str{ lc($lrms_cluster->{lrms_type}) };
    }

    $lrms_cluster->{totalcpus} = 0;
    my ($number_of_running_jobs) = 0;
    $lrms_cluster->{cpudistribution} = "";
    my @cpudist = 0;

    foreach my $node (keys %hoh_pbsnodes){

	next if $options->{dedicated_node_string}
	    and $hoh_pbsnodes{$node}{properties}
	        !~ m/$options->{dedicated_node_string}/;

	my $nodestate = $hoh_pbsnodes{$node}{"state"};

	next if $nodestate=~/down/ or $nodestate=~/offline/;

        if ($nodestate=~/(?:,|^)busy/) {
           $lrms_cluster->{totalcpus} += $hoh_pbsnodes{$node}{$npstring};
	   $cpudist[$hoh_pbsnodes{$node}{$npstring}] +=1;
	   $number_of_running_jobs += $hoh_pbsnodes{$node}{$npstring};
	   next;
        }

	$lrms_cluster->{totalcpus} += $hoh_pbsnodes{$node}{$npstring};

	$cpudist[$hoh_pbsnodes{$node}{$npstring}] += 1;

	if ($hoh_pbsnodes{$node}{"jobs"}){
	    $number_of_running_jobs++;
	    my ( @comma ) = ($hoh_pbsnodes{$node}{"jobs"}=~ /,/g);
	    $number_of_running_jobs += @comma;
	}
    }

    for (my $i=0; $i<=$#cpudist; $i++) {
	next unless ($cpudist[$i]);
	$lrms_cluster->{cpudistribution} .= " ".$i."cpu:".$cpudist[$i];
    }

    # read the qstat -n information about all jobs
    # queued cpus, total number of cpus in all jobs

    $lrms_cluster->{usedcpus} = 0;
    my $totalcpus = 0;
    unless (open QSTATOUTPUT,  "$path/qstat -n |") {
	$log->error("Error in executing qstat") and die;
    }
    while (my $line= <QSTATOUTPUT>) {
	next unless $line =~ m/^\d+/;

	my ($jid, $user, $queue, $jname, $sid, $nds, $tsk, $reqmem,
	    $reqtime, $state, $etime) = split " ", $line;
	$totalcpus += $nds;
	if ( $state eq "R" ) {
	    $lrms_cluster->{usedcpus} += $nds;
	}
    }
    close QSTATOUTPUT;

    $lrms_cluster->{queuedcpus} = $totalcpus - $lrms_cluster->{usedcpus};

    unless (open QSTATOUTPUT,  "$path/qstat -Q |") {
	$log->error("Error in executing qstat") and die;
    }
    while (my $line= <QSTATOUTPUT>) {
	if ( $. == 1 or $. == 2 ) {next} # Skip header lines
	my @a = split " ", $line;
	push @{$lrms_cluster->{queue}}, $a[0];
    }
    close QSTATOUTPUT;
}


sub queue_info ($) {
    my $qname = shift;
    my $path = $options->{pbs_bin_path};

    my $lrms_queue = {};

    # add this queue to the info tree
    $lrms_info->{queues}{$qname} = $lrms_queue;

    # read the queue information for the queue entry from the qstat

    my %qstat;
    unless (open QSTATOUTPUT,   "$path/qstat -f -Q $qname |") {
	$log->error("Error in executing qstat: $path/qstat -f -Q $qname") and die;
    }
    while (my $line= <QSTATOUTPUT>) {
	if ($line =~ m/ = /) {
	    chomp($line);
	    my ($qstat_var,$qstat_value) = split("=", $line);
	    $qstat_var =~ s/\s+//g;
	    $qstat_value =~ s/\s+//g;
	    $qstat{$qstat_var}=$qstat_value;
	}
    }
    close QSTATOUTPUT;

    my %keywords =   ( 'max_running' => 'maxrunning',
		       'max_user_run' => 'maxuserrun',
		       'max_queuable' => 'maxqueuable' );

    foreach my $k (keys %keywords) {
	if (defined $qstat{$k} ) {
	    $lrms_queue->{$keywords{$k}} = int($qstat{$k});
	}
    }

    %keywords = ( 'resources_max.cput' => 'maxcputime',
		  'resources_min.cput' => 'mincputime',
		  'resources_default.cput' => 'defaultcput',
		  'resources_max.walltime' => 'maxwalltime',
		  'resources_min.walltime' => 'minwalltime',
		  'resources_default.walltime' => 'defaultwallt' );

    foreach my $k (keys %keywords) {
	if ( defined $qstat{$k} ) {
	    my @time=split(":",$qstat{$k});
	    $lrms_queue->{$keywords{$k}} = $time[0]*60+$time[1];
	}
    }
    $lrms_queue->{mincputime} += 1 if defined $lrms_queue->{mincputime};

    # determine the queue status from the LRMS
    # Used to be set to 'active' if the queue can accept jobs
    # Now lists the number of available processors, "0" if no free
    # cpus. Negative number signals some error state of PBS
    # (reserved for future use).

    $lrms_queue->{status} = -1;
    $lrms_queue->{running} = 0;
    $lrms_queue->{queued} = 0;
    $lrms_queue->{totalcpus} = 0;
    if ( ($qstat{enabled} =~ /True/) and ($qstat{started} =~ /True/)) {
	unless (open QSTATOUTPUT,   "$path/qstat -Q $qname |") {
	    $log->error("Failed running $path/qstat -f -Q $qname") and die;
	}
	while (my $line = <QSTATOUTPUT> ) {
	    if ( $line =~ /^$qname\s+/) {
		my @a = split " ", $line;
		my $cpus = $lrms_info->{cluster}{totalcpus};
		if ( $cpus < $a[1] or $a[1] < 1 ) {
		    $lrms_queue->{totalcpus} = $cpus;
		} else {
		    $lrms_queue->{totalcpus} = $a[1];
		}
		$lrms_queue->{status} = $lrms_queue->{totalcpus} - $a[2];
		if ($lrms_queue->{status} < 0) {
		    $lrms_queue->{status} = 0;
		}
		$lrms_queue->{running} = $a[6];
		$lrms_queue->{queued} = $a[5];
		$lrms_queue->{total} = $a[2];
		last;
	    }
	}
	close QSTATOUTPUT;
    }
}

sub jobs_info ($) {

    # LRMS job IDs from Grid Manager
    my $jids = shift;

    my $lrms_jobs = {};

    # add users to the info tree
    $lrms_info->{jobs} = $lrms_jobs;

    # Path to LRMS commands
    my $path = $options->{pbs_bin_path};

    # assume all jobs dead unless found alive
    for my $jid (@$jids) {
        $lrms_jobs->{$jid}{status} = 'EXECUTED';
    }

    # try to get rank from maui
    my %showqrank;
    if (lc($options->{pbs_scheduler}) eq 'maui') {
        my $showq = $options->{maui_bin_path}
                  ? $options->{maui_bin_path}."/showq" : "showq";
        unless (open SHOWQOUTPUT, "$showq |") {
            $log->erorr("Failed running $showq") and die;
        }
        my $idle = -1;
        while (my $line = <SHOWQOUTPUT>) {
            if ($line =~ /^IDLE.+/) {
                $idle = 0;
                $line = <SHOWQOUTPUT>;
                $line = <SHOWQOUTPUT>;
            }
            next if $idle == -1;
            if ($line =~ /^(\d+).+/) {
                $idle++;
                $showqrank{$1} = $idle;
            }
        }
        close SHOWQOUTPUT;
    }

    my %tkeywords =   ( 'resources_used.walltime' => 'walltime',
			'resources_used.cput' => 'cputime',
			'Resource_List.walltime' => 'reqwalltime',
			'Resource_List.cputime' => 'reqcputime');

    unless (open QSTATOUTPUT, "$path/qstat -f |") {
       $log->error("Failed running $path/qstat -f") and die;
    }

    # for faster lookups
    my %jidh; $jidh{$_} = 1 for @$jids;

    my ($jid, $rank, $qname, $state, $owner) = ();

    while (my $line = <QSTATOUTPUT>) {

	if ($line =~ /^Job Id:\s+(\d+.*)/) {

            my $newjid = $1;

            # before processing a new job, finish with the previous one
            if ($jid) {

	        if ($state ne 'R') {
                    # running jobs have no queue rank
	            if ($jid =~ /^(\d+).+/ and defined $showqrank{$1}) {
	                $lrms_jobs->{$jid}{rank} = $showqrank{$1};
	            } else {
	                $lrms_jobs->{$jid}{rank} = $rank;
	            }
	        }
                $lrms_jobs->{$jid}{Job_Owner} = $owner;

	        ++$user_jobs_running{$qname}{$owner}
		    if $state eq 'R' or $state eq 'E';
	        ++$user_jobs_queued{$qname}{$owner}
		    if $state eq 'Q';

                # INLRMS:X job state mapping
                if ( $state eq "R" ) {
                   $lrms_jobs->{$jid}{status} = "R";
                } elsif ( $state eq "Q" ) {
                   $lrms_jobs->{$jid}{status} = "Q";
                } elsif ( $state eq "E" ) {
                   $lrms_jobs->{$jid}{status} = "E";
                } elsif ( $state eq "U" ) {
                   $lrms_jobs->{$jid}{status} = "S";
                } else {
                   $lrms_jobs->{$jid}{status} = "O";
                }

            }

            # forget old job
            ($jid, $rank, $qname, $state, $owner) = ();

	    # Check if the new job is on the list.
	    # Otherwise it will not be processed.
            $jid = $newjid if $jidh{$newjid};

	    next;
	}

	next unless $jid;

	my ( $k, $v ) = split ' = ', $line;

	$k =~ s/\s+(.*)/$1/;
	chomp $v;

	if ( $tkeywords{$k} ) {
	    my @t = split ':',$v;
	    $lrms_jobs->{$jid}{$tkeywords{$k}} = $t[0]*60+$t[1];
        } elsif ( $k eq 'exec_host' ) {
            push @{$lrms_jobs->{$jid}{nodes}}, $v;
	} elsif ( $k eq 'comment' ) {
	    push @{$lrms_jobs->{$jid}{comment}}, "LRMS: $v"; 
	} elsif ($k eq 'resources_used.vmem' and $v =~ /(\d+)kb/i ) {
	    $lrms_jobs->{$jid}{mem} = $1;
	} elsif ( $k eq 'queue_rank' ) {
	    $rank = $v;
	} elsif ( $k eq 'queue' ) {
	    $qname = $v;
	} elsif ( $k eq 'Job_Owner' and $v =~ /(\S+)@/ ) {
	    $owner = $1;
	} elsif ( $k eq 'job_state' ) {
	    $state = $v;
	}
    }
    close QSTATOUTPUT;
}


sub users_info($$) {
    my ($qname, $accts) = @_;
    my $path = $options->{pbs_bin_path};

    my $lrms_users = {};

    # add users to the info tree
    my $lrms_queue = $lrms_info->{queues}{$qname};
    $lrms_queue->{users} = $lrms_users;


    # Check that users have access to the queue
    unless (open QSTATOUTPUT,   "$path/qstat -f -Q $qname |") {
	$log->error("Failed running $path/qstat -f -Q $qname") and die;
    }
    my $acl_user_enable = 0;
    while (my $line= <QSTATOUTPUT>) {
        chomp $line;
	if ( $line =~ /\s*acl_user_enable/ ) {
	    my ( $k ,$v ) = split ' = ', $line;
	    unless ( $v eq 'False' ) {
	      $acl_user_enable = 1;
	    }
	}
	if ( $line =~ /\s*acl_users/ ) {
	    my ( $k ,$v ) = split ' = ', $line;
	    unless ( $v eq 'False' ) {
	      # This condition is kept here in case the reason
	      # for it being there in the first place was that some
	      # version or flavour of PBS really has False as an alternative
	      # to usernames to indicate the absence of user access control
	      # A Corrallary: Dont name your users 'False' ...
		foreach my $a ( @{$accts} ) {
		    if ( grep { $_ eq $a } $v ) {
		      push @{$lrms_queue->{acl_users}}, $a;
		    } else {
			$log->info("Grid users mapped to $a do not ".
				      "have access to queue $qname.");
		    }
		}
	    }
	}
    }
    close QSTATOUTPUT;

    delete $lrms_queue->{acl_users} unless $acl_user_enable;

    foreach my $u ( @{$accts} ) {
        if (lc($options->{pbs_scheduler}) eq 'maui') {
            my $showbf = $options->{maui_bin_path}
                       ? $options->{maui_bin_path}."/showq" : "showq";
            if ($options->{dedicated_node_string}) {
                $showbf .= " -f $options->{dedicated_node_string}";
            }
	    my $maui_freecpus;
	    unless (open SHOWBFOUTPUT, " $showbf -u $u |"){
		$log->error("Failed running $showbf -u $u") and die;
	    }
	    while (my $line= <SHOWBFOUTPUT>) {
		if ($line =~ /^partition/) {
		    last;
		}
		if ($line =~ /no procs available/) {
		    $maui_freecpus = { 0 => 0 };
		    last;
		}
		if ($line =~ /(\d+).+available for\s+([\w:]+)/) {
		    my @tmp= reverse split /:/, $2;
		    my $minutes=$tmp[1] + 60*$tmp[2] + 24*60*$tmp[3];
		    $maui_freecpus = { $1 => $minutes };
		}
		if ($line =~ /(\d+).+available with no timelimit/) {
		    $maui_freecpus = { $1 => 0 }; # 0 means unlimited time
		    last;
		}
	    }
            $user_jobs_queued{$qname}{$u} = 0 unless $user_jobs_queued{$qname}{$u};
	    $lrms_users->{$u}{queuelength} = $user_jobs_queued{$qname}{$u};
	    $lrms_users->{$u}{freecpus} = $maui_freecpus;
	}
	else {
            my $freecpus = 0;
            $user_jobs_running{$qname}{$u} = 0 unless $user_jobs_running{$qname}{$u};
	    if ($lrms_queue->{maxuserrun}
            and $lrms_queue->{maxuserrun} - $user_jobs_running{$qname}{$u} < $lrms_queue->{status} ) {
		$freecpus = $lrms_queue->{maxuserrun} - $user_jobs_running{$qname}{$u};
	    } else {
		$freecpus = $lrms_queue->{status};
	    }
	    $lrms_users->{$u}{queuelength} = $lrms_queue->{queued};
	    $freecpus = 0 if $freecpus < 0;
	    if ($lrms_queue->{maxcputime}) {
		$lrms_users->{$u}{freecpus} = { $freecpus => $lrms_queue->{maxcputime} };
            } else {
		$lrms_users->{$u}{freecpus} = { $freecpus => 0 }; # unlimited
	    }
	}
    }
}



1;
