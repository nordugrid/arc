#!/bin/bash

source $ARC_LOCATION/libexec/config_parser.sh

ARC_CONFIG=${ARC_CONFIG:-/etc/arc.conf}
config_parse_file $ARC_CONFIG || return 1

config_update_from_section "common"
config_update_from_section "infosys"
config_update_from_section "grid-manager"


# Initializes environment variables:  CONDOR_CONFIG, CONDOR_LOCATION
# Valued defines in arc.conf take priority over pre-existing environment
# variables. Also defines CONDOR_BIN_PATH (the location of Condor executables)
# Condor executables are located using the following cues:
# 1. condor_bin_path option in arc.conf
# 2. condor_location option in arc.conf
# 3. CONDOR_LOCATION environment variable
# 4. PATH environment variable

# Synopsis:
# 
#   source config_parser.sh
#   config_parse_file /etc/arc.conf || exit 1
#   config_update_from_section "common"
#   source configure-condor-env.sh || exit 1


if [ ! -z "$CONFIG_condor_config" ];   then CONDOR_CONFIG=$CONFIG_condor_config; fi
if [ ! -z "$CONFIG_condor_location" ]; then CONDOR_LOCATION=$CONFIG_condor_location; fi
condor_version=$(type -p condor_version)
CONDOR_BIN_PATH=${condor_version%/*}
if [ ! -z "$CONDOR_LOCATION" ]; then CONDOR_BIN_PATH="$CONDOR_LOCATION/bin"; fi
if [ ! -z "$CONFIG_condor_bin_path" ]; then CONDOR_BIN_PATH=$CONFIG_condor_bin_path; fi;
if [ ! -x "$CONDOR_BIN_PATH/condor_version" ]; then
    echo 'Condor executables not found!';
    return 1;
fi
echo "Using Condor executables from: $CONDOR_BIN_PATH"
export CONDOR_LOCATION CONDOR_CONFIG CONDOR_BIN_PATH

