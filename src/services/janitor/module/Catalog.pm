package Janitor::Catalog;

BEGIN {
	if (defined(Janitor::Janitor::resurrect)) { 
		eval 'require Log::Log4perl';
		unless ($@) { 
			import Log::Log4perl qw(:resurrect get_logger);
		}
	}

	eval 'require Time::HiRes';
	unless ($@) {
		import Time::HiRes qw(time);
	}
}
###l4p my $logger = get_logger("Janitor::Catalog");

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




use strict;
use warnings;



use RDF::Redland;

use Janitor::Catalog::BaseSystem;
use Janitor::Catalog::MetaPackage;
use Janitor::Catalog::Package;
use Janitor::Catalog::TarPackage;
use Janitor::Catalog::DebianPackage;

use Janitor::Catalog::NoteCollection;
use Janitor::Catalog::MetaPackageCollection;
use Janitor::Catalog::Tag;

######################################################################
# Constructor, opens the catalog and returns a object for accessing it.
######################################################################

=item new($class, $file, $name)

The constructor for this class has two arguments, the name of the catalog file
and the name of the catalog. It uses RDF::Redland to open the calaog and returns an object for accessing it.

=cut

sub new {
	my ($class, undef, $name) = @_;

	my $verbose = 0;

	# create storage
	my $storage = new RDF::Redland::Storage("hashes", "",
				"new='yes',hash-type='memory'");
	unless (defined  $storage) {
		printf STDERR "Can not create RDF::Redland::Storage\n";
		return undef;
	}

	# create model
	my $model = new RDF::Redland::Model($storage, "");
	unless (defined $model) {
		printf STDERR "Can not create RDF::Redland::Model for storage\n";
		return undef;
	}

	my $self = {
		_model => $model,
		_query_counter => 0,
		_query_time => 0,
		_verbose => $verbose,
		_name => $name,
		_mask => {},
	};
	
	
	return bless $self, $class;
}

###########################################################################
# Destructor
###########################################################################

=item DESTROY - The destructor

It just prints some statistics.

=cut 

sub DESTROY {
	my ($self) = @_;
	if ($self->{_verbose} > 1) {
		printf STDERR "querycount: %d\n", $self->{_query_counter};
		printf STDERR "querytime: %.2fs\n", $self->{_query_time};
		printf STDERR "time per query: %.2fms\n";
					 (1000*$self->{_query_time})/$self->{_query_counter};
	}
}

=item $obj->add ($file)

This method adds the content of the $file to the catalog.

=cut

# XXX what to do with the name?

