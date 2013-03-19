
if [ -z "$pkgdatadir" ]; then echo 'pkgdatadir must be set' 1>&2; exit 1; fi

. "$pkgdatadir/config_parser.sh" || exit $?

ARC_CONFIG=${ARC_CONFIG:-/etc/arc.conf}
config_parse_file $ARC_CONFIG || exit $?

config_import_section "common"
config_import_section "infosys"
config_import_section "grid-manager"


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

