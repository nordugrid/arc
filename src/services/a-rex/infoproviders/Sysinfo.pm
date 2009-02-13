package Sysinfo;

use Sys::Hostname;

use Exporter;
@ISA = ('Exporter');     # Inherit from Exporter
@EXPORT_OK = qw(cpuinfo processid diskinfo diskspaces);

use LogUtils;

use strict;

our $log = LogUtils->getLogger(__PACKAGE__);


# Return PIDs of named commands owned by the current user
# Only one pid is returned per command name

sub processid (@) {

    my @procs = `ps -u $< -o pid,comm 2>/dev/null`;
    if ( $? != 0 ) {
        $log->info("Failed running: ps -u $< -o pid,comm");
        $log->warning("Failed listing processes");
        return {};
    }
    shift @procs; # throw away header line

    # make hash of comm => pid
    my %all;
    /\s*(\d+)\s+(.+)/ and $all{$2} = $1 for @procs;

    my %pids;
    foreach my $name ( @_ ) {
        $pids{$name} = $all{$name} if $all{$name};
    }
    return \%pids;
}


sub cpuinfo {

    my $info = {};

    if (-f "/proc/cpuinfo") {
        # Linux variant

        open (CPUINFO, "</proc/cpuinfo")
            or $log->warning("Failed opening /proc/cpuinfo: $!");
        while ( my $line = <CPUINFO> ) {
            if ($line=~/^model name\s*:\s+(.*)$/) {
                $info->{cpumodel} = $1;
            } elsif ($line=~/^cpu MHz\s+:\s+(.*)$/) {
                $info->{cpufreq} = int $1;
            }
        }
        close CPUINFO;

    } elsif (-x "/usr/sbin/system_profiler") {
        # OS X

        my @lines = `/usr/sbin/system_profiler SPHardwareDataType`;
        $log->warning("Failed running system_profiler: $!") if $?;
        for my $line ( @lines ) {
            if ($line =~ /Processor Name:\s*(.*)/) {
                $info->{cpumodel} = $1;
            } elsif ($line =~ /Processor Speed:\s*([.\d]+) (\w+)/) {
                if ($2 eq "MHz") {
                    $info->{cpufreq} = int $1;
                } elsif ($2 eq "GHz") {
                    $info->{cpufreq} = int 1000*$1;
                }
            }
        }

    } elsif (-x "/usr/bin/kstat" ) {
        # Solaris

        my %chips;
        eval {
            require Sun::Solaris::Kstat;
            my $ks = Sun::Solaris::Kstat->new();
            my $cpuinfo = $ks->{cpu_info};
            die "key not found: cpu_info" unless defined $cpuinfo;
            my $info = $cpuinfo->{0}{"cpu_info0"};
            die "key not found: cpu_info0" unless defined $info;
            # $info->{cpumodel} = $info->{cpu_type}; # like sparcv9
            $info->{cpumodel} = $info->{implementation}; # like UltraSPARC-III+
            $info->{cpufreq} = int $info->{clock_MHz};
        };
        if ($@) {
            $log->error("Failed running module Sun::Solaris::Kstat: $@");
        }

    } else {
        $log->warning("Unsupported operating system");
    }

    return $info;

}


#
# Returns disk space (total and free) in MB on a filesystem
#
sub diskinfo ($) {
    my $path = shift;
    my ($diskfree, $disktotal, $mountpoint);

    if ( -d "$path") {
        # check if on afs
        if ($path =~ m#/afs/#) {
            my @dfstring =`fs listquota $path 2>/dev/null`;
            if ($? != 0) {
                $log->warning("Failed running: fs listquota $path");
            } elsif ($dfstring[-1] =~ /\s+(\d+)\s+(\d+)\s+\d+%\s+\d+%/) {
                $disktotal = int $1/1024;
                $diskfree  = int(($1 - $2)/1024);
                $mountpoint = '/afs';
            } else {
                $log->warning("Failed interpreting output of: fs listquota $path");
            }
        # "ordinary" disk
        } else {
            my @dfstring =`df -k $path 2>/dev/null`;
            if ($? != 0) {
                $log->warning("Failed running: df -k $path");

            # The first column may be printed on a separate line.
            # The relevant numbers are always on the last line.
            } elsif ($dfstring[-1] =~ /\s+(\d+)\s+\d+\s+(\d+)\s+\d+%\s+(\/\S*)$/) {
    	        $disktotal = int $1/1024;
    	        $diskfree  = int $2/1024;
                $mountpoint = $3;
            } else {
                $log->warning("Failed interpreting output of: df -k $path");
            }
        }
    } else {
        $log->warning("Not a directory: $path");
    }

    return undef if not defined $disktotal;
    return {megstotal => $disktotal, megsfree => $diskfree, mountpoint => $mountpoint};
}


# Given a list of paths, it finds the set of mount points of the filesystems
# containing the paths. It then returns a hash with these keys:
#     ndisks: number of distinct nount points
#     freesum: sum of free space on all mount points
#     freemin: minimum free space of any mount point
#     freemax: maximum free space of any mount point
#     totalsum: sum of total space on all mountpoints
#     totalmin: minimum total space of any mount point
#     totalmax: maximum total space of any mount point
#     errors: the number of non-existing paths
sub diskspaces {
    my ($freesum, $freemin, $freemax);
    my ($totalsum, $totalmin, $totalmax);
    my $errors = 0;
    my %mounts;
    for my $path (@_) {
        my $di = diskinfo($path);
        if ($di) {
            my ($total, $free, $mount) = ($di->{megstotal},$di->{megsfree},$di->{mountpoint});
            $mounts{$mount}{free} = $free;
            $mounts{$mount}{total} = $total;
        } else {
            ++$errors;
        }
    }
    for my $stats (values %mounts) {
        if (defined $freesum) {
            $freesum += $stats->{free};
            $freemin = $stats->{free} if $freemin > $stats->{free};
            $freemax = $stats->{free} if $freemax < $stats->{free};
            $totalsum += $stats->{total};
            $totalmin = $stats->{total} if $totalmin > $stats->{total};
            $totalmax = $stats->{total} if $totalmax < $stats->{total};
        } else {
            $freesum = $freemin = $freemax = $stats->{free};
            $totalsum = $totalmin = $totalmax = $stats->{total};
        }
    }
    return ( ndisks => scalar keys %mounts,
             freesum => $freesum || 0,
             freemin => $freemin || 0,
             freemax => $freemax || 0,
             totalsum => $totalsum || 0,
             totalmin => $totalmin || 0,
             totalmax => $totalmax || 0,
             errors => $errors );
}


1;
