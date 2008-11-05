#
# set environment variables:
#   LSF_BIN_PATH
#   CONFIG_architecture
#

##############################################################
# Reading configuration from $ARC_CONFIG
##############################################################

if [ -z "$pkglibdir" ]; then echo 'pkglibdir must be set' 1>&2; exit 1; fi

if [ ! -f "$pkglibdir/config_parser.sh" ] ; then
    echo "$pkglibdir/config_parser.sh not found." 1>&2
    exit 1
fi

. "$pkglibdir/config_parser.sh"

ARC_CONFIG=${ARC_CONFIG:-/etc/arc.conf}
config_parse_file $ARC_CONFIG 1>&2 || exit $?

config_import_section "common"
config_import_section "infosys"
config_import_section "grid-manager"
config_import_section "cluster"

# Also read queue section
if [ ! -z "$joboption_queue" ]; then
  config_import_section "queue/$joboption_queue"
fi

# Path to LSF commands
LSF_BIN_PATH=${LSF_BIN_PATH:-$CONFIG_lsf_bin_path}
if [ ! -d ${LSF_BIN_PATH} ] ; then
    echo "Could not set LSF_BIN_PATH." 1>&2
    exit 1
fi

