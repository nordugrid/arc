#
# set environment fork variables:
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

# Script returned ok
true
