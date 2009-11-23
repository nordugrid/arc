package Janitor::Execute;


=head1 NAME

Janitor::Execute - Executes a command passed to the Janitor

=head1 SYNOPSIS

use Janitor::Execute;

=head1 DESCRIPTION

This class is used for executing external programs. This is done in a
syncronous way, i.e. this process waits until the child process has died.

=head1 METHODS

=over 4

=cut
 
BEGIN {
	if (defined(Janitor::Janitor::resurrect)) { 
		eval 'require Log::Log4perl';
		unless ($@) { 
			import Log::Log4perl qw(:resurrect get_logger);
		}
	}
}

use warnings;
use strict;

use POSIX qw(:sys_wait_h EAGAIN setgid getgid getegid setuid getuid geteuid);
use Cwd qw(getcwd);

use Janitor::InstallLogger;

###l4p my $logger = Log::Log4perl::get_logger("Janitor::Execute");

=item Janitor::Execute::new($workdir, $logfile, $chroot)

The constructor. Its first parameter is the name of the directory which shall
be the cwd of the programm to execute. The second parameter is the location of
the file to use as logfile. Iff the third parameter is true then chroot is called
prior to the execution of the program. Root permissions are necessary for this.

The second and the third parameters are optional.

=cut 

sub new {
	my ($class, $workdir, $logfile, $chroot) = @_;

	my $file_appender;

	# XXX uncomment this for debuging of all executions
	#$logfile = "/tmp/execute.log" unless defined $logfile;

	my $self = {
		_workdir => $workdir,
		_chroot => $chroot,
		_logfile => $logfile,
		_retval => undef,
		_signal => undef,
		_waitval => undef,
		_uid => undef,
		_gid => undef,
	};
	return bless $self, $class;
}

######################################################################
# sets and returns the uid which will be used to execute the command
######################################################################

=item $obj->uid($uid)

Sets and returns the uid which will be used to execute the command. The
parameter is optional.

=cut

sub uid {
	my ($self, $uid) = @_;
	$self->{_uid} = $uid if defined $uid;
	return $self->{_uid};
}

######################################################################
# sets and returns the gid which will be used to execute the command
######################################################################

=item $obj->gid($gid)

Sets and returns the gid which will be used to execute the command. The
parameter is optional.

=cut

sub gid {
	my ($self, $gid) = @_;
	$self->{_gid} = $gid if defined $gid;
	return $self->{_gid};
}

######################################################################
# 
######################################################################

=item $obj->execute(@command)

Executes the given command. For this it forks a child process which exec
the command. The parent process waits for the child to finish.
Currently timeouts are not supported.

This method returns a non-zero value if the execution succeeded.

=cut

