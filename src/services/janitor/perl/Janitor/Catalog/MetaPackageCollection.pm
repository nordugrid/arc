package Janitor::Catalog::MetaPackageCollection;

use Janitor::Catalog::MetaPackage;

use strict;

=head1 NAME

Janitor::Catalog::MetaPackageCollection - A Collection of all MetaPackagesread.

=head1 SYNOPSIS

use Janitor::Catalog::MetaPackageCollection;

=head1 DESCRIPTION

This class is a singleton. It stores all MetaPackages by key. All objects
added to the Collection must be marked immutable.

=head1 METHODS

=over 4

=cut

our $singleton = undef;

######################################################################
######################################################################

=item get($id)

Returns the Note with id $id or undef if there is no such Note.

=cut

sub get {
	my ($class,$id) = @_;
	unless (defined $singleton) {
		$singleton = bless {}, $class;
	}
	return $singleton->{$id};
}


######################################################################
######################################################################

=item add($id, $obj)

Adds the object $obj with the id $id to the collection.

=cut


sub add {
	my ($class, $id, $obj) = @_;
	unless (defined $singleton) {
		$singleton = bless {}, $class;
	}
	# XXX what if such an object already is in the collection?
	$singleton->{$id} = $obj;
}

1;

=head1 SEE ALSO

Janitor::Catalog::Note

=cut

