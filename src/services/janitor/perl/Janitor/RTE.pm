package Janitor::RTE;

use Janitor::Util qw(dir_lock dir_unlock dir_lock_remove);
use Digest::MD5 qw(md5_hex);
use File::stat;
use Switch;

use strict;


=head1 NAME

Janitor::RTE - Stores the metadata about an RTE.

=head1 SYNOPSIS

use Janitor::RTE;

=head1 DESCRIPTION

This class is used to store the metadata about an RTE on the hard drive.

=head1 METHODS

=over 4

=cut

use constant {
	FAILED		=> 1,
	INSTALLED_A	=> 2,
	INSTALLED_M	=> 3,
	VALIDATED	=> 4,
	BROKEN		=> 5,
	INSTALLABLE	=> 6,
	REMOVAL_PENDING	=> 7,
	UNKNOWN		=> 8,
};

use constant statefiles => qw(
	NOSTATE
	state/failed
	state/installed_a
	state/installed_m
	state/validated
	state/broken
	state/installable
	state/removal_pending
	state/unknown
);

######################################################################
# This class has the following attributes:
#	_connected	(true if connected)
#	_id		(the id of the RTE)
#	_jobname	(the name of the job for (log messages))
#	_rtedir		(the directory storing informations about this rte)
#	_rteregdir	(the directory used for storing)
######################################################################

=item new($rteregdir, $jobname)

The constructor.  Creates a new object. $rteregdir is the directory in which on
all rte is stored. $jobname is the name of the job who is triggering the creation
of the object. This value is used for logging.

=cut

sub new {
	my ($class, $rteregdir, $jobname) = @_;

	$jobname = "SOMEJOB" unless defined $jobname;

	my $self = {
		_rteregdir => $rteregdir,
		_jobname => $jobname,
		_connected => 0,
	};
	
	return bless $self, $class;
}

######################################################################
######################################################################

=item connect(@RTE)

Initializes the empt object and connects it to the RTE @RTE. If such an
RTE does not exists already it is crated. In any case it is locked
until disconnect() or remove() is called. If successfull true is returned.

=cut

sub connect {
	my ($self, @RTE) = @_;

	my $id = _calculate_RTE_ID(@RTE);
	my $rtedir = $self->{_rteregdir} . "/" . $id;

	my $ret = dir_lock($rtedir, $self->{_jobname});

	if ($ret == 0) {
		# couldn't lock
		return 0;
	} elsif (! -d "$rtedir/state") {
		# we cretated it, so do some initializing
		my $f;
		mkdir "$rtedir/jobs";
		mkdir "$rtedir/state";
		open $f, ">$rtedir/" . (statefiles)[UNKNOWN];
		open $f, ">$rtedir/list";
		print $f "$_\n" foreach (@RTE);
	}


	$self->{_id} = $id;
	$self->{_rtedir} = $rtedir;
	$self->{_connected} = 1;
	
	return 1;
}

=item connect($id)

Similar to connect(), but it connects to an already existing RTE which is
chosen by id. If no such RTE exists 0 is returned.

=cut


sub reconnect {
	my ($self, $id) = @_;

	my $rtedir = $self->{_rteregdir} . "/" . $id;

	my $ret = dir_lock($rtedir, $self->{_jobname});

	if ($ret == 0) {
		# couldn't lock
		return 0;
	} elsif (! -d "$rtedir/state") {
		# we cretated it, we don't want to.
		dir_lock_remove($rtedir, 1);
		return 0;
	}


	$self->{_id} = $id;
	$self->{_rtedir} = $rtedir;
	$self->{_connected} = 1;
	
	return 1;
}

######################################################################
######################################################################

=item connect

Disconnects from the RTE. After this the object can be reused.

=cut

sub disconnect {
	my ($self) = @_;

	dir_unlock($self->{_rtedir});

	$self->{_id} = undef;
	$self->{_rtedir} = undef;
	$self->{_connected} = 0;
	
	return 1;
}

=item remove

Disconnects from the RTE and removes it. After this the object can be reused.

=cut

sub remove {
	my ($self) = @_;

	dir_lock_remove($self->{_rtedir}, 1);

	$self->{_id} = undef;
	$self->{_rtedir} = undef;
	$self->{_connected} = 0;
	
	return 1;
}

=item state($new)

If $new is defined it sets the state of the RTE, otherwise it returns the
current state. If an error occurs it calles die.

=cut

sub state {
	my ($self, $new) = @_;

	if (defined $new) {
		my $old;
		for (my $i = 1; $i < scalar (statefiles); $i++)  {
			my $file = $self->{_rtedir} . "/" . (statefiles)[$i];
			if (-e $file) {
				$old = $file;
				last;
			}
		}
		rename $old, $self->{_rtedir} . "/" . (statefiles)[$new]
			or die "Can't rename statefile: $!\n";

		return $new;
	} else {
		# there is no state with number 0
		for (my $i = 1; $i < scalar (statefiles); $i++)  {
			my $file = $self->{_rtedir} . "/" . (statefiles)[$i];
			return $i if -e $file;
		}

		die "Can't get the current state of the RTE!\n";	
	}
}

