package condor_env;

use strict;
use warnings;

BEGIN {
    use base 'Exporter';
    our @EXPORT = qw( configure_condor_env );
}

# Initializes environment variables: CONDOR_BIN_PATH
# Values defined in arc.conf take priority over previously set environment
# variables.
# Condor executables are located using the following cues:
# 1. condor_bin_path option in arc.conf
# 2. PATH environment variable

# Synopsis:
#
#   use IniParser;
#   use condor_env;
#  
#   my $parser = IniParser->new('/etc/arc.conf');
#   my %config = $parser->get_section("common");
#   configure_condor_env(%config) or die "Condor executables not found";

# Returns 1 if Condor executables were NOT found, 0 otherwise.

sub configure_condor_env(%) {
    my %config = @_;
    if ($config{condor_bin_path}) {
        $ENV{CONDOR_BIN_PATH} = $config{condor_bin_path};
    }
    else {
        for (split ':', $ENV{PATH}) {
            $ENV{CONDOR_BIN_PATH} = $_ and last if -x "$_/condor_version";
        }
    }
    return 0 unless -x "$ENV{CONDOR_BIN_PATH}/condor_version";
    if ($config{condor_config}) {
        $ENV{CONDOR_CONFIG} = $config{condor_config};
    }
    else {
        $ENV{CONDOR_CONFIG} = "/etc/condor/condor_config";
    }
    return 0 unless -e "$ENV{CONDOR_CONFIG}";
    return 1;
}

1;
