package LogUtils;

# Object-oriented usage example:
# 
#    LogUtils::level("DEBUG");
#    $log = LogUtils->getLogger("MyProg.MyClass");
#    $log->warning("Oops!");
#    $log->error("Can't go on!");

# Procedural usage example:
#
#    start_logging('INFO');
#    warning("Oops!");
#    error("Can't go on!");

use POSIX;
use FileHandle;
use File::Basename;

use Exporter;
@ISA = ('Exporter');     # Inherit from Exporter
@EXPORT_OK = qw(start_logging error warning info debug);

use strict;

our %names = (FATAL => 0, ERROR => 1, WARNING => 2, INFO => 3, DEBUG => 4, VERBOSE => 5);

our $loglevel = 1; # default level is WARNING

our $default_logger = LogUtils->getLogger(basename($0));

# For backwards compatibility

sub start_logging($) {
    level(shift);
}

# set loglevel for all loggers
sub level {
    return $loglevel unless  @_;
    my $level = shift;
    $loglevel = $names{$level};
    fatal("No such loglevel '$level'") unless $loglevel;
    return $loglevel;
}

# constructor

sub getLogger {
    my $class = shift;
    my $self = {name => (shift || '')};
    bless $self, $class;
    return $self;
}

sub verbose {
    return unless $loglevel > 4;
    unshift(@_, $default_logger) unless ref($_[0]) eq __PACKAGE__;
    my ($self, $msg) = @_;
    $self->_log('VERBOSE',$msg);
}

sub debug {
    return unless $loglevel > 3;
    unshift(@_, $default_logger) unless ref($_[0]) eq __PACKAGE__;
    my ($self, $msg) = @_;
    $self->_log('DEBUG',$msg);
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
    $name = $name ? "$name: " : "";
    print STDERR "$name$severity: $msg\n";
}

sub test {
    LogUtils::level('INFO');
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
    start_logging('DEBUG');
    debug('geee');
    info('beee');
    warning('meee');
    error('mooo');
}

#test();
#test2();

1;