=item rte_list

Returns the RTE this object represents as a list of strings. In case of error
an empty list is returned.

=cut

sub rte_list {
	my ($self) = @_;

	open my $f, "<" . $self->{_rtedir} . "/list"  or return ();
	my @rte = <$f>;
	chomp @rte;

	return @rte;
}
	

sub unuse_it {
	my ($self, $jobid) = @_;
	if (defined $jobid) {
		unlink $self->{_rtedir} . "/jobs/" . $jobid;
	}
}

=item used($id)

This can be used in three different ways. If $id is defined it is added to the
list of jobs which use this RTE. If $id is not defined and the method is used
in list context it returns the list of jobs currently using the rte. In scalar
context it returns true iff the RTE is currently used.

=cut

sub used {
	my ($self, $id) = @_;

	my $jdir = $self->{_rtedir} . "/jobs";

	if (defined $id) {
		open my $f, "> $jdir/$id"	or return 0;
		printf $f "%s\n", time;

		return 1;

	} else {
		return unless defined wantarray;

		my @ret;

		opendir my $dir, $jdir
			or die "Can't open directory $jdir: $!";

		while (my $entry = readdir $dir) {
			next if $entry =~ m/^\./;
			next unless -f "$jdir/$entry";

			push @ret, $entry;
			last unless wantarray;
		}

		closedir $dir;
	
		return wantarray ? @ret : (@ret != 0);
	}
}

=item last_used

Returns the timestamp of the last change of the list of jobs using
the RTE.

=cut

sub last_used {
	my ($self) = @_;
	my $st =stat($self->{_rtedir} . "/jobs")	or return undef;
	return $st->mtime;
}

=item id

Returns the id of this RTE. To be used with reconnect.

=cut

sub id {
	my ($self) = @_;
	return $self->{_id};
}

=item attach_data

Attaches a list of strings to the RTE. This is used to store information on how
the RTE is actually deployed. But this class knows nothing about it, it just
stores a list of strings. This method can be executed multiple times and the
arguments will be accumulated.

=cut

sub attach_data {
	my ($self, @data) = @_;
	open my $f, ">>" . $self->{_rtedir} . "/data";
	print $f "$_\n" foreach ( @data );
}

=item retrieve_data

Returns the data attached with attach_data. It is returned in the same order as
attached.

=cut

sub retrieve_data {
	my ($self) = @_;
	my @ret;

	open my $f, "<" . $self->{_rtedir} . "/data" or return undef;
	@ret = <$f>;
	chomp @ret;

	return @ret;
}	

=item all_rtes($rte_dir)

Returns a list of all RTEs by id.

=cut

sub all_rtes {
	my ($rte_dir) = @_;
	my @ret;

	opendir my $dir, $rte_dir or return ();
	while (my $entry = readdir $dir) {
		next if $entry =~ m/^\./;
		next unless -d "$rte_dir/$entry";
		push @ret, $entry;
	}
	closedir $dir;

	return @ret;
}

=item state2string($state)

Converts the state given as an integer to a descriptive string.

=cut

sub state2string {
	my ($state) = @_;

	return undef unless defined $state;

	my $ret;
	switch ($state) {
		case FAILED		{ $ret = "FAILED"; }
		case INSTALLED_A	{ $ret = "INSTALLED_A"; }
		case INSTALLED_M	{ $ret = "INSTALLED_M"; }
		case VALIDATED		{ $ret = "VALIDATED"; }
		case BROKEN		{ $ret = "BROKEN"; }
		case INSTALLABLE	{ $ret = "INSTALLABLE"; }
		case REMOVAL_PENDING	{ $ret = "REMOVAL_PENDING"; }
		case UNKNOWN		{ $ret = "UNKNOWN"; }
	}
	return $ret;
}

=item string2state($string)

Converts the state given as a string to an integer.

=cut

sub string2state {
	my ($string) = @_;

	return undef unless defined $string;

	my $ret;
	switch ($string) {
		case "FAILED"		{ $ret = FAILED; }
		case "INSTALLED_A"	{ $ret = INSTALLED_A; }
		case "INSTALLED_M"	{ $ret = INSTALLED_M; }
		case "VALIDATED"	{ $ret = VALIDATED; }
		case "BROKEN"		{ $ret = BROKEN; }
		case "INSTALLABLE"	{ $ret = INSTALLABLE; }
		case "REMOVAL_PENDING"	{ $ret = REMOVAL_PENDING; }
		case "UNKNOWN"		{ $ret = UNKNOWN; }
	}
	return $ret;
}

######################################################################
# given a list of RTEs it calculates an uniq id which is later used
# as the name for the directory.
#######################################################################
sub _calculate_RTE_ID {
	my (@RTE) = @_;
	my $first;

	@RTE  = sort { $a cmp $b } @RTE;

	($first = $RTE[0]) =~ s/[^a-z0-9]/_/ig;

	return $first . "-" . md5_hex join "\000", @RTE;
}

1;

=head1 SEE ALSO

Janitor::Job

=cut

