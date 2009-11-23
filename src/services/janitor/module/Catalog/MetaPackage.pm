package Janitor::Catalog::MetaPackage;

use strict;

=head1 NAME

Janitor::Catalog::MetaPackage - Manages information about one metapackage

=head1 SYNOPSIS

use Janitor::Catalog::MetaPackage;

=head1 DESCRIPTION

MetaPackages represent the Runtime Environments (REs) in the traditional
sense of ARC. The Packages are means to implement the RE, but not to be mistaken
for the REs themselves. The availability of Base Systems on which the
packages depend, render it obvious that there must be alternatives in packages
to chose from to fulfill the requirements of a MetaPackage.

=head1 METHODS

=over 4

=item new()

The Constructor. Set are

=over 4

=item id - unique identifier of the MetaPackage

=item name - human-interpretable name

=item homepage - web location at which to find further details

=item description - longer text explaining the functionality of the package

=item lastupdate - date of last change

=item immutable - internal indicator if the information may be amended

=item tag - reference to array of tags

=back

=cut

sub new {
	my ($class) = @_;

	my $self = {
		_id => undef,
		_name => undef,
		_homepage => undef,
		_description => undef,
		_lastupdate => undef,
		_immutable => 0,
		_tag => [],
	};
	
	bless $self, $class;
	return $self;
}

######################################################################
######################################################################

=item name($name)

Sets and gets the name of the MetaPackage.

=cut

sub name {
	my ($self, $name) = @_;
	if (defined $name) {
		die "Trying to change an immutable attribute of Class BaseSystem"
			if defined $self->{_immutable} and $self->{_immutable};
		$self->{_name} = $name;
	}
	return $self->{_name};
}

######################################################################
######################################################################

=item id($id)

Sets and gets the id of the metapackage.

=cut

sub id {
	my ($self, $id) = @_;
	if (defined $id) {
		die "Trying to change an immutable attribute of Class BaseSystem"
			if defined $self->{_immutable} and $self->{_immutable};
		$self->{_id} = $id;
	}
	return $self->{_id};
}

######################################################################
######################################################################

=item description($description)

Sets and gets the description of the metapackage.

=cut

sub description {
	my ($self, $description) = @_;
	if (defined $description) {
		die "Trying to change an immutable attribute of Class BaseSystem"
			if defined $self->{_immutable} and $self->{_immutable};
		$self->{_description} = $description;
	}
	return $self->{_description};
}

######################################################################
######################################################################

=item homepage($homepage)

Sets and gets the homepage of the metapackage.

=cut

sub homepage {
	my ($self, $homepage) = @_;
	if (defined $homepage) {
		die "Trying to change an immutable attribute of Class BaseSystem"
			if defined $self->{_immutable} and $self->{_immutable};
		$self->{_homepage} = $homepage;
	}
	return $self->{_homepage};
}

######################################################################
######################################################################

=item lastupdate($lastupdate)

Sets and gets the lastupdate of the metapackage.

=cut

sub lastupdate {
	my ($self, $lastupdate) = @_;
	if (defined $lastupdate) {
		die "Trying to change an immutable attribute of Class BaseSystem"
			if defined $self->{_immutable} and $self->{_immutable};
		$self->{_lastupdate} = $lastupdate;
	}
	return $self->{_lastupdate};
}

######################################################################
######################################################################

=item tag($tag)

If $tag is undef it is added to the list of tags. In any case a list of
all tags of the metapackage is returned.

=cut

sub tag {
	my ($self, $tag) = @_;
	if (defined $tag) {
		die "Trying to change an immutable attribute of Class BaseSystem"
			if defined $self->{_immutable} and $self->{_immutable};
		push @{$self->{_tag}}, $tag if defined $tag;
	}
	return @{$self->{_tag}};
}

######################################################################
#######################################################################

=item immutable($immutable)

If $immutable is true the object is marked as immuatable. After this it is
not possible anymore to change its values. Once marked immutable this process
can not be reversed.

=back

=cut

sub immutable {
	my ($self, $immutable) = @_;
	$self->{_immutable} = $immutable if (defined $immutable and $immutable);
	return $self->{_immutable};
}

1;

=name1 SEE ALSO

Janitor::Catalog::Package;
Janitor::Catalog::Catalog;

=cut
