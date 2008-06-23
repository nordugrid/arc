package Fork;

our @ISA = ('Exporter');
our @EXPORT_OK = qw(get_lrms_info get_lrms_options_schema);

use LogUtils;

use strict;

##########################################
# Saved private variables
##########################################

our %lrms_info;
our $options;

our $log = LogUtils->getLogger("LRMSInfo.fork");

############################################
# Public subs
#############################################

sub get_lrms_options_schema {
    # no extra options needed for fork
    return {};
}

sub get_lrms_info(\%) {

    $options = shift;

    cluster_info();

    $lrms_info{queues} = {};

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

    return \%lrms_info
}


##########################################
# Private subs
##########################################

sub totalcpus {
    # number of cpus is calculated from the /proc/cpuinfo,
    # by default it is set to 1
    
    my ($cpus) = 0;
    unless (open CPUINFOFILE, "</proc/cpuinfo") {
	$log->error("can't read the /proc/cpuinfo") and die;
    }   
    while(my $line = <CPUINFOFILE>) {
	chomp($line);
	if ($line =~ m/^processor/) {
	    $cpus++;	
	}
    }
    close CPUINFOFILE;
    $cpus ||= 1;
    return $cpus;
}

sub cluster_info () {

    my %lrms_cluster;

    $lrms_cluster{lrms_type} = "fork";
    $lrms_cluster{lrms_version} = "";
    $lrms_cluster{totalcpus} = totalcpus();

    # Since fork is a single machine backend all there will only be one machine available
    $lrms_cluster{cpudistribution} = $lrms_cluster{totalcpus}."cpu:1";

    # usedcpus on a fork machine is determined from the 1min cpu
    # loadaverage and cannot be larger than the totalcpus

    my ($oneminavg, $fiveminavg, $fifteenminavg, $processnumber, $lastprocessid );
    unless (open LOADAVGFILE, "</proc/loadavg") {
	$log->error("can't read the /proc/loadavg") and die;
    }
    while(my $line = <LOADAVGFILE>) {
	chomp($line);
	($oneminavg, $fiveminavg, $fifteenminavg, $processnumber, $lastprocessid ) =  split(/\s+/, $line);
    }
    close LOADAVGFILE;

    if (defined $oneminavg) {
	$lrms_cluster{usedcpus} = int $oneminavg;
	if ($lrms_cluster{usedcpus} >= $lrms_cluster{totalcpus}) {
	    $lrms_cluster{usedcpus} = $lrms_cluster{totalcpus};
	}    
    }
    else {
	$lrms_cluster{usedcpus}  = 0;
    }    

    # no LRMS queuing jobs on a fork machine, fork has no queueing ability
    $lrms_cluster{queuedcpus} = 0;

    # add this cluster to the info tree
    $lrms_info{cluster} = \%lrms_cluster;
}

sub queue_info ($) {

    my $qname = shift;
    my %queue_options = %{$options->{queues}{$qname}};

    my %lrms_queue;

    # $lrms_queue{running} (number of active jobs in a fork system)
    # is calculated by making use of the 'ps axr' command

    $lrms_queue{running}= 0;
    unless (open PSCOMMAND,  "ps axr |") {
	$log->error("error in executing ps axr") and die;
    }
    while(my $line = <PSCOMMAND>) {
	chomp($line);
	next if ($line =~ m/PID TTY/);
	next if ($line =~ m/ps axr/);
	next if ($line =~ m/cluster-fork/);     
	$lrms_queue{running}++;
    }
    close PSCOMMAND;

    $lrms_queue{totalcpus} = totalcpus();

    $lrms_queue{status} = $lrms_queue{totalcpus}-$lrms_queue{running};

    # reserve negative numbers for error states
    # Fork is not real LRMS, and cannot be in error state
    if ($lrms_queue{status}<0) {
	$log->debug("lrms_queue{status} = $lrms_queue{status}");
	$lrms_queue{status} = 0;
    }

    # maxcputime

    unless ( $lrms_queue{maxcputime} = `/bin/sh -c "ulimit -t"` ) {
	$log->debug("Could not determine max cputime with ulimit -t");
	$lrms_queue{maxcputime} = "";
    };
    chomp $lrms_queue{maxcputime};

    # queued
    $lrms_queue{queued} = 0;
    $lrms_queue{maxqueuable} = "";
      
    $lrms_queue{maxrunning} = "";
    $lrms_queue{maxuserrun} = $lrms_queue{maxrunning};
    $lrms_queue{mincputime} = "";
    $lrms_queue{defaultcput} = "";
    $lrms_queue{minwalltime} = "";
    $lrms_queue{defaultwallt} = "";
    $lrms_queue{maxwalltime} = $lrms_queue{maxcputime};

    # add this queue to the info tree
    $lrms_info{queues}{$qname} = \%lrms_queue;
}


