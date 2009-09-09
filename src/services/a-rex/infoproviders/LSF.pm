package LSF;

@ISA = ('Exporter');
@EXPORT_OK = ('cluster_info',
	      'queue_info',
	      'jobs_info',
	      'users_info');
use LogUtils ( 'start_logging', 'error', 'warning', 'debug' ); 
use strict;

##########################################
# Saved private variables
##########################################

our $lsf_profile_path;
our $lsf_profile;
our $lshosts_command;
our $bhosts_command;
our $bqueues_command;
our $bqueuesl_command;
our $bjobs_command;
our $lsid_command;
our $total_cpus="0";

##########################################
# Private subs
##########################################

sub lsf_env($$){
my ($path)=shift;
$lsf_profile_path=shift;

$lsf_profile=`source $lsf_profile_path`;
$lshosts_command="$path/lshosts -w";
$bhosts_command = "$path/bhosts -w";
$bqueues_command = "$path/bqueues -w";
$bqueuesl_command = "$path/bqueues -l";
$bjobs_command = "$path/bjobs -W -w";
$lsid_command="$path/lsid";
}

sub totalcpus {
   my %lsfnodes;
   my $ncpus=0;
   if ( $total_cpus eq "0"){
      read_lsfnodes(\%lsfnodes);

      #calculate totals
      foreach my $node (keys %lsfnodes){
         if( ($lsfnodes{$node}{"node_status"} eq "ok") || ($lsfnodes{$node}{"node_status"} eq "closed_Full") || ($lsfnodes{$node}{"node_status"} eq "closed_Excl") || ($lsfnodes{$node}{"node_status"} eq "closed_Busy") || ($lsfnodes{$node}{"node_status"} eq "closed_Adm") ){
            $ncpus +=  $lsfnodes{$node}{"node_ncpus"};
         }
      }
      return $ncpus;
   }
   return $total_cpus;
}

sub read_lsfnodes ($){
    my ($hashref) =shift;

    my ($node_count) = 0;
    my ($cpu_count) = 0;

    unless (open LSFHOSTSOUTPUT,   "$lshosts_command |") {
	debug("Error in executing lshosts command: $lshosts_command");
	die "Error in executing lshosts: $lshosts_command\n";
    }

    while (my $line= <LSFHOSTSOUTPUT>) {
	if (! ($line =~ '^HOST_NAME')) {
	    chomp($line);
	    my ($nodeid,$OStype,$model,$cpuf,$ncpus,$maxmem,$maxswp)= split(" ", $line);
	    ${$hashref}{$nodeid}{"node_hostname"} = $nodeid; 
	    ${$hashref}{$nodeid}{"node_os_type"} = $OStype;  
            ${$hashref}{$nodeid}{"node_model"} = $model;     
            ${$hashref}{$nodeid}{"node_cpuf"} = $cpuf;
            ${$hashref}{$nodeid}{"node_maxmem"} = $maxmem;
            ${$hashref}{$nodeid}{"node_max_swap"} = $maxswp;

            if($ncpus  != "-") {
                ${$hashref}{$nodeid}{"node_ncpus"} = $ncpus;
            } 
	    else { 
                ${$hashref}{$nodeid}{"node_ncpus"} = 1;
            }
       }
    }
    close LSFHOSTSOUTPUT;

    unless (open LSFBHOSTSOUTPUT, "$bhosts_command |") {
    	debug("Error in executing bhosts command: $bhosts_command");
    	die "Error in executing bhosts: $bhosts_command\n";
    }

    while (my $line= <LSFBHOSTSOUTPUT>) {
    	if (! ( ($line =~ '^HOST_NAME') || ($line =~ '^My cluster') || ($line =~ '^My master') ) ) {
    	    chomp($line);
    	    # HOST_NAME  	STATUS       JL/U    MAX  NJOBS    RUN  SSUSP  USUSP	RSV
            my ($nodeid,$status,$lju,$max,$njobs,$run,$ssusp,$ususp,$rsv) = split(" ", $line);
            ${$hashref}{$nodeid}{"node_used_slots"} = $njobs;
            ${$hashref}{$nodeid}{"node_running"} = $run;
            ${$hashref}{$nodeid}{"node_suspended"} = $ssusp + $ususp;
            ${$hashref}{$nodeid}{"node_reserved"} = $rsv;
            ${$hashref}{$nodeid}{"node_status"} = $status;
        }
    }
    close LSFBHOSTSOUTPUT;
}

sub type_and_version {
  my (@lsf_version) = `$lsid_command -V 2>&1`;
  my ($type) ="LSF";
  my ($version)="0.0";
  if($lsf_version[0] =~ /^Platform/) {
    my @s = split(/ +/,$lsf_version[0]);
    $type=$s[1];
    $version=$s[2];
  }
  
  my (@result) = [$type,$version];
  return ($type,$version);
}

