#
# set environment variables:
#   LL_BIN_PATH
#

##############################################################
# Reading configuration
##############################################################

if [ -z "$pkglibdir" ]; then echo 'pkglibdir must be set' 1>&2; exit 1; fi

. "$pkglibdir/config_parser.sh" || exit $?

config_parse_default 1>&2 || exit $?

config_import_section "common"
config_import_section "infosys"
config_import_section "grid-manager"


# Path to ll commands
LL_BIN_PATH=${LL_BIN_PATH:-$CONFIG_ll_bin_path}
if [ ! -d ${LL_BIN_PATH} ] ; then
    echo "Could not set LL_BIN_PATH." 1>&2
    exit 1
fi

# Local scratch disk
RUNTIME_LOCAL_SCRATCH_DIR=${RUNTIME_LOCAL_SCRATCH_DIR:-$CONFIG_scratchdir}
export RUNTIME_LOCAL_SCRATCH_DIR

