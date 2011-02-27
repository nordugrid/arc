package Janitor::Catalog;

use Exporter;
@ISA = qw(Exporter);
@EXPORT_OK = qw();


=head1 NAME

Janitor::Catalog - Implements the Interface to the Catalog

=head1 SYNOPSIS

use Janitor::Catalog;

=head1 DESCRIPTION

The Catalog is implemented as an RDF document.  This class transforms
the RDF into Perl classes. The writing to the document has not yet been
implemented.

=head1 METHODS

=over 4

=cut



use POSIX;
use XML::Simple;

use strict;
use warnings;

use Janitor::Catalog::BaseSystem;
use Janitor::Catalog::MetaPackage;
use Janitor::Catalog::Package;
use Janitor::Catalog::TarPackage;
use Janitor::Catalog::DebianPackage;

use Janitor::Catalog::NoteCollection;
use Janitor::Catalog::MetaPackageCollection;
use Janitor::Catalog::Tag;

use Janitor::Logger;

my $logger = Janitor::Logger->get_logger("Janitor::Catalog");

######################################################################
# Constructor, opens the catalog and returns a object for accessing it.
######################################################################

=item new($class, $file, $name)

The constructor for this class has two arguments, the name of the catalog file
and the name of the catalog. It uses RDF::Redland to open the calaog and returns an object for accessing it.

=cut

sub new {
	my ($this, undef, $name) = @_;

	my $class = ref($this) || $this;
	my $self = {
		_name => $name,
		_mask => {},
	};

	# our catalog storage
	$self->{BaseSystem} = {};
	$self->{MetaPackage} = {};
	$self->{TarPackage} = {};
	$self->{DebianPackage} = {};
	$self->{VirtualBaseSystem} = {};
	$self->{Tag} = {};

	return bless $self, $class;
}

###########################################################################
# utility functions for working with the hash tree returned by XML::Simple
###########################################################################

# walks a tree of hashes and arrays while applying a function to each hash.
sub _hash_tree_apply {
	my ($ref, $func) = @_;
	if (not ref($ref)) {
		return;
	} elsif (ref($ref) eq 'ARRAY') {
		map {_hash_tree_apply($_,$func)} @$ref;
		return;
	} elsif (ref($ref) eq 'HASH') {
		&$func($ref);
		map {_hash_tree_apply($_,$func)} values %$ref;
		return;
	} else {
		return;
	}
}

# Strips namespace prefixes from the keys of the hash passed by reference
sub _hash_strip_prefixes {
	my ($h) = @_;
	my %t;
	while (my ($k,$v) = each %$h) {
		next if $k =~ m/^xmlns/;
		$k =~ s/^\w+://;
		$t{$k} = $v;
	}
	%$h=%t;
	return;
}


=item $obj->add ($file)

This method adds the content of the $file to the catalog.

=cut

# XXX what to do with the name?

