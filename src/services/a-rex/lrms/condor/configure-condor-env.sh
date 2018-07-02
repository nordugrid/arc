
# Conditionaly enable performance logging
init_perflog

# Initializes environment variables: CONDOR_BIN_PATH
# Valued defines in arc.conf take priority over pre-existing environment
# variables.
# Condor executables are located using the following cues:
# 1. condor_bin_path option in arc.conf
# 2. PATH environment variable

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

# FIX: Recent versions (8.5+?) of HTCondor does not show all jobs when running condor_q, but only own
# Solution according Brain Bockelman GGUS 123947
_condor_CONDOR_Q_ONLY_MY_JOBS=false                                                                 
export _condor_CONDOR_Q_ONLY_MY_JOBS                                                                 
_condor_CONDOR_Q_DASH_BATCH_IS_DEFAULT=false
export _condor_CONDOR_Q_DASH_BATCH_IS_DEFAULT

