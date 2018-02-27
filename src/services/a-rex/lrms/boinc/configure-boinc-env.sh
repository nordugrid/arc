#
# set environment variables for boinc
#

##############################################################
# Reading configuration from $ARC_CONFIG
##############################################################

if [ -z "$pkglibexecdir" ]; then echo 'pkglibexecdir must be set' 1>&2; exit 1; fi

blocks="-b arex -b infosys -b common"

ARC_CONFIG=${ARC_CONFIG:-/etc/arc.conf}
eval $( $pkglibexecdir/arcconfig-parser ${blocks} -c ${ARC_CONFIG} --export bash )

# Script returned ok
true
