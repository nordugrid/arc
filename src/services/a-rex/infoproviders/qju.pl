#!/usr/bin/perl

use File::Basename;
use lib dirname($0);
use Getopt::Long;
use POSIX qw(ceil);
use LogUtils ( 'start_logging', 'error', 'warning', 'debug' ); 
use LRMS ( 'select_lrms',
	   'cluster_info',
	   'queue_info',
	   'jobs_info',
	   'users_info');
use Shared ( 'print_ldif_data','post_process_config');
use strict;

# Constructs queueldif, userldif and jobldif for NorduGrid Information System
#

####################################################################
# Full definitions of the infosys attributes are documented in
# "The NorduGrid/ARC Information System", 2005-05-09.
####################################################################

####################################################################
# The "output" or interface towards Grid Information system is
# defined by elements in @qen, @jen and @uen arrays. The interface
# towards LRMS is defined in %Shared::lrms_info hash
####################################################################

# From the queue
my (@qen) = ('dn',
	     'objectclass',
	     'nordugrid-queue-name',
	     'nordugrid-queue-status',
	     'nordugrid-queue-comment',
	     'nordugrid-queue-schedulingpolicy',
	     'nordugrid-queue-homogeneity',
	     'nordugrid-queue-nodecpu',
	     'nordugrid-queue-nodememory',
	     'nordugrid-queue-architecture',
	     'nordugrid-queue-opsys',
	     'nordugrid-queue-benchmark',
	     'nordugrid-queue-maxrunning',
	     'nordugrid-queue-maxqueuable',
	     'nordugrid-queue-maxuserrun',
	     'nordugrid-queue-maxcputime',
	     'nordugrid-queue-mincputime',
	     'nordugrid-queue-defaultcputime',
	     'nordugrid-queue-maxwalltime',
	     'nordugrid-queue-minwalltime',
	     'nordugrid-queue-defaultwalltime',
	     'nordugrid-queue-running',
	     'nordugrid-queue-gridrunning',
	     'nordugrid-queue-localqueued',
	     'nordugrid-queue-gridqueued',
	     'nordugrid-queue-prelrmsqueued',
	     'nordugrid-queue-totalcpus',
	     'Mds-validfrom',
	     'Mds-validto'
	     );

# Nordugrid info group
my (@gen) = ('dn',
	     'objectclass',
	     'nordugrid-info-group-name',
	     'Mds-validfrom',
	     'Mds-validto');

# From every job
my (@jen) = ('dn',
	     'objectclass',
	     'nordugrid-job-globalid',
	     'nordugrid-job-globalowner',
	     'nordugrid-job-jobname',
	     'nordugrid-job-execcluster',
	     'nordugrid-job-execqueue',
	     'nordugrid-job-executionnodes',
	     'nordugrid-job-submissionui',
	     'nordugrid-job-submissiontime',
	     'nordugrid-job-sessiondirerasetime',
	     'nordugrid-job-proxyexpirationtime',
	     'nordugrid-job-completiontime',
	     'nordugrid-job-runtimeenvironment',
	     'nordugrid-job-gmlog',
	     'nordugrid-job-clientsoftware',
	     'nordugrid-job-stdout',
	     'nordugrid-job-stderr',
	     'nordugrid-job-stdin',
	     'nordugrid-job-cpucount',
	     'nordugrid-job-reqcputime',
	     'nordugrid-job-reqwalltime',
	     'nordugrid-job-queuerank',
	     'nordugrid-job-comment',
	     'nordugrid-job-usedcputime',
	     'nordugrid-job-usedwalltime',
	     'nordugrid-job-usedmem',
	     'nordugrid-job-exitcode',
	     'nordugrid-job-errors',
	     'nordugrid-job-status',
	     'nordugrid-job-rerunable',
	     'Mds-validfrom',
	     'Mds-validto');

# From every user
my (@uen) = ('dn',
	     'objectclass',
	     'nordugrid-authuser-name',
	     'nordugrid-authuser-sn',
	     'nordugrid-authuser-freecpus',
	     'nordugrid-authuser-diskspace',
	     'nordugrid-authuser-queuelength',
	     'Mds-validfrom',
	     'Mds-validto');

########################################################
# qju.pl's (internal) data structures
########################################################

my ($totaltime) = time;

our ( %config, %gmjobs, %gm_queued, %users, %frontend_status,
      $gridrunning, $gridqueued, $queue_pendingprelrms, $queue_prelrmsqueued );

########################################################
# qju.pl's subroutines
########################################################

sub sort_by_cn{
    $a    =~ m/\/CN=([^\/]+)(\/Email)?/;
    my ($cn_a) = $1;
    $b    =~ m/\/CN=([^\/]+)(\/Email)?/;
    my ($cn_b) = $1;	
    $cn_a cmp $cn_b;
}

