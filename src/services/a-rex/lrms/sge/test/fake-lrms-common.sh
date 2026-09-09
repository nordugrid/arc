# Minimal common layer used to exercise the SGE backend scripts in isolation.

pkgdatadir=$TEST_ROOT/pkgdata
pkglibexecdir=$TEST_ROOT/libexec
TMPDIR=$TEST_ROOT/tmp
export TMPDIR

init_perflog () { :; }

common_init () {
    export GRAMI_FILE
    CONFIG_sge_root=$TEST_ROOT/sge
    CONFIG_sge_bin_path=$TEST_ROOT/bin
    CONFIG_sge_wakeupperiod=0
    CONFIG_sge_query_retries=0
    CONFIG_sge_accounting_retries=1
    perflogdir=
    time_hardlimit_ratio=2
    memory_hardlimit_ratio=2
    RUNTIME_NODE_SEES_FRONTEND=yes
    RUNTIME_LOCAL_SCRATCH_DIR=$TEST_ROOT/scratch

    if [ -n "${ARC_CONFIG:-}" ] && [ -r "$ARC_CONFIG" ]; then
        . "$ARC_CONFIG"
    fi
    if [ -n "${GRAMI_FILE:-}" ] && [ -r "$GRAMI_FILE" ]; then
        . "$GRAMI_FILE"
    fi

    joboption_directory=${joboption_directory:-$TEST_ROOT/session}
    joboption_gridid=${joboption_gridid:-gridjob}
    joboption_arg_0=${joboption_arg_0:-/bin/true}
    joboption_count=${joboption_count:-1}
    joboption_exclusivenode=${joboption_exclusivenode:-false}

    . "$pkgdatadir/configure-sge-env.sh"
}

control_path () {
    case $3 in
        '') printf '%s/jobs/%s' "$1" "$2" ;;
        *)  printf '%s/jobs/%s.%s' "$1" "$2" "$3" ;;
    esac
}
