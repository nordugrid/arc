package Janitor::Catalog::Note;

use strict;

my $DEBUG = 1;

=head1 NAME

Janitor::Catalog::Note - Manages information about a Note. Abstract class;

=head1 SYNOPSIS

use Janitor::Catalog::Note;

=head1 DESCRIPTION

This class is used to store the informations about one Note.

=head1 METHODS

=over 4

=cut

######################################################################
######################################################################

=item new()

The constructor.  Creates a new, empty object.
This class has the following attributes:

=over 4

=item description	- the Description of the Note)

=item id		- the RDF ID)

=item immutable	- if true the objects can not be changed anymore)

=item name		- the name of the Note)

=back

=cut

sub new {
	return bless {}, shift;
}

######################################################################
######################################################################

=item fill()

=cut

sub fill {
	my ($self, %val) = @_;
	foreach my $key (keys %val) {
		if ($key eq "name") {
			$self->name($val{$key});
		} elsif ($key eq "id") {
			$self->id($val{$key});
		} elsif ($key eq "description") {
			$self->description($val{$key});
		}
	}
}

######################################################################
######################################################################

=item name($name)

Sets and gets the name of the note. The argument is optional.

=cut

sub name {
	my ($self, $name) = @_;
	if (defined $name) {
		die "Trying to change immutable attribute of Class Note"
			if defined $self->{_immutable} and $self->{_immutable};
		$self->{_name} = $name;
	}
	return $self->{_name};
}

######################################################################
######################################################################

=item id($id)

Sets and gets the id of the note. The argument is optional.

=cut

sub id {
	my ($self, $id) = @_;
	if (defined $id) {
		die "Trying to change immutable attribute of Class Note"
			if defined $self->{_immutable} and $self->{_immutable};
		$self->{_id} = $id;
	}
	return $self->{_id};
}

######################################################################
######################################################################

=item description($description)

Sets and gets the description of the note.

=cut

sub description {
	my ($self, $description) = @_;
	if (defined $description) {
		die "Trying to change immutable attribute of Class Note"
			if defined $self->{_immutable} and $self->{_immutable};
		$self->{_description} = $description;
	}
	return $self->{_description};
}

######################################################################
######################################################################

=item immutable($immutable)

If $immutable is true the Note is marked as immuatable. After this it is
not possible anymore to change its values. Once marked immutable this process
can not be reversed.

=back

=cut

sub immutable {
	my ($self, $immutable) = @_;
	$self->{_immutable} = $immutable if (defined $immutable and $immutable != 0);
	return $self->{_immutable};
}

1;

=head1 SEE ALSO

Janitor::Catalog::BaseSystem
Janitor::Catalog::Tag

=cut

