#
# set environment variables:
#   PBS_BIN_PATH
#

##############################################################
# Reading configuration from $ARC_CONFIG
##############################################################

basedir=`dirname $0`
basedir=`cd $basedir; pwd`

if [ ! -f "$basedir/config_parser.sh" ] ; then
    echo "$basedir/config_parser.sh not found." 1>&2
    exit 1
fi

. "$basedir/config_parser.sh"

ARC_CONFIG=${ARC_CONFIG:-/etc/arc.conf}
config_parse_file $ARC_CONFIG 1>&2 || exit $?

config_import_section "common"
config_import_section "infosys"
config_import_section "grid-manager"


# Path to PBS commands
PBS_BIN_PATH=${PBS_BIN_PATH:-$CONFIG_pbs_bin_path}
if [ ! -d ${PBS_BIN_PATH} ] ; then
    echo "Could not set PBS_BIN_PATH." 1>&2
    exit 1
fi

