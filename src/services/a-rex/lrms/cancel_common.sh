#
# Common block for cancel scripts
# must be called with the grami file as argument
# remember to set $joboption_lrms

if [ -z "$joboption_lrms" ]; then echo 'joboption_lrms must be set' 1>&2; exit 1; fi
if [ -z "$pkglibdir" ]; then echo 'pkglibdir must be set' 1>&2; exit 1; fi

. ${pkglibdir}/configure-${joboption_lrms}-env.sh || exit $?

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