sub add {
	my ($self, $file) = @_;

	unless ( -e $file) {
		$logger->error("Can not add file to catalog: $file does not exist");
		return;
	}

	if (not -e '/usr/bin/xmllint') {
		$logger->error("/usr/bin/xmllint not found");
		die;
	}
	my $cmd = "/usr/bin/xmllint --c14n '$file'";
	my $bytes = `$cmd`;
	if ((my $status = $?) != 0) {
		if (WIFEXITED($status)) {
			$logger->error("$cmd: Command failed with exit code ".WEXITSTATUS($status));
		} elsif (WIFSIGNALED($status)) {
			$logger->error("$cmd: Command killed by signal ".WTERMSIG($status));
		}
		die;
	}
	my $ref;
	eval {
		$ref = XMLin($bytes, ForceArray => 1, KeepRoot => 1);
	};
	if ($@) {
		$logger->error("XML::Simple returned with an error");
		die;
	}

	# strip namespaces
	_hash_tree_apply($ref, \&_hash_strip_prefixes);

	my $xml = $ref->{RDF}[0];
	if (not ref $xml eq "HASH") {
		$logger->error("RDF root element not found") and die;
	}

	# temporary catalog storage
	my $tcat = {};
	$tcat->{BaseSystem} = {};
	$tcat->{MetaPackage} = {};
	$tcat->{TarPackage} = {};
	$tcat->{DebianPackage} = {};
	$tcat->{VirtualBaseSystem} = {};
	$tcat->{Tag} = {};

	my @classes = (qw( BaseSystem MetaPackage TarPackage DebianPackage VirtualBaseSystem Tag ));

	for my $class (@classes) {
		for my $obj (@{$xml->{$class}}) {

			my $id = $obj->{about};
			unless (defined $id and not ref $id) {
				$logger->warning("Found $class without ID");
				next;
			}

			##########################################################################
			# check well-formedness and simplify structure
			##########################################################################

			my @badattrs;

			# convert eventual arrays of scalars to scalars (pick first value from array)
			for my $attr (qw( label name url distribution description short_description distribution lastupdated homepage )) {
				if (ref $obj->{$attr} eq 'ARRAY' and @{$obj->{$attr}}) {
					my $val = $obj->{$attr}[0];
					$obj->{$attr} = $val;
					push @badattrs, $attr if ref $val; # must be scalar
				} elsif (ref $obj->{$attr}) {
					push @badattrs, $attr;
				}
			}
			# check that these are array of scalars. Convert scalar to array.
			for my $attr (qw( package debconf )) {
				next unless defined $obj->{$attr};
				if (not ref $obj->{$attr}) {
					$obj->{$attr} = [ $obj->{$attr} ];
				} elsif (ref $obj->{$attr} ne 'ARRAY') {
					push @badattrs, $attr;
				} else {
					for my $val (@{$obj->{$attr}}) {
						if (ref $val) {
							push @badattrs, $attr;
							last;
						}
					}
				}
			}
			# check references and simplify them
			for my $attr (qw( basesystem instance tag depends )) {
				next unless defined $obj->{$attr};
				if (ref $obj->{$attr} ne 'ARRAY') {
					push @badattrs, $attr;
				} else {
					my @list;
					for my $val (@{$obj->{$attr}}) {
						if (ref $val ne 'HASH' or not defined $val->{resource}
											   or ref $val->{resource}) {
							push @badattrs, $attr;
							last;
						}
						push @list, $val->{resource};
					}
					$obj->{$attr} = \@list;
				}
			}

			if (@badattrs) {
				$logger->warning("$class '$id' attribute \"$_\" has wrong syntax") for @badattrs;
				next;
			}

			if ($class eq 'TarPackage' or $class eq 'DebianPackage') {
				unless (@{$obj->{basesystem}} == 1) {
					$logger->warning("$class '$id' does not have exactly one basesystem");
					next;
				}
				$obj->{basesystem} = $obj->{basesystem}[0];
			} elsif ($class eq 'MetaPackage') {
				unless ($obj->{instance} and @{$obj->{instance}}) {
					$logger->verbose("$class '$id' has no instance, ignoring");
					next;
				}
			}

			# set the name
			my $name = $obj->{name} || $obj->{label};
			unless (defined $name) {
				$logger->warning("$class '$id' has no name and no label. Skipping it");
				next;
			}
			delete $obj->{about};
			delete $obj->{label};
			$obj->{name} = $name;

			$tcat->{$class}{$id} = $obj;
		}
			
	}

	# save contents of temporary store
	for my $class (@classes) {
		while (my ($key, $val) = each %{$tcat->{$class}}) {
			$self->{$class}{$key} = $val;
		}
	}

	return;
}


##########################################################################
# Check cross-references. To be called after all catalogs have been added.
##########################################################################

sub check_references {
	my ($self) = @_;

	unless (%{$self->{BaseSystem}}) {
		$logger->error("No BaseSystems found") and die;
	}
	unless (%{$self->{MetaPackage}}) {
		$logger->error("No MetaPackages found") and die;
	}
	unless (%{$self->{TarPackage}}) {
		$logger->error("No TarPackages found") and die;
	}

	my $once_more = 1;

	# loop until there are no more invalid cross-references
	while ($once_more) {

		$once_more = 0;

		my %allpackages = ( %{$self->{TarPackage}}, %{$self->{DebianPackage}} );
		while (my ($id, $pkg) = each %allpackages) {

			my $bsid = $pkg->{basesystem};
			my $bs = $self->{BaseSystem}{$bsid} || $self->{VirtualBaseSystem}{$bsid};
			unless ($bs) {
				$logger->warning("Package '$id' references invalid basesystem '$bsid'");
				delete $self->{TarPackage}{$id};
				delete $self->{DebianPackage}{$id};
				$once_more = 1;
			}

			for my $depid (@{$pkg->{depends}}) {
				my $dep = $self->{TarPackage}{$depid} || $self->{DebianPackage}{$depid} || $self->{MetaPackage}{$depid};
				unless ($dep) {
					$logger->warning("Package '$id' references invalid dependency '$depid'");
					delete $self->{TarPackage}{$id};
					delete $self->{DebianPackage}{$id};
					$once_more = 1;
				}
			}
		}

		while (my ($id, $mp) = each %{$self->{MetaPackage}}) {

			for my $pkgid (@{$mp->{instance}}) {
				my $pkg = $self->{TarPackage}{$pkgid} || $self->{DebianPackage}{$pkgid};
				unless ($pkg) {
					$logger->warning("MetaPackage '$id' references invalid instance '$pkgid'");
					delete $self->{MetaPackage}{$id};
					$once_more = 1;
				}
			}
			for my $tagid (@{$mp->{tag}}) {
				my $tag = $self->{Tag}{$tagid};
				unless ($tag) {
					$logger->warning("MetaPackage '$id' references invalid tag '$tagid'");
					delete $self->{MetaPackage}{$id};
					$once_more = 1;
				}
			}
		}
	}

	return;
}

