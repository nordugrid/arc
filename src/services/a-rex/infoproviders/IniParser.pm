package IniParser;

use strict;
use warnings;

# Configuration parser classes for arc.conf

######  IniParser
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
#   my %gmopts = $parser->get_section('grid-manager');      # gm options which are not user-specific
#   my %useropts = $parser->get_section('grid-manager/.');  # gm options for the default user (this
#                                                           # section is instantiated automatically
#                                                           # if the controldir command was used)
# 
# The [grid-manager] section is treated specially.  Options that are
# user-specific are put in separate pseudo-sections [grid-manager/<username>].
# <username> reffers to the user that is initiated by a 'control' command. The
# 'controldir' command initiates user '.'. Each pseudo-section has it's own
# 'controldir' option. Other user-specific options are: 'sessiondir', 'cachedir',
# 'cachesize', 'cachelifetime', 'norootpower', 'maxrerun',
# 'maxtransferfiles' and 'defaultttl'. No substituions are made and user names
# '.' and '*' are not handled specially.
#

######  SubstitutingIniParser
#
# Synopsis:
#
#   use IniParser;
#
#   my $parser = SubstitutingIniParser->new("/etc/arc.conf")
#       or die 'Cannot parse config file';
#
# This class is just like IniParser, but substitutions are made and sections
# for user names like @filename are expanded into separate sections for each
# individual user.
#

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

        # handle runaway LF in CRLF and LFCR
        $line =~ s/^\r//;
        $line =~ s/\r$//;

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

sub has_section($$) {
    my ($self,$sname) = @_;
    return defined $self->{config}{$sname};
}

# list all subsections of a section, but not the section section itself

sub list_subsections($$) {
    my ($self,$sname) = @_;
    my %ssnames = ();
    for (keys %{$self->{config}}) {
        $ssnames{$1}='' if m|^$sname/(.+)|;
    }
    return keys %ssnames;
}
1;

########################################################
package SubstitutingIniParser;
our @ISA = ('IniParser');

sub new($$$) {
    my ($this,$arcconf,$arc_location) = @_;
    my $self = $this->SUPER::new($arcconf);
    return undef unless $self;
    _substitute($self, $arc_location);
    return $self;
}

sub _substitute {
    my ($self, $arc_location) = @_;
    my $config = $self->{config};

    my $lrmsstring = $config->{'grid-manager'}{lrms}
                  || $config->{common}{lrms};
    my ($lrms, $defqueue) = split " ", $lrmsstring || '';

    die 'Gridmap user list feature is not supported anymore. Please use @filename to specify user list.'
        if $config->{'grid-manager/*'};

    # expand user sections whose user name is like @filename
    my @users = $self->list_subsections('grid-manager');
    for my $user (@users) {
        my $section = "grid-manager/$user";
        next unless $user =~ m/^\@(.*)$/;
        my $path = $1;
        my $fh;
        # read in user names from file
        if (open ($fh, "< $path")) {
            while (my $line = <$fh>) {
                chomp (my $newsection = "grid-manager/$line");
                next if exists $config->{$newsection};         # Duplicate user!!!!
                $config->{$newsection} = { %{$config->{$section}} }; # shallow copy
            }
            close $fh;
            delete $config->{$section};
        } else {
            die "Failed opening file to read user list from: $path: $!";
        }
    }

    # substitute per-user options
    @users = $self->list_subsections('grid-manager');
    for my $user (@users) {
        my @pw;
        my $home;
        if ($user ne '.') {
            @pw = getpwnam($user);
            die "getpwnam failed for user: $user: $!" unless @pw;
            $home = $pw[7];
        } else {
            $home = "/tmp";
        }

        my $opts = $config->{"grid-manager/$user"};

        # Default for controldir, sessiondir
        if ($opts->{controldir} eq '*') {
            $opts->{controldir} = $pw[7]."/.jobstatus" if @pw;
        }
        if (not $opts->{sessiondir} or $opts->{sessiondir} eq '*') {
            $opts->{sessiondir} = "$home/.jobs";
        }

        my $controldir = $opts->{controldir};
        my @sessiondirs = split /\[separator\]/, $opts->{sessiondir};

        my $substitute_opt = sub {
            my ($key) = @_;
            my $val = $opts->{$key};
            return unless defined $val;

            #  %R - session root
            $val =~ s/%R/$sessiondirs[0]/g if $val =~ m/%R/;
            #  %C - control dir
            $val =~ s/%C/$controldir/g if $val =~ m/%C/;
            if (@pw) {
                #  %U - username
                $val =~ s/%U/$user/g       if $val =~ m/%U/;
                #  %u - userid
                #  %g - groupid
                #  %H - home dir
                $val =~ s/%u/$pw[2]/g      if $val =~ m/%u/;
                $val =~ s/%g/$pw[3]/g      if $val =~ m/%g/;
                $val =~ s/%H/$home/g       if $val =~ m/%H/;
            }
            #  %L - default lrms
            #  %Q - default queue
            $val =~ s/%L/$lrms/g           if $val =~ m/%L/;
            $val =~ s/%Q/$defqueue/g       if $val =~ m/%Q/;
            #  %W - installation path
            $val =~ s/%W/$arc_location/g   if $val =~ m/%W/;
            #  %G - globus path
            my $G = $ENV{GLOBUS_LOCATION} || '/usr';
            $val =~ s/%G/$G/g              if $val =~ m/%G/;

            $opts->{$key} = $val;
        };
        &$substitute_opt('controldir');
        &$substitute_opt('sessiondir');
        &$substitute_opt('cachedir');
    }

    # authplugin, localcred, helper: not substituted
}
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
    $self->{muopts} = [qw(sessiondir cachedir)];
    $self->{suopts} = [qw(cachesize cachelifetime norootpower maxrerun maxtransferfiles defaultttl)];
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
        delete $thisuser->{$_} for @{$self->{muopts}};
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


sub test {
    require Data::Dumper; import Data::Dumper qw(Dumper);
    my $parser = SubstitutingIniParser->new('/tmp/arc.conf','/usr') or die;
    print Dumper($parser);
    print "@{[$parser->list_subsections('gridftpd')]}\n";
    print "@{[$parser->list_subsections('group')]}\n";
}

#test();

1;
