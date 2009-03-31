package Janitor::Job;

use Janitor::Util qw(dir_lock dir_unlock dir_lock_remove);


use strict;


=head1 NAME

Janitor::Job - Stores the Information about a Job.

=head1 SYNOPSIS

use Janitor::Job;

=head1 DESCRIPTION

Objects of this class are used to store all the data we have about a job.

=head1 METHODS

=over 4

=cut

use constant {
	PREPARED	=> 1,
	INITIALIZED	=> 2,
};

use constant statefiles => qw(
	NOSTATE
	prepared
	initialized
);


######################################################################
# The constructor. This class has the following attributes:
#	_jobdir		(the direcotry which is used to store information about this job)
#	_jobregdir	(the directory which is used to store informations about all jobs)
#	_name		(the name of the Job)
######################################################################

=item new($name, $jobregdir)

The costructor. 

=cut

sub new {
	my ($class, $name, $jobregdir) = @_;

	my $self = {
		_jobregdir => $jobregdir,
		_jobdir => $jobregdir . "/" . $name,
		_name => $name,
	};
	
	return bless $self, $class;
}

######################################################################
######################################################################

=item create(@RTE)

This method is used to register a new job. The new job is in the state
PREPARED.

=cut

sub create {
	my ($self, @RTE) = @_;

	# we want to create it. So we can assume it does not exists. If it exists its a
	# bug, so only wait for a short time.
	my $ret = dir_lock($self->{_jobdir}, $self->{_name}, 3);
	if ($ret == 1) {
		# we got the lock on the directory
		if ( defined $self->state ) {
			dir_unlock($self->{_jobdir});
			return undef;	# already exists
		}
		open my $f, ">" . $self->{_jobdir} . "/" . (statefiles)[PREPARED];
	} else {
		# we couldn't get the lock in 3 seconds, this is an error in any case
		return undef;
	}

	my $f;

	open $f, ">" . $self->{_jobdir} . "/created";
	printf $f "%s\n", time;

	open $f, ">" . $self->{_jobdir} . "/rte";
	print $f "$_\n" foreach (@RTE);

	return 1;
}

=item open

This method opens a previously created job.

=cut

sub open {
	my ($self) = @_;

	# we want to open it.
	my $ret = dir_lock($self->{_jobdir}, $self->{_name});

	if ($ret == 0) {
		return 1;
	} elsif ( ! defined $self->state) {
		# we created it, so remove it
		dir_lock_remove($self->{_jobdir}, 1);
		return 1;
	}

	return 0;
}

=item disconnect

The method disconnects removes the lock from the job.

=cut

sub disconnect {
	my ($self) = @_;
	dir_unlock($self->{_jobdir});
}

=item remove

Removes the Job.

=cut


sub remove {
	my ($self) = @_;
	
	dir_lock_remove($self->{_jobdir}, 1);

	return 1;
}

=item initialize($rte_key, @manual)

This method is used to change the state of the RTE from PREPARED to
INITIALIZED. The arguments are stored and can be retrieved with the
methods rte_key and manual. They are used to store informations about
the way of deployment of the RTE used by the job.

=cut

sub initialize {
	my ($self, $rte_key, @manual) = @_;
	
	return undef if $self->state == INITIALIZED;

	my $f;

	CORE::open $f, ">" . $self->{_jobdir} . "/rte_key";
	printf $f "%s\n", $rte_key if defined $rte_key;

	CORE::open $f, ">" . $self->{_jobdir} . "/manual";
	print $f "$_\n", foreach (@manual);

	rename $self->{_jobdir} . "/prepared", $self->{_jobdir} . "/initialized";

	return 1;
}

=item rte_key, manual

Returns the $rte_key, $manual argument of initialize()

=cut


sub rte_key {
	my ($self) = @_;

	return undef unless $self->state == INITIALIZED;
	my $f;
	CORE::open $f, "<" . $self->{_jobdir} . "/rte_key";
	my @rte_key = <$f>;
	chomp @rte_key;

	return $rte_key[0];
}


sub manual {
	my ($self) = @_;

	return undef unless $self->state == INITIALIZED;
	my $f;
	CORE::open $f, "<" . $self->{_jobdir} . "/manual";
	my @manual = <$f>;
	chomp @manual;

	return @manual;
}

=item created

Returns the time this job was registered.

=cut

sub created {
	my ($self) = @_;
	
	my $f;
	CORE::open $f, "<" . $self->{_jobdir} . "/created";
	my $created = <$f>;
	chomp $created;

	my $now = time;
	if ($created <= $now and $now - $created < 60 * 60 * 24 * 365) {
		return $created
	} else {
		return undef;
	}
}

=item id

Returns the id, i.e. the name, of this job.

=cut


sub id {
	my ($self) = @_;
	return $self->{_name};
}

=item rte

Returns the rtes used by this job as given to create().

=cut

sub rte {
	my ($self) = @_;
	my @rte;

	if (! -e $self->{_jobdir} . "/rte") {
		# it should exist, as we are in INITIALIZED or PREPARED
		return undef;
	}
	my $f;
	CORE::open $f, "<" . $self->{_jobdir} . "/rte";
	@rte = <$f>;
	close $f;

	chomp @rte;

	return @rte;
}

=item state

Returns the current state of the job.

=cut

sub state {
	my ($self, $new) = @_;

	if ( -e $self->{_jobdir} . "/initialized") {
		return INITIALIZED;
	} elsif ( -e $self->{_jobdir} . "/prepared") {
		return PREPARED;
	}

	# this should be never reached
	return undef;
}

=item all_jobs($jobregdir)

Returns a list of all registered jobs as a Janitor::Job-object. To
use these objects open must be called.

=cut

sub all_jobs{
	my ($jobregdir) = @_;
	my @ret;

	opendir my $dir, $jobregdir or return ();
	while (my $entry = readdir $dir) {
		next if $entry =~ m/^\./;
		next unless -d "$jobregdir/$entry";

		push @ret, new Janitor::Job($entry, $jobregdir);
	}
	closedir $dir;

	return @ret;
}

1;

=head1 SEE ALSO

Janitor::RTE

=cut

