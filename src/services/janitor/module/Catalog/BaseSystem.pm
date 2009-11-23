package Janitor::Catalog::BaseSystem;

use Janitor::Catalog::Note;

@ISA = (Janitor::Catalog::Note);

use strict;

my $DEBUG = 1;

=head1 NAME

Janitor::Catalog::BaseSystem - Manages information about a single BaseSystem

=head1 SYNOPSIS

use Janitor::Catalog::BaseSystem;

=head1 DESCRIPTION

The traditional term 'Runtime Environment' does not care about
the operating system it is installed on. With an automation of the
installation process, the understanding of operating-system dependencies
becomes essential. This class represents such an operating system that
hosts the runtime environment. With the upcopming virtualisation, the
understanding of BaseSystems will become more important, as several
preconfigured base system may compete for the additional installation of
a particular package to fulfill the requirements of a Runtime Environment.

=head1 METHODS

=over 4

=cut

######################################################################
######################################################################

=item new()

The constructor.  It creates a new, still empty object of this class.

This class has the following additional attributes:

=over 4

=item lastupdate	- the time of the last update

=item distribution	- the kind of distribution (Debian, SLC, ...)j

=item url		- where to find an image of this basesystem

=item short_description	- a human friendly short name of the basesystem

=cut

sub new {
	return bless {}, shift;
}

=item fill()

=cut

sub fill {
	my ($self, %val) = @_;
	$self->SUPER::fill(%val);
	foreach my $key (keys %val) {
		if ($key eq "last_update") {
			$self->last_update($val{$key});
		} elsif ($key eq "distribution") {
			$self->distribution($val{$key});
		} elsif ($key eq "url") {
			$self->url($val{$key});
		} elsif ($key eq "short_description") {
			$self->short_description($val{$key});
		}
	}
}

######################################################################
######################################################################

=item distribution($distribution)

Sets and gets the distribution filed of the basesystem.

=cut

sub distribution {
	my ($self, $distribution) = @_;
	if (defined $distribution) {
		die "Trying to change immutable attribute of Class BaseSystem"
			if defined $self->{_immutable} and $self->{_immutable};
		$self->{_distribution} = $distribution;
	}
	return $self->{_distribution};
}

######################################################################
######################################################################

=item url($url)

Sets and gets the url url of the basesystem.

=cut

sub url {
	my ($self, $url) = @_;
	if (defined $url) {
		die "Trying to change immutable attribute of Class BaseSystem"
			if defined $self->{_immutable} and $self->{_immutable};
		$self->{_url} = $url;
	}
	return $self->{_url};
}

######################################################################
######################################################################

=item short_description($short_description)

Sets and gets the short_description url of the basesystem.

=cut

sub short_description {
	my ($self, $short_description) = @_;
	if (defined $short_description) {
		die "Trying to change immutable attribute of Class BaseSystem"
			if defined $self->{_immutable} and $self->{_immutable};
		$self->{_short_description} = $short_description;
	}
	return $self->{_short_description};
}

=head1 SEE ALSO

Janitor::Catalog::MetaPackage
Janitor::Catalog::Package

=cut

