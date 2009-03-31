package Janitor::TarPackageShared;

use Janitor::Util qw(dir_lock dir_unlock dir_lock_remove);

use strict;


=head1 NAME

Janitor::TarPackageShared - 

=head1 SYNOPSIS

use Janitor::TarPackageShared;

=head1 DESCRIPTION

This class is used to store informations about a tar package deployed on
a shared medium.

=head1 METHODS

=over 4

=cut

use constant {
	INSTALLED	=> 1,
	UNKNOWN		=> 2,
};

use constant statefiles => qw(
	NOSTATE
	state/installed
	state/unknown
);


=item new($name, $packageregdir)

The constructor.  Creates a new object. $name is the name of the package and
used as an id. $packageregdir is the place where we store all the things.

=cut

sub new {
	my ($class, $name, $packageregdir) = @_;

	my $self = {
		_packageregdir => $packageregdir,
		_packagedir => $packageregdir . "/" . $name,
		_name => $name,
		_connected => 0,
	};
	
	return bless $self, $class;
}

=item connect, reconnect

This methods links the object to the data stored in the file system. This data
is locked until either disconnect or remove is called.

If there is no data on this package in the filesystem connect creates it and
reconnect failes.

=cut

sub connect {
	my ($self) = @_;

	my $ret = dir_lock($self->{_packagedir}, $self->{_name}, 3);

	if ($ret != 1) {
		# we couldn't get the lock
		return 0;

	} elsif ( ! -d $self->{_packagedir} . "/state") {
		# we created it, so put it into unknown state
		mkdir $self->{_packagedir} . "/used";
		mkdir $self->{_packagedir} . "/state";
		open my $f, ">" . $self->{_packagedir} . "/" . (statefiles)[UNKNOWN];
	
	}

	$self->{_connected} = 1;

	return 1;
}

sub reconnect {
	my ($self) = @_;

	my $ret = dir_lock($self->{_packagedir}, $self->{_name});

	if ($ret != 1) {
		# we couldn't get the lock
		return 0;

	} elsif ( ! -d $self->{_packagedir} . "/state") {
		# we created it, so remove it
		dir_lock_remove($self->{_packagedir}, 1);
		return 0;
	}

	$self->{_connected} = 1;

	return 1;
}

=item disconnect, remove

This methods remove the lock on the file system data representing this package.
The method remove additionaly removes it.

=cut

sub disconnect {
	my ($self) = @_;
	dir_unlock($self->{_packagedir}) if $self->{_connected};
	$self->{_connected} = 0;
}

sub remove {
	my ($self) = @_;
	dir_lock_remove($self->{_packagedir}, 1);
	$self->{_connected} = 0;
}

=item installed($inst_dir, $runtimescript)

This method changes the state of the package from UNKNOWN to INSTALLED. The
values $inst_dir and $runtimescript are stored and can be retrieved later
with inst_dir and runtime.

=cut

sub installed {
	my ($self, $inst_dir, $runtimescript) = @_;

	if ($self->state != UNKNOWN) {
		return undef;
	}

	my $f;
	
	open $f, ">" . $self->{_packagedir} . "/inst_dir";
	printf $f "%s\n", $inst_dir if defined $inst_dir;

	open $f, ">" . $self->{_packagedir} . "/runtime";
	printf $f "%s\n", $runtimescript if defined $runtimescript;

	$self->state(INSTALLED);

	return 1;
}

=item inst_dir

Retuns the value of $inst_dir parameter in the call to installed.

=cut

sub inst_dir {
	my ($self) = @_;

	return undef unless $self->state == INSTALLED;

	my @instdir;
	open my $f, "<" . $self->{_packagedir} . "/inst_dir" or return undef;
	@instdir = <$f>;
	chomp @instdir;

	return $instdir[0];
}

=item runtime

Retuns the value of $runtimescript parameter in the call to installed.

=cut

sub runtime {
	my ($self) = @_;

	return undef unless $self->state == INSTALLED;

	my @runtime;
	open my $f, "<" . $self->{_packagedir} . "/runtime" or return undef;
	@runtime = <$f>;
	chomp @runtime;

	return $runtime[0];
}
	
=item used($id)

If $id is defined it is added to the list of users of this package. If $id is
not defined and the method is called in list context it returns the list of
users. In scalar context it returns true iff there are users.

=cut

sub used {
	my ($self, $id) = @_;

	my $udir = $self->{_packagedir} . "/used";

	if (defined $id) {
		open my $f, "> $udir/$id";
		printf $f "%s\n", time;

		return 1;

	} else {
		return unless defined wantarray;

		my @ret;

		opendir my $dir, $udir
			or die "Can't open directory $udir: $!";

		while (my $entry = readdir $dir) {
			next if $entry =~ m/^\./;
			next unless -f "$udir/$entry";

			push @ret, $entry;
			last unless wantarray;
		}

		closedir $dir;
	
		return wantarray ? @ret : (@ret != 0);
	}
}

=item not_used($id)

This method removes $id from the list of users or this package.

=cut

sub not_used {
	my ($self, $id) = @_;

	return 0 unless defined $id;

	unlink $self->{_packagedir} . "/used/" . $id;

	return 1;
}

=item id

Returns the id, i.e. the name, of this package.

=cut

sub id {
	return $_[0]->{_name};
}

=item state($new)

If $new is defined this method sets the state to $new. If $new is undefined it
returns the current state.

=cut

sub state {
	my ($self, $new) = @_;

	my $old = undef;

	for (my $i = 1; $i < scalar (statefiles); $i++)  {
		if (-e $self->{_packagedir} . "/" . (statefiles)[$i]) {
			$old = $i;
			last;
		}
	}

	if (!defined $old) {
		die "Can't get current state of package!\n";
	}

	if (defined $new and $old != $new) {
		my $oldfile = $self->{_packagedir} . "/" . (statefiles)[$old];
		my $newfile = $self->{_packagedir} . "/" . (statefiles)[$new];
		rename $oldfile, $newfile	or die "Can't rename statefile: $!\n";
		return $new;
	}

	return $old;
}

=item all_package($pdir)

Returns a list of all packages as Janitor::TarPacakgeShared objects. To use these
objects reconnect must be called.

=cut

sub all_packages {
	my ($pdir) = @_;
	my @ret;

	opendir my $dir, $pdir or return ();
	while (my $entry = readdir $dir) {
		next if $entry =~ m/^\./;
		next unless -d "$pdir/$entry";

		push @ret, new Janitor::TarPackageShared($entry, $pdir);
	}
	closedir $dir;

	return @ret;
}

1;

# EOF