sub qju_parse_command_line_options() { 

    # Config defaults

    $config{ttl}                =  600;
    $config{gm_mount_point}     = "/jobs";
    $config{gm_port}            = 2811;
    $config{homogeneity}        = "True";
    $config{providerlog}        = "/var/log/grid/infoprovider.log";
    $config{defaultttl}         = "604800";
    $config{"x509_user_cert"}   = "/etc/grid-security/hostcert.pem";
    $config{ng_location}        = $ENV{NORDUGRID_LOCATION} ||= "/opt/nordugrid";
    $config{gridmap} 	 = "/etc/grid-security/grid-mapfile";

    my ($print_help);

    #Command line option "queue" refers to queue block name, the name of
    # the corresponding actual lrms queue is read into $config{name}

    GetOptions("dn:s" => \$config{dn},	     	  
	       "queue:s" => \$config{queue}, 
	       "config:s" => \$config{conf_file}, 
	       "valid-to:i" => \$config{ttl},   
	       "loglevel:i" => \$config{loglevel},
	       "help|h" => \$print_help   
	       ); 
 
    if ($print_help) { 
	print "\n  
     		script usage: 
		mandatory arguments: --dn
				     --queue
				     --config
				      
		optional arguments:  --valid-to 
				     --loglevel
		this help:	     --help\n";
	exit;
    }



    if (! ( $config{dn} and
	    $config{queue} and
	    $config{conf_file} ) ) {
	error("a command line argument is missing, see --help ");
    };
}


sub qju_read_conf_file () {

    my ($cf) = $config{conf_file};

    # Whole content of the config file
    my (%parsedconfig) = Shared::arc_conf( $config{conf_file} );

    # Copy blocks that are relevant to qju.pl
    # Better not to use cluster block values in the queue block
    my (@blocks) = ('common', 'grid-manager', 'infosys',
		    "queue/$config{queue}", 'gridftpd/jobs');
    my ($var);
    foreach my $block ( @blocks ) {
	foreach $var ( keys %{$parsedconfig{$block}} ) {
	    $config{$var} = $parsedconfig{$block}{$var};
	}
    }
}

sub qju_get_host_name () {
   if (! (exists $config{hostname})){
      chomp ($config{hostname} = `/bin/hostname -f`);
   }
}

sub read_grid_mapfile () {
    
    unless (open MAPFILE, "<$config{gridmap}") {
	error("can't open gridmapfile at $config{gridmap}");
    }   
    while(my $line = <MAPFILE>) {
	chomp($line);
	if ( $line =~ m/\"([^\"]+)\"\s+(\S+)/ ) {
	    $users{$1}{localid} = $2; 	
	}
    }
    close MAPFILE;

    return %users;
}

sub process_status (@) {

    my (@names) = @_;

    # return the PIDs of named processes, or zero if not running

    my (%ids);

    foreach my $name ( @names ) {
	my ($id) = `pgrep $name`;
	if ( $? != 0 ) {
	    ( my ($id) = `ps -C $name --no-heading`) =~ s/(\d+).*/$1/;
	    if ($? != 0) {
		warning("Failed checking the $name process");}}
	if ( $id ) {
	    $ids{$name} = $id;}
	else {
	    $ids{$name} =  0;}
	chomp $ids{$name};
    }
    return %ids;
}