=item name()

This method returns the name of the catalog.

=cut

sub name
{
	my ($self) = @_;
	my $name = $self->{_name};
	$name = "[NONAME]" unless defined $name and $name ne "";
	return $name;
}



###########################################################################
# Given a list of metapackages (by name) this funtions returns a list of all
# basesystems (as an object) which support all these metapackages.
###########################################################################

=item basesystems_supporting_metapackges(@mp)

Given a list of metapackages (by name) this method returns a list of
basesystems (as Janitor::Catalog::BaseSystem) which support all of these metapackages.

=cut

sub basesystems_supporting_metapackages {
	my ($self, @pakete) = @_;
	my $query = undef;
	my %basesystems;
	my @ids;

	#####
	# At first check if all requested packages are allowed
	#####
	foreach my $package (@pakete) {

		my $id;
		while (my ($tmpid, $tmph) = each %{$self->{MetaPackage}}) {
			next unless $tmph->{name} eq $package;
			$id = $tmpid;
			last;
		}
		keys %{$self->{MetaPackage}}; # reset iterator
		push @ids, $id;
		return () unless (defined $id and $self->{_mask}->{$id});
	}

	#####
	# query for each package, which basesystems support it
	#####
	foreach my $p_id (@ids) {

		my $mp = $self->{MetaPackage}{$p_id};
		for my $pkgid (@{$mp->{instance}}) {
			my $pkg = $self->{TarPackage}{$pkgid} || $self->{DebianPackage}{$pkgid};
			my $id = $pkg->{basesystem};
			next unless $self->{_mask}->{$id};
			if (! defined $basesystems{$id}) {
				$basesystems{$id}{$_} = 1 for @ids;
			}
			delete $basesystems{$id}{$p_id};
		}
	}

	#####
	# we still have to check if the dependencies are satisfied
	#####
	foreach my $b_key ( keys %basesystems ) {
		next if keys %{$basesystems{$b_key}};

		foreach my $p_key (@ids) {
			my ($r, @x) = $self->_check_dependencies($b_key, $p_key);
			unless ($r) {
				$basesystems{$b_key}{$p_key} = 1;
				last;
			}
		}
	}

	#####
	# and now check wich basesystems supports all packages
	#####
	my @ret = ();
	foreach my $b_key ( keys %basesystems ) {
		next if keys %{$basesystems{$b_key}};
		push @ret, $self->_BaseSystemByKey($b_key);
	}

	return @ret;
}

###########################################################################
###########################################################################

=item _check_dependencies($baseid, $packageid) - internal
	
Checks if the dependencies of MetaPackage $package (by id) are satisfied on
basesystem $baseid (by id). The method returns a tupel: The first element is true iff the
dependencies are satisfyable, the second element is a list of packages (by id)
which must be installed to provide the requested MetaPackage. If circles in
the dependecies are found then the first element of the list is false.

=cut

