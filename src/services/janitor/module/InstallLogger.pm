package Janitor::InstallLogger;

BEGIN {
	if (defined(Janitor::Janitor::resurrect)) { 
		eval 'require Log::Log4perl';
		unless ($@) { 
			import Log::Log4perl qw(:resurrect get_logger);
		}
	}
}

###l4p my $logger = get_logger("Janitor::Installer");

=head1 NAME

Janitor::InstallLogger - writes log messages to a file

=head1 SYNOPSIS

use Janitor::InstallLogger;

=head1 DESCRIPTION

The Janitor uses Log4perl for logging. But as this is optional it uses this
home grown Logger for logging in places where logging must be done. The only
such place is the installation of packages by L<Janitor::Execute|Janitor::Execute>.

=head1 METHODS

=over 4

=item Janitor::InstallLogger::new($logfile, $mode)

Creates an InstallLogger which logs to $logfile. The permissions of this file
are set to $mode. The logfile itself is openend. It will be closed again in the
destructor.

=cut

sub new {
	my ($class, $logfile, $mode) = @_;

	$logfile = "/dev/null" unless defined $logfile;

	my $self = {
		_logfile => $logfile,
		_log => undef,
	};

	bless $self, $class;

	my $f;
	if (open ($f, ">>$logfile")) {
		$self->{_log} = \$f;
		if (defined $mode and $logfile ne "/dev/null") {
			unless (chmod $mode, $logfile) {
				my $msg = "Can not chmod $logfile: $!\n";
###l4p				if (1) { $logger->error($msg); } else {
					print STDERR "janitor: $msg";
###l4p				}
			}
		}
	} else {
		$self->{_log} = undef;
		my $msg = "Can not open $logfile: $!\n";
###l4p		if (1) { $logger->error($msg); } else {
			print STDERR "janitor: $msg";
###l4p		}
	}
	
	return $self;
}

sub _log {
	my ($self,$level,$msg) = @_;

	if (defined $self->{_log}) {
		my $f = ${$self->{_log}};
		print $f "$level $msg\n";
	} else {
###l4p		$logger->info("can not write this to logfile: $msg");
	}
	return;
}

=item $obj->debug, $obj->info, $obj->warn, $obj->error, $obj->fatal

Writes a message to the log file.

=cut

sub debug { 
	my($self,$msg) = @_;
	$self->_log("DEBUG",$msg);
}

sub info { 
	my($self,$msg) = @_;
	$self->_log("INFO",$msg);
}

sub warn { 
	my($self,$msg) = @_;
	$self->_log("WARN",$msg);
}

sub error { 
	my($self,$msg) = @_;
	$self->_log("ERROR",$msg);
}

sub fatal { 
	my($self,$msg) = @_;
	$self->_log("FATAL",$msg);
}

# close the log file
sub DESTROY {
	my $self = shift;
	if (defined $self->{_log}) {
		my $f = ${$self->{_log}};
		close $f;
	}
	return;
}

1;