sub add {
	my ($self, $file) = @_;

	unless ( -e $file) {
###l4p 		$logger->error("Can not add file to catalog: "
###l4p					. "$file does not exist");
		return;
	}

	my $uri = new RDF::Redland::URI("file:$file");	
	my $parser = new RDF::Redland::Parser("rdfxml", "application/rdf+xml");
	unless (defined $parser) {
		printf STDERR "Can not create RDF::Redland::Parser\n";
		return undef;
	}

	my $model = $self->{_model};

	# add everything to the model
	for ( my $stream = $parser->parse_as_stream($uri,$uri);
			!$stream->end; $stream->next) {
		$model->add_statement($stream->current);
	}
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
	my $results;


	#####
	# At first check if all requested packages are allowed
	#####
	foreach my $package (@pakete) {
		$query = sprintf "
			PREFIX kb: <http://knowarc.eu/kb#>
			SELECT ?id
			WHERE {
				?id a kb:MetaPackage .
				?id kb:name ?name .
				FILTER ( ?name = \"%s\" ) .
			}", $package;
		my $id = prepare_uri($self->query_with_one_answer($query));
		push @ids, $id;
		return () unless (defined $id and $self->{_mask}->{$id});
	}

	#####
	# query for each package, which basesystems support it
	#####
	foreach my $p_id (@ids) {
		$query = sprintf "
			PREFIX kb: <http://knowarc.eu/kb#>
			SELECT ?baseid
			WHERE {
				%s kb:instance ?b .
				?b kb:basesystem ?baseid .
			}", $p_id;

		for (my $res = $self->query($query);!$res->finished;$res->next_result) {
			for (my $i = 0; $i < $res->bindings_count; $i++) {
				my $id = &prepare_uri($res->binding_value($i)->as_string);
				next unless $self->{_mask}->{$id};
				if (! defined $basesystems{$id}) {
					$basesystems{$id} = 1;
				} else {
					$basesystems{$id}++;
				}
			}
		}
	}

	#####
	# we still have to check if the dependencies are satisfied
	#####
	foreach my $b_key ( keys %basesystems ) {
		next unless $basesystems{$b_key} == scalar @pakete;
	 
		foreach my $p_key (@ids) {
			my ($r, @x) = $self->_check_dependencies($b_key, $p_key);
			unless ($r) {
				$basesystems{$b_key} = 0;
				last;
			}
		}
	}
 
	#####
	# and now check wich basesystems supports all packages
	#####
	my @ret = ();
	foreach my $key ( keys %basesystems ) {
		if ($basesystems{$key} == scalar @pakete) {
			push @ret, $self->_BaseSystemByKey($key);
		}
	}

	return @ret;
}

###########################################################################
###########################################################################

=item _check_dependencies($baseid, $packageid) - internal
 
Checks if the Dependencies of MetaPackage $package (by id) are satisfied on
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
	my $query = sprintf "
		PREFIX kb: <http://knowarc.eu/kb#>
		SELECT ?instance
		WHERE {
			%s kb:instance ?instance .
			?instance kb:basesystem %s .
		}", $packageid, $baseid;

	# there should be exactly one answer
	for (my $res = $self->query($query);!$res->finished;$res->next_result) {
		for (my $i = 0; $i < $res->bindings_count; $i++) {
			push @todo, &prepare_uri($res->binding_value($i)->as_string);
		}
	}

	# XXX here we need some more logic: For example DebinPackage is only
	# installable on Debian, etc.

	# and now check the dependencies
	while (my $instance = pop @todo) {

		# check if it is a new dependency
		next if defined $seen{$instance};
		$seen{$instance} = 1;

		push @ret, $instance;

		# check if there is such an instance
		my $b = $self->get_object($instance, "kb:basesystem");
		if (! defined $b) {
			printf STDERR "Can not find package with id \"%s\" "
				. "while checking dependencies of \"%s\"\n",
				$instance, $packageid;
			return (0,undef);
		}

		# check if the basesystem is right
		$b = prepare_uri($b);
		if ($baseid ne $b) {
			printf STDERR "found unexpected basesystem entry \"%s\" "
				. "while checking meta package \"%s\". Expected \"%s\"\n",
				$b, $packageid, $baseid;
			return (0,undef);
		}
 
		# and now get the dependencies
		$query = sprintf "
			PREFIX kb: <http://knowarc.eu/kb#>
			SELECT ?instance
			WHERE { %s kb:depends ?instance }", $instance;
	
		my @temp = (); 
		for (my $res = $self->query($query); !$res->finished; $res->next_result) {
			for (my $i = 0; $i < $res->bindings_count; $i++) {
				my $val = $res->binding_value($i);
				next unless defined $val; # optionals
				push @temp, &prepare_uri($val->as_string);
			}
		}

		foreach my $i ( @temp )  {
			my $type = &prepare_uri($self->get_object($i, "a"));
			if ($type eq "<http://knowarc.eu/kb#MetaPackage>") {
				$query = sprintf "
					PREFIX kb: <http://knowarc.eu/kb#>
					SELECT ?instance
					WHERE {
						%s kb:instance ?instance .
						?instance kb:basesystem %s .
					}", $i, $baseid;
				$instance = $self->query_with_one_answer($query);

				# can't deploy this MetaPackage 
				return (0,undef)	unless defined $instance;

				push @todo, &prepare_uri($instance);
			} else {
				push @todo, $i;
			}

		}
	}

	return (1, reverse @ret);
}

