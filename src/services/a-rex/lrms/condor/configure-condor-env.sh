
if [ -z "$pkgdatadir" ]; then echo 'pkgdatadir must be set' 1>&2; exit 1; fi

. "$pkgdatadir/config_parser_compat.sh" || exit $?

ARC_CONFIG=${ARC_CONFIG:-/etc/arc.conf}
config_parse_file $ARC_CONFIG || exit $?

config_import_section "common"
config_import_section "infosys"
config_import_section "grid-manager"

# performance logging: if perflogdir or perflogfile is set, logging is turned on. So only set them when enable_perflog_reporting is ON
unset perflogdir
unset perflogfile
enable_perflog=${CONFIG_enable_perflog_reporting:-no}
if [ "$CONFIG_enable_perflog_reporting" == "yes" ]; then
   perflogdir=${CONFIG_perflogdir:-/var/log/arc/perfdata}
   d=`date +%F`
   perflogfile="${perflogdir}/backends${d}.perflog"
fi

# Initializes environment variables: CONDOR_BIN_PATH
# Valued defines in arc.conf take priority over pre-existing environment
# variables.
# Condor executables are located using the following cues:
# 1. condor_bin_path option in arc.conf
# 2. PATH environment variable

# Synopsis:
# 
#   . config_parser.sh
#   config_parse_file /etc/arc.conf || exit 1
#   config_import_section "common"
#   . configure-condor-env.sh || exit 1


if [ ! -z "$CONFIG_condor_bin_path" ]; 
then 
    CONDOR_BIN_PATH=$CONFIG_condor_bin_path; 
else
    condor_version=$(type -p condor_version)
    CONDOR_BIN_PATH=${condor_version%/*}
fi;

if [ ! -x "$CONDOR_BIN_PATH/condor_version" ]; then
    echo 'Condor executables not found!';
    return 1;
fi
echo "Using Condor executables from: $CONDOR_BIN_PATH"
export CONDOR_BIN_PATH

if [ ! -z "$CONFIG_condor_config" ];
then
    CONDOR_CONFIG=$CONFIG_condor_config;
else
    CONDOR_CONFIG="/etc/condor/condor_config";
fi;

if [ ! -e "$CONDOR_CONFIG" ]; then
    echo 'Condor config file not found!';
    return 1;
fi
echo "Using Condor config file at: $CONDOR_CONFIG"
export CONDOR_CONFIG
