package Janitor::Catalog::DebianPackage;
@ISA = (Janitor::Catalog::Package);

use strict;

=head1 NAME

Janitor::Catalog::DebianPackage - Manages information about one debian package.

=head1 SYNOPSIS

use Janitor::Catalog::DebianPackage;

=head1 DESCRIPTION

This class is a decendant of Janitor::Catalog::Package. It represents
details of information otherwise only found in the Debian package
annotation.

To deploy packages of this kind, the virtualisation of the ARC
infrastructure is needed. This is not yet supported by the Janitor.

=head1 METHODS

=over 4

=cut

######################################################################
# The constructor.  This class has the following attributes:
#	_package (a list of the packages to be installed)
#	_debconf (the entries to feed to debconf)
######################################################################
sub new {
	return bless {}, shift;
}

######################################################################
######################################################################

=item package(@package)

Sets and gets the list of debian packages which must be installed

=cut

sub package {
	my ($self, @package) = @_;
	$self->{_package} = @package if @package;
	return $self->{_package};
}

######################################################################
######################################################################

=item debconf(@debconf)

Sets and gets the list of debconf entries which must be feeded to debconf
prior to the installation.

=cut

sub debconf {
	my ($self, @debconf) = @_;
	$self->{_debconf} = @debconf if @debconf;
	return $self->{_debconf};
}

######################################################################
######################################################################

=item 

FIXME: Daniel, it this the URL from the debian/control?

=cut

sub url {
	my ($self, $url) = @_;
	$self->{_url} = $url if defined $url;
	return $self->{_url};
}

=back

=cut