sub gmjobs_info () {
    
    my ($controldir) = $config{controldir};

    my (%gmjobs);

    my (%gm_queued);
    
    my (@gmkeys) = ( "lrms",
		     "queue",
		     "localid",
		     "args",
		     "subject",
		     "starttime",
		     "notify",
		     "rerun",
		     "downloads",
		     "uploads",
		     "jobname",
		     "gmlog",
		     "cleanuptime",
		     "delegexpiretime",
		     "clientname",
		     "clientsoftware",
		     "sessiondir",
		     "diskspace",
		     "failedstate",
		     "exitcode",
		     "stdin",
		     "comment",
		     "mem",
		     "WallTime",
		     "CpuTime",
		     "reqwalltime",
		     "reqcputime",
		     "completiontime",
		     "runtimeenvironment");
    
    my $queue_pendingprelrms=0;
    my $queue_prelrmsqueued=0;
    my @gmqueued_states = ("ACCEPTED","PENDING:ACCEPTED","PREPARING","PENDING:PREPARING","SUBMIT"); 
    my @gmpendingprelrms_states =("PENDING:ACCEPTED","PENDING:PREPARING" );

    # read the list of jobs from the jobdir and create the @gridmanager_jobs 
    # the @gridmanager_jobs contains the IDs from the job.ID.status
    #

    unless (opendir JOBDIR,  $controldir ) {
	error("can't access the directory of the jobstatus files at $config{controldir}");
    }

    my (@allfiles) = readdir JOBDIR;
    @allfiles= grep /\.status/, @allfiles;
    closedir JOBDIR;
    my (@gridmanager_jobs) = map {$_=~m/job\.(.+)\.status/; $_=$1;} @allfiles;

    # read the gridmanager jobinfo into a hash of hashes %gmjobs 
    # filter out jobs not belonging to this $queue
    # fill the %gm_queued{SN} with number of gm_queued jobs for every grid user
    # count the prelrmsqueued, pendingprelrms grid jobs belonging to this queue

    foreach my $ID (@gridmanager_jobs) {
	my $gmjob_local       = $controldir."/job.".$ID.".local";
	my $gmjob_status      = $controldir."/job.".$ID.".status";
	my $gmjob_failed      = $controldir."/job.".$ID.".failed";  
	my $gmjob_description = $controldir."/job.".$ID.".description"; 
	my $gmjob_diag        = $controldir."/job.".$ID.".diag"; 
	
	unless ( open (GMJOB_LOCAL, "<$gmjob_local") ) {
	    warning( "Can't read the $gmjob_local jobfile, skipping..." );
            next;
	}
	my @local_allines = <GMJOB_LOCAL>;
	
	# Check that this job belongs to correct queue (block).  
	# Accepted values are
	# 1. /^queue=/ line missing completely
	# 2. /^queue=$/ 
	# 3. /^queue=$config{queue}$/

	my ( @queue_line ) = grep /^queue=/, @local_allines;
        
	unless ( scalar(@queue_line) == 0 ) {
	    chomp $queue_line[0];
	    unless ( ( $queue_line[0] eq "queue=" )
		     or ( $queue_line[0] eq "queue=$config{queue}" )
		     ) {
		close GMJOB_LOCAL;
		next;
	    }
	}
	
	# parse the content of the job.ID.local into the %gmjobs hash 
	foreach my $line (@local_allines) {		  
	    $line=~m/^(\w+)=(.+)$/; 		   	
	    $gmjobs{$ID}{$1}=$2;    
	}	 
	close GMJOB_LOCAL;
	
	# read the job.ID.status into "status"
	open (GMJOB_STATUS, "<$gmjob_status");
	my @status_allines=<GMJOB_STATUS>;    
	chomp (my $job_status_firstline=$status_allines[0]);    
	$gmjobs{$ID}{"status"}= $job_status_firstline;
	close GMJOB_STATUS;    

        # set the job_gridowner of the job (read from the job.id.local)
        # which is used as the key of the %gm_queued
        my $job_gridowner = $gmjobs{$ID}{"subject"};

        # count the gm_queued jobs per grid users (SNs) and the total
        if ( grep /^$gmjobs{$ID}{"status"}$/, @gmqueued_states ) {
	    $gm_queued{$job_gridowner}++;
            $queue_prelrmsqueued++;
        }

        # count the GM PRE-LRMS pending jobs
        if ( grep /^$gmjobs{$ID}{"status"}$/, @gmpendingprelrms_states ) {
           $queue_pendingprelrms++;      
        }
	
	#Skip the remaining files if the jobstate "DELETED"
	next if ( $gmjobs{$ID}{"status"} eq "DELETED");

	# Grid Manager job state mappings to Infosys job states

	my (%map_always) = ( 'ACCEPTED' => 'ACCEPTING',
			     'PENDING:ACCEPTED' => 'ACCEPTED',
			     'PENDING:PREPARING' => 'PREPARED',
			     'PENDING:INLRMS' => 'EXECUTED',
			     'CANCELING' => 'KILLING');

	my (%map_if_gm_up) = ( 'SUBMIT' => 'SUBMITTING');

	my (%map_if_gm_down) = ( 'PREPARING' => 'ACCEPTED',
				 'FINISHING' => 'EXECUTED',
				 'SUBMIT' => 'PREPARED');
	
	if ( grep ( /^$gmjobs{$ID}{status}$/, keys %map_always ) ) {
	    $gmjobs{$ID}{status} = $map_always{$gmjobs{$ID}{status}};
	}
	if ( grep ( /^$gmjobs{$ID}{status}$/, keys %map_if_gm_up ) and
	     $frontend_status{'grid-manager'} ) {
	    $gmjobs{$ID}{status} = $map_if_gm_up{$gmjobs{$ID}{status}};
	}
	if ( grep( /^$gmjobs{$ID}{status}$/, keys %map_if_gm_down ) and
	     ! $frontend_status{'grid-manager'} ) {
	    $gmjobs{$ID}{status} = $map_if_gm_down{$gmjobs{$ID}{status}};
	}
	
	# Comes the splitting of the terminal job state
	# check for job failure, (job.ID.failed )   "errors"
	
	if (-e $gmjob_failed) {
	    open (GMJOB_FAILED, "<$gmjob_failed");
	    my @failed_allines=<GMJOB_FAILED>;    
	    chomp  @failed_allines;
	    my ($temp_errorstring) = join " ", @failed_allines;
	    $temp_errorstring =~ s/\s+$//;     
	    if (length $temp_errorstring >= 87) {
		$temp_errorstring = substr ($temp_errorstring, 0, 87);
	    }       
	    $gmjobs{$ID}{"errors"}="$temp_errorstring";
	    close GMJOB_FAILED;
	}

	
	if ($gmjobs{$ID}{"status"} eq "FINISHED") {

	    #terminal job state mapping

	    if ( defined $gmjobs{$ID}{"errors"} ) {
		if ($gmjobs{$ID}{"errors"} =~
		    /User requested to cancel the job/) {
		    $gmjobs{$ID}{"status"} = "KILLED";
		} elsif ( defined $gmjobs{$ID}{"errors"} ) {
		    $gmjobs{$ID}{"status"} = "FAILED";
		}
	    }

	    #job-completiontime

	    my @file_stat = stat $gmjob_status;
	    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)
		= gmtime ($file_stat[9]);
	    my $file_time = sprintf ( "%4d%02d%02d%02d%02d%02d%1s",
				      $year+1900,$mon+1,$mday,$hour,
				      $min,$sec,"Z");    
	    $gmjobs{$ID}{"completiontime"} = "$file_time";	
	}
	
	# Make a job comment if the GM is dead

	unless ($frontend_status{'grid-manager'}) {
	    $gmjobs{$ID}{comment} = "GM: The grid-manager is down";
	}         
	
	# do something with the job.ID.description file
	# read the stdin,stdout,stderr

	open (GMJOB_DESC, "<$gmjob_description");
	my @desc_allines=<GMJOB_DESC>;    
	chomp (my $rsl_string=$desc_allines[0]);      
	if ($rsl_string=~m/"stdin"\s+=\s+"(\S+)"/) {
	    $gmjobs{$ID}{"stdin"}=$1;
	}
	if ($rsl_string=~m/"stdout"\s+=\s+"(\S+)"/) {
	    $gmjobs{$ID}{"stdout"}=$1;	
	}
	if ($rsl_string=~m/"stderr"\s+=\s+"(\S+)"/) {
	    $gmjobs{$ID}{"stderr"}=$1;		
	}
	if ($rsl_string=~m/"count"\s+=\s+"(\S+)"/) {
	    $gmjobs{$ID}{"count"}=$1;	
	}
	else {
	    $gmjobs{$ID}{"count"}="1";
	}
	if ($rsl_string=~m/"cputime"\s+=\s+"(\S+)"/i) {
	    my $reqcputime_sec=$1;
	    $gmjobs{$ID}{"reqcputime"}= int $reqcputime_sec/60;	
	}
	if ($rsl_string=~m/"walltime"\s+=\s+"(\S+)"/i) {
	    my $reqwalltime_sec=$1;
	    $gmjobs{$ID}{"reqwalltime"}= int $reqwalltime_sec/60;	
	}
    
	my @runtimes = ();
	my $rsl_tail = $rsl_string;
	while ($rsl_tail =~ m/\("runtimeenvironment"\s+=\s+"([^\)]+)"/i) {
	    push(@runtimes, $1);
	    $rsl_tail = $'; # what's left after the match
	}
	$gmjobs{$ID}{"runtimeenvironment"} = \@runtimes;

	close GMJOB_DESC;
	
	#read the job.ID.diag file        

	if (-s $gmjob_diag) {
	    open (GMJOB_DIAG, "<$gmjob_diag");
	    my ($kerneltime) = "";
	    my ($usertime) = "";
	    while ( my $line = <GMJOB_DIAG>) {	       
		$line=~m/nodename=(\S+)/ and
		    $gmjobs{$ID}{"exec_host"}.=$1."+";
		$line=~m/WallTime=(\d+)\./ and
		    $gmjobs{$ID}{"WallTime"} = ceil($1/60);		 
		$line=~m/exitcode=(\d+)/ and
		    $gmjobs{$ID}{"exitcode"}="exitcode".$1;
		$line=~m/UsedMemory=(\d+)kB/ and   
		    $gmjobs{$ID}{"UsedMem"} = ceil($1);
		$line=~m/KernelTime=(\d+)\./ and 
		    $kerneltime=$1;
		$line=~m/UserTime=(\d+)\./  and 
		    $usertime=$1;  			       
	    }
	    if ($kerneltime ne "" and $usertime ne "") {
		$gmjobs{$ID}{"CpuTime"}= ceil( ($kerneltime + $usertime)/ 60);
	    } else {
		$gmjobs{$ID}{"CpuTime"} = "0";
	    }
	    close GMJOB_DIAG;	   	       
	} 	 

	foreach my $k (@gmkeys) {
	    if (! $gmjobs{$ID}{$k}) {
		$gmjobs{$ID}{$k} = "";
	    }
	}    
    }
   
    return (\%gmjobs, \%gm_queued, \$queue_pendingprelrms, \$queue_prelrmsqueued);
}

