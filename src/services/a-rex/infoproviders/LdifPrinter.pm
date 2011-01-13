package LdifPrinter;

sub new {
    my ($this, $handle) = @_; 
    my $class = ref($this) || $this;
    binmode $handle, ':encoding(utf8)';
    print $handle "# extended LDIF\n#\n# LDAPv3\n";
    my $self = {fh => $handle, dn => undef, nick => undef, attrs => undef};
    return bless $self, $class;
}

sub begin {
    my ($self, $dnkey, $name) = @_;
    $self->_flush() if defined $self->{dn};
    unshift @{$self->{dn}}, "$dnkey=$name";
    unshift @{$self->{nick}}, "$name";
}

sub attribute {
    my ($self, $attr, $value) = @_;
    push @{$self->{attrs}}, [$attr, $value];
}

sub attributes {
    my ($self, $data, $prefix, @keys) = @_;
    my $attrs = $self->{attrs} ||= [];
    push @$attrs, ["$prefix$_", $data->{$_}] for @keys;
}

sub end {
    my ($self) = @_;
    $self->_flush();
    shift @{$self->{dn}};
    shift @{$self->{nick}};
}

sub _flush {
    my ($self) = @_;
    my $fh = $self->{fh};
    my $attrs = $self->{attrs};
    my $dn = join ",", @{$self->{dn}};
    my $nick = join ", ", @{$self->{nick}};
    return unless defined $attrs;
    print $fh "\n# $nick\ndn: $dn\n";
    for my $pair (@$attrs) {
        my ($attr, $val) = @$pair;
        next unless defined $val;
        if (not ref $val) {
            print $fh "$attr: $val\n";
        } elsif (ref $val eq 'ARRAY') {
            print $fh "$attr: $_\n" for @$val;
        } else {
            die "Not an ARRAY reference in: $attr";
        }
    }
    $self->{attrs} = undef;
}



#### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ####

sub test {
    my $data;
    my $printer = LdifPrinter->new(*STDOUT);
    $printer->begin(o => "glue");
    $data = { objectClass => "organization", o => "glue" };
    $printer->attributes("", $data, qw(objectClass o));
    $printer->begin(GLUE2GroupID => "grid");
    $printer->attribute(objectClass => "GLUE2GroupID");
    $data = { GLUE2GroupID => "grid" };
    $printer->attributes("GLUE2", $data, qw( GroupID ));
    $printer->end();
    $printer->end();
} 

#test;

1;