sub queue_info_user ($$$) {
    my ($path) = shift;
    my ($qname) = shift;
    my ($user) = shift;
    my (%lrms_queue);

    #calculate running cpus and queues available
    unless ($user eq ""){
       $user = "-u " . $user;
    }
    unless (open BQOUTPUT,  "$bqueues_command $user $qname|") {
       debug("Error in executing bqueues command: $bqueues_command $user $qname");
       die "Error in executing bqueues: $bqueues_command \n";
    }

    while (my $line= <BQOUTPUT>) {
       if (! ($line =~ '^QUEUE')) {
         chomp($line);
         my ($q_name,$q_priority,$q_status,$q_mjobs,$q_mslots,$q_mslots_proc,$q_mjob_slots_host,$q_num_jobs,$q_job_pending,$q_job_running,$q_job_suspended) = split(" ", $line);
         $lrms_queue{maxrunning} = "$q_mjobs";
         if ($q_mjobs eq "-") {
            $lrms_queue{totalcpus} = totalcpus();         
         }else{
            $lrms_queue{totalcpus} = "$q_mjobs";
         }
         $lrms_queue{maxuserrun} = "$q_mslots";
         $lrms_queue{running}= $q_job_running;
         $lrms_queue{status} = $q_status;
         $lrms_queue{queued} = $q_job_pending;
         #what is the actual number of queueable jobs?
         $lrms_queue{maxqueuable} = "$q_mjobs";
       }
    }
    close BQOUTPUT;

    $lrms_queue{defaultcput} = "";
    $lrms_queue{defaultwallt} = "";
    $lrms_queue{maxcputime} = "";
    $lrms_queue{maxwalltime} = "";

    unless (open BQOUTPUT,  "$bqueuesl_command $user $qname|") {
       debug("Error in executing bqueues command: $bqueuesl_command $user $qname");
       die "Error in executing bqueues: $bqueuesl_command \n";
    }
    my $lastline ="";
    while (my $line= <BQOUTPUT>) {
       if ( ($line =~ '^ CPULIMIT')) {
         my $line2=<BQOUTPUT>;
         chomp($line2);
         my (@mcput)= split(" ", $line2);
	 if ($lastline =~ '^DEFAULT'){
            $lrms_queue{defaultcput} = "$mcput[0]";
         } else {
            $lrms_queue{maxcputime} = "$mcput[0]";
            if ($lrms_queue{maxwalltime} == "") {
              $lrms_queue{maxwalltime} = "$mcput[0]";
            }
         }
       }
       if ( ($line =~ '^ RUNLIMIT')) {
         my $line2=<BQOUTPUT>;
         chomp($line2);
         my (@mcput)= split(" ", $line2);
	 if ($lastline =~ '^DEFAULT'){
            $lrms_queue{defaultwallt} = "$mcput[0]";
         } else {
           $lrms_queue{maxwalltime} = "$mcput[0]";
         }
       }
       $lastline = $line;
    }
    close BQOUTPUT;

    $lrms_queue{mincputime} = "0";
    $lrms_queue{minwalltime} = "0";

    return %lrms_queue;
}

sub get_jobinfo($){
   my $id = shift;
   my %job;

   unless (open BJOUTPUT,  "$bjobs_command $id|") {
       debug("Error in executing bjobs command: $bjobs_command $id");
       die "Error in executing bjobs: $bjobs_command \n";
    }

    while (my $line= <BJOUTPUT>) {
       if (! ($line =~ '^JOBID')) {
         chomp($line);
         my ($j_id,$j_user,$j_stat,$j_queue,$j_fromh,$j_exech,$j_name,$j_submittime,$j_projname,$j_cput,$j_mem,$j_swap,$j_pids,$j_start,$j_finish) = split(" ", $line);       
    $job{id} = $j_id;
    # Report one node per job. Needs improoving for multi-CPU jobs.
    $job{nodes} = [ $j_exech ];
    $job{cput} = $j_cput;
    $job{mem} = $j_mem;
    $job{start} = $j_start;
    $job{finish} = $j_finish;
    if ($j_stat eq "RUN"){
       $job{status} = "R"; 
    }
    if ($j_stat eq "PEND"){
       $job{status} = "Q"; 
    }
    if ($j_stat eq "PSUSP" || $j_stat eq "USUSP" || $j_stat eq "SSUSP"){
       $job{status} = "S"; 
    }
    if ($j_stat eq "DONE" || $j_stat eq "EXIT"){
       $job{status} = "E"; 
    }
    if ($j_stat eq "UNKWN" || $j_stat eq "WAIT" || $j_stat eq "ZOMBI"){
       $job{status} = "O"; 
    }
    
       }
    }
    close BJOUTPUT;
    
    return %job;
 }

############################################
# Public subs
#############################################