sub count_grid_jobs_in_lrms ($) {
    my $lrmsjobs = shift;
    my $gridrunning = 0;
    my $gridqueued  = 0;
    my %local2gm;
    $local2gm{$gmjobs{$_}{localid}} = $_ for keys %gmjobs;
    foreach my $jid (keys %$lrmsjobs) {
        next unless exists $local2gm{$jid}; # not a grid job
        next unless exists $$lrmsjobs{$jid}; # probably executed
        next unless exists $$lrmsjobs{$jid}{status}; # probably executed
        my $status = $$lrmsjobs{$jid}{status};
        next if $status eq ""; # probably executed
        if ($status eq 'R' || $status eq 'S') {
            $gridrunning++;
        } else {
            $gridqueued++;
        }
    }
    return $gridrunning, $gridqueued;
}



sub diskspace ($) {
    my ($path) = shift;
    my ($space);
    if ( -d "$path") {
	# check if on afs
	if ($path =~ /\/afs\//) {
	    $space =`fs listquota $path 2>/dev/null`;	
	    if ($? != 0) {
		warning("Failed checking diskspace for $path");
	    }
	    if ($space) {
		$space =~ /\n\S+\s+(\d+)\s+(\d+)\s+\d+%\s+\d+%/;
		$space = int (($1 - $2)/1024);
	    }
	    # "ordinary" disk
	} else {
	    $space =`df -k -P $path 2>/dev/null`;
	    if ($? != 0) {
		warning("Failed checking diskspace for $path");
	    }
	    if ($space) {
		$space =~ /\n\S+\s+\d+\s+\d+\s+(\d+)\s+\d+/;
		$space=int $1/1024;
	    }
	}
    } else {
	error("sessiondir $path was not found");
    }
    return $space;
}

