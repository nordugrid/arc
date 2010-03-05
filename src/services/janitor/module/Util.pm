package Janitor::Util;

use Exporter;
@ISA = qw(Exporter);     # Inherit from Exporter
@EXPORT_OK = qw(remove_directory all_runscripts asGID asUID dir_lock dir_unlock dir_lock_remove sane_rte_name sane_job_id);

=head1 NAME

Jantior::Util - Some functions used by different parts of the Janitor

=head1 SYNOPSIS

use Jantior::Util qw(remove_directory all_runscripts asGID asUID dir_lock dir_unlock dir_lock_remove)

=head1 FUNCTIONS

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
###l4p my $logger = get_logger("Janitor::Util");

use warnings;
use strict;

use DirHandle;
use Sys::Hostname;
use File::Temp qw(tempdir);
use File::stat;


######################################################################
######################################################################

=item remove_directory($dir)

Removes the directory $dir and all its content. Returns true iff successfull.

=cut

sub remove_directory {
	my ($dir) = @_;

	my $error_flag = 0;

	my $d = new DirHandle($dir);
	unless (defined $d) {
		# cant open directory. Try to rmdir it, maybe we are lucky and
		# it is emtpy
		rmdir $dir;
		return 0;	# but there was an error...
	}

	while (defined (my $name = $d->read)) {
		next if $name eq "." or $name eq "..";

		# links to directories return true in -d ...
		if ( ! -l "$dir/$name" and -d "$dir/$name" ) {
			remove_directory("$dir/$name") or $error_flag = 1;
		} else {
			unlink "$dir/$name" or $error_flag = 1;
 		}
	}

	undef $d;

	rmdir $dir or $error_flag = 1;

	return $error_flag == 0;
}

######################################################################
# This function searches in $dir for manually installed runtime environments. A
# list of these is returned.
#
# it was implemented this way:
# @runtimeenvironment= `find $config{runtimedir} -type f ! -name ".*" ! -name "*~"` ;
######################################################################

=item all_runscripts($dir)

This function searches in $dir for RTEs which are installed in the old
fashioned manual way. They are returned as a list.

=cut

sub all_runscripts {
	my ($dir) =  @_;
	my @ret;
	_getfiles($dir, "", \@ret);
	return @ret;

	sub _getfiles {
		my ($dir, $part_dir, $r) = @_;
		my $d = new DirHandle($dir); 
		unless (defined $d) {
###l4p 			$logger->error("Cannot open directory $dir: $!");
			return;
		}
		while (my $e = $d->read) {
			next if $e eq "." or $e eq "..";	# always skip these
			&_getfiles("$dir/$e", "$part_dir$e/", $r) if -d "$dir/$e";
			if ( -f "$dir/$e" ) {
				next if $e =~ /^\./;
				next if $e =~ /~$/;
				push @$r, "$part_dir$e";
			}
		}
		undef $d;
	}
}

######################################################################
# Expects as $in a user ID, either by name or numeric. Returns the
# corresponding numeric UID
######################################################################

=item asUID($in)

If $in is a user name, then its uid is returned.
If $in is already numeric it is returned directly.

=cut

sub asUID {
	my $in = shift;

	if (defined $in) {
		my ($name, $passwd, $uid) = getpwnam($in);
		if (defined $uid) {
			return $uid;
		} elsif ($in =~ m/^[0-9]+$/) {
			return $in;
		}
###l4p 		$logger->error("Unknown user: \"". $in . "\"");
	} else {
###l4p 		$logger->debug("asUID called with argument <undef>, expected UID");
	}

	return undef;
}

######################################################################
# Expects as $in a group ID, either by name or numeric. Returns the
# corresponding numeric GID
######################################################################

=item asGID($in)

Same as asUID($in) but for group ids.

=cut

sub asGID {
	my $in = shift;

	if (defined $in) {
		my ($name, $passwd, $uid) = getgrnam($in);
		if (defined $uid) {
			return $uid;
		} elsif ($in =~ m/^[0-9]+$/) {
			return $in;
		}
###l4p 		$logger->error("Unknown group: \"". $in . "\"");
	} else {
###l4p 		$logger->debug("asGID called with argument <undef>, expected GID");
	}

	return undef;
}