sub jobs_info ($) {

    my $jids = shift;

    my %lrms_jobs;
    my @s;

    foreach my $id (@$jids){

        my $node = `hostname`;
        chomp($node);
        $lrms_jobs{$id}{nodes} = [ $node ];

        unless ( $node ) {
	    $log->debug("hostname did not work?");
	    $lrms_jobs{$id}{nodes} = [];
	}
        
	unless ( $lrms_jobs{$id}{mem} = `ps h -o vsize -p $id` ) {
	    $log->debug("ps -h -o vsize -p \$ID did not work?");
	    $lrms_jobs{$id}{mem} = "";
	}
        chomp($lrms_jobs{$id}{mem});


	unless ( $lrms_jobs{$id}{walltime} = `ps h -o etime -p $id` ) {
	    $log->debug("ps  -o etime -p \$ID did not work?");
	    $lrms_jobs{$id}{walltime} = 0,"\n";
	}
	@s = split ":|-", $lrms_jobs{$id}{walltime};
	#etime return format is [[dd-]hh:]mm:ss
	my $length=scalar @s;
	if ($length==2){
        $lrms_jobs{$id}{walltime} = $s[0]+int($s[1]/60);
	}
	if ($length==3){
           $lrms_jobs{$id}{walltime} = 60*$s[0]+$s[1]+int($s[2]/60);
	} 
	if ($length==4){
           $lrms_jobs{$id}{walltime} = 1440*$s[0]+60*$s[1]+$s[2]+int($s[3]/60);
	} 
	chomp($lrms_jobs{$id}{walltime});


	unless ( $lrms_jobs{$id}{cputime} = `ps h -o cputime -p $id` ) {
	    $log->debug("ps  -o cputime -p \$ID did not work?");
	    $lrms_jobs{$id}{cputime} = 0,"\n";
	}
	@s = split ":|-", $lrms_jobs{$id}{cputime};
	#cputime return format is [dd-]hh:mm:ss
	my $length=scalar @s;
	if ($length==3){
	   $lrms_jobs{$id}{cputime} = 60*$s[0]+$s[1]+int($s[2]/60);
	} 
	if ($length==4){
	   $lrms_jobs{$id}{cputime} = 1440*$s[0]+60*$s[1]+$s[2]+int($s[3]/60);
	} 
	#to get the true CPU time of the job we need to find all its child processes and add their CPU time
        my $tid;
        my $tempstr="";
	unless ( $tempstr = `ps hax -o ppid,pid,cputime` ) {
       		$log->debug("ps  hax did not work?");
		$tempstr="";	
       	}
	@s=split "\n",$tempstr;
	my @t = grep /^\s*$id\s/, @s;
	while($tempstr){
           $tempstr=$t[0];
           $tempstr=~ s/^\s+//;
           @s = split / /,$tempstr;
           $tid= $s[1];
	   if ($tid==""){last;}
	   my @k = split ":|-", $s[2];
	   #cputime return format is [dd-]hh:mm:ss
	   $length=scalar @k;
 	   if ($length==3){
	      $lrms_jobs{$id}{cputime} += 60*$k[0]+$k[1]+int($k[2]/60);
	   } 
	   if ($length==4){
	      $lrms_jobs{$id}{cputime} += 1440*$k[0]+60*$k[1]+$k[2]+int($k[3]/60);
	   } 
	   unless ( $tempstr = `ps hax -o ppid,pid,cputime` ) {
      	      $log->debug("cputime calculation failed");
	      $tempstr="";	
      	   }
	   @s=split "\n",$tempstr;
	   @t = grep /^\s*$tid\s/, @s;
	   if (@t==0){
	   	$tempstr="";
	   }
	}
	chomp ($lrms_jobs{$id}{cputime}); 
       
	$lrms_jobs{$id}{reqwalltime} = "0";
	$lrms_jobs{$id}{reqcputime} = "0";   
	$lrms_jobs{$id}{comment} = ["LRMS: Running under fork"];
	$lrms_jobs{$id}{status} = "R";
        $lrms_jobs{$id}{rank} = "0";
       
    }

    # add users to the big tree
    $lrms_info{jobs} = \%lrms_jobs;
}


sub users_info($\@) {
    my $qname = shift;
    my $accts = shift;

    my %lrms_users;

    my $lrms_queue = $lrms_info{queues}{$qname};

    # freecpus
    # queue length

    my $job_limit;
    unless ( exists $options->{fork_job_limit} ) {
       $job_limit = 1;
    }
    elsif ($options->{fork_job_limit} eq "cpunumber") {
        $job_limit = $lrms_queue->{totalcpus};
    }
    else {
       $job_limit = $options->{fork_job_limit};
    }
    foreach my $u ( @{$accts} ) {
	$lrms_users{$u}{freecpus} = $job_limit - $lrms_queue->{running};        
	$lrms_users{$u}{queuelength} = "$lrms_queue->{queued}";
    }
    # add users to the big tree
    $lrms_queue->{users} = \%lrms_users;
}
	      
1;
