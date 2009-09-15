#
# set environment variables:
#

##############################################################
# Reading configuration from $ARC_CONFIG
##############################################################

if [ -z "$pkglibdir" ]; then echo 'pkglibdir must be set' 1>&2; exit 1; fi

. "$pkglibdir/config_parser.sh" || exit $?

ARC_CONFIG=${ARC_CONFIG:-/etc/arc.conf}
config_parse_file $ARC_CONFIG 1>&2 || exit $?

config_import_section "common"
config_import_section "infosys"
config_import_section "grid-manager"
config_import_section "cluster"

# Path to slurm commands
#SLURM_BIN_PATH=${SLURM_BIN_PATH:-$CONFIG_SLURM_bin_path}
SLURM_BIN_PATH=${CONFIG_SLURM_bin_path:-/usr/bin}
if [ ! -d ${SLURM_BIN_PATH} ] ; then
    echo "Could not set SLURM_BIN_PATH." 1>&2
    exit 1
fi

# Local scratch disk
# TODO : why? is this correct? read from arc.conf first?
RUNTIME_LOCAL_SCRATCH_DIR=${RUNTIME_LOCAL_SCRATCH_DIR:-$CONFIG_scratchdir}
export RUNTIME_LOCAL_SCRATCH_DIR

# Paths to SLURM commands
squeue="$SLURM_BIN_PATH/squeue"
scontrol="$SLURM_BIN_PATH/scontrol"
sinfo="$SLURM_BIN_PATH/sinfo"
scancel="$SLURM_BIN_PATH/scancel"
sbatch="$SLURM_BIN_PATH/sbatch"

# Verifies that a SLURM jobid is set, and is an integer
function verify_jobid {
    joboption_jobid="$1"
    # Verify that the jobid is somewhat sane.
    if [ -z ${joboption_jobid} ];then
	echo "error: joboption_jobid is not set" 1>&2
	return 1
    fi
    # jobid in slurm is always an integer, so anything else is an error.
    if [ "" != $(sed s/[0-9]//g <(echo ${joboption_jobid})) ];then
	echo "error: non-numeric characters in joboption_jobid: ${joboption_jobid}" 1>&2
	return 1
    fi
    return 0
}
