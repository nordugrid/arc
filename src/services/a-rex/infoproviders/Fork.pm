package Fork;

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
use POSIX qw(ceil floor);
use Sys::Hostname;
our @ISA = ('Exporter');
our @EXPORT_OK = ('cluster_info',
	      'queue_info',
	      'jobs_info',
	      'users_info');
use LogUtils ( 'start_logging', 'error', 'warning', 'debug' ); 

##########################################
# Saved private variables
##########################################

our (%lrms_queue);
our $running = undef; # total running jobs in a queue

# the queue passed in the latest call to queue_info, jobs_info or users_info
my $currentqueue = undef;

# Resets queue-specific global variables if
# the queue has changed since the last call
sub init_globals($) {
    my $qname = shift;
    if (not defined $currentqueue or $currentqueue ne $qname) {
        $currentqueue = $qname;
        %lrms_queue = ();
        $running = undef;
    }
}

##########################################
# Private subs
##########################################

sub cpu_threads_cores_sockets {

    my $nsockets; # total number of physical cpu sockets
    my $ncores;   # total number of cpu cores
    my $nthreads; # total number of hardware execution threads

    if (-f "/proc/cpuinfo") {
        # Linux variant

        my %sockets; # cpu socket IDs
        my %cores;   # cpu core IDs

        open (CPUINFO, "</proc/cpuinfo")
            or warning("Failed opening /proc/cpuinfo: $!");
        while ( my $line = <CPUINFO> ) {
            if ($line=~/^processor\s*:\s+(\d+)$/) {
                ++$nthreads;
            } elsif ($line=~/^physical id\s*:\s+(\d+)$/) {
                ++$sockets{$1};
            } elsif ($line=~/^core id\s*:\s+(\d+)$/) {
                ++$cores{$1};
            }
        }
        close CPUINFO;

        # count total cpu cores and sockets
        $ncores = scalar keys %cores;
        $nsockets = scalar keys %sockets;

        if ($nthreads) {
            # if /proc/cpuinfo does not provide socket and core IDs,
            # assume every thread represents a separate cpu
            $ncores = $nthreads unless $ncores;
            $nsockets = $nthreads unless $nsockets;
        } else {
            warning("Failed parsing /proc/cpuinfo");
        }

    } elsif (-x "/usr/sbin/system_profiler") {
        # OS X

        my @lines = `system_profiler SPHardwareDataType`;
        warning("Failed running system_profiler: $!") if $?;
        for my $line ( @lines ) {
            if ($line =~ /Number Of Processors:\s*(.+)/) {
                $nsockets = $1;
            } elsif ($line =~ /Total Number Of Cores:\s*(.+)/) {
                $ncores = $1;
                $nthreads = $1; # Assume 1 execution thread per core
            }
        }
        unless ($nsockets and $ncores) {
            warning("Failed parsing output of system_profiler");
        }

    } elsif (-x "/usr/bin/kstat" ) {
        # Solaris

        my %chips;
        eval {
            require Sun::Solaris::Kstat;
            my $ks = Sun::Solaris::Kstat->new();
            my $cpuinfo = $ks->{cpu_info};
            die "key not found: cpu_info" unless defined $cpuinfo;
            for my $id (keys %$cpuinfo) {
               my $info = $cpuinfo->{$id}{"cpu_info$id"};
               die "key not found: cpu_info$id" unless defined $info;
               $chips{$info->{chip_id}}++;
               $nthreads++;
            }
        };
        if ($@) {
            error("Failed running module Sun::Solaris::Kstat: $@");
        }
        # assume each core is in a separate socket
        $nsockets = $ncores = scalar keys %chips;

    } else {
        warning("Cannot query CPU info: unsupported operating system");
    }

    return ($nthreads,$ncores,$nsockets);
}

# Produces stats for all processes on the system