sub _check_dependencies {
	my ($self, $baseid, $packageid) = @_;

	my @todo;
	my %seen;
	my @ret;

	my $mp = $self->{MetaPackage}{$packageid};
	return (0,undef) unless $mp;

	for my $pkgid (@{$mp->{instance}}) {
		my $pkg = $self->{TarPackage}{$pkgid} || $self->{DebianPackage}{$pkgid};
		my $bsid = $pkg->{basesystem};
		push @todo, $pkgid if $bsid eq $baseid;
	}
	return (0,undef) unless @todo;

	# XXX here we need some more logic: For example DebianPackage is only
	# installable on Debian, etc.

	# and now check the dependencies
	while (my $pkgid = pop @todo) {

		# check if it is a new dependency
		next if defined $seen{$pkgid};
		$seen{$pkgid} = 1;

		push @ret, $pkgid;
		my $pkg = $self->{TarPackage}{$pkgid} || $self->{DebianPackage}{$pkgid};

		# check if the basesystem is right
		my $bsid = $pkg->{basesystem};
		unless ($bsid eq $baseid) {
			$logger->warning("found unexpected basesystem entry \"$bsid\" while checking "
				. "dependencies of meta package \"$packageid\". Expected \"$baseid\"");
			return (0,undef);
		}

		# and now get the dependencies
		for my $depid (@{$pkg->{depends}}) {
			if (my $tmp = $self->{MetaPackage}{$depid}) {
				# resolve instances that have the right basesystem
				my $installable = 0;
				for my $tpkgid (@{$tmp->{instance}}) {
					my $tpkg = $self->{TarPackage}{$tpkgid} || $self->{DebianPackage}{$tpkgid};
					my $tbsid = $tpkg->{basesystem};
					push @todo, $tpkgid if $tbsid eq $baseid;
					$installable = 1;
				}
				# this dependency is not installable
				return (0,undef) unless $installable;
			} else {
				push @todo, $depid;
			}
		}

	}

	return (1, reverse @ret);
}

######################################################################
# Returns a tupel. The first element is true if the requests metapackages (by
# name) can be provides on the requestes basesystem (by object). The second
# element is a  list of packages to be installed to provide all requested
# metapackages (by name) on the specified basesystem (by object).
#
# XXX This function only does some minimal checking. If it is not possible or
# XXX not allowed to provide the metapackages on this basesystem the return
# XXX value is not meaningful. So use basesystems_supporting_metapackages prior
# XXX to this function.
######################################################################

=item PackagesToBeInstalled($bs, @mp)

Given a basesystem $bs (as Janitor::Catalog::Basesystem) and a list of metapackages @mp
(as string) this method returns a list of packages (as Janitor::Catalog::Package) which
must be installed to provide the requested metapackages.

This method returns a tupel. The first element is true iff the requested
combination of metapackages is supported by the basesystem.  The second
element is the list of packages.

=cut

sub PackagesToBeInstalled {
	my ($self, $bs, @mp) = @_;

	my @ret = ();
	my %seen;

	foreach my $mpname ( @mp ) {

		my ($mpid, $mph);
		while (my ($tmpid, $tmph) = each %{$self->{MetaPackage}}) {
			next unless $tmph->{name} eq $mpname;
			$mph = $tmph;
			$mpid = $tmpid;
			last;
		}
		keys %{$self->{MetaPackage}}; # reset iterator
		return (0, undef) unless $mph;

		my ($retval, @r) = $self->_check_dependencies($bs->id, $mpid);

		return (0, undef) unless $retval;

		while (my $id = shift @r) {
			next if defined $seen{$id};
			$seen{$id} = 1;
			push @ret, $self->_PackageByKey($id);
		}
	}

	return (1, @ret);
}


######################################################################
######################################################################

=item  _BaseSystemByKey($URI) -- internal

The triplets in the RDF are transformed to a Perl object.

=cut

sub _BaseSystemByKey {
	my ($self, $key) = @_;

	my $bsh = $self->{BaseSystem}{$key};
	return undef unless $bsh;

	my $bs = new Janitor::Catalog::BaseSystem();
	$bs->fill(%$bsh, id => $key);
	$bs->immutable(1);
	return $bs;
}

######################################################################
######################################################################

=item MetapackagesByPackageKey($URI)

Returns a list of MetaPackages as determined by the URI of the basepackage
it refers to. No check on the class of the URI is performed in this step.

=cut

sub MetapackagesByPackageKey {
	my ($self, $pid) = @_;

	my @ret = ();
	while (my ($mpid, $mp) = each %{$self->{MetaPackage}}) {
		for my $pkgid (@{$mp->{instance}}) {
			next unless $pkgid eq $pid;
			push @ret, $self->_MetaPackageByKey($mpid);
		}
	}

	return @ret;
}


