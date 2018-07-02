#
# set environment variables:
#   LL_BIN_PATH
#

# performance logging: if perflogdir or perflogfile is set, logging is turned on. So only set them when enable_perflog_reporting is ON
unset perflogdir
unset perflogfile
enable_perflog=${CONFIG_enable_perflog_reporting:-no}
if [ "$CONFIG_enable_perflog_reporting" == "expert-debug-on" ]; then
   perflogdir=${CONFIG_perflogdir:-/var/log/arc/perfdata}
   perflogfile="${perflogdir}/backends.perflog"
fi

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
RUNTIME_LOCAL_SCRATCH_DIR=${RUNTIME_LOCAL_SCRATCH_DIR:-$CONFIG_scratchdir}
export RUNTIME_LOCAL_SCRATCH_DIR

