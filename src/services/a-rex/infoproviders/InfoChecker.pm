package InfoChecker;

use base Exporter;

use strict;

# Class to check that a data structure conforms to a schema. Data and schema
# are both nested perl structures consisting of hashes and arrays nested to any
# depth. This function will check that data and schema have the same nesting
# structure. For hashes, all required keys in the schema must also be defined
# in the data. A "*" value in the schema marks that key optional. A "*" key in
# the schema matches all unmatched keys in the data (if any). Arrays in the
# schema should have exactly one element, and this element will be matched
# against all elements in the corresponding array in the data.

# Constructor
#
# Arguments:
#   $schema - reference to the schema structure

sub new($$) {
    my ($this,$schema) = @_;
    my $class = ref($this) || $this;
    die "Schema not a reference" unless ref($schema);
    my $self = {schema => $schema};
    bless $self, $class;
    return $self;
}

#
# Arguments:
#   $data     - reference to a data structure that should be checked
#   $strict   - (optional) if true, extra hash keys in data will be reported.
#               Otherwise only missing keys are reported.
#
# Returns:
#   @errors - list of error messages, one for each mismatch found during
#             checking

sub verify($$;$) {
    my ($self,$data,$strict) = @_;
    $self->{strict} = $strict;
    $self->{errors} = [];
    $self->_verify_part("",$data,$self->{schema});
    return @{$self->{errors}};
}

sub _verify_part($$$$); # prototype it, because it's a recursive function

sub _verify_part($$$$) {
    my ($self,$subject,$data,$schema) = @_;

    unless (defined $data) {
        push @{$self->{errors}}, "$subject is undefined";
        return 1; # tell caller this entry can be deleted
    }
    unless ( ref($data) eq ref($schema) ) {
        my $type = ref($schema) ? ref($schema) : "SCALAR";
        push @{$self->{errors}}, "$subject has wrong type, $type expected";
        return 1; # tell caller this entry can be deleted
    }

    # process a hash reference
    if ( ref($schema) eq "HASH" ) {

        # deal with hash keys other than '*'
        my @templkeys = grep { $_ ne "*" } keys %$schema;
        for my $key ( sort @templkeys ) {
            my $subj = $subject."{$key}";
            if ( exists $data->{$key} ) {
                # check that existing key value is valid
                my $can_delete = $self->_verify_part($subj, $data->{$key},
                                                     $schema->{$key});
                # delete it if it's not valid
                if ($can_delete and $self->{strict}) {
                    push @{$self->{errors}}, "$subj deleting it";
                    delete $data->{$key};
                }
            } elsif ($schema->{$key} eq "*") {
                # do nothing:
                # this missing key is optional
            } elsif (ref($schema->{$key}) eq "ARRAY"
                and $schema->{$key}[0] eq "*") {
                # do nothing:
                # this missing key is optional, it points to optional array
            } elsif (ref($schema->{$key}) eq "HASH"
                and keys(%{$schema->{$key}}) == 1
                and exists $schema->{$key}{'*'} ) {
                # do nothing:
                # this missing key is optional, it points to optional hash
            } else {
                push @{$self->{errors}}, "$subj is missing";
            }
        }

        # deal with '*' hash key in schema
        if ( grep { $_ eq "*" } keys %$schema ) {
            for my $datakey ( sort keys %$data ) {

                # skip keys that have been checked already
                next if grep { $datakey eq $_ } @templkeys;
                my $subj = "${subject}{$datakey}";

                # check that the key's value is valid
                my $can_delete = $self->_verify_part($subj, $data->{$datakey},
                                                     $schema->{"*"});
                # delete it if it's not valid
                if ($can_delete and $self->{strict}) {
                    push @{$self->{errors}}, "$subj deleting it";
                    delete $data->{$datakey};
                }
            }

        # no '*' key in schema, reverse checking may be performed
        } elsif ($self->{strict}) {
            for my $datakey ( sort keys %$data) {
                my $subj = "${subject}{$datakey}";
                unless (exists $schema->{$datakey}) {
                    push @{$self->{errors}}, "$subj is not part of schema";
                    push @{$self->{errors}}, "$subj deleting it";
                    delete $data->{$datakey};
                }
            }
        }

    # process an array reference
    } elsif ( ref($schema) eq "ARRAY" ) {
        for ( my $i=0; $i < @$data; $i++ ) {
            my $subj = "${subject}[$i]";

            # check that data element is valid
            my $can_delete = $self->_verify_part($subj, $data->[$i],
                                                 $schema->[0]);
            # delete it if it's not valid
            if ($can_delete and $self->{strict}) {
                push @{$self->{errors}}, "$subj deleting it";
                splice @$data, $i, 1;
                --$i;
            }
        }

    # process a scalar: nothing to do here
    } elsif ( not ref($data)) {

    # nothing else than scalars and HASH and ARRAY references are allowed in
    # the schema
    } else {
        my $type = ref($schema);
        push @{$self->{errors}},
                     "$subject bad value in schema, ref($type) not allowed";
    }

    return 0;
}


#### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ####

sub test() {
    my $schema = {
        totalcpus => '',
        freecpus => '',
        jobs => {
            '*' => { owner => '' }
        },
        users => [
            { dn => '' }
        ]
    };
    
    my $data = {
        freecpus => undef,
        jobs => {
            id1 => { owner => 'val' },
            id2 => 'something else'
        },
        users => [{dn => 'joe', extra => 'dummy'}, 'bad user', { }]
    };
    
    require Data::Dumper; import Data::Dumper;
    print "Before: ",Dumper($data);
    print "Checker: options->$_\n" foreach InfoChecker->new($schema)->verify($data,1);
    print "After: ",Dumper($data);
}

#test;

1;
