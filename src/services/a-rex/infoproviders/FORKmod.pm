package FORKmod;

our @ISA = ('Exporter');
our @EXPORT_OK = qw(get_lrms_info get_lrms_options_schema);

use POSIX qw(ceil floor);
use Sys::Hostname;
use LogUtils;
use Sysinfo;

use strict;

##########################################
# Saved private variables
##########################################

our $lrms_info;
our $options;

our $running = 0;
our $hostname = hostname();
our $cputhreadcount;

our $log = LogUtils->getLogger("LRMSInfo.fork");

############################################
# Public subs
#############################################

sub get_lrms_options_schema {
    return {
            'queues' => {
                '*' => {
                    'users'       => [ '' ],
                    'fork_job_limit' => '*'
                }
            },
            'jobs' => [ '' ]
        };
}


sub get_lrms_info(\%) {

    $options = shift;

    my ($sysname, $nodename, $release, $version, $machine) = POSIX::uname();
    my $meminfo = Sysinfo::meminfo();
    my $cpuinfo = Sysinfo::cpuinfo();
    $cputhreadcount = $cpuinfo->{cputhreadcount};

    cluster_info();

    my $jids = $options->{jobs};
    jobs_info($jids);

    $lrms_info->{queues} = {};

    my $isfree;
    my %queues = %{$options->{queues}};
    for my $qname ( keys %queues ) {
        queue_info($qname);
        $isfree = $lrms_info->{queues}{$qname}{status} ? 1 : 0;
    }

    for my $qname ( keys %queues ) {
        my $users = $queues{$qname}{users};
        users_info($qname,$users);
    }

    my $node = $lrms_info->{nodes}{$hostname} = {};

    $node->{isavailable} = 1;
    $node->{isfree} = $isfree;
    $node->{machine} = $machine;
    $node->{sysname} = $sysname;
    $node->{lcpus} = $cpuinfo->{cputhreadcount} if $cpuinfo->{cputhreadcount};
    $node->{pcpus} = $cpuinfo->{cpusocketcount} if $cpuinfo->{cpusocketcount};
    $node->{pmem} = $meminfo->{pmem} if $meminfo->{pmem};
    $node->{vmem} = $meminfo->{vmem} if $meminfo->{vmem};

    return $lrms_info
}


##########################################
# Private subs
##########################################

# Produces stats for all processes on the system

sub process_info() {
    my @pslines = `ps -e -o ppid,pid,vsz,time,etime,user,comm`;
    if ($? != 0) {
        $log->warning("Failed running ps -e -o ppid,pid...");
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
            $log->warning("Invalid cputime: $cputime");
            $cputime = 0;
        }
        # matches time formats like: 21:29.44, 12:21:29, 3-12:21:29
        if ($etime =~ /^(?:(?:(\d+)-)?(\d+):)?(\d+):(\d\d(?:\.\d+)?)$/) {
            my ($days,$hours,$minutes,$seconds) = (($1||0), ($2||0), $3, $4);
            $etime = $seconds + 60*($minutes + 60*($hours + 24*$days));
        } elsif ($etime eq '-') {
            $etime = 0; # a zombie ?
        } else {
            $log->warning("Invalid etime: $etime");
            $etime = 0;
        }
        my $pi = { ppid => $ppid, pid => $pid, vsize => $vsize, user => $user,
                   cputime => $cputime, etime => $etime, comm => $comm };
        push @procinfo, $pi,
    }
    return @procinfo;
}

sub cluster_info () {

    my $lrms_cluster = {};
    $lrms_info->{cluster} = $lrms_cluster;

    $lrms_cluster->{lrms_type} = "fork";
    $lrms_cluster->{lrms_version} = "0.9";

    my $cpuinfo = Sysinfo::cpuinfo();
    $lrms_cluster->{totalcpus} = $cputhreadcount;

    # Since fork is a single machine backend all there will only be one machine available
    $lrms_cluster->{cpudistribution} = $lrms_cluster->{totalcpus}."cpu:1";

    # usedcpus on a fork machine is determined from the 1min cpu
    # loadaverage and cannot be larger than the totalcpus

    if (`uptime` =~ /load averages?:\s+([.\d]+),?\s+([.\d]+),?\s+([.\d]+)/) {
        $lrms_cluster->{usedcpus} = ($1 <= $lrms_cluster->{totalcpus})
                         ? floor(0.5+$1) : $lrms_cluster->{totalcpus};
    } else {
        $log->warning("Failed getting load averages");
        $lrms_cluster->{usedcpus} = 0;
    }
    $lrms_cluster->{runningjobs} = $lrms_cluster->{usedcpus};

    # no LRMS queuing jobs on a fork machine, fork has no queueing ability
    $lrms_cluster->{queuedjobs} = 0;
    $lrms_cluster->{queuedcpus} = 0;
}


