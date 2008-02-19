package config_parser;

use strict;
use warnings;


# Configuration parser module for perl
#
# Synopsis:
#
#   use config_parser;
#
#   config_parse_file("/etc/arc.conf") or die 'Cannot parse config file';
#   print config_get_section_names();                        # list all section names
#   my %config = (gm_port => 2811, gm_mount_point => '/jobs');    # set some defaults
#   %config = config_update_from_section('common', %config);      # import a section
#   %config = config_update_from_section('infosys', %config);     # import a section
#   %config = config_update_from_section('queue/atlas', %config); # import a section


BEGIN {
    use base 'Exporter';

    # Set the version for version checking.
    our $VERSION = '1.000';
    our @EXPORT = qw(
        config_parse_file
        config_update_from_section
        config_get_section_names
    );
}

my %data=();
my @sectionnames=();

#
# Expects the filename of the arc.conf file. 
# Returns False if it cannot open the file.
#
sub config_parse_file($) {
    my $sectionname;
    my $arcconf=shift;

    # Clear old data
    %data=();
    @sectionnames=();

    unless (open(CONFIGFILE, $arcconf)) {
        return undef;
    }
    while (my $line =<CONFIGFILE>) {
       next if $line =~/^\s*#/;
       next if $line =~/^\s*$/;
    
       if ($line =~/^\s*\[(.+)\]/ ) {
          $sectionname = $1;
          push @sectionnames, $sectionname;
          next;
       }
       # Single or double quotes can be used. Quotes are removed from the values
       next unless $line =~ /^(\w+)\s*=\s*(["'])(.*)(\2)\s*$/;
       my $key=$1;
       my $value=$3;
       if (not defined $data{$sectionname}{$key}) {
          $data{$sectionname}{$key} = $value;
       }
       else {
          $data{$sectionname}{$key} .= "[separator]".$value;
       }
    }
    close CONFIGFILE;
    return 1;
}

#
# Arguments:
#  - a section name from the configuration file
#  - a hash with previous configuration options (optional)
# Returns:
#  - an updated hash with all the options defined in the input hash, plus options
#    defined in the requested section.  New options overwrite old ones.
#
sub config_update_from_section($%) {
    my ($sn, %config) = @_;
    $config{$_}=$data{$sn}{$_} for keys %{$data{$sn}};
    return %config;
} 

#
# Returns the list of all sections found in the config file.
#
sub config_get_section_names() {
    return @sectionnames;
}


1;
