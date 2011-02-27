package Janitor::Logger;

use POSIX;
use FileHandle;
use File::Basename;

use strict;

=head1 NAME

Janitor::Logger - A simple logger module.

=head1 SYNOPSIS

    use Janitor::Logger;
    
    ## reset STDERR's log level
    Janitor::Logger::destination(*STDERR, $Janitor::Logger::ERROR);
    ## add some more log destinations
    Janitor::Logger::destination('/tmp/detailed.log', $Janitor::Logger::DEBUG);
    Janitor::Logger::destination('/tmp/errors.log', $Janitor::Logger::ERROR);
    
    $log = Janitor::Logger->get_logger("MyProg::MyClass");
    ...
    $log->warning("Oops!");
    $log->fatal("Can't go on!");

=head1 DESCRIPTION

This module provides simple logger that can log to multiple destination files
simultaneously. Each destination file has it's own log level. By default,
STDERR (with log level WARNING) is the only destination. Additional log
destinations have to be initialized by calling B<destination>().

=head1 FUNCTIONS

=over 4

=item B<destination>(I<filename>, I<loglevel>)

Adds I<filename> to the list of log destinations. Only log entries of severity
I<loglevel> or higher will be written. I<filename> is opened for appending.
The possible log levels are:

    $Janitor::Logger::DEBUG
    $Janitor::Logger::VERBOSE
    $Janitor::Logger::INFO
    $Janitor::Logger::WARNING
    $Janitor::Logger::ERROR
    $Janitor::Logger::FATAL

=back

=head1 OBJECT-ORIENTED INTERFACE

=over 4

=item B<get_logger>(I<name>)

Constructor - creates a named logger object. I<name> is an arbitrary string
that will prefix log messages emitted by this logger object.

=item B<debug>(I<msg>)

=item B<verbose>(I<msg>)

=item B<info>(I<msg>)

=item B<warning>(I<msg>)

=item B<error>(I<msg>)

=item B<fatal>(I<msg>)

Emit a log message of appropiate severity.

=over 4

=back

=back

=cut

our ($FATAL, $ERROR, $WARNING, $INFO, $VERBOSE, $DEBUG) = (0, 1, 2, 3, 4, 5);
our @lnames = qw(FATAL ERROR WARNING INFO VERBOSE DEBUG);

# common settings to all logger instances
our %logfiles = (
    *STDERR => [ *STDERR, $WARNING]    # enable by default logging to STDERR
);

sub destination {
    my ($file, $level) = @_;
    $level = $FATAL if $level < $FATAL;
    $level = $DEBUG if $level > $DEBUG;
    if ($file eq *STDERR) {
        $logfiles{*STDERR} = [*STDERR, $level];
    } else {
        # if file is already added, close it first
        if (exists $logfiles{$file}) {
            close $logfiles{$file}[0];
            delete $logfiles{$file};
        }
        my $fh = new FileHandle ">>$file"
            or die "Error opening logfile $file: $!";
        $fh->autoflush(1); # make it unbuffered
        $logfiles{$file} = [$fh, $level];
    }
    return;
}


sub get_logger {
    my $class = shift;
    my $self = {name => (shift || 'Janitor')};
    bless $self, $class;
    return $self;
}

sub fatal {
    my ($self, $msg) = @_;
    $self->_log($FATAL,$msg);
}

sub error {
    my ($self, $msg) = @_;
    $self->_log($ERROR,$msg);
}

sub warning {
    my ($self, $msg) = @_;
    $self->_log($WARNING,$msg);
}

sub info {
    my ($self, $msg) = @_;
    $self->_log($INFO,$msg);
}

sub verbose {
    my ($self, $msg) = @_;
    $self->_log($VERBOSE,$msg);
}

sub debug {
    my ($self, $msg) = @_;
    $self->_log($DEBUG,$msg);
}


sub _log($$$) {
    my ($self, $level, $msg) = @_;
    for (values %logfiles) {
        my ($fh, $l) = @$_;
        next if $level > $l;
        print $fh $self->_format($level,$msg);
    }
}


sub _format {
    my ($self,$level,$msg) = @_;
    my $name = $self->{name};
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime (time);
    my $timestamp = POSIX::strftime("%Y-%m-%d %H:%M:%S", $sec,$min,$hour,$mday,
                                    $mon,$year,$wday,$yday,$isdst);
    $name = $name ? "$name: " : "";
    return "$timestamp $name$lnames[$level]: $msg\n";
}



END {

    # close all filehadles

    for (values %logfiles) {
        my ($fh, $l) = @$_;
        close $fh unless $l eq *STDERR;
    }
}


sub test {
    my $logfile = "log.$$";
    Janitor::Logger::destination(*STDERR, $Janitor::Logger::WARNING);
    Janitor::Logger::destination($logfile, $Janitor::Logger::DEBUG);

    my $log = Janitor::Logger->get_logger();
    $log->warning("Hi");
    $log = Janitor::Logger->get_logger("main");
    $log->warning("Hi");
    $log = Janitor::Logger->get_logger("main.sub");
    $log->warning("Hi");
    Janitor::Logger->get_logger("main.sub.too")->info("Boo");
    Janitor::Logger->get_logger("main.sub.too")->debug("Hoo");
    print "Contents of $logfile:\n". `cat $logfile; rm $logfile`;
}

#test();

1;

__END__

