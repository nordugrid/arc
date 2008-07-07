package ConfigParser;

use strict;
use warnings;

# Configuration parser module for perl
#
# Synopsis:
#
#   use ConfigParser;
#
#   my $parser = ConfigParser->new("/etc/arc.conf")
#       or die 'Cannot parse config file';
#
#   my %config = ( gm_port => 2811,                  # set defaults
#                  gm_mount_point => '/jobs'
#                );
#
#   my %section = $parser->get_section('common');    # get hash with all options in a section
#   %config = (%config, %section);                   # merge section, overwriting existing keys
#
#   %section = $parser->get_section('queue/atlas');
#   %config = (%config, %section);
#
#   print $parser->list_subsections('gridftpd');      # list all subsections of 'gridftpd', but not
#                                                     # the 'gridftpd' section itself


sub new($$) {
    my ($this,$arcconf) = @_;
    my $class = ref($this) || $this;
    my $self = { config => {} };
    bless $self, $class;
    return $self->_parse($arcconf);
}

# Expects the filename of the arc.conf file. 
# Returns false if it cannot open the file.

sub _parse($$) {
    my ($self,$arcconf) = @_;
    my %config;

    return undef unless open(CONFIGFILE, $arcconf);

    # current section name and options
    my $sname;
    my $sopts;

    while (my $line =<CONFIGFILE>) {

        # skip comments and empy lines
        next if $line =~/^\s*#/;
        next if $line =~/^\s*$/;
    
        # new section stsrts here
        if ( $line =~/^\s*\[(.+)\]/ ) {
            my $newsname = $1;

            # store completed section
            $config{$sname} = $sopts if $sname;

            # start fresh section
            $sname = $newsname;
            $sopts = {};
            next;
        }

        # Single or double quotes can be used. Quotes are removed from the values
        next unless $line =~ /^(\w+)\s*=\s*(["'])(.*)(\2)\s*$/;

        my $opt=$1;
        my $value=$3;

        # 'name' option is special: it names a subsection
        if ($opt eq 'name') {
            $sname =~ s#([^/]+).*#$1/$value#;
        }

        if (not defined $sopts->{$opt}) {
           $sopts->{$opt} = $value;
        }
        else {
           $sopts->{$opt} .= "[separator]".$value;
        }
    }
    close CONFIGFILE;

    # store last section
    $config{$sname} = $sopts if $sname;

    $self->{config} = \%config;
    return $self;
}

# Returns a hash with all options defined in a section. If the section does not
# exist, it returns an empty hash

sub get_section($$) {
    my ($self,$sname) = @_;
    return $self->{config}{$sname} ? %{$self->{config}{$sname}} : ();
} 

# list all subsections of a section, but not the section section itself

sub list_subsections($$) {
    my ($self,$sname) = @_;
    my @ssnames = ();
    for (keys %{$self->{config}}) {
        push @ssnames, $1 if /$sname\/(.*)/;
    }
    return @ssnames;
}


sub test {
    require Data::Dumper; import Data::Dumper qw(Dumper);
    my $parser = ConfigParser->new('/etc/arc.conf') or die;
    print Dumper($parser);
    print "@{[$parser->list_subsections('gridftpd')]}\n";
    print "@{[$parser->list_subsections('group')]}\n";
}

#test();

1;
