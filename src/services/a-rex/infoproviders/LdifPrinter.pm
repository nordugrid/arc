package LdifPrinter;

use MIME::Base64;

sub new {
    my ($this, $handle) = @_; 
    my $class = ref($this) || $this;
    # This would only affect comment lines, the rest is guaranteed to be ASCII
    binmode $handle, ':encoding(utf8)';
    print $handle "# extended LDIF\n#\n# LDAPv3\n";
    my $self = {fh => $handle, dn => undef, nick => undef, attrs => undef};
    return bless $self, $class;
}

sub begin {
    my ($self, $dnkey, $name) = @_;
    $self->_flush() if defined $self->{dn};
    unshift @{$self->{dn}}, safe_dn("$dnkey=$name");
    unshift @{$self->{nick}}, safe_comment("$name");
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
    return unless defined $attrs;
    my $dn = join ",", @{$self->{dn}};
    my $nick = join ", ", @{$self->{nick}};
    print $fh "\n";
    print $fh fold78("# $nick");
    print $fh fold78(safe_attrval("dn", $dn));
    for my $pair (@$attrs) {
        my ($attr, $val) = @$pair;
        next unless defined $val;
        if (not ref $val) {
            print $fh fold78(safe_attrval($attr, $val));
        } elsif (ref $val eq 'ARRAY') {
            print $fh fold78(safe_attrval($attr, $_)) for @$val;
        } else {
            die "Not an ARRAY reference in: $attr";
        }
    }
    $self->{attrs} = undef;
}

#
# Make a string safe to use as a Relative Distinguished Name, cf. RFC 2253
#
sub safe_dn {
    my ($rdn) = @_;
    # Escape with \ the following characters ,;+"\<> Also escape # at the
    # beginning and space at the beginning and at the end of the string.
    $rdn =~ s/((?:^[#\s])|[,+"\\<>;]|(?:\s$))/\\$1/g;
    # Encode CR, LF and NUL characters (necessary except when the string
    # is further base64 encoded)
    $rdn =~ s/\x0D/\\0D/g;
    $rdn =~ s/\x0A/\\0A/g;
    $rdn =~ s/\x00/\\00/g;
    return $rdn;
}

#
# Construct an attribute-value string safe to use in LDIF, fc. RFC 2849
#
sub safe_attrval {
    my ($attr, $val) = @_;
    return "${attr}:: ".encode_base64($val,'') if $val =~ /^[\s,:<]/
                                               or $val =~ /[\x0D\x0A\x00]/
                                               or $val =~ /[^\x00-\x7F]/;
    return "${attr}: $val";
}

#
# Leave comments as they are, just encode CR, LF and NUL characters
#
sub safe_comment {
    my ($line) = @_;
    $line =~ s/\x0D/\\0D/g;
    $line =~ s/\x0A/\\0A/g;
    $line =~ s/\x00/\\00/g;
    return $line;
}

#
# Fold long lines and add a final newline. Handles comments specially.
#
sub fold78 {
    my ($tail) = @_;
    my $is_comment = "#" eq substr($tail, 0, 1);
    my $contchar = $is_comment ? "# " : " ";
    my $output = "";
    while (length $tail > 78) {
        $output .= substr($tail, 0, 78) . "\n";
        $tail = $contchar . substr($tail, 78);
    }
    return "$output$tail\n";
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

