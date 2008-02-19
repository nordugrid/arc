#!/bin/sh -f
#
# set environment variables:
#   LL_BIN_PATH
#

##############################################################
# Reading configuration from $ARC_CONFIG
##############################################################

if [ ! -f "$NORDUGRID_LOCATION/libexec/config_parser.sh" ] ; then
    echo "$NORDUGRID_LOCATION/libexec/config_parser.sh not found." 1>&2
    exit 1
fi

. $NORDUGRID_LOCATION/libexec/config_parser.sh

ARC_CONFIG=${ARC_CONFIG:-/etc/arc.conf}
config_parse_file $ARC_CONFIG 1>&2 || exit $?

config_update_from_section "common"
config_update_from_section "infosys"
config_update_from_section "grid-manager"


# Path to ll commands
LL_BIN_PATH=${LL_BIN_PATH:-$CONFIG_ll_bin_path}
if [ ! -d ${LL_BIN_PATH} ] ; then
    echo "Could not set LL_BIN_PATH." 1>&2
    exit 1
fi

# Local scratch disk
RUNTIME_LOCAL_SCRATCH_DIR=${RUNTIME_LOCAL_SCRATCH_DIR:-$CONFIG_scratchdir}
export RUNTIME_LOCAL_SCRATCH_DIR

