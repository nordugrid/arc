package IniParser;

use strict;
use warnings;

# Configuration parser module for perl
#
# Synopsis:
#
#   use IniParser;
#
#   my $parser = IniParser->new("/etc/arc.conf")
#       or die 'Cannot parse config file';
#
#   my %common = $parser->get_section('common');    # get hash with all options in a section
#   my %queue = $parser->get_section('queue/atlas');
#
#   print $parser->list_subsections('gridftpd');      # list all subsections of 'gridftpd', but not
#                                                     # the 'gridftpd' section itself
# 
# The [grid-manager] section is treated specially.  Options that are
# user-specific are put in separate pseudo-sections [grid-manager/<username>].
# User-specific options are: sessiondir cachedir remotecachedir helper
# cachesize maxrerun maxtransferfiles defaultttl mail. <username> reffers to
# the user that is initiated by a 'control' command. Each pseudo-section has
# it's own 'controldir' option. No substituions are made and use names '.' and
# '*' are not handled specially.

sub new($$) {
    my ($this,$arcconf) = @_;
    my $class = ref($this) || $this;
    open(my $fh,  "< $arcconf") || return undef;
    my $self = { config => {} };
    bless $self, $class;
    $self->{config} = _parse($fh);
    close($fh);
    return $self;
}


# Expects the filename of the arc.conf file. 
# Returns false if it cannot open the file.

sub _parse($) {
    my ($fh) = @_;

    my $config = {};

    # current section
    my $section = Section->new('common');

    while (my $line =<$fh>) {

        # skip comments and empty lines
        next if $line =~/^\s*;/;
        next if $line =~/^\s*#/;
        next if $line =~/^\s*$/;
    
        # new section starts here
        if ($line =~ /^\s*\[([\w\-\.\/]+)\]\s*$/) {
            my $sname = $1;

            $section->register($config);

            if      ($sname =~ m/^vo/)         { $section = SelfNamingSection->new($sname,'id');
            } elsif ($sname =~ m/^group/)      { $section = SelfNamingSection->new($sname,'name');
            } elsif ($sname =~ m/^queue/)      { $section = SelfNamingSection->new($sname,'name');
            } elsif ($sname eq 'grid-manager') { $section = GMSection->new($sname);
            } else                             { $section = Section->new($sname);
            }

        # single or double quotes can be used. Quotes are removed from the values
        } elsif ($line =~ /^(\w+)\s*=\s*(["']?)(.*)(\2)\s*$/) {
            my ($opt,$val) = ($1,$3);
            $section->add($opt,$val);

        # bad line, ignore it for now
        } else { }
    }

    $section->register($config);

    delete $config->{common} unless %{$config->{common}};

    return $config;
}

# Returns a hash with all options defined in a section. If the section does not
# exist, it returns an empty hash

sub get_section($$) {
    my ($self,$sname) = @_;
    return $self->{config}{$sname} ? %{$self->{config}{$sname}} : ();
} 

# Returns the list of all sections

sub list_sections($) {
    my ($self) = @_;
    return keys %{$self->{config}};
}

# list all subsections of a section, but not the section section itself

sub list_subsections($$) {
    my ($self,$sname) = @_;
    my %ssnames = ();
    for (keys %{$self->{config}}) {
        $ssnames{$1}='' if m|^$sname/([^/]+)|;
    }
    return keys %ssnames;
}


sub test {
    require Data::Dumper; import Data::Dumper qw(Dumper);
    my $parser = IniParser->new('/etc/arc.conf') or die;
    print Dumper($parser);
    print "@{[$parser->list_subsections('gridftpd')]}\n";
    print "@{[$parser->list_subsections('group')]}\n";
}

#test();

1;

########################################################
package Section;

sub new($$) {
    my ($this,$name) = @_;
    my $class = ref($this) || $this;
    my $self = { name => $name, data => {} };
    bless $self, $class;
    return $self;
}
sub add($$$) {
   my ($self,$opt,$val) = @_;
   my $data = $self->{data};
   my $old = $data->{$opt};
   $data->{$opt} = $old ? $old."[separator]".$val : $val;
}
sub register($$) {
   my ($self,$config) = @_;
   my $name = $self->{name};
   my $orig = $config->{$name} || {};
   my $new = $self->{data};
   $config->{$name} = { %$orig, %$new };
}
1;
########################################################
package SelfNamingSection;
use base "Section";

sub new($$$) {
    my ($this,$name,$nameopt) = @_;
    my $self = $this->SUPER::new($name);
    $self->{nameopt} = $nameopt;
    return $self;
}
sub add($$$) {
    my ($self,$opt,$val) = @_;
    if ($opt eq $self->{nameopt}) {
        $self->{name} =~ s|(/[^/]+)?$|/$val|;
    } else {
        $self->SUPER::add($opt,$val);
    }
}
1;
########################################################
package GMSection;
use base "Section";

sub new($) {
    my ($this) = @_;
    my $self = $this->SUPER::new('grid-manager');
    # OBS sessiondir is not treated
    $self->{muopts} = [qw(sessiondir cachedir remotecachedir helper)];
    $self->{suopts} = [qw(cachesize maxrerun maxtransferfiles defaultttl mail)];
    $self->{thisuser} = {};
    $self->{allusers} = {};
    $self->{controldir} = undef;
    return $self;
}
sub add($$$) {
    my ($self,$opt,$val) = @_;
    my $thisuser = $self->{thisuser};
    if ($opt eq 'controldir') {
        $self->{controldir} = $val;
    } elsif ($opt eq 'control') {
        my ($dir, @usernames) = split /\s+/, $val;
        $thisuser->{controldir} = $dir;
        $self->{allusers}{$_} = $thisuser for @usernames;
        $thisuser = $self->{thisuser} = {%$thisuser}; # make copy
        delete $thisuser->{$_} for grep {$_ ne 'sessiondir'} @{$self->{muopts}};
    } elsif (grep {$opt eq $_} @{$self->{muopts}}) {
        my $old = $thisuser->{$opt};
        $thisuser->{$opt} = $old ? $old."[separator]".$val : $val;
    } elsif (grep {$opt eq $_} @{$self->{suopts}}) {
        $thisuser->{$opt} = $val;
    } else {
        $self->SUPER::add($opt,$val);
    }
}
sub register($$) {
    my ($self,$config) = @_;
    my $dir = $self->{controldir};
    if ($dir) {
        my $thisuser = $self->{thisuser};
        $thisuser->{controldir} = $dir;
        $self->{allusers}{'.'} = $thisuser;
    }
    my $allusers = $self->{allusers};
    $config->{"grid-manager/$_"} =  $allusers->{$_} for keys %$allusers;
    $self->SUPER::register($config);
}
1;
