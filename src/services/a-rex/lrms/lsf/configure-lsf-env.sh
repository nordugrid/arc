#
# set environment variables:
#   LSF_BIN_PATH
#   CONFIG_lsf_architecture
#

# Conditionaly enable performance logging
init_perflog

# Path to LSF commands
LSF_BIN_PATH=$CONFIG_lsf_bin_path
if [ ! -d ${LSF_BIN_PATH} ] ; then
    echo "Could not set LSF_BIN_PATH." 1>&2
    exit 1
fi

