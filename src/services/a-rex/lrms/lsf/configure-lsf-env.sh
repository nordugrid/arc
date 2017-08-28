#
# set environment variables:
#   LSF_BIN_PATH
#   CONFIG_lsf_architecture
#

##############################################################
# Reading configuration from $ARC_CONFIG
##############################################################

if [ -z "$pkglibexecdir" ]; then echo 'pkglibexecdir must be set' 1>&2; exit 1; fi

# Order matters
blocks="-b cluster -b arex -b infosys -b common"
if [ ! -z "$joboption_queue" ]; then
  blocks="-b queue/$joboption_queue $blocks"
fi

ARC_CONFIG=${ARC_CONFIG:-/etc/arc.conf}
eval $( $pkgdatadir/arcconfig-parser ${blocks} -c ${ARC_CONFIG} --export bash )

# performance logging: if perflogdir or perflogfile is set, logging is turned on. So only set them when enable_perflog_reporting is ON
unset perflogdir
unset perflogfile
enable_perflog=${CONFIG_enable_perflog_reporting:-no}
if [ "$CONFIG_enable_perflog_reporting" == "expert-debug-on" ]; then
   perflogdir=${CONFIG_perflogdir:-/var/log/arc/perfdata}
   perflogfile="${perflogdir}/backends.perflog"
fi

# Path to LSF commands
LSF_BIN_PATH=$CONFIG_lsf_bin_path
if [ ! -d ${LSF_BIN_PATH} ] ; then
    echo "Could not set LSF_BIN_PATH." 1>&2
    exit 1
fi

