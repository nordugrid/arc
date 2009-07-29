package Janitor::Catalog::Package;

use strict;

=head1 NAME

Janitor::Catalog::Package - Manages information about one package.

=head1 SYNOPSIS

use Janitor::Catalog::Package;

=head1 DESCRIPTION

The package implements the Runtime Environment on a particular Base System.

=head1 METHODS

=over 4

=item new()

The constructor.

=cut

######################################################################
# The constructor. This class has the following attributes:
#	_id		(the RTF ID)
#	_basesystem	(the basesystem as RTF ID)
#	_depends	(the dependencies as an arry of RTF IDs)
######################################################################
sub new {
	return bless {}, shift;
}

######################################################################
######################################################################

=item id($id)

Sets and gets the id of the package.

=cut

sub id {
	my ($self, $id) = @_;
	$self->{_id} = $id if defined $id;
	return $self->{_id};
}

######################################################################
######################################################################

=item basesystem($basesystem)

Sets and gets the basesystem of the package.

=back

=cut

sub basesystem {
	my ($self, $basesystem) = @_;
	$self->{_basesystem} = $basesystem if defined $basesystem;
	return $self->{_basesystem};
}

######################################################################
######################################################################
# XXX currently not used
sub _depends {
	my ($self, @depends) = @_;
	$self->{_depends} = @depends if @depends;
	return $self->{_depends};
}

1;

=name1 SEE ALSO

Janitor::Catalog::MetaPackage

=cut
