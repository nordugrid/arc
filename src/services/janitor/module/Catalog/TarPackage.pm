package Janitor::Catalog::TarPackage;
@ISA = (Janitor::Catalog::Package);

use strict;

=head1 NAME

Janitor::Catalog::TarPackage - Manages information about one tar package.

=head1 SYNOPSIS

use Janitor::Catalog::TarPackage;

=head1 DESCRIPTION

This class is a descendant of Janitor::Catalog::Package. Below only the additional
methods are listed.

=cut

######################################################################
######################################################################

=over 4

=item The constructor

This class has the following attributes:

=over 8

=item url - a string describing where to get the tar file

=back

=back

=cut

sub new {
	return bless {}, shift;
}

######################################################################
######################################################################

=head1 METHODS

=over 4

=item url($url)

Sets and gets the url of the tar package. The URL refers the location
to download the tar file.

=back

=cut

sub url {
	my ($self, $url) = @_;
	$self->{_url} = $url if defined $url;
	return $self->{_url};
}

=name1 SEE ALSO

Janitor::Catalog::Package

=cut