sub execute {
	my ($self, @command) = @_;

 	# F.Orellana: changed 0400 to 0600. Don't see why the log file
 	# be non-writeable. - And it caused errors.
 	my $instlogger = new Janitor::InstallLogger($self->{_logfile}, 0600); 

	# info is used as a flag to describe if we are at least at log level
	# info. If not, the pipe to the child is not created.
	my $info = defined $self->{_logfile};

###l4p 	$logger->debug("Execute::execute: ".join(" ", @command));

	if ($info) {
 		$instlogger->info("current time: " . scalar localtime);
 		$instlogger->info("=" x 75);
 		$instlogger->info("command to execute: ");
		for (my $i = 0; $i < @command; $i++) {
 			$instlogger->info("   ARGV[" . $i . "] = >" . $command[$i] . "<");
		}
		
		# we only need the pipe if we are at info level
		unless (pipe(READ, WRITE)) {
###l4p 			$logger->error("Can not create pipe: " . $!);
 			$instlogger->error("Can not create pipe: " . $!);
			return 0;
		}

	}

	FORK: {
		if (my $pid = fork) {
			# parent

			if ($info) {
				close WRITE;
				# Log output of the child's execution
				while ( <READ> ) {
					chomp($_);
 					$instlogger->info($_);
				}
 				$instlogger->info("=" x 75);
				close READ;
			}

			# do the cleanup
 			$instlogger->debug("child closed output, calling waitpid()") if $info;
			my $ret = waitpid($pid,0);

			# XXX it's documented that close() calls wait(). So
			# shouldn't this always be true? It was never. Odd.
			if ($ret == -1) {
				my $msg = "Somebody else reaped my child,"
					. " assuming it did well...";
###l4p 				$logger->error($msg);
 				$instlogger->error($msg);
			}

			$self->{_waitval} = $?;
			if (WIFEXITED($?)) {
				my $msg = "Child returned " . WEXITSTATUS($?);
 				$instlogger->info($msg) if $info;
###l4p 				$logger->debug($msg);
				$self->{_retval} = WEXITSTATUS($?);
###l4p 				#$logger->debug("$self->{_retval}");
			} elsif (WIFSIGNALED($?)) {
				my $msg = "Child killed by INT " . WTERMSIG($?);
 				$instlogger->error($msg);
###l4p 				$logger->error($msg);
				$self->{_signal} = WTERMSIG($?);
			} else {
				my $msg = "Something odd happened to my child... "
					. "Assume this is an error\n";
 				$instlogger->error($msg);
###l4p 				$logger->error($msg);
 				$instlogger->info("current time: " . scalar localtime) if $info;
				return 0;
			}

 			$instlogger->info("current time: " . scalar localtime) if $info;;
			
			return (defined $self->{_retval} and $self->{_retval} == 0);

		} elsif (defined $pid) {
			# child

			# go to the working directory
			if (defined $self->{_workdir}) {
				my $ret = chdir $self->{_workdir};
				unless ($ret) {
					my $msg = "Cannot chdir into \""
						.  $self->{_workdir} . "\": $!";
###l4p 					$logger->error($msg);
 					$instlogger->fatal($msg);
					exit(1);
				}	
			}

 			$instlogger->info("working directory: \"" . &getcwd . "\"") if $info;

			# if requested do the chroot.
			if (defined $self->{_chroot} and $self->{_chroot} != 0) {
				# change the root directory
 				$instlogger->info("chroot to working directory") if $info;
				unless (chroot "." and chdir "/") {
					my $msg = "Cannot chroot to working directory: $!";
###l4p 					$logger->error($msg);
 					$instlogger->fatal($msg);
					exit(1);
				}

				# setting up a clean envirionment
				%ENV = ();
				$ENV{TMPDIR} = "/tmp";
				$ENV{TMP} = "/tmp";
				$ENV{LANG} = "C";
				$ENV{HOME} = "/root";
				$ENV{PATH} = "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/bin/X11";
			}
			
			if ($info) {
 				$instlogger->debug("environment variables: ");
				foreach my $key ( keys %ENV ) {
 					$instlogger->debug("   ".$key." = >".$ENV{$key}."<");
				}
			}

			# change group id. This may be allowed only to root, so
			# we have to do this first
			if (defined $self->{_gid} or $( ne $) ) {
				my $flag = ($< == 0 and $> != 0);

				($<,$>) = ($>,$<) if $flag; # change euid to root

				$) =~ m/^([0-9]+)\s+/;
				
				# F.Orellana: pretty optimistic to count on just one group membership.
				# Moreover, I get:
				# Argument "121 121" isn't numeric in scalar assignment at Janitor/Execute.pm line 204.
				# FIXED!
				
				my $new = "$1";			
				$new = "$self->{_gid}"
						if defined $self->{_gid};
						
				$( = $new;	# real gid
				$) = $new;	# effective gid

				my $err = $!;
				if ($( ne "$new" and $( !~ m/^$new\s+/ or
				    $) ne "$new" and $) !~ m/^$new\s+/) {
					my $msg = "Can not change gid: $err, $(, $new, $)";
 					$instlogger->fatal($msg);
###l4p 					$logger->error($msg);
					exit 1;
				}

				($<,$>) = ($>,$<) if $flag; # and revert
			}

			# and now we change the user id
			if (defined $self->{_uid} or $< ne $> ) {
				my $new = $>;
				$new = $self->{_uid} if defined $self->{_uid};

				if ($< == 0 and $> != 0) {
					($<,$>) = ($>,$<) # change euid to root
				}

				$< = $new;	# the real uid
				$> = $new;	# the effecive uid

				my $err = $!;
				if ($< ne $new or $> ne $new) {
					my $msg = "Can not change uid: $err";
 					$instlogger->fatal($msg);
###l4p 					$logger->error($msg);
					exit 1;
				}
			}

 			$instlogger->info("user id:   $< (real) $> (effective)");
 			$instlogger->info("group ids: $( (real) $) (effective)");

			# setup the input and output
			open (STDIN, '</dev/null');
			if ($info) {
				open (STDOUT, '>&WRITE');
				open (STDERR, '>&WRITE');
				close (READ);
			} else {
				open (STDOUT, '>/dev/null');
				open (STDERR, '>/dev/null');
			}

			# XXX we should also clean up other open filehandles


			if ($info) {
 				$instlogger->info("current time: " . scalar localtime);
 				$instlogger->info("calling exec(), the output of the command follows");
 				$instlogger->info("=" x 75);
			}
			# and now exec the command
			unless (exec { $command[0] } @command) {
				my $msg = "can not exec command: $!";
 				$instlogger->fatal($msg);
###l4p 				$logger->error($msg);
				exit 1;
			}
		} elsif ($! == EAGAIN) {
			# the recoverable fork error
			sleep 1;
			redo FORK;
		} else {
			# some real error
###l4p 			$logger->error("can not fork(): $!");
			return 0;
		}
	}

}

######################################################################
# returns the shell return value from the last execute command
# returns undef if there was no execute by now or if the process was killed by
# a signal
######################################################################

=item $obj->retval

This method returns the return value of the child process. If the process was
killed by a signal it returns undef.

=cut
sub retval {
	return shift->{_retval};
}

######################################################################
# If the executed process was killed by a signal, this function returns the
# signal number.
######################################################################

=item $obj->signal

This method returns the number of signal which killed the child process.

=cut

sub signal {
	return shift->{_signal};
}

1;

=back

=head1 EXAMPLE

Below is a simple example which shows how to use this class.

    use Janitor::Execute qw(new);
    my $e = new Janitor::Execute($workdir, "/tmp/logfile");
    if ($e->execute("/bin/echo", "Hello World!")) {
	print "successfully executed /bin/echo\n";
    } else {
	print "error while executing /bin/echo\n";
	if (defined $e->retval) {
		print "return value: %s\n", $e->retval;
	} elsif (defined $e->signal) {
		print "killed by signal %s\n", $e->signal;
	} else {
		print "unable to fork?\n";
	}
    }

After this some debug informations can be found in F</tmp/logfile>.
The output of the command is also stored in this file.

=name1 SEE ALSO

Janitor.8

=cut
