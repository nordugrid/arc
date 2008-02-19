package PBS;

use File::Basename;
use lib dirname($0);
@ISA = ('Exporter');

# Module implements these subroutines for the LRMS interface

@EXPORT_OK = ('cluster_info',
	      'queue_info',
	      'jobs_info',
	      'users_info');

use LogUtils ( 'start_logging', 'error', 'warning', 'debug' ); 
use strict;

##########################################
# Saved private variables
##########################################

our(%lrms_queue);

##########################################
# Private subs
##########################################

sub read_pbsnodes ($) { 

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
	    ($node_var,$node_value) = split (/ = /, $line);
	    $node_var =~ s/\s+//g;
	    chop $node_value;  	     
	}	     
	$hoh_pbsnodes{$nodeid}{$node_var} = $node_value;     
    } 
    close PBSNODESOUT;
    return %hoh_pbsnodes;
}


############################################
# Public subs
#############################################

sub cluster_info ($) {

    # Path to LRMS commands
    my ($config) = shift;
    my ($path) = $$config{pbs_bin_path};

    # Return data structure %lrms_cluster{$keyword}
    # should contain the keywords listed in LRMS.pm.
    #
    # All values should be defined, empty values "" are ok if field
    # does not apply to particular LRMS.

    my (%lrms_cluster);

    #determine the flavour and version of PBS"
    my $qmgr_string=`$path/qmgr -c "list server"`;
    if ( $? != 0 ) {    
	warning("Can't run qmgr");
    }
    if ($qmgr_string =~ /pbs_version = \b(\D+)_(\d\S+)\b/) {
      $lrms_cluster{lrms_type} = $1;
      $lrms_cluster{lrms_version} = $2;
    }
    else {
	$qmgr_string =~ /pbs_version = \b(\d\S+)\b/;
	$lrms_cluster{lrms_type}="torque";
	$lrms_cluster{lrms_version}=$1;
    }

    # processing the pbsnodes output by using a hash of hashes %hoh_pbsnodes
    my ( %hoh_pbsnodes ) = read_pbsnodes( $path );

    my ($npstring);
    my (%np_str) = ( 'openpbs' => 'np',
		     'spbs' => 'np',
		     'torque' => 'np',
		     'pbspro' => 'np');
    
    if ( ! exists $np_str{lc($lrms_cluster{lrms_type})}) {
	error("The given flavour of PBS $lrms_cluster{lrms_type} ".
	      "is not supported");
    } else {
	$npstring = $np_str{ lc($lrms_cluster{lrms_type}) }
    }

    $lrms_cluster{totalcpus} = 0;
    my ($number_of_running_jobs) = 0;
    $lrms_cluster{cpudistribution} = "";
    my (@cpudist) = 0;

    foreach my $node (keys %hoh_pbsnodes){

	if ( exists $main::config{dedicated_node_string} ) {
	    unless ( $hoh_pbsnodes{$node}{"properties"} =~
		     m/$main::config{dedicated_node_string}/) {
		next;
	    }
	}

	my ($nodestate) = $hoh_pbsnodes{$node}{"state"};      

	if ($nodestate=~/down/ or $nodestate=~/offline/) {
	    next;
	}
        if ($nodestate=~/(?:,|^)busy/) {
           $lrms_cluster{totalcpus} += $hoh_pbsnodes{$node}{$npstring};
	   $cpudist[$hoh_pbsnodes{$node}{$npstring}] +=1;
	   $number_of_running_jobs += $hoh_pbsnodes{$node}{$npstring};
	   next;
        }
	

	$lrms_cluster{totalcpus} += $hoh_pbsnodes{$node}{$npstring};

	$cpudist[$hoh_pbsnodes{$node}{$npstring}] += 1;

	if ($hoh_pbsnodes{$node}{"jobs"}){
	    $number_of_running_jobs++;
	    my ( @comma ) = ($hoh_pbsnodes{$node}{"jobs"}=~ /,/g);
	    $number_of_running_jobs += @comma;
	} 
    }      

    for (my $i=0; $i<=$#cpudist; $i++) {
	next unless ($cpudist[$i]);  
	$lrms_cluster{cpudistribution} .= " ".$i."cpu:".$cpudist[$i];
    }

    # read the qstat -n information about all jobs
    # queued cpus, total number of cpus in all jobs

    $lrms_cluster{usedcpus} = 0;
    my ($totalcpus) = 0;
    unless (open QSTATOUTPUT,  "$path/qstat -n 2>/dev/null |") {
	error("Error in executing qstat");
    }
    while (my $line= <QSTATOUTPUT>) {       
	if ( ! $line =~ m/^\d+/) {
	    next;
	}
	my ($jid, $user, $queue, $jname, $sid, $nds, $tsk, $reqmem,
	    $reqtime, $state, $etime) = split " ", $line;
	$totalcpus += $nds;
	if ( $state eq "R" ) {
	    $lrms_cluster{usedcpus} += $nds;
	}
    }   
    close QSTATOUTPUT;
    $lrms_cluster{queuedcpus} = $totalcpus - $lrms_cluster{usedcpus};


    # Names of all LRMS queues

    @{$lrms_cluster{queue}} = ();
    unless (open QSTATOUTPUT,  "$path/qstat -Q 2>/dev/null |") {
	error("Error in executing qstat");
    }
    while (my $line= <QSTATOUTPUT>) {
	if ( $. == 1 or $. == 2 ) {next} # Skip header lines
	my (@a) = split " ", $line;
	push @{$lrms_cluster{queue}}, $a[0]; 
    }
    close QSTATOUTPUT;

    return %lrms_cluster;
}