sub process_info() {
    my $command = "ps -e -o ppid,pid,vsz,time,etime,user,comm";
    my @pslines = `$command`;
    if ($? != 0) {
        warning("Failed running (non-zero exit status): $command");
        return ();
    }
    shift @pslines; # drop header line

    my @procinfo;
    for my $line (@pslines) {
        my ($ppid,$pid,$vsize,$cputime,$etime,$user,$comm) = split ' ', $line, 7;

        # matches time formats like: 21:29.44, 12:21:29, 3-12:21:29
        if ($cputime =~ /^(?:(?:(\d+)-)?(\d+):)?(\d+):(\d\d(?:\.\d+)?)$/) {
            my ($days,$hours,$minutes,$seconds) = (($1||0), ($2||0), $3, $4);
            $cputime = $seconds + 60*($minutes + 60*($hours + 24*$days));
        } else {
            warning("Invalid cputime: $cputime");
            $cputime = 0;
        }
        # matches time formats like: 21:29.44, 12:21:29, 3-12:21:29
        if ($etime =~ /^(?:(?:(\d+)-)?(\d+):)?(\d+):(\d\d(?:\.\d+)?)$/) {
            my ($days,$hours,$minutes,$seconds) = (($1||0), ($2||0), $3, $4);
            $etime = $seconds + 60*($minutes + 60*($hours + 24*$days));
        } elsif ($etime eq '-') {
            $etime = 0; # a zombie ?
        } else {
            warning("Invalid etime: $etime");
            $etime = 0;
        }
        my $pi = { ppid => $ppid, pid => $pid, vsize => $vsize, user => $user,
                   cputime => $cputime, etime => $etime, comm => $comm };
        push @procinfo, $pi,
    }
    return @procinfo;
}


############################################
# Public subs
#############################################

sub cluster_info ($) {
    my ($config) = shift;

    my (%lrms_cluster);

    $lrms_cluster{lrms_type} = "fork";
    $lrms_cluster{lrms_version} = "1";

    # only enforcing per-process cputime limit
    $lrms_cluster{has_total_cputime_limit} = 0;

    my ($cputhreads) = cpu_threads_cores_sockets();
    $lrms_cluster{totalcpus} = $cputhreads;

    # Since fork is a single machine backend all there will only be one machine available
    $lrms_cluster{cpudistribution} = $lrms_cluster{totalcpus}."cpu:1";

    # usedcpus on a fork machine is determined from the 1min cpu
    # loadaverage and cannot be larger than the totalcpus

    if ( `uptime` =~ /load averages?:\s+([.\d]+,?[.\d]+),?\s+([.\d]+,?[.\d]+),?\s+([.\d]+,?[.\d]+)/ ) {
        my $usedcpus = $1;
        $usedcpus =~ tr/,/./;
        $lrms_cluster{usedcpus} = ($usedcpus <= $lrms_cluster{totalcpus})
                       ? floor(0.5+$usedcpus) : $lrms_cluster{totalcpus};
    } else {
        error("Failed getting load averages");
        $lrms_cluster{usedcpus} = 0;
    }

    #Fork does not support parallel jobs
    $lrms_cluster{runningjobs} = $lrms_cluster{usedcpus};

    # no LRMS queuing jobs on a fork machine, fork has no queueing ability
    $lrms_cluster{queuedcpus} = 0;
    $lrms_cluster{queuedjobs} = 0;
    $lrms_cluster{queue} = [ ];
    return %lrms_cluster;
}

