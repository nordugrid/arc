#
# set environment variables:
#   LL_BIN_PATH
#

# Conditionaly enable performance logging
init_perflog

# Path to ll commands
LL_BIN_PATH=$CONFIG_ll_bin_path
if [ ! -d ${LL_BIN_PATH} ] ; then
    echo "Could not set LL_BIN_PATH." 1>&2
    exit 1
fi

# Consumable resources
LL_CONSUMABLE_RESOURCES=${LL_CONSUMABLE_RESOURCES:-$CONFIG_ll_consumable_resources}

# Enable parallel single jobs 
LL_PARALLEL_SINGLE_JOBS=${LL_PARALLEL_SINGLE_JOBS:-$CONFIG_ll_parallel_single_jobs}

# Local scratch disk
SCRATCH=`echo ${CONFIG_scratchdir} | cut -d' ' -f1`
RUNTIME_LOCAL_SCRATCH_DIR=${RUNTIME_LOCAL_SCRATCH_DIR:-$SCRATCH}
export RUNTIME_LOCAL_SCRATCH_DIR

