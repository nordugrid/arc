#
# Common block for cancel scripts
# must be called with the grami file as argument
# remember to set $joboption_lrms

if [ -z ${basedir} ] ; then
    echo "basedir not set."  1>&2
    exit 1
fi
if [ ! -f "${basedir}/configure-${joboption_lrms}-env.sh" ] ; then
    echo "${basedir}/configure-${joboption_lrms}-env.sh not found." 1>&2
    exit 1
fi
. ${basedir}/configure-${joboption_lrms}-env.sh

arg_file=$1
##############################################################
# Source the argument file.
##############################################################
if [ -z "$arg_file" ] ; then
   echo "Arguments file should be specified" 1>&2
   exit 1
fi
if [ ! -f $arg_file ] ; then
   echo "Missing arguments file" 1>&2
   exit 1
fi

. $arg_file

