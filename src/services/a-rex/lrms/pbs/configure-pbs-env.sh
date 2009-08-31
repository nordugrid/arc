#
# set environment variables:
#   PBS_BIN_PATH
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


# Path to PBS commands
PBS_BIN_PATH=${PBS_BIN_PATH:-$CONFIG_pbs_bin_path}
if [ ! -d ${PBS_BIN_PATH} ] ; then
    echo "Could not set PBS_BIN_PATH." 1>&2
    exit 1
fi

