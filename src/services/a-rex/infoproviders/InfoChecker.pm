package InfoChecker;

use base Exporter;

use strict;

# Class to check that a data structure conforms to a schema. Data and schema
# are both nested perl structures consisting of hashes and arrays nested to any
# depth. This function will check that data and schema have the same nesting
# structure. For hashes, all keys in the schema must also be defined in the
# data. A "*" key in the schema matches all unmatched keys in the data (if
# any). Arrays in the schema should have exactly one element, and this element
# will be matched against all elements in the corresponding array in the data.

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

    unless ( ref($data) eq ref($schema) ) {
        my $type = ref($schema) ? ref($schema) : "SCALAR";
        push @{$self->{errors}}, "$subject: wrong type, $type expected";
    }

    # process a hash reference
    elsif ( ref($schema) eq "HASH" ) {

        # deal with keys other than '*'
        my @templkeys = grep { $_ ne "*" } keys %$schema;
        for my $key ( sort @templkeys ) {
            my $subj = $subject."->{$key}";
            if ( defined $data->{$key} ) {
                $self->_verify_part($subj, $data->{$key}, $schema->{$key});
            } elsif ($schema->{$key} eq "*") {
                # this key is optional
            } elsif (ref($schema->{$key}) eq "ARRAY"
                and $schema->{$key}[0] eq "*") {
                # this key is optional, because it points to optional array
            #} elsif (ref($schema->{$key}) eq "HASH"
            #    and keys(%{$schema->{$key}}) == 1
            #    and defined $schema->{$key}{'*'} ) {
            #    # this key is optional, because it points to optional hash
            } else {
                push @{$self->{errors}}, "$subj: is missing";
            }
        }

        # deal with '*' key in schema
        if ( grep { $_ eq "*" } keys %$schema ) {
            for my $datakey ( sort keys %$data ) {
                # skip keys that have been checked already
                next if grep { $datakey eq $_ } @templkeys;
                my $subj = "${subject}->{$datakey}";
                $self->_verify_part($subj, $data->{$datakey}, $schema->{"*"});
            }
        }

        # no '*' key in schema, reverse checking may be performed
        elsif ($self->{strict}) {
            for my $datakey ( sort keys %$data) {
                my $subj = "${subject}->{$datakey}";
                push @{$self->{errors}}, "$subj: is not part of schema"
                    unless (exists $schema->{$datakey});
            }
        }
    }

    # process an array reference
    elsif ( ref($schema) eq "ARRAY" ) {
        for ( my $i=0; $i < @$data; $i++ ) {
            my $subj = "${subject}->[$i]";
            $self->_verify_part($subj, $data->[$i], $schema->[0]);
        }
    }

    # process a scalar: nothing to do here
    elsif ( not ref($data)) {
    }
    else {
        my $type = ref($schema);
        push @{$self->{errors}}, "$subject: bad schema, $type not allowed";
    }
}


sub test() {
    my $schema = {
        totacpus => '',
        freecpus => '',
        jobs => {
            '*' => { owner => '' }
        },
        users => [
            { dn => '' }
        ]
    };
    
    my $data = {
        freecpus => '2',
        jobs => {
            id1 => { owner => 'val' },
            id2 => 'something else'
        },
        users => [{dn => 'joe', extra => 'dummy'}, { }, 'bad user']
    };
    
    print "Checker: lrms_info$_\n" foreach InfoChecker->new($schema)->verify($data,1);
}

#test;

1;