######################################################################
# This function is a helper for the installation process. It locks $dir for
# $job. If $dir does not exists, it is created. If defined $maxwait is the time
# in seconds which is waited to get the lock. If it is not possible to lock the
# directory in this zero is returned. 
######################################################################

=item dir_lock($dir, $job, $maxwait)

Locks directory $dir for job $job. It waits for maximal $maxwait seconds or
forever if $maxwait is undefined.

=cut

sub dir_lock {
	my ($dir, $job, $maxwait) = @_;
	my $hostname = &hostname;

	$dir =~ s/\/*$//;

	if (defined $maxwait and $maxwait > 0) {
		$maxwait = time + $maxwait;
	} else {
		$maxwait = 0;
	}

###l4p	my $lastlog = time;

	WAITAGAIN: {
		# wait until it is not locked
		while ( -e "$dir/lock" ) {
			if ( $maxwait and time > $maxwait ) {
###l4p				$logger->info("Job $job: giving up waiting on lock on $dir!");
				return 0;
			}
###l4p			if ( time - $lastlog >= 5 ) {
###l4p				$lastlog = time;
###l4p				$logger->info("Job $job: waiting for lock on $dir!");
###l4p			}
			sleep 1;
		}

		if ( -e $dir ) {
			# if it exists, try to lock it
			my $n = "$dir/lock.$hostname.$$";
			open my $f, ">$n" or redo WAITAGAIN;
			my $ret = link $n, "$dir/lock";
			if ( $ret or stat($n)->nlink == 2) {
				# sucessfull since link returned true or the link
				# count of $n changend to 2.
				unlink $n;
				return 1;		# we made it :-)
			} else {
				# couldn't lock
				unlink $n;
				redo WAITAGAIN;
			}
		} else {
			# the directory does not exists, create it
			(my $updir = $dir) =~ s#/[^/]+$##; #
			my $tmpdir = tempdir(".tempXXXXXXX", DIR => $updir);	
			chmod(0755, $tmpdir);
			unless ( open(my $f ,">".$tmpdir."/lock") ) {
				my $msg = "Can not create lockfile in $tmpdir: $!\n";
###l4p 				$logger->fatal($msg);
				die $msg;
			}
			my $ret = rename($tmpdir,$dir);
			unless ($ret) { # it seems somebody else was faster, retry
				unlink($tmpdir."/lock");
				rmdir $tmpdir;
				redo WAITAGAIN;
			}
			# success
		}
	}
	# fall through is also possible
	
	return 1;
}

=item dir_unlock($dir)

Unlock the directory $dir.

=cut

sub dir_unlock {
	unlink $_[0] . "/lock";
}

######################################################################
# removes a runtimedir in a safe way. It does not care, if it is still in use!
# if $force is true even locked directories are deleted. If $force is false, it
# waits until the lock is released.
######################################################################

=item dir_lock_remove($dir, $force)

Removes a directory in a save way by locking it before touching.

=cut

sub dir_lock_remove {
	my ($dir, $force) = @_;
	$dir =~ s/\/*$//;
	$force = 0 unless defined $force;
	(my $updir = $dir) =~ s#/[^/]+$##; #

	my $tmpdir = tempdir(".tempXXXXXXX", DIR => $updir);	

	dir_lock($dir)		unless $force;

	rename $dir, "$tmpdir/dir";
	unless (remove_directory($tmpdir)) {
###l4p 		$logger->error("Removing directory $tmpdir failed. Clean up yourself!\n");
		return 0;		
	}

	return 1;
}

######################################################################
######################################################################

=item sane_rte_name(@RTE)

Checks if all the RTEs have a sane and acceptable name.

=cut
sub sane_rte_name {
	my @forbidden = (
		'/(\.){1,2}/',	# /../ and /./
		'^(\.){1,2}/',	# ./ and ../
		'/(\.){1,2}$',	# /. and /..
		'^/', '^$', '^\.$');

	foreach my $rte ( @_ ) {
		foreach my $regexp ( @forbidden ) {
			return 0 if $rte =~ m#$regexp#;
		}
	}
	return 1;
}

=item sane_job_id(jobid)

Checks if jobid is a valid jobid.

=cut
sub sane_job_id {
	my ($jobid) = @_;
	
	if (defined $jobid and $jobid =~ m/^[0-9]+$/) {
		return 1;
	}
	return 0;
}


1;

=back

=name1 SEE ALSO

janitor(8)

=cut
