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
    my $self = { data => {}, sections => [] };
    bless $self, $class;
    return $self->_parse($arcconf);
}

# Expects the filename of the arc.conf file. 
# Returns false if it cannot open the file.

sub _parse($$) {
    my ($self,$arcconf) = @_;
    my %data;
    my @sections;

    return undef unless open(CONFIGFILE, $arcconf);

    my $sectionname;
    while (my $line =<CONFIGFILE>) {
       next if $line =~/^\s*#/;
       next if $line =~/^\s*$/;
    
       if ($line =~/^\s*\[(.+)\]/ ) {
          $sectionname = $1;
          push @sections, $sectionname;
          next;
       }
       # Single or double quotes can be used. Quotes are removed from the values
       next unless $line =~ /^(\w+)\s*=\s*(["'])(.*)(\2)\s*$/;
       my $opt=$1;
       my $value=$3;
       if (not defined $data{$sectionname}{$opt}) {
          $data{$sectionname}{$opt} = $value;
       }
       else {
          $data{$sectionname}{$opt} .= "[separator]".$value;
       }
    }
    close CONFIGFILE;

    $self->{data} = \%data;
    $self->{sections} = \@sections;
    return $self;
}

# Returns a hash with all options defined in a section. If the section does not
# exist, it returns an empty hash

sub get_section($$) {
    my ($self,$sectionname) = @_;
    return $self->{data}{$sectionname} ? %{$self->{data}{$sectionname}} : ();
} 

# list all subsections of a section, but not the section section itself

sub list_subsections($$) {
    my ($self,$sectionname) = @_;
    my @subsect = ();
    for (@{$self->{sections}}) {
        /$sectionname\/(.*)/ and push @subsect, $1;
    }
    return @subsect;
}


sub test {
    require Data::Dumper import Data::Dumper qw(Dumper);
    my $parser = ConfigParser->new('/etc/arc.conf') or die;
    print Dumper($parser);
    print "@{[$parser->list_subsections('gridftpd')]}\n";
}

#test();

1;
