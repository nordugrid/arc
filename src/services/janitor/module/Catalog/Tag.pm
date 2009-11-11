package Janitor::Catalog::Tag;

use Janitor::Catalog::Note;

@ISA = (Janitor::Catalog::Note);

use strict;

my $DEBUG = 1;

=head1 NAME

Janitor::Catalog::Tag - Manages information about a single Tag.

=head1 SYNOPSIS

use Janitor::Catalog::Tag;

=head1 DESCRIPTION

=head1 METHODS

=over 4

=cut

######################################################################
# The constructor. This class has no additional attributes.
######################################################################

=item new()

The constructor.  It creates a new, still empty object of this class.

=cut

sub new {
	return bless {}, shift;
}

=head1 SEE ALSO

Janitor::Catalog::Note

=cut