sub queue_info ($$) {

    # Path to LRMS commands
    my ($config) = shift;
    my ($path) = $$config{pbs_bin_path};

    # Name of the queue to query
    my ($qname) = shift;

    # The return data structure is %lrms_queue.
    # In this template it is defined as persistent module data structure,
    # because it is later used by jobs_info(), and we wish to avoid
    # re-construction of it. If it were not needed later, it would be defined
    # only in the scope of this subroutine, as %lrms_cluster previously.

    # Return data structure %lrms_queue{$keyword}
    # should contain the keyvords listed in LRMS.pm.
    #
    # All values should be defined, empty values "" are ok if field
    # does not apply to particular LRMS.

    # read the queue information for the queue entry from the qstat

    my (%qstat);
    unless (open QSTATOUTPUT,   "$path/qstat -f -Q $qname 2>/dev/null |") {
	error("Error in executing qstat: $path/qstat -f -Q $qname");
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

    my (%keywords) = ( 'max_running' => 'maxrunning',
		       'max_user_run' => 'maxuserrun',
		       'max_queuable' => 'maxqueuable' );

    foreach my $k (keys %keywords) {
	if (defined $qstat{$k} ) {
	    $lrms_queue{$keywords{$k}} = $qstat{$k};
	} else {
	    $lrms_queue{$keywords{$k}} = "";
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
	    $lrms_queue{$keywords{$k}} = ($time[0]*60+$time[1]+($k eq 'resources_min.cput'?1:0));
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
	unless (open QSTATOUTPUT,   "$path/qstat -Q $qname 2>/dev/null |") {
	    error("Error in executing qstat: $path/qstat -f -Q $qname");
	}
	while (my $line = <QSTATOUTPUT> ) {
	    if ( $line =~ /^$qname\s+/) {
		my (@a) = split " ", $line;
		if ($a[1] < 1) {
		    # the maximum nr of cpus available to the queue is 0
		    # This indicates that there is no set limit and the
		    # total nr of cpus should be used instead
		    my %h = read_pbsnodes ($path);
		    my $cpus=0;
		    for my $n (values %h) {
		      if (exists $n->{np}){
			$cpus += $n->{np};
		      }
		      elsif (exists $n->{pcpus}) {
			$cpus += $n->{pcpus};
		      }
		    }
		    $a[1] = $cpus;
		    $lrms_queue{totalcpus} = $cpus;
		}
		$lrms_queue{status} = $a[1] - $a[2];
		if ( $lrms_queue{status} < 0 ) {
		    $lrms_queue{status} = 0;
		}
		$lrms_queue{running} = $a[6];
		$lrms_queue{queued} = $a[5];
		$lrms_queue{total} = $a[2];
		last;
	    }
	}
	close QSTATOUTPUT;
    }

    return %lrms_queue;
}

my ( %user_jobs_running, %user_jobs_queued );

sub jobs_info ($$@) {

    # Path to LRMS commands
    my ($config) = shift;
    my ($path) = $$config{pbs_bin_path};

    # Name of the queue to query
    my ($qname) = shift;

    # LRMS job IDs from Grid Manager (jobs with "INLRMS" GM status)
    my ($jids) = shift;

    # Return data structure %lrms_jobs{$lrms_local_job_id}{$keyword}
    # should contain the keywords listed in LRMS.pm.
    #
    # All values should be defined, empty values "" are ok if field
    # does not apply to particular LRMS.

    my (%lrms_jobs);

    # Fill %lrms_jobs here (da implementation)

    # rank is treated separately as it does not have an entry in
    # qstat output, comment because it is an array, and mem
    # because "kB" needs to be stripped from the value
    my (%skeywords) = ('job_state' => 'status',
		       'exec_host' => 'node');
    
    my (%tkeywords) = ( 'resources_used.walltime' => 'walltime',
			'resources_used.cput' => 'cputime',
			'Resource_List.walltime' => 'reqwalltime',
			'Resource_List.cputime' => 'reqcputime');
 
    my ($alljids) = join ' ', @{$jids};
    my ($jid) = 0;
    my ($rank) = 0;
    unless (open QSTATOUTPUT,   "$path/qstat -f $alljids 2>/dev/null |") {
	error("Error in executing qstat: $path/qstat -f $alljids");
    }
    while (my $line = <QSTATOUTPUT>) {       
	if ($line =~ /^Job Id:\s+(\d+.*)/) {
	    $jid = 0;
	    foreach my $k ( @{$jids}) {
		if ( $1 =~ /^$k/ ) { 
		    $jid = $k;
		    last;
		}
	    }
	    next;
	}

	if ( ! $jid ) {next}

	my ( $k, $v );
	( $k, $v ) = split ' = ', $line;

	$k =~ s/\s+(.*)/$1/;
	chomp $v;

	if ( defined $skeywords{$k} ) {
	    $lrms_jobs{$jid}{$skeywords{$k}} = $v;
	} elsif ( defined $tkeywords{$k} ) {
	    my ( @t ) = split ':',$v;
	    $lrms_jobs{$jid}{$tkeywords{$k}} = $t[0]*60+$t[1];
	} elsif ( $k eq 'comment' ) {
	    @{$lrms_jobs{$jid}{comment}} = $v;
	} elsif ($k eq 'resources_used.vmem') {
	    $v =~ s/(\d+).*/$1/;
	    $lrms_jobs{$jid}{mem} = $v;
	}
	
	if ( $k eq 'Job_Owner' ) {
	    $v =~ /(\S+)@/;
	    $lrms_jobs{$jid}{Job_Owner} = $1;
	}
	if ( $k eq 'job_state' ) {
	    if ($v eq 'R') {
		$lrms_jobs{$jid}{rank} = "";
	    } else {
		$rank++;
		$lrms_jobs{$jid}{rank} = $rank;
	    }
	    if ($v eq 'R' or 'E'){
		++$user_jobs_running{$lrms_jobs{$jid}{Job_Owner}};
	    }
	    if ($v eq 'Q'){ 
		++$user_jobs_queued{$lrms_jobs{$jid}{Job_Owner}};
	    }
	}
    }
    close QSTATOUTPUT;

    my (@scalarkeywords) = ('status', 'rank', 'mem', 'walltime', 'cputime',
			    'reqwalltime', 'reqcputime', 'node');

    foreach $jid ( keys %lrms_jobs ) {
	foreach my $k ( @scalarkeywords ) {
	    if ( ! defined $lrms_jobs{$jid}{$k} ) {
		$lrms_jobs{$jid}{$k} = "";
	    }
	}
	if ( ! defined $lrms_jobs{$jid}{comment} ) {
	    @{$lrms_jobs{$jid}{comment}} = [];
	}
    }

    return %lrms_jobs;
}


sub users_info($$@) {

    # Path to LRMS commands
    my ($config) = shift;
    my ($path) = $$config{pbs_bin_path};

    # Name of the queue to query
    my ($qname) = shift;

    # Unix user names mapped to grid users
    my ($accts) = shift;

    # Return data structure %lrms_users{$unix_local_username}{$keyword}
    # should contain the keywords listed in LRMS.pm.
    #
    # All values should be defined, empty values "" are ok if field
    # does not apply to particular LRMS.

    my (%lrms_users);


    # Check that users have access to the queue
    unless (open QSTATOUTPUT,   "$path/qstat -f -Q $qname 2>/dev/null |") {
	error("Error in executing qstat: $path/qstat -f -Q $qname");
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
		    if (  grep /$a/, $v ) {
		      push @main::acl_users, $a;
		    }
		    else {		       
			warning("Grid user $a does not ".
				"have access in queue $qname.");
		    }
		}
	    }
	}
    }
    close QSTATOUTPUT;
    if ($acl_user_enable) {
      @main::acl_users = ('No such user') unless @main::acl_users; 
    }
    else
    {
      @main::acl_users = (); 
    }
    # Uses saved module data structure %lrms_queue, which
    # exists if queue_info is called before
    if ( ! exists $lrms_queue{status} ) {
	%lrms_queue = queue_info( $path, $qname );
    }

    foreach my $u ( @{$accts} ) {
	if (lc($$config{scheduling_policy}) eq "maui") {
	    my $maui_freecpus;
	    my $showbf = (defined $$config{maui_bin_path}) ? $$config{maui_bin_path}."/showbf" : "showbf";
	    unless (open SHOWBFOUTPUT, " $showbf -u $u |"){
		error("error in executing $showbf -u $u");
	    }
	    while (my $line= <SHOWBFOUTPUT>) {				    
		if ($line =~ /^partition/) {  				    
		    last;
		}
		if ($line =~ /no procs available/) {
		    $maui_freecpus= 0;
		    last;
		}
		if ($line =~ /(\d+).+available for\s+([\w:]+)/) {
		    my @tmp= reverse split /:/, $2;
		    my $minutes=$tmp[1] + 60*$tmp[2] + 24*60*$tmp[3];
		    $maui_freecpus .= $1.":".$minutes;
		}
		if ($line =~ /(\d+).+available with no timelimit/) {  	    
		    $maui_freecpus.= $1;
		    last; 
		}
	    }
	    $lrms_users{$u}{freecpus} = $maui_freecpus;
	}
	else {
	    if (exists $lrms_queue{maxuserrun} and $lrms_queue{maxuserrun} > 0 and ($lrms_queue{maxuserrun} - $user_jobs_running{$u}) < $lrms_queue{status} ) {
		$lrms_users{$u}{freecpus} = $lrms_queue{maxuserrun} - $user_jobs_running{$u};
	    }
	    else {
		$lrms_users{$u}{freecpus} = $lrms_queue{status};
	    }
	    $lrms_users{$u}{queuelength} = "$lrms_queue{queued}";
	    if ($lrms_users{$u}{freecpus} < 0) {
		$lrms_users{$u}{freecpus} = 0;
	    }
	    if (defined $lrms_queue{maxcputime} and $lrms_queue{maxcputime}>0 and $lrms_users{$u}{freecpus} > 0) {
		$lrms_users{$u}{freecpus} .= ':'.$lrms_queue{maxcputime};
	    }
	}
    }
    return %lrms_users;
}
	      


1;
