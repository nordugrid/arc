package condor_env;

use strict;
use warnings;

BEGIN {
    use base 'Exporter';
    our @EXPORT = qw( configure_condor_env );
}

# Initializes environment variables:  CONDOR_CONFIG, CONDOR_LOCATION
# Values defined in arc.conf take priority over previously set environment
# variables. Also defines CONDOR_BIN_PATH (the location of Condor executables)
# Condor executables are located using the following cues:
# 1. condor_bin_path option in arc.conf
# 2. condor_location option in arc.conf
# 3. CONDOR_LOCATION environment variable
# 4. PATH environment variable

# Synopsis:
#
#   use ConfigParser;
#   use condor_env;
#  
#   my $parser = ConfigParser->new('/etc/arc.conf');
#   my %config = $parser->get_section("common");
#   configure_condor_env(%config) or die "Condor executables not found"; 


sub configure_condor_env(%) {
    my %config = @_;
    $ENV{CONDOR_CONFIG} = $config{condor_config} if $config{condor_config};
    $ENV{CONDOR_LOCATION} = $config{condor_location} if $config{condor_location};
    for (split ':', $ENV{PATH}) {
        $ENV{CONDOR_BIN_PATH} = $_ and last if -x "$_/condor_version";
    }
    $ENV{CONDOR_BIN_PATH} = "$ENV{CONDOR_LOCATION}/bin" if $ENV{CONDOR_LOCATION};
    $ENV{CONDOR_BIN_PATH} = $config{condor_bin_path} if $config{condor_bin_path};
    return 0 unless -x "$ENV{CONDOR_BIN_PATH}/condor_version";
    return 1;
}

1;