sub grid_diskspace (@) {
    my (@localids) = @_;

    my (%gridarea, %capacity);

    my ($commonspace) = diskspace($config{sessiondir});
    foreach my $u (@localids) {

        my ($name,$passwd,$uid,$gid,$quota,$comment,$gcos,$home,$shell,$expire) = getpwnam($u);

	if ( $name eq "" ) {
	    error("$u is not listed in the passwd file");
	}
	if ($config{"sessiondir"} =~ /^\s*\*\s*$/) {
	    $gridarea{$u} = $home."/.jobs";	 
	    $capacity{$u} = diskspace($gridarea{$u});
 	} else {
	    $gridarea{$u} = $config{sessiondir};
	    $capacity{$u} = $commonspace;
	}
    }

    foreach my $sn (keys %users) {
	$users{$sn}{gridarea} = $gridarea{$users{$sn}{localid}};
	$users{$sn}{diskspace} = $capacity{$users{$sn}{localid}};
    }
}


sub ldif_queue (%) {
    my ($p) = shift;
    my (%lrms_queue) = %{$p};

    my (%c);

    $c{'dn'} = [ "$config{dn}" ];
    $c{'objectclass'} = [ 'Mds', 'nordugrid-queue' ];
    $c{'nordugrid-queue-name'} = [ "$config{queue}" ];
    
    if ($config{allownew} eq "no") {
	$c{'nordugrid-queue-status'} =
	    [ 'inactive, grid-manager does not accept new jobs' ];
    } elsif (not $frontend_status{'grid-manager'}) {
	$c{'nordugrid-queue-status'} = [ 'inactive, grid-manager is down' ];   
    } elsif  (not ($frontend_status{'gridftpd'}) )  {
	$c{'nordugrid-queue-status'} = [ 'inactive, gridftpd is down' ];  
    } elsif ( $lrms_queue{status} < 0 ) {
	$c{'nordugrid-queue-status'} =
	    [ 'inactive, LRMS interface returns negative status' ];
    } else {
	$c{'nordugrid-queue-status'} = [ 'active' ];}

    if ( defined $config{comment} ) {
	$c{'nordugrid-queue-comment'} = [ "$config{comment}" ];
    }
    $c{'nordugrid-queue-schedulingpolicy'} = [ "$config{scheduling_policy}" ];
    $c{'nordugrid-queue-homogeneity'} = [ "$config{homogeneity}" ];
    $c{'nordugrid-queue-nodecpu'} = [ "$config{nodecpu}" ];
    $c{'nordugrid-queue-nodememory'} = [ "$config{nodememory}" ];
    if ( defined $config{architecture}) {
	$c{'nordugrid-queue-architecture'} = [ "$config{architecture}" ];
    }
    $c{'nordugrid-queue-opsys'} = [ split /\[separator\]/, $config{opsys} ];
    
    $c{'nordugrid-queue-benchmark'} = [];

    if ( defined $config{benchmark} ) {
	foreach my $listentry (split /\[separator\]/, $config{benchmark}) {
	    my ($bench_name,$bench_value) = split(/\s+/, $listentry);
	    push @{$c{'nordugrid-queue-benchmark'}},
	    "$bench_name \@ $bench_value";}
    }
    
    $c{'nordugrid-queue-maxrunning'} =  [ "$lrms_queue{maxrunning}" ];
    $c{'nordugrid-queue-maxqueuable'} = [ "$lrms_queue{maxqueuable}" ];
    $c{'nordugrid-queue-maxuserrun'} =  [ "$lrms_queue{maxuserrun}" ];
    $c{'nordugrid-queue-maxcputime'} =  [ "$lrms_queue{maxcputime}" ];
    $c{'nordugrid-queue-mincputime'} =  [ "$lrms_queue{mincputime}" ];
    $c{'nordugrid-queue-defaultcputime'} =
	[ "$lrms_queue{defaultcput}" ];
    $c{'nordugrid-queue-maxwalltime'} =  [ "$lrms_queue{maxwalltime}" ];
    $c{'nordugrid-queue-minwalltime'} =  [ "$lrms_queue{minwalltime}" ];
    $c{'nordugrid-queue-defaultwalltime'} =
	[ "$lrms_queue{defaultwallt}" ];
    $c{'nordugrid-queue-running'} = [ "$lrms_queue{running}" ];
    $c{'nordugrid-queue-gridrunning'} = [ "$gridrunning" ];   
    $c{'nordugrid-queue-gridqueued'} = [ "$gridqueued" ];    
    $c{'nordugrid-queue-localqueued'} = [ ($lrms_queue{queued} - $gridqueued) ];   
    
    if (defined $queue_prelrmsqueued) {
        $c{'nordugrid-queue-prelrmsqueued'} = [ "$queue_prelrmsqueued" ];
    }
    if ($config{queue_node_string}) {
	$c{'nordugrid-queue-totalcpus'} = [ "$lrms_queue{totalcpus}" ];
    } 
    elsif ( $config{totalcpus} ) {
	$c{'nordugrid-queue-totalcpus'} = [ "$config{totalcpus}" ];
    }
    elsif ( $lrms_queue{totalcpus} ) {
	$c{'nordugrid-queue-totalcpus'} = [ "$lrms_queue{totalcpus}" ];      
    }	
    
    my ( $valid_from, $valid_to ) =
	Shared::mds_valid($config{ttl});
    $c{'Mds-validfrom'} = [ "$valid_from" ];
    $c{'Mds-validto'} = [ "$valid_to" ];
    
    return %c;

}

