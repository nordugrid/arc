# set environment variables:
#   SGE_BIN_PATH
#   SGE_ROOT
#   SGE_CELL
#   SGE_QMASTER_PORT
#   SGE_EXECD_PORT
#

# Conditionaly enable performance logging
init_perflog

##############################################################
# Initialize SGE environment variables
##############################################################

SGE_ROOT=${CONFIG_sge_root:-$SGE_ROOT}

if [ -z "$SGE_ROOT" ]; then
    echo 'SGE_ROOT not set' 1>&2
    return 1
fi

SGE_CELL=${SGE_CELL:-default}
SGE_CELL=${CONFIG_sge_cell:-$SGE_CELL}
export SGE_ROOT SGE_CELL

if [ ! -z "$CONFIG_sge_qmaster_port" ]; then
    export SGE_QMASTER_PORT=$CONFIG_sge_qmaster_port
fi

if [ ! -z "$CONFIG_sge_execd_port" ]; then
    export SGE_EXECD_PORT=$CONFIG_sge_execd_port
fi

##############################################################
# Find path to SGE executables
##############################################################

# 1. use sge_bin_path config option, if set
if [ ! -z "$CONFIG_sge_bin_path" ]; then
    SGE_BIN_PATH=$CONFIG_sge_bin_path;
fi

# 2. otherwise see if qsub can be found in the path
if [ -z "$SGE_BIN_PATH" ]; then
    qsub=$(type -p qsub)
    SGE_BIN_PATH=${qsub%/*}
    unset qsub
fi

if [ ! -x "$SGE_BIN_PATH/qsub" ]; then
    echo 'SGE executables not found! Check that sge_bin_path is defined' 1>&2
    return 1
fi

export SGE_BIN_PATH
