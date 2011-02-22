package Shared;

# Subroutines common to cluster.pl and qju.pl
#

use Exporter;
@ISA = ('Exporter');     # Inherit from Exporter
@EXPORT_OK = ( 'mds_valid',
	       'post_process_config',
	       'print_ldif_data',
               'diskspace',
               'diskspaces');
use LogUtils ( 'start_logging', 'error', 'warning', 'debug' ); 
use strict;

sub post_process_config (%) {
    # post processes %main::config
    my($config) = shift @_;

    # Architechture
    if ( exists $$config{architecture} ) {
	if ($$config{architecture} =~ /adotf/) {
	    $$config{architecture} = `uname -m`;
	    chomp $$config{architecture};
	}
    }

    # Homogeneity
    if ( exists $$config{homogeneity} ) {
        $$config{homogeneity} = uc $$config{homogeneity};
    }
    
    # Compute node cpu type
    if ( exists $$config{nodecpu} ) {
	if ($$config{nodecpu}=~/adotf/i) { 
	    my ($modelname,$mhz);
	    unless (open  CPUINFO, "</proc/cpuinfo") {
		error("error in opening /proc/cpuinfo");
	    }   
	    while (my $line= <CPUINFO>) {
		if ($line=~/^model name\s+:\s+(.*)$/) {		
		    $modelname=$1;         
		}
		if ($line=~/^cpu MHz\s+:\s+(.*)$/) {
		    $mhz=$1;
		}
	    }
	    close CPUINFO;
	    $$config{nodecpu} = "$modelname @ $mhz MHz";
	}
    }
}


sub mds_valid ($){
    my ($ttl) = (@_);  
    my ($from, $to);
    my $seconds=time;
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
	gmtime ($seconds);
    $from = sprintf "%4d%02d%02d%02d%02d%02d%1s", $year+1900, $mon+1, $mday,$hour,$min,$sec,"Z";
    ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = gmtime ($seconds+$ttl);
    $to = sprintf "%4d%02d%02d%02d%02d%02d%1s", $year+1900,$mon+1,$mday,$hour,$min,$sec,"Z";
    return $from, $to;
}


sub print_ldif_data (@%) {
    my ($fields) = shift;
    my ($data) = shift;
    my ($k, $cc);

    # No empty value fields are written. Set provider_loglevel in
    # arc.conf to "2" to get info what is not written ;-)

    foreach $k (@{$fields}) {
	if ( exists $data->{$k} ) {
	    if ( defined $$data{$k}[0] ) {
		if ( $$data{$k}[0] ne "" ) {
		    foreach $cc (@{$data->{$k}}) {
			print "$k: $cc\n";
		    }
		}
	    }
#	    else {
#		debug("Key $k contains an empty value.");
#	    }
	    
	}
#	else {
#	    debug("Key $k not defined.");
#	}
    }
    print "\n";
}

#
# Returns disk space (total and free) in MB on a filesystem
#
sub diskspace ($) {
    my $path = shift;
    my ($diskfree, $disktotal, $mountpoint);

    if ( -d "$path") {
        # check if on afs
        if ($path =~ m#/afs/#) {
            my @dfstring =`fs listquota $path 2>/dev/null`;
            if ($? != 0) {
                warning("Failed running: fs listquota $path");
            } elsif ($dfstring[-1] =~ /\s+(\d+)\s+(\d+)\s+\d+%\s+\d+%/) {
                $disktotal = int $1/1024;
                $diskfree  = int(($1 - $2)/1024);
                $mountpoint = '/afs';
            } else {
                warning("Failed interpreting output of: fs listquota $path");
            }
        # "ordinary" disk
        } else {
            my @dfstring =`df -k $path 2>/dev/null`;
            if ($? != 0) {
                warning("Failed running: df -k $path");

            # The first column may be printed on a separate line.
            # The relevant numbers are always on the last line.
            } elsif ($dfstring[-1] =~ /\s+(\d+)\s+\d+\s+(\d+)\s+\d+%\s+(\/\S*)$/) {
                $disktotal = int $1/1024;
                $diskfree  = int $2/1024;
                $mountpoint = $3;
            } else {
                warning("Failed interpreting output of: df -k $path");
            }
        }
    } else {
        warning("No such directory: $path");
    }

    return (0,0,'') if not defined $disktotal;
    return ($disktotal, $diskfree, $mountpoint);
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
        my ($total, $free, $mount) = diskspace($path);
        $mounts{$mount}{free} = $free;
        $mounts{$mount}{total} = $total;
        ++$errors unless $total;
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