sub cluster_info ($) {
    my ($config) = shift;
    lsf_env($$config{lsf_bin_path},$$config{lsf_profile_path});

    #init
    my %lrms_cluster;
    my %lsfnodes;
    $lrms_cluster{totalcpus} = 0;
    $lrms_cluster{usedcpus} = 0;
    $lrms_cluster{queuedcpus} = 0;
    $lrms_cluster{runningjobs} = 0;
    $lrms_cluster{queuedjobs} = 0;
    my @cpudist;
    $lrms_cluster{cpudistribution} = "";
    $lrms_cluster{queue} = [];

    #lookup batch type and version
    ($lrms_cluster{lrms_type},$lrms_cluster{lrms_version}) = type_and_version();   

    #get info on nodes in cluster
    read_lsfnodes(\%lsfnodes);

    #calculate totals
    foreach my $node (keys %lsfnodes){
       if( ($lsfnodes{$node}{"node_status"} eq "ok") || ($lsfnodes{$node}{"node_status"} eq "closed_Full") || ($lsfnodes{$node}{"node_status"} eq "closed_Excl") || ($lsfnodes{$node}{"node_status"} eq "closed_Busy") || ($lsfnodes{$node}{"node_status"} eq "closed_Adm") ){
          my $ncpus =  $lsfnodes{$node}{"node_ncpus"};
	  # we use lshosts output, maybe we should use bhosts?
	  $lrms_cluster{totalcpus} += $ncpus;
          $lrms_cluster{usedcpus} += $lsfnodes{$node}{"node_used_slots"};
          $cpudist[$ncpus] += 1;
       }
    }
    #write cpu distribution string of the form: 1cpu:15 4cpu:4
    for (my $i=0; $i<=$#cpudist; $i++) {
       next unless ($cpudist[$i]);
       $lrms_cluster{cpudistribution} .= " " . $i . "cpu:" . $cpudist[$i];
    }
   
    #calculate queued cpus and queues available

    unless (open BQOUTPUT,   "$bqueues_command|") {
       debug("Error in executing bqueues command: $bqueues_command ");
       die "Error in executing bqueues: $bqueues_command \n";
    }
    my @queues;
    while (my $line= <BQOUTPUT>) {
       if (! ($line =~ '^QUEUE')) {
         chomp($line);
         my ($q_name,$q_priority,$q_status,$q_mjobs,$q_mslots,$q_mslots_proc,$q_mjob_slots_host,$q_num_jobs,$q_job_pending,$q_job_running,$q_job_suspended) = split(" ", $line);
	 #TODO: total number of jobs in queue is not equal to queued cpus.
         $lrms_cluster{queuedcpus}+=$q_num_jobs;
         $lrms_cluster{runningjobs}+=$q_job_running;
         $lrms_cluster{queuedjobs}+=$q_job_pending;
         push @queues, $q_name;
       }
    }  
    close BQOUTPUT;
    @{$lrms_cluster{queue}} = @queues;
    return %lrms_cluster;
}

sub queue_info($$){
    my ($config) = shift;
    my ($qname) = shift;
    lsf_env($$config{lsf_bin_path},$$config{lsf_profile_path});
    return queue_info_user($$config{lsf_bin_path},$qname,""); 
}

sub jobs_info ($$@) {
    my ($config) = shift;
    my ($qname) = shift;
    my ($jids) = shift;
    lsf_env($$config{lsf_bin_path},$$config{lsf_profile_path});

    my (%lrms_jobs);

    my (%job);
    my (@s);
    foreach my $id (@$jids){
       %job = get_jobinfo($id);
       $lrms_jobs{$id}{status}=$job{status};
       $lrms_jobs{$id}{nodes}=$job{nodes};
       $lrms_jobs{$id}{mem}=$job{mem};
       $lrms_jobs{$id}{cputime}=$job{cput};
       $lrms_jobs{$id}{walltime}="";

       $lrms_jobs{$id}{reqwalltime}="";
       $lrms_jobs{$id}{reqcputime}="";
       $lrms_jobs{$id}{comment}=["job started: $job{start}"];
       $lrms_jobs{$id}{rank}="";
       #TODO fix to support parallel jobs
       $lrms_jobs{$id}{cpus}=1;  
    }

    return %lrms_jobs;

}


sub users_info($$@) {
    my ($config) = shift;
    my ($qname) = shift;
    my ($accts) = shift;
    lsf_env($$config{lsf_bin_path},$$config{lsf_profile_path});

    my (%lrms_users);
    my (%queue);

    foreach my $u ( @{$accts} ) {
        %queue = queue_info_user( $$config{lsf_bin_path}, $qname, $u );
	$lrms_users{$u}{freecpus} = $queue{maxrunning}-$queue{runnning};
	$lrms_users{$u}{queuelength} = "$queue{queued}";
    }
    return %lrms_users;
}
	      
1;
