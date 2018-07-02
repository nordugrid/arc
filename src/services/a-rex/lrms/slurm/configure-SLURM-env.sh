#
# set environment variables:
#

# Conditionaly enable performance logging
init_perflog

# Path to slurm commands
SLURM_BIN_PATH=${CONFIG_slurm_bin_path:-/usr/bin}
if [ ! -d ${SLURM_BIN_PATH} ] ; then
    echo "Could not set SLURM_BIN_PATH." 1>&2
    exit 1
fi

# Paths to SLURM commands
squeue="$SLURM_BIN_PATH/squeue"
scontrol="$SLURM_BIN_PATH/scontrol"
sinfo="$SLURM_BIN_PATH/sinfo"
scancel="$SLURM_BIN_PATH/scancel"
sbatch="$SLURM_BIN_PATH/sbatch"
sacct="$SLURM_BIN_PATH/sacct"

# Verifies that a SLURM jobid is set, and is an integer
verify_jobid () {
    joboption_jobid="$1"
    # Verify that the jobid is somewhat sane.
    if [ -z ${joboption_jobid} ];then
	echo "error: joboption_jobid is not set" 1>&2
	return 1
    fi
    # jobid in slurm is always an integer, so anything else is an error.
    if [ "x" != "x$(echo ${joboption_jobid} | sed s/[0-9]//g )" ];then
	echo "error: non-numeric characters in joboption_jobid: ${joboption_jobid}" 1>&2
	return 1
    fi
    return 0
}