sub queue_info ($) {
    my $qname = shift;
    my $qopts = $options->{queues}{$qname};

    my $lrms_queue = {};
    $lrms_info->{queues}{$qname} = $lrms_queue;

    my $cpuinfo = Sysinfo::cpuinfo();
    $lrms_queue->{totalcpus} = $cputhreadcount;

    $lrms_queue->{running} = $running;
    $lrms_queue->{status} = $lrms_queue->{totalcpus} - $running;
    $lrms_queue->{status} = 0 if $lrms_queue->{status} < 0;

    my $job_limit;
    if ( not $qopts->{fork_job_limit} ) {
       $job_limit = 1;
    } elsif ($qopts->{fork_job_limit} eq "cpunumber") {
        $job_limit = $lrms_queue->{totalcpus};
    } else {
       $job_limit = $qopts->{fork_job_limit};
    }
    $lrms_queue->{maxrunning} = $job_limit;
    $lrms_queue->{maxuserrun} = $job_limit;
    $lrms_queue->{maxqueuable} = $job_limit;

    chomp( my $maxcputime = `ulimit "-t"` );
    if ($maxcputime =~ /^\d+$/) {
        $lrms_queue->{maxcputime} = $maxcputime;
        $lrms_queue->{maxwalltime} = $maxcputime;
    } elsif ($maxcputime ne 'unlimited') {
        $log->warning("Could not determine max cputime with ulimit -t");
    }

    $lrms_queue->{queued} = 0;
    #$lrms_queue->{mincputime} = "";
    #$lrms_queue->{defaultcput} = "";
    #$lrms_queue->{minwalltime} = "";
    #$lrms_queue->{defaultwallt} = "";
}


sub jobs_info ($) {
    my $jids = shift;

    my $lrms_jobs = {};
    $lrms_info->{jobs} = $lrms_jobs;

    my @procinfo = process_info();

    foreach my $id (@$jids){

        $lrms_jobs->{$id}{nodes} = [ $hostname ];

        my ($proc) = grep { $id eq $_->{pid} } @procinfo;
        if ($proc) {

            # number of running jobs. Will be used in queue_info
            ++$running;

            # sum cputime of all child processes
            my $cputime = $proc->{cputime};
            $_->{ppid} == $id and $cputime += $_->{cputime} for @procinfo;

            $lrms_jobs->{$id}{mem} = $proc->{vsize};
            $lrms_jobs->{$id}{walltime} = $proc->{etime};
            $lrms_jobs->{$id}{cputime} = $cputime;
            $lrms_jobs->{$id}{status} = 'R';
            $lrms_jobs->{$id}{rank} = 0;
            $lrms_jobs->{$id}{cpus} = 1;
            #$lrms_jobs->{$id}{reqwalltime} = "";
            #$lrms_jobs->{$id}{reqcputime} = "";
            $lrms_jobs->{$id}{comment} = [ "LRMS: Running under fork" ];
        } else {
            $lrms_jobs->{$id}{status} = 'EXECUTED';
        }
    }
}


sub users_info($$) {
    my $qname = shift;
    my $accts = shift;

    # add users to the big tree
    my $lrms_users = {};
    my $lrms_queue = $lrms_info->{queues}{$qname};
    $lrms_queue->{users} = $lrms_users;

    # freecpus
    # queue length

    foreach my $u ( @{$accts} ) {
        my $freecpus = $lrms_queue->{maxuserrun} - $lrms_queue->{running};
        $lrms_users->{$u}{freecpus} = { $freecpus => 0 };
        $lrms_users->{$u}{queuelength} = $lrms_queue->{queued};
    }
}

1;
