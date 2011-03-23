#
# set environment variables:
#   LL_BIN_PATH
#

##############################################################
# Reading configuration from $ARC_CONFIG
##############################################################

if [ -z "$pkgdatadir" ]; then echo 'pkgdatadir must be set' 1>&2; exit 1; fi

. "$pkgdatadir/config_parser.sh" || exit $?

ARC_CONFIG=${ARC_CONFIG:-/etc/arc.conf}
config_parse_file $ARC_CONFIG 1>&2 || exit $?

config_import_section "common"
config_import_section "infosys"
config_import_section "grid-manager"

# Also read queue section
if [ ! -z "$joboption_queue" ]; then
  config_import_section "queue/$joboption_queue"
fi

# Path to ll commands
LL_BIN_PATH=$CONFIG_ll_bin_path
if [ ! -d ${LL_BIN_PATH} ] ; then
    echo "Could not set LL_BIN_PATH." 1>&2
    exit 1
fi

# Consumable resources
LL_CONSUMABLE_RESOURCES=${LL_CONSUMABLE_RESOURCES:-$CONFIG_ll_consumable_resources}

# Local scratch disk
RUNTIME_LOCAL_SCRATCH_DIR=${RUNTIME_LOCAL_SCRATCH_DIR:-$CONFIG_scratchdir}
export RUNTIME_LOCAL_SCRATCH_DIR

