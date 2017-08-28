package LogUtils;

# Object-oriented usage example:
# 
#    LogUtils::level("VERBOSE");
#    $log = LogUtils->getLogger("MyProg.MyClass");
#    $log->warning("Oops!");
#    $log->error("Can't go on!");

# Procedural usage example:
#
#    start_logging('INFO');
#    warning("Oops!");
#    error("Can't go on!");

use strict;
use warnings;

use POSIX;
use FileHandle;
use File::Basename;

use Exporter;
our @ISA = ('Exporter');     # Inherit from Exporter
our @EXPORT_OK = qw(start_logging error warning info verbose debug);

our %names = (FATAL => 0, ERROR => 1, WARNING => 2, INFO => 3, VERBOSE => 4, DEBUG => 5);

our $loglevel = 2; # default level is WARNING
our $ts_enabled = 0; # by default do not print timestamps
our $indented = ""; # do not indent by default

our $default_logger = LogUtils->getLogger(basename($0));

# redirect perl warnings to ARC logging format, 
# and attempt to limit number of warnings if not verbose
my %WARNS if ($loglevel < 4);
$SIG{__WARN__} = sub {
    my $message = shift;
    chomp($message);
    if ( $loglevel < 4 ) {
		if (exists($WARNS{$message})) {
	        if ($WARNS{$message} == 1) {
				$default_logger->warning("\'PERL: $message\' repeated more than once, skipping... set loglevel to VERBOSE to see all messages.");
	            $WARNS{$message} = 2;
	            return;
	        }
	    }
	    else { 
			$default_logger->warning("PERL: $message");  
			$WARNS{$message} = 1;
			return;
		}
    }
	else { 
		$default_logger->warning("PERL: $message"); 
		return;
	}
};


# For backwards compatibility

sub start_logging($) {
    level(shift);
}

# set loglevel for all loggers
sub level {
    return $loglevel unless  @_;
    my $level = shift;
    if (defined $names{$level}) {
        $loglevel = $names{$level};
    } elsif ($level =~ m/^\d+$/ and $level < keys %names)  {
        $loglevel = $level;
    } else {
        fatal("No such loglevel '$level'");
    }
    return $loglevel;
}

# enable/disable printing of timestamps
sub timestamps {
    return $ts_enabled unless  @_;
    return $ts_enabled = shift() ? 1 : 0;
}

sub indentoutput { 
   my ($indent) = @_;
   if ($indent) { 
     $indented = "\t";
   } else {
     $indented = "";
   }
}

# constructor

sub getLogger {
    my $class = shift;
    my $self = {name => (shift || '')};
    bless $self, $class;
    return $self;
}

sub debug {
    return unless $loglevel > 4;
    unshift(@_, $default_logger) unless ref($_[0]) eq __PACKAGE__;
    my ($self, $msg) = @_;
    $self->_log('DEBUG',$msg);
}

sub verbose {
    return unless $loglevel > 3;
    unshift(@_, $default_logger) unless ref($_[0]) eq __PACKAGE__;
    my ($self, $msg) = @_;
    $self->_log('VERBOSE',$msg);
}

sub info {
    return unless $loglevel > 2;
    unshift(@_, $default_logger) unless ref($_[0]) eq __PACKAGE__;
    my ($self, $msg) = @_;
    $self->_log('INFO',$msg);
}

sub warning {
    return unless $loglevel > 1;
    unshift(@_, $default_logger) unless ref($_[0]) eq __PACKAGE__;
    my ($self, $msg) = @_;
    $self->_log('WARNING',$msg);
}

# Causes program termination
sub error {
    unshift(@_, $default_logger) unless ref($_[0]) eq __PACKAGE__;
    my ($self, $msg) = @_;
    $self->_log('ERROR',$msg);
    exit 1;
}

# Causes program termination
sub fatal {
    unshift(@_, $default_logger) unless ref($_[0]) eq __PACKAGE__;
    my ($self, $msg) = @_;
    $self->_log('FATAL',$msg);
    exit 2;
}

sub _log {
    my ($self,$severity,$msg) = @_;
    my $name = $self->{name};
    $name = $name ? "$name" : "";
    # strip any newline in the message, substitute with '  \\  ', for nicer error reporting
    $msg =~  s/[\r\n]/  \\\\  /g;
    print STDERR  $indented.($ts_enabled ? "[".timestamp()."] " : "")."[$name] [$severity] [$$] $msg\n";
}

sub timestamp {
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime();
    return POSIX::strftime("%Y-%m-%d %H:%M:%S", $sec,$min,$hour,$mday,
                           $mon,$year,$wday,$yday,$isdst);
}

sub test {
    LogUtils::level('INFO');
    LogUtils::timestamps(1);
    my $log = LogUtils->getLogger();
    $log->warning("Hi");
    $log = LogUtils->getLogger("main");
    $log->warning("Hi");
    $log = LogUtils->getLogger("main.sub");
    $log->warning("Hi");
    $log = LogUtils->getLogger("main.sub.one");
    $log->warning("Hi");
    LogUtils->getLogger("main.sub.too")->info("Boo");
    LogUtils->getLogger("main.sub.too")->debug("Hoo");
}

sub test2 {
    start_logging('VERBOSE');
    debug('geee');
    info('beee');
    warning('meee');
    error('mooo');
}

#test();
#test2();

1;