######################################################################
# Returns a tupel. The first element is true if the requestes metapackages (by
# name) can be provides on the requestes basesystem (by object). The second
# element is a 
# list of packages to be installed to provide all requested
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

	foreach my $mp ( @mp ) {
		my $query = sprintf "
			PREFIX kb: <http://knowarc.eu/kb#>
			SELECT ?id
			WHERE {
				?id a kb:MetaPackage .
				?id kb:name ?name .
				FILTER ( ?name = \"%s\" ) .
			}", $mp;
		my $id = prepare_uri($self->query_with_one_answer($query));

		my ($retval, @r) = $self->_check_dependencies($bs->id, $id);
		
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

	my $bs = Janitor::Catalog::NoteCollection->get($key);

	return $bs if defined $bs;

	# there is no such thing in the collection, so search for
	# it in the RDF
	my $query = sprintf "
                PREFIX kb: <http://knowarc.eu/kb#>
                SELECT *
                WHERE { %s ?a ?b }", $key;

	my %values;
	$values{"id"} = $key;
	for  (my $res = $self->query($query); !$res->finished; $res->next_result) {
		next unless defined $res->binding_value(0);
		next unless defined $res->binding_value(1);

		my $pred = prepare_uri($res->binding_value(0)->as_string);

		if ($pred =~ m%^<http://knowarc.eu/kb#(.*)>$%) {
			$values{$1} = $res->binding_value(1)->as_string;
		}
	}

	$bs = new Janitor::Catalog::BaseSystem();
	$bs->fill(%values);
	$bs->immutable(1);

	unless (defined $bs->name) {
		printf STDERR "There is no valid basesystem with id \"%s\" " .
			"in the catalog.\n", $key;
		return undef;
	}

	Janitor::Catalog::NoteCollection->add($key, $bs);

	return $bs
}






######################################################################
######################################################################

=item MetapackagesByBasepackageKey($URI)

Returns a list of MetaPackages as determined by the URI of the basepackage
it refers to. No check on the class of the URI is performed in this step.

=cut

sub MetapackagesByPackageKey {
	my ($self, $pid) = @_;


	my $query = sprintf "
                PREFIX kb: <http://knowarc.eu/kb#>
                SELECT ?metaid
		WHERE { 
			?metaid    a           kb:MetaPackage .
			?metaid    kb:instance %s .
		}", $pid;
# 			?metaid    kb:name     ?name .



	my $mp = undef;

	my @ret = ();
	for (my $res = $self->query($query); !$res->finished; $res->next_result) {
		if($res->bindings_count > 0){
			my $metaid = $res->binding_value(0);
			$metaid = prepare_uri($metaid->as_string);
			push(@ret, $self->_MetaPackageByKey($metaid));

		}
	}

	return @ret;

}





######################################################################
######################################################################

=item BaseSystems()

Returns a list of all basesystems as objects of the type Catalog::BaseSystem.

=cut

sub BaseSystems {
	my ($self, $all) = @_;

	$all = 0 unless defined $all;

	my $query =  "
		PREFIX kb: <http://knowarc.eu/kb#>
		SELECT ?baseid
		WHERE { ?baseid a kb:BaseSystem . }";

	my @ret = ();
	for (my $res = $self->query($query); !$res->finished; $res->next_result) {
		for (my $i = 0; $i < $res->bindings_count; $i++) {
			my $val = $res->binding_value($i);
			next unless defined $val; # optionals
			my $id = prepare_uri($val->as_string);
			next unless $all or $self->{_mask}->{$id};
			push @ret, $self->_BaseSystemByKey(prepare_uri($val->as_string));
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

	my $mp = Janitor::Catalog::MetaPackageCollection->get($key);

	return $mp if defined $mp;

	# no such MetaPackage yet, have a look at the RDF
	
	$mp = new Janitor::Catalog::MetaPackage();
	$mp->id($key);

	my $query = sprintf "
                PREFIX kb: <http://knowarc.eu/kb#>
                SELECT *
                WHERE { %s ?a ?b }", $key;

	for  (my $res = $self->query($query); !$res->finished; $res->next_result) {
		next unless defined $res->binding_value(0);
		next unless defined $res->binding_value(1);

		my $pred = prepare_uri($res->binding_value(0)->as_string);
		my $val = $res->binding_value(1)->as_string;

		if ($pred eq "<http://knowarc.eu/kb#name>") {
			$mp->name($val);
		} elsif ($pred eq "<http://knowarc.eu/kb#description>") {
			$mp->description($val);
		} elsif ($pred eq "<http://knowarc.eu/kb#homepage>") {
			$mp->homepage($val);
		} elsif ($pred eq "<http://knowarc.eu/kb#lastupdated>") {
			$mp->lastupdate($val);
		} elsif ($pred eq "<http://knowarc.eu/kb#tag>") {
			$mp->tag($self->_TagByKey(prepare_uri($val)));
		}
	}

	$mp->immutable(1);

	unless (defined $mp->name) {
		printf STDERR "There is no valid MetaPackage with id \"%s\" " .
			"in the catalog.\n", $key;
		return undef;
	}

	Janitor::Catalog::MetaPackageCollection->add($key, $mp);


	return $mp;
}

######################################################################
######################################################################

sub _TagByKey {
	my ($self, $key) = @_;

	my $tag = Janitor::Catalog::NoteCollection->get($key);

	return $tag if defined $tag;

	# there is no such thing in the collection, so search for
	# it in the RDF
	my $query = sprintf "
                PREFIX kb: <http://knowarc.eu/kb#>
                SELECT *
                WHERE { %s ?a ?b }", $key;

	my %values;
	$values{"id"} = $key;
	for  (my $res = $self->query($query); !$res->finished; $res->next_result) {
		next unless defined $res->binding_value(0);
		next unless defined $res->binding_value(1);

		my $pred = prepare_uri($res->binding_value(0)->as_string);

		if ($pred =~ m%^<http://knowarc.eu/kb#(.*)>$%) {
			$values{$1} = $res->binding_value(1)->as_string;
		}
	}

	$tag = new Janitor::Catalog::Tag();
	$tag->fill(%values);
	$tag->immutable(1);

	unless (defined $tag->name) {
		printf STDERR "There is no valid Tag with id \"%s\" " .
			"in the catalog.\n", $key;
		return undef; }

	Janitor::Catalog::NoteCollection->add($key, $tag);

	return $tag;
}

######################################################################
# returns a list of all MetaPackages by object.
######################################################################

=item MetaPackages()

Returns a list of all metapackages as Catalog::MetaPackage.

=cut

sub MetaPackages {
	my ($self, $all) = @_;

	$all = 0 unless defined $all;

	my $query = "
		PREFIX kb: <http://knowarc.eu/kb#>
		SELECT ?metaid
		WHERE { ?metaid a kb:MetaPackage. }";
	
	my @ret = ();
	for  (my $res = $self->query($query); !$res->finished; $res->next_result) {
                for (my $i = 0; $i < $res->bindings_count; $i++) {
                        my $val = $res->binding_value($i);
                        next unless defined $val; # optionals
			my $id = prepare_uri($val->as_string);
			next unless $all or $self->{_mask}->{$id};
                        push @ret, $self->_MetaPackageByKey($id);
                }
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

	my $p;

	my $TAR = 0;
	my $DEB = 1;
	my $OTHER = 2;

	my $typ = prepare_uri($self->get_one_of_three($key, "a", "?type"));
	$typ =~ s%^<http://knowarc.eu/kb#(.*)>$%kb:$1%;
	if ($typ eq "kb:TarPackage") {
		$typ = $TAR;
		$p = new Janitor::Catalog::TarPackage();
	} elsif ($typ eq "kb:DebianPackage") {
		$typ = $DEB;
		$p = new Janitor::Catalog::DebianPackage();
	} else {
		$typ = $OTHER;
		$p = new Janitor::Catalog::Package();
	}

	$p->id($key);

	my $query = sprintf "
                PREFIX kb: <http://knowarc.eu/kb#>
                SELECT *
                WHERE { %s ?a ?b }", $key;

	for  (my $res = $self->query($query); !$res->finished; $res->next_result) {
		next unless defined $res->binding_value(0);
		next unless defined $res->binding_value(1);

		my $pred = prepare_uri($res->binding_value(0)->as_string);
		my $val = $res->binding_value(1)->as_string;

		if ($pred eq "<http://knowarc.eu/kb#url>") {
			$p->url($val) if ($typ == $TAR);
		} elsif ($pred eq "<http://knowarc.eu/kb#basesystem>") {
			$p->basesystem(prepare_uri($val));
		}
	}

	# XXX $p->depends is missing
	
	return $p;
}

######################################################################
# Returns a list of all packages as object.
######################################################################

=item Packages()

Returns a list of all packages as Catalog::Package or a subclass of it.

=cut

sub Packages {
	my ($self) = @_;

	my @query;
	
	foreach my $kind ( ("kb:TarPackage", "kb:DebianPackage") ) {
		push @query, sprintf "
			PREFIX kb: <http://knowarc.eu/kb#>
			SELECT ?id
			WHERE { ?id a %s. }", $kind;
	}
	
	my @ret = ();
	foreach my $query( @query ) {
		for  (my $res = $self->query($query); !$res->finished; $res->next_result) {
			for (my $i = 0; $i < $res->bindings_count; $i++) {
				my $val = $res->binding_value($i);
				next unless defined $val; # optionals
				push @ret, $self->_PackageByKey(prepare_uri($val->as_string));
			}
		}
	}

	return @ret;
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

	foreach my $item (@list){
		if ($item =~ m/^([^:]*)::(.*)$/) {
			$self->_Allow_DenyHelper("
				PREFIX kb: <http://knowarc.eu/kb#>
				SELECT ?id
				WHERE {
					?id a kb:MetaPackage .
					?id kb:tag ?tag .
					?tag a kb:Tag .
					?tag kb:name ?name .
					FILTER regex(?name, \"%s\") .
				}", $which, $item);
		} else {
			$self->_Allow_DenyHelper("
				PREFIX kb: <http://knowarc.eu/kb#>
				SELECT ?id
				WHERE {
					?id a kb:MetaPackage .
					?id kb:name ?name .
					FILTER regex(?name, \"%s\") .
				 }", $which, $item);
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

	foreach my $item (@list){
		if ($item =~ m/^[^:]*::.*$/) {
			$self->_Allow_DenyHelper("
				PREFIX kb: <http://knowarc.eu/kb#>
				SELECT ?id
				WHERE {
					?id a kb:BaseSystem .
					?id kb:name ?name .
					FILTER regex(?name, \"%s\") .
				 }", $which, $item);
		} else {
			$self->_Allow_DenyHelper("
				PREFIX kb: <http://knowarc.eu/kb#>
				SELECT ?id
				WHERE {
					?id a kb:BaseSystem .
					?id kb:short_description ?name .
					FILTER regex(?name, \"%s\") .
				 }", $which, $item);
		}
	}
} 

######################################################################
# Allows or denies all metapackages and all basesystems. Altough not
# private these methods are not meant for general use but only for
# temporary use during hacking on other parts of the Janitor.
######################################################################

sub AllowAll {
	my $self = shift;
	$self->_Allow_DenyHelper("
		PREFIX kb: <http://knowarc.eu/kb#>
		SELECT ?id
		# %s
		WHERE { ?id a kb:BaseSystem }", 1, ("dummy"));
	$self->_Allow_DenyHelper("
		PREFIX kb: <http://knowarc.eu/kb#>
		SELECT ?id
		# %s
		WHERE { ?id a kb:MetaPackage }", 1, ("dummy"));
}

sub DenyAll {
	my $self = shift;
	$self->{_mask} = {};
}

######################################################################
# This method is performing the actual work for AllowBaseSystem and 
# AllowMetaPackage and DisallowMetaPackage
######################################################################

sub _Allow_DenyHelper {
	my ($self, $q, $flag, @mp) = @_;	

	foreach my $mp ( @mp ) {
		my $query = sprintf $q, $mp;

		for  (my $res = $self->query($query); !$res->finished; $res->next_result) {
			for (my $i = 0; $i < $res->bindings_count; $i++) {
				my $val = $res->binding_value($i);
				next unless defined $val; # optionals
###l4p				$logger->debug(($flag ? 'allow' : 'deny') . prepare_uri($val->as_string));
				$self->{_mask}->{prepare_uri($val->as_string)} = $flag;
			}
		}
	}
}



######################################################################
######################################################################
######################################################################
######################################################################

=head2 RDF-interfacing the catalog

From here on this module only contains code for easing the query
of the catalog.

=cut

######################################################################
######################################################################

=item query($query)

Executes a SPARQL query and returns the answer as itself it is
returned by the Redland RDF function.

=cut

sub query {
	my ($self, $q) = @_;
	my $ret;
	my $s;
	my $e;

	print STDERR "SPARQL Query: $q\n" if $self->{_verbose} > 2;

	$s = time;
        $ret = $self->{_model}->query_execute(
                new RDF::Redland::Query($q, undef, undef, "sparql"));
	$e = time;

	$self->{_query_time} += $e - $s;
	$self->{_query_counter}++;

	printf STDERR "needed time: %fms\n", ($e - $s) * 1000 if $self->{_verbose} > 3;

	return $ret;
}

###########################################################################
###########################################################################

=item query_with_one_answer($query)

This method is for the special case of queries which have only one or
none answer. A complete RDF query is passed as an argument

=cut

sub query_with_one_answer {
	my ($self, $query) =  @_;
	my $ret = undef;

	for (my $res = $self->query($query); !$res->finished; $res->next_result) {
		for (my $i = 0; $i < $res->bindings_count; $i++) {
			my $val = $res->binding_value($i);
			next unless defined $val; # optionals
			if (defined $ret) {
				printf STDERR "more than one answer in query_with_one_answer()\n";
				printf STDERR "query was: %s\n", $query;
				exit 1;
			}
			$ret = $val->as_string;
		}
	}
	return $ret;
}

######################################################################
######################################################################

=item get_one_of_three($subject, $predicate, $object)

Calls
I<query_with_one_answer>
to retrieve a matching triplet.

=cut

sub get_one_of_three {
        my ($self, $subject, $predicate, $object) = @_;
        my $query = sprintf "
                PREFIX kb: <http://knowarc.eu/kb#>
                SELECT *
                WHERE { %s %s %s }", $subject, $predicate, $object;

        return $self->query_with_one_answer($query);
}

######################################################################
######################################################################

=item get_object($subject,$predicate)

Performs a query to retrieve the triplet with that
I<subject>
and
I<predicate>
fixed. Only the filled parameter, the
I<object>, is returned.

=cut

sub get_object {
	my ($self, $subject, $predicate) = @_;
	return $self->get_one_of_three($subject, $predicate, "?a");
}

=item get_object($subject,$predicate)

As before, but now multiple objects are expected and returned as an array.

=cut

sub get_objects {
	my ($self, $subject, $predicate) = @_;
	my @ret;

	my $query = sprintf "
                PREFIX kb: <http://knowarc.eu/kb#>
                SELECT *
                WHERE { %s %s ?a }", $subject, $predicate;

	for  (my $res = $self->query($query); !$res->finished; $res->next_result) {
		for (my $i = 0; $i < $res->bindings_count; $i++) {
			my $val = $res->binding_value($i);
			next unless defined $val; # optionals
			push @ret, $val->as_string
		}
	}
	return @ret;
}

######################################################################
# A bit ugly, but in the answers <> are convertet to []. To use it,
# we have to convert it back...
######################################################################

sub prepare_uri {
	my ($a) = @_;
	return undef unless defined $a;
	$a =~ s/^\[([^]]*)\]$/<$1>/;
	return $a;
}



1;

=back

=head1 SEE ALSO

Janitor::Catalog::BaseSystem,
Janitor::Catalog::MetaPackage,
Janitor::Catalog::Package,
Janitor::Catalog::TarPackage,
Janitor::Catalog::DebianPackage

=cut