######################################################################
######################################################################

=item _MetaPackageByKey($URI) - internal

Returns a single MetaPackage as determined by the URI that has
been determined in prior queries. No check on the class of the URI
is performed in this step.

=cut

sub _MetaPackageByKey {
	my ($self, $key) = @_;

	my $mph = $self->{MetaPackage}{$key};
	return undef unless $mph;

	my $mp = new Janitor::Catalog::MetaPackage();
	$mp->id($key);
	$mp->name($mph->{name});
	$mp->description($mph->{description});
	$mp->homepage($mph->{homepage});
	$mp->lastupdate($mph->{lastupdated});
	$mp->tag($self->{Tag}{$_}{name}) for @{$mph->{tag}};
	$mp->immutable(1);
	return $mp;
}


######################################################################
# returns a list of all MetaPackages by object.
######################################################################

=item MetaPackages()

Returns a list of all metapackages as Catalog::MetaPackage.

=cut

sub MetaPackages {
	my ($self, $all) = @_;
	
	my @ret = ();
	keys %{$self->{MetaPackage}}; # reset iterator
	while (my ($mpid, $mp) = each %{$self->{MetaPackage}}) {
		next unless $all or $self->{_mask}->{$mpid};
		push @ret, $self->_MetaPackageByKey($mpid);
	}

	return @ret;
}

######################################################################
######################################################################

=item _PackageByKey(@{URI list}) - internal

Returns a package by the URI in the RDF document as determined
by prior queries. The package may either be a TarPackage, a
Debian package or of the Package class itself if the type is
unknown to this version of the script.

=cut

sub _PackageByKey {
	my ($self,$key) = @_;

	my ($p, $h);

	if ($h = $self->{TarPackage}{$key}) {
		$p = new Janitor::Catalog::TarPackage();
		$p->id($key);
		$p->url($h->{url});
		$p->basesystem($h->{basesystem});
	} elsif ($h = $self->{DebianPackage}{$key}) {
		$p = new Janitor::Catalog::DebianPackage();
		$p->id($key);
		$p->basesystem($h->{basesystem});
	} elsif ($h = $self->{Package}{$key}) {
		$p = new Janitor::Catalog::Package();
		$p->id($key);
	}

	return $p
}


######################################################################
# Allows and Denies the listed MetaPackages (by name or tag) to be used.
######################################################################

=item AllowMetaPackage(@list), DenyMetaPackage(@list)

Allows or denies all listed metapackages (by name) to be used. Tags and
Wildcards are supported.

=cut

# just add the 1 or 0
sub AllowMetaPackage { $_[0]->_AllowDenyMetaPackage(1, @_); }
sub DenyMetaPackage{ $_[0]->_AllowDenyMetaPackage(0, @_); } 

sub _AllowDenyMetaPackage {
	# undef because $self is added to the list of arguments twice
	my ($self, $which, undef, @list) = @_;

	for my $regex (@list) {
		while (my ($pkgid, $pkg) = each %{$self->{MetaPackage}}) {
			$self->{_mask}{$pkgid} = $which if $pkg->{name} =~ m|$regex|;
		}
	}
}


######################################################################
######################################################################

=item AllowBaseSystem(@list), DenyBaseSystem(@list)

Allows or denies all listed basesystems (by name) to be used. Wildcards are supported.

=cut

# just add the 1 or 0
sub AllowBaseSystem { $_[0]->_AllowDenyBaseSystem(1, @_); }
sub DenyBaseSystem { $_[0]->_AllowDenyBaseSystem(0, @_); }

sub _AllowDenyBaseSystem {
	# undef because $self is added to the list of arguments twice
	my ($self, $which, undef, @list) = @_;

	for my $regex (@list) {
		if ($regex =~ m/^[^:]*::.*$/) {
			while (my ($bsid, $bs) = each %{$self->{BaseSystem}}) {
				$self->{_mask}{$bsid} = $which if $bs->{name} =~ m|$regex|;
			}
		} else {
			while (my ($bsid, $bs) = each %{$self->{BaseSystem}}) {
				$self->{_mask}{$bsid} = $which if $bs->{short_description} =~ m/$regex/;
			}
		}
	}
}


sub test {
	require Data::Dumper;
	my $cat = Janitor::Catalog->new();
	$cat->add('/tmp/a.rdf');
	$cat->check_references();
	print(Data::Dumper::Dumper $cat);
}

#test();

1;
