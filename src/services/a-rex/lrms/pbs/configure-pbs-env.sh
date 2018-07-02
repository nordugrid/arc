#
# set environment variables:
#   PBS_BIN_PATH
#

# Conditionaly enable performance logging
init_perflog

# Path to PBS commands
PBS_BIN_PATH=${CONFIG_pbs_bin_path:-/usr/bin}
if [ ! -d ${PBS_BIN_PATH} ] ; then
    echo "Could not set PBS_BIN_PATH." 1>&2
    exit 1
fi