sub ldif_jobs_group_entry() {
    
    my (%c);
    
    $c{dn} = [ "nordugrid-info-group-name=jobs, $config{dn}"];
    $c{objectclass} = [ 'Mds', 'nordugrid-info-group' ];
    $c{'nordugrid-info-group-name'} = [ 'jobs' ];
    my ( $valid_from, $valid_to ) =
	Shared::mds_valid($config{ttl});
    $c{'Mds-validfrom'} = [ "$valid_from" ];
    $c{'Mds-validto'} = [ "$valid_to" ];

    return %c;
}


sub ldif_job ($%$%) {
    my ($gmid) = shift;
    my ($gmjob) = shift;
    my ($lrmsid) = shift;
    my ($lrmsjob) = shift;

    my (%c);

    my $nordugrid_globalid="gsiftp://".$config{hostname}.
	":".$config{gm_port}.$config{gm_mount_point}."/".$gmid; 
    $c{'dn'} = [ "nordugrid-job-globalid=$nordugrid_globalid, nordugrid-info-group-name=jobs, $config{dn}" ];
    
    $c{'objectclass'} = [ 'Mds', 'nordugrid-job'];
    
    $c{'nordugrid-job-globalid'} = [ "$nordugrid_globalid" ];
    
    $c{'nordugrid-job-globalowner'} = [ "$gmjob->{subject}" ];
    
    $c{'nordugrid-job-jobname'} = [ "$gmjob->{jobname}" ];
    
    $c{'nordugrid-job-submissiontime'} = [ "$gmjob->{starttime}" ];
    
    $c{'nordugrid-job-execcluster'} = [ "$config{hostname}" ];
    
    $c{'nordugrid-job-execqueue'} = [ "$config{queue}" ];
    
    if ( not $gmjob->{count}) {
	$gmjob->{count} = 1; }
    $c{'nordugrid-job-cpucount'} = [ "$gmjob->{count}" ];

    if ($gmjob->{"errors"}) {
	$c{'nordugrid-job-errors'} = [ "$gmjob->{errors}" ];
    } else {
	$c{'nordugrid-job-errors'} = [ "" ];
    }

    if ($gmjob->{"exitcode"}) {
	$gmjob->{"exitcode"}=~ s/^exitcode//;}
    $c{'nordugrid-job-exitcode'} = [ "$gmjob->{exitcode}" ];

    $c{'nordugrid-job-sessiondirerasetime'} =
	[ "$gmjob->{cleanuptime}" ];

    my (@io) = ('stdin','stdout','stderr','gmlog'); 
    foreach my $k (@io) {
	if ( defined $gmjob->{$k} ) {
	    $c{"nordugrid-job-$k"} = [ "$gmjob->{$k}" ];
	}
    }

    $c{'nordugrid-job-runtimeenvironment'} = $gmjob->{"runtimeenvironment"};

    $c{'nordugrid-job-submissionui'} = [ "$gmjob->{clientname}" ];

    $c{'nordugrid-job-clientsoftware'} =
	[ "$gmjob->{clientsoftware}" ];

    $c{'nordugrid-job-proxyexpirationtime'} =
	[ "$gmjob->{delegexpiretime}" ];

    if ( $gmjob->{"status"} eq "FAILED" ) {
       if ( $gmjob->{"failedstate"} ) {
           $c{'nordugrid-job-rerunable'} = [ "$gmjob->{failedstate}" ];
       }
       else {
           $c{'nordugrid-job-rerunable'} = [ 'none' ];
       }
    }
    
    $c{'nordugrid-job-comment'} = [ "$gmjob->{comment}" ];

    if ($gmjob->{"status"} eq "INLRMS") {

	$c{'nordugrid-job-usedmem'} = [ "$lrmsjob->{mem}" ];
	$c{'nordugrid-job-usedwalltime'} = [ "$lrmsjob->{walltime}" ];
	$c{'nordugrid-job-usedcputime'} = [ "$lrmsjob->{cputime}" ];
	$c{'nordugrid-job-reqwalltime'} = [ "$lrmsjob->{reqwalltime}" ];
	$c{'nordugrid-job-reqcputime'} =  [ "$lrmsjob->{reqcputime}" ];
	$c{'nordugrid-job-executionnodes'} = $lrmsjob->{nodes} if @{$lrmsjob->{nodes}};

	# LRMS-dependent attributes taken from LRMS when the job
	# is in state 'INLRMS'

	#nordugrid-job-status
	# take care of the GM latency, check if the job is in LRMS
	# according to both GM and LRMS, GM might think the job 
	# is still in LRMS while the job have already left LRMS		     

	if ($lrmsjob->{status}) {
	    $c{'nordugrid-job-status'} = [ "INLRMS: $lrmsjob->{status}" ];
	    if ( $c{'nordugrid-job-comment'}[0] eq "" ) {
		@{ $c{'nordugrid-job-comment'} } = @{ $lrmsjob->{comment} };
	    } else {
		push ( @{ $c{'nordugrid-job-comment'} },
		       @{ $lrmsjob->{comment} } );
	    }
	} else {
	    $c{'nordugrid-job-status'} = [ 'EXECUTED' ];
	}      

	$c{'nordugrid-job-queuerank'} = [ "$lrmsjob->{rank}" ];

    } else {  

	# LRMS-dependent attributes taken from GM when
	# the job has passed the 'INLRMS' state

	$c{'nordugrid-job-status'} = [ "$gmjob->{status}" ];
	
	if ( $gmjob->{WallTime} ) {
	   $c{'nordugrid-job-usedwalltime'} = ["$gmjob->{WallTime}"];	  
	}	
        else {
	   $c{'nordugrid-job-usedwalltime'} = ["0"];
	}
	if ( $gmjob->{CpuTime} ) {
	   $c{'nordugrid-job-usedcputime'} = ["$gmjob->{CpuTime}"];	
	}
        else {
	   $c{'nordugrid-job-usedcputime'} = ["0"];
	}
	if ( $gmjob->{exec_host} ) {
	   $c{'nordugrid-job-executionnodes'} = [ split /\+/, $gmjob->{exec_host} ];
	}
	if ( $gmjob->{UsedMem} ) {
	   $c{'nordugrid-job-usedmem'} = ["$gmjob->{UsedMem}"];
	}
	if ( $gmjob->{reqcputime} ) {
	   $c{'nordugrid-job-reqcputime'} = ["$gmjob->{reqcputime}"];
	}
	if ( $gmjob->{reqwalltime} ) {
	   $c{'nordugrid-job-reqwalltime'} = ["$gmjob->{reqwalltime}"];
	}
	$c{'nordugrid-job-completiontime'} =
	    [ "$gmjob->{completiontime}" ];
    }  	 

    my ( $valid_from, $valid_to ) =
	Shared::mds_valid($config{ttl});
    $c{'Mds-validfrom'} = [ "$valid_from" ];
    $c{'Mds-validto'} = [ "$valid_to" ];

    return %c;
}


sub ldif_users_group_entry() {
    
    my (%c);
    
    $c{dn} = [ "nordugrid-info-group-name=users, $config{dn}" ];
    $c{objectclass} = [ 'Mds', 'nordugrid-info-group' ];
    $c{'nordugrid-info-group-name'} = [ 'users' ];
    my ( $valid_from, $valid_to ) =
	Shared::mds_valid($config{ttl});
    $c{'Mds-validfrom'} = [ "$valid_from" ];
    $c{'Mds-validto'} = [ "$valid_to" ];

    return %c;
}

    
sub ldif_user ($$%) {
    
    my ($usernumber) = shift;
    my ($sn) = shift;
    my ($lrms_users) = shift;

    my (%c);

    #nordugrid-authuser-name= CN from the SN  + unique number 
    $sn =~ m/\/CN=([^\/]+)(\/Email)?/;
    my ($cn) = $1;
    
    $c{dn} = [ "nordugrid-authuser-name=${cn}...$usernumber, nordugrid-info-group-name=users, $config{dn}" ];
    $c{objectclass} = [ 'Mds', 'nordugrid-authuser' ];
    $c{'nordugrid-authuser-name'} = [ "${cn}...$usernumber" ];
    $c{'nordugrid-authuser-sn'} = [ "$sn" ];
    
    #nordugrid-authuser-diskspace
    $c{'nordugrid-authuser-diskspace'} = [ "$users{$sn}{diskspace}" ];
    
    #nordugrid-authuser-freecpus
    if ( $queue_pendingprelrms ) {    
       $c{'nordugrid-authuser-freecpus'} = [ "0" ];
    }
    elsif ( $$lrms_users{$users{$sn}{localid}}{freecpus} ) {
       $c{'nordugrid-authuser-freecpus'} =
	 [ "$$lrms_users{$users{$sn}{localid}}{freecpus}" ];
    }
    else {
       $c{'nordugrid-authuser-freecpus'} = [ "0" ];
    }   
    #nordugrid-authuser-queuelength  
    $c{'nordugrid-authuser-queuelength'} =
	[ ( $gm_queued{$sn} + $$lrms_users{$users{$sn}{localid}}{queuelength} ) ];   
    
    my ( $valid_from, $valid_to ) =
	Shared::mds_valid($config{ttl});
    $c{'Mds-validfrom'} = [ "$valid_from" ];
    $c{'Mds-validto'} = [ "$valid_to" ];
    
    return %c;
}


###############################################
#  main
###############################################

# --- Digest /etc/arc.conf ----------------------

qju_parse_command_line_options();

qju_read_conf_file();

qju_get_host_name();

post_process_config(\%config);

start_logging( $config{provider_loglevel}, $config{providerlog} );

# --- Select LRMS and check if services are running ---

select_lrms( \%config );

my (@checklist) = ('grid-manager', 'gridftpd' );
%frontend_status = process_status ( @checklist );

# --- Queue info from lrms ----------------------------------------

my ($timing) = time;

my (%lrms_queue) = queue_info( \%config, $config{queue} );

$timing = time - $timing;
debug("LRMS queue_info timing: $timing");

# --- Jobs info from grid-manager -----------------

$timing = time;

(my $gmjobsref, my $gm_queuedref, my $pendingprelrmsref, my $prelrmsqueuedref) = gmjobs_info();

%gmjobs = %{$gmjobsref};

%gm_queued = %{$gm_queuedref};

$queue_pendingprelrms = ${$pendingprelrmsref};

$queue_prelrmsqueued = ${$prelrmsqueuedref};


$timing = time - $timing;
debug("gmjobs_info timing: $timing");

# --- Jobs info from lrms  -----------------

$timing = time;

my (@lrms_jids) = map { $gmjobs{$_}{localid} } ( grep { $gmjobs{$_}{status} =~ /INLRMS/ } keys %gmjobs );

my (%lrms_jobs) = jobs_info( \%config,
			     $config{queue},
			     \@lrms_jids);

$timing = time - $timing;
debug("LRMS jobs_info timing: $timing");

# --- The number of running and queued grid jobs in lrms ---

( $gridrunning, $gridqueued ) = count_grid_jobs_in_lrms( \%lrms_jobs );

# --- User info from grid-mapfile and filesystem goes to %users -------

# Due to common many to one mapping from grid identities to local
# unix accounts different queries here are indexed with localids

$timing = time;

read_grid_mapfile();
my (@localids);
foreach my $sn ( keys %users) {
    my ($a) = $users{$sn}{localid};
    if ( ! grep /^$a$/, @localids) {push @localids, $a};
}

grid_diskspace(@localids);

$timing = time - $timing;
debug("User info timing: $timing");

# --- User info from lrms -------

$timing = time;

my (%lrms_users) = users_info( \%config,
			       $config{queue},
			       \@localids );

$timing = time - $timing;
debug("lrms_users timing: $timing");


############################
# start printing ldif data #
############################

$timing = time;

# queue

my (%ldif_queue_data) = ldif_queue( \%lrms_queue );
print_ldif_data(\@qen,\%ldif_queue_data);

# Jobs

# The nordugrid-info-group=jobs entry
my (%ldif_jobs_group_entry_data) = ldif_jobs_group_entry();
print_ldif_data( \@gen, \%ldif_jobs_group_entry_data );

foreach my $gmid (sort { $gmjobs{$b}{"starttime"} cmp
			     $gmjobs{$a}{"starttime"} } keys %gmjobs) {

    my ($lrmsid) = $gmjobs{$gmid}{localid};
    my (%ldif_job_data) = ldif_job( $gmid,   \%{$gmjobs{$gmid}},
				    $lrmsid, \%{$lrms_jobs{$lrmsid}}
				    );
    print_ldif_data(\@jen,\%ldif_job_data);
}

# Users

# The nordugrid-info-group=users entry 
my (%ldif_users_group_entry_data) = ldif_users_group_entry();
print_ldif_data( \@gen, \%ldif_users_group_entry_data );

# create the user entries from the LRMS-authorized members of
# the %users hash

my $usernumber = 0;
foreach my $sn (sort sort_by_cn keys %users) {
    # check if there is an "acl list" of authorized LRMS users
    # and if the mapping of the grid user is on this list
    if ( @main::acl_users and !(grep /$users{$sn}/, @main::acl_users)) {
	#   print "we skip $users{$sn} \n";
      next;
    }
    $usernumber++;
    my (%ldif_user_data) = ldif_user( $usernumber, $sn, \%lrms_users );
    print_ldif_data( \@uen, \%ldif_user_data );
}

$timing = time - $timing;
debug("ldif output timing: $timing");

$totaltime = time - $totaltime;
debug("TOTALTIME in qju.pl: $totaltime");

exit;