sub queue_info ($$) {
    my ($config) = shift;
    my ($qname) = shift;

    init_globals($qname);

    if (defined $running) {
        # job_info was already called, we know exactly how many grid jobs
        # are running
        $lrms_queue{running} = $running;

    } else {
        # assuming that the submitted grid jobs are cpu hogs, approximate
        # the number of running jobs with the number of running processes

        $lrms_queue{running}= 0;
        unless (open PSCOMMAND,  "ps axr |") {
            error("error in executing ps axr");
        }
        while(my $line = <PSCOMMAND>) {
            chomp($line);
            next if ($line =~ m/PID TTY/);
            next if ($line =~ m/ps axr/);
            next if ($line =~ m/cluster-fork/);     
            $lrms_queue{running}++;
        }
        close PSCOMMAND;
    }

    my ($cputhreads) = cpu_threads_cores_sockets();
    $lrms_queue{totalcpus} = $cputhreads;

    $lrms_queue{status} = $lrms_queue{totalcpus}-$lrms_queue{running};

    # reserve negative numbers for error states
    # Fork is not real LRMS, and cannot be in error state
    if ($lrms_queue{status}<0) {
	debug("lrms_queue{status} = $lrms_queue{status}");
	$lrms_queue{status} = 0;
    }

    my $job_limit;
    $job_limit = 1;
    if ( $$config{maxjobs} ) {
       #extract lrms maxjobs from config option
      
       my @maxes = split(' ', $$config{maxjobs});
       my $len=@maxes;
       if ($len > 1){ 
         $job_limit = $maxes[1];
         if ($job_limit eq "cpunumber") {
           $job_limit = $lrms_queue{totalcpus};
         }
       }
    }
    $lrms_queue{maxrunning} = $job_limit;
    $lrms_queue{maxuserrun} = $job_limit;
    $lrms_queue{maxqueuable} = $job_limit;

    chomp( my $maxcputime = `ulimit "-t"` );
    if ($maxcputime =~ /^\d+$/) {
        $lrms_queue{maxcputime} = $maxcputime;
    } elsif ($maxcputime eq 'unlimited') {
        $lrms_queue{maxcputime} = "";
    } else {
        warning("Could not determine max cputime with ulimit -t");
        $lrms_queue{maxcputime} = "";
    }

    $lrms_queue{queued} = 0;
    $lrms_queue{mincputime} = "";
    $lrms_queue{defaultcput} = "";
    $lrms_queue{minwalltime} = "";
    $lrms_queue{defaultwallt} = "";
    $lrms_queue{maxwalltime} = $lrms_queue{maxcputime};

    return %lrms_queue;
}


sub jobs_info ($$@) {
    my ($config) = shift;
    my ($qname) = shift;
    my ($jids) = shift;

    init_globals($qname);

    my (%lrms_jobs);
    my @procinfo = process_info();

    foreach my $id (@$jids){

        $lrms_jobs{$id}{nodes} = [ hostname ];

        my ($proc) = grep { $id == $_->{pid} } @procinfo;
        if ($proc) {

            # number of running jobs. Will be used in queue_info
            ++$running;

            # sum cputime of all child processes
            my $cputime = $proc->{cputime};
            $_->{ppid} == $id and $cputime += $_->{cputime} for @procinfo;

            $lrms_jobs{$id}{mem} = $proc->{vsize};
            $lrms_jobs{$id}{walltime} = ceil $proc->{etime}/60;
            $lrms_jobs{$id}{cputime} = ceil $cputime/60;
            $lrms_jobs{$id}{status} = 'R';
            $lrms_jobs{$id}{comment} = [ "LRMS: Running under fork" ];
        } else {
            $lrms_jobs{$id}{mem} = '';
            $lrms_jobs{$id}{walltime} = '';
            $lrms_jobs{$id}{cputime} = '';
            $lrms_jobs{$id}{status} = ''; # job is EXECUTED
            $lrms_jobs{$id}{comment} = [ "LRMS: no longer running" ];
        }

	$lrms_jobs{$id}{reqwalltime} = "";
	$lrms_jobs{$id}{reqcputime} = "";   
        $lrms_jobs{$id}{rank} = "0";
	#Fork backend does not support parallel jobs
	$lrms_jobs{$id}{cpus} = "1"; 
    }

    return %lrms_jobs;
}


sub users_info($$@) {
    my ($config) = shift;
    my ($qname) = shift;
    my ($accts) = shift;

    init_globals($qname);

    my (%lrms_users);

    # freecpus
    # queue length

    if ( ! exists $lrms_queue{status} ) {
	%lrms_queue = queue_info( $config, $qname );
    }
    
    foreach my $u ( @{$accts} ) {
	$lrms_users{$u}{freecpus} = $lrms_queue{maxuserrun} - $lrms_queue{running};        
	$lrms_users{$u}{queuelength} = "$lrms_queue{queued}";
    }
    return %lrms_users;
}
	      
1;
