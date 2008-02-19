package LogUtils;

# Logging utilities
#
# Three levels: error, warning, debug
# logging an error will kill the calling function

use File::Basename;
use lib dirname($0);
use Exporter;
@ISA = ('Exporter');     # Inherit from Exporter
@EXPORT_OK = ( 'start_logging', 'error', 'warning', 'debug');
use strict;


our ($loglevel, $logfile);
$loglevel=2;
$logfile="/var/log/infoprovider.log";

sub start_logging($$) {
    $loglevel = shift;
    $logfile = shift;
}


sub write_log {
    use POSIX;
    #records logging info into the $logfile read from configuration
    my ($message) = shift;  
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
	localtime (time);
    my $timestamp =
	POSIX::strftime("%b %d %T", $sec,$min,$hour,$mday,
			$mon,$year,$wday,$yday,$isdst); 
    my $scriptname = $0; 
    
    open PROVIDERLOG, ">>$logfile";
    print PROVIDERLOG  "$timestamp [$$] $scriptname: $message\n";
    close PROVIDERLOG; 
#    print "$timestamp [$$] $scriptname: $message\n";
}

sub error {
    write_log(@_);
    die(@_);
}

sub warning {
    if ($loglevel < 1 ) {return 0}
    write_log(@_);
}

sub debug {
    if ($loglevel < 2 ) {return 0}
    write_log(@_);
}

1;
