package Shared;

# Subroutines common to cluster.pl and qju.pl
#

use File::Basename;
use lib dirname($0);
use Exporter;
@ISA = ('Exporter');     # Inherit from Exporter
@EXPORT_OK = ( 'mds_valid',
	       'arc_conf',
	       'post_process_config',
	       'print_ldif_data');
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

    # [common] lrms
    if (exists $$config{lrms} ) {
      my ($config_lrms, $config_defqueue) = split " ", $$config{lrms};
      $$config{lrms} = $config_lrms;
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


sub arc_conf ($) {
    my ($conf_file) = shift;
    my (%parsedconfig,
	$variable_name,
	$variable_value);
    
    # Parse the arc.conf directly into a hash
    # $parsedconfig{blockname}{variable_name}
    
    unless (open (CONFIGFILE, "<$conf_file")) {
	error("can not open $conf_file configuration file");
    }
    
    my $blockname;
    while (my $line =<CONFIGFILE>) {
	next if $line =~/^#/;
	next if $line =~/^$/;
	next if $line =~/^\s+$/;
	
	if ($line =~/\[(.+)\]/ ) {
	    $blockname = $1;
	    next;}
	
	unless ($line =~ /=\s*".*"\s*$/) {
	    warning("skipping incorrect arc.conf line: $line");
	    next;}
	
	$line =~m/^(\w+)\s*=\s*"(.*)"\s*$/;
	$variable_name=$1;
	$variable_value=$2;
	unless ($parsedconfig{$blockname}{$variable_name}) {
	    $parsedconfig{$blockname}{$variable_name} = $variable_value;}
	else {
	    $parsedconfig{$blockname}{$variable_name} .= "[separator]".$variable_value;
	}   
    }
    close CONFIGFILE;
    
    return %parsedconfig;
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

1;
