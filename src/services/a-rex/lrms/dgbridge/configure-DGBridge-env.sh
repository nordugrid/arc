#
# set environment variables for the desktop grid bridge:
#

##############################################################
# Reading configuration from $ARC_CONFIG
##############################################################

if [ -z "$pkglibexecdir" ]; then echo 'pkglibexecdir must be set' 1>&2; exit 1; fi

# Order matters
blocks="-b lrms -b arex -b infosys -b common"

ARC_CONFIG=${ARC_CONFIG:-/etc/arc.conf}
eval $( $pkgdatadir/arcconfig-parser ${blocks} -c ${ARC_CONFIG} --export bash )

dgbridge_stage_dir=$CONFIG_dgbridge_stage_dir
dgbridge_stage_prepend=$CONFIG_dgbridge_stage_prepend

#set 3G values
EDGES_3G_TIMEOUT=24
EDGES_3G_RETRIES=3

# Script returned ok
true
