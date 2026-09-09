#!/bin/sh

# Focused contract tests for the command interfaces shared by SGE and Altair
# Grid Engine.  They deliberately run the backend scripts, with only the ARC
# common layer and scheduler commands replaced by deterministic test doubles.

failures=0
tests=0

ok () {
    tests=$((tests + 1))
    printf 'ok %d - %s\n' "$tests" "$1"
}

not_ok () {
    tests=$((tests + 1))
    failures=$((failures + 1))
    printf 'not ok %d - %s\n' "$tests" "$1"
    [ ! -r "$TEST_ROOT/stderr" ] || sed 's/^/  /' "$TEST_ROOT/stderr"
}

assert_success () {
    description=$1
    shift
    if "$@"; then ok "$description"; else not_ok "$description"; fi
}

assert_failure () {
    description=$1
    shift
    if "$@"; then not_ok "$description"; else ok "$description"; fi
}

assert_grep () {
    description=$1
    pattern=$2
    path=$3
    if grep -E -- "$pattern" "$path" >/dev/null 2>&1; then
        ok "$description"
    else
        not_ok "$description"
    fi
}

assert_not_grep () {
    description=$1
    pattern=$2
    path=$3
    if grep -E -- "$pattern" "$path" >/dev/null 2>&1; then
        not_ok "$description"
    else
        ok "$description"
    fi
}

require_file () {
    variable=$1
    eval "path=\${$variable:-}"
    if [ -z "$path" ] || [ ! -r "$path" ]; then
        printf 'missing test input %s (%s)\n' "$variable" "${path:-unset}" 1>&2
        exit 99
    fi
}

require_file SGE_SUBMIT_SCRIPT
require_file SGE_SCAN_SCRIPT
require_file SGE_CANCEL_SCRIPT
require_file SGE_CONFIGURE_ENV
require_file SGE_TEST_SRCDIR

TEST_ROOT=$(mktemp -d "${TMPDIR:-/tmp}/arc-sge-test.XXXXXX") || exit 99
export TEST_ROOT
trap 'rm -rf "$TEST_ROOT"' 0 1 2 15

mkdir -p "$TEST_ROOT/bin" "$TEST_ROOT/pkgdata" "$TEST_ROOT/libexec" \
    "$TEST_ROOT/tmp" "$TEST_ROOT/session" "$TEST_ROOT/scratch"

# Copy the generated scripts so their normal basedir lookup resolves to the
# fake common layer.  Replacing the token also permits direct source-tree runs.
for script_spec in \
    "$SGE_SUBMIT_SCRIPT:submit-sge-job" \
    "$SGE_SCAN_SCRIPT:scan-sge-job" \
    "$SGE_CANCEL_SCRIPT:cancel-sge-job"
do
    source_path=${script_spec%%:*}
    target_name=${script_spec##*:}
    sed 's|@posix_shell@|/bin/sh|g' "$source_path" > "$TEST_ROOT/$target_name"
    chmod +x "$TEST_ROOT/$target_name"
done

cp "$SGE_TEST_SRCDIR/fake-lrms-common.sh" "$TEST_ROOT/lrms_common.sh"
cp "$SGE_TEST_SRCDIR/fake-submit-common.sh" "$TEST_ROOT/pkgdata/submit_common.sh"
cp "$SGE_TEST_SRCDIR/fake-scan-common.sh" "$TEST_ROOT/pkgdata/scan_common.sh"
cp "$SGE_TEST_SRCDIR/fake-cancel-common.sh" "$TEST_ROOT/pkgdata/cancel_common.sh"
cp "$SGE_CONFIGURE_ENV" "$TEST_ROOT/pkgdata/configure-sge-env.sh"
cp "$SGE_TEST_SRCDIR/fake-gm-kick" "$TEST_ROOT/libexec/gm-kick"
chmod +x "$TEST_ROOT/libexec/gm-kick"

cp "$SGE_TEST_SRCDIR/fake-sge-command" "$TEST_ROOT/bin/fake-sge-command"
chmod +x "$TEST_ROOT/bin/fake-sge-command"
for command_name in qsub qstat qacct qdel qconf qhost; do
    ln -s fake-sge-command "$TEST_ROOT/bin/$command_name"
done

reset_scheduler () {
    : > "$TEST_ROOT/calls"
    : > "$TEST_ROOT/qsub.output"
    : > "$TEST_ROOT/qconf.spl"
    : > "$TEST_ROOT/qconf.sp"
    : > "$TEST_ROOT/qconf.sc"
    : > "$TEST_ROOT/qstat.xml"
    : > "$TEST_ROOT/qstat.job.output"
    : > "$TEST_ROOT/qacct.output"
    : > "$TEST_ROOT/qdel.output"
    printf '0\n' > "$TEST_ROOT/qsub.rc"
    printf '0\n' > "$TEST_ROOT/qconf.rc"
    printf '0\n' > "$TEST_ROOT/qstat.rc"
    printf '1\n' > "$TEST_ROOT/qstat.job.rc"
    printf '0\n' > "$TEST_ROOT/qacct.rc"
    printf '0\n' > "$TEST_ROOT/qdel.rc"
    rm -f "$TEST_ROOT/submitted.job" "$TEST_ROOT/stdout" "$TEST_ROOT/stderr" \
        "$TEST_ROOT/qsub.break_grami" "$TEST_ROOT/qsub.cwd"
}

write_submit_grami () {
    grami_path=$1
    count=$2
    cat > "$grami_path" <<EOF
joboption_directory='$TEST_ROOT/session'
joboption_gridid='arcjob'
joboption_arg_0='/bin/true'
joboption_count='$count'
joboption_queue='short.q'
joboption_rsl_project='atlas'
joboption_jobname='SGE regression'
joboption_exclusivenode='false'
EOF
}

run_submit () {
    grami_path=$1
    "$TEST_ROOT/submit-sge-job" "$grami_path" > "$TEST_ROOT/stdout" 2> "$TEST_ROOT/stderr"
}

run_submit_config () {
    config_path=$1
    grami_path=$2
    "$TEST_ROOT/submit-sge-job" --config "$config_path" "$grami_path" \
        > "$TEST_ROOT/stdout" 2> "$TEST_ROOT/stderr"
}

run_submit_relative () {
    grami_name=${1##*/}
    (
        cd "$TEST_ROOT" || exit 1
        ./submit-sge-job "$grami_name"
    ) > "$TEST_ROOT/stdout" 2> "$TEST_ROOT/stderr"
}

# qsub's terse protocol must be used and accepted only alongside status zero.
reset_scheduler
write_submit_grami "$TEST_ROOT/success.grami" 1
printf '73001\n' > "$TEST_ROOT/qsub.output"
assert_success 'terse qsub success is accepted' run_submit "$TEST_ROOT/success.grami"
assert_not_grep 'debug tracing is disabled by default' 'DEBUG event=' "$TEST_ROOT/stderr"
assert_grep 'submission log correlates ARC and scheduler IDs' \
    'arc_job=arcjob .*event=submitted assigned_sge_job=73001' "$TEST_ROOT/stderr"
assert_grep 'qsub uses terse output and ARC directive prefix' \
    '^qsub -terse -b n -shell y -C #\$ -wd .+ -v __SGE_PREFIX__O_WORKDIR=.+ -S .+ .+/job\.script$' "$TEST_ROOT/calls"
assert_grep 'qsub ignores submission-directory request defaults' \
    '/sge-qsub\.[A-Za-z0-9]+$' "$TEST_ROOT/qsub.cwd"
assert_grep 'the terse job ID is persisted in GRAMi' '^joboption_jobid=73001$' "$TEST_ROOT/success.grami"
assert_grep 'submission epoch is persisted for accounting identity' '^joboption_sge_submit_time=[0-9]+$' "$TEST_ROOT/success.grami"
assert_grep 'unique native job name is persisted for live identity' \
    '^joboption_sge_job_name=arc_SGE_regression_arcjob$' "$TEST_ROOT/success.grami"
runtime_cd_line=`sed -n '/^  if cd "\$RUNTIME_JOB_DIR"; then$/{=;q;}' "$TEST_ROOT/submitted.job"`
rte_stage1_line=`sed -n '/^# TEST_RTE_STAGE1$/{=;q;}' "$TEST_ROOT/submitted.job"`
if [ -n "$runtime_cd_line" ] && [ -n "$rte_stage1_line" ] \
    && [ "$runtime_cd_line" -lt "$rte_stage1_line" ]; then
    ok 'stage-one runtime hooks run after entering the effective runtime directory'
else
    not_ok 'stage-one runtime hooks run after entering the effective runtime directory'
fi
assert_grep 'the runtime directory becomes HOME before stage one' \
    '^    HOME=\$RUNTIME_JOB_DIR$' "$TEST_ROOT/submitted.job"
assert_grep 'the generated workload preserves the conventional submit directory' \
    '^SGE_O_WORKDIR=.*/session$' "$TEST_ROOT/submitted.job"
assert_grep 'the conventional submit directory is exported for the workload' \
    '^export SGE_O_WORKDIR$' "$TEST_ROOT/submitted.job"
assert_grep 'queue directive is generated' '^#\$ -q short\.q$' "$TEST_ROOT/submitted.job"
assert_grep 'project directive is generated' '^#\$ -P atlas$' "$TEST_ROOT/submitted.job"
first_sge_directive=`sed -n '/^#\$ /{p;q;}' "$TEST_ROOT/submitted.job"`
if [ "$first_sge_directive" = '#$ -clear' ]; then
    ok 'ARC starts its ordered Grid Engine options with -clear'
else
    not_ok 'ARC starts its ordered Grid Engine options with -clear'
fi

reset_scheduler
write_submit_grami "$TEST_ROOT/relative.grami" 1
printf '73014\n' > "$TEST_ROOT/qsub.output"
assert_success 'relative GRAMi path remains valid throughout submission' \
    run_submit_relative "$TEST_ROOT/relative.grami"
assert_grep 'job ID is appended to the original relative GRAMi file' \
    '^joboption_jobid=73014$' "$TEST_ROOT/relative.grami"
assert_not_grep 'submission does not create a second GRAMi in the session' \
    '^joboption_jobid=' "$TEST_ROOT/session/relative.grami"

reset_scheduler
write_submit_grami "$TEST_ROOT/mapped-queues.grami" 1
printf "CONFIG_sge_queues='batch.q gpu.q'\n" > "$TEST_ROOT/mapped-queues.conf"
printf '73008\n' > "$TEST_ROOT/qsub.output"
assert_success 'ARC share maps to native Grid Engine queues' \
    run_submit_config "$TEST_ROOT/mapped-queues.conf" "$TEST_ROOT/mapped-queues.grami"
assert_grep 'native queues are emitted as one qsub queue list' '^#\$ -q batch\.q,gpu\.q$' "$TEST_ROOT/submitted.job"

reset_scheduler
write_submit_grami "$TEST_ROOT/malformed.grami" 1
printf 'Your job 73002 has been submitted\n' > "$TEST_ROOT/qsub.output"
assert_failure 'malformed qsub terse output fails submission' run_submit "$TEST_ROOT/malformed.grami"
assert_not_grep 'a malformed job ID is not persisted' '^joboption_jobid=' "$TEST_ROOT/malformed.grami"

reset_scheduler
write_submit_grami "$TEST_ROOT/jsv-prose.grami" 1
printf 'Your job 73002 ("jsv_adjusted") has been submitted.\n' > "$TEST_ROOT/qsub.output"
assert_success 'documented non-terse success after a JSV rewrite is accepted' \
    run_submit "$TEST_ROOT/jsv-prose.grami"
assert_grep 'the ordinary qsub success ID is persisted' \
    '^joboption_jobid=73002$' "$TEST_ROOT/jsv-prose.grami"

reset_scheduler
write_submit_grami "$TEST_ROOT/jsv-prose-noperiod.grami" 1
printf 'Your job 73016 ("compatible") has been submitted\n' > "$TEST_ROOT/qsub.output"
assert_success 'legacy non-terse success without punctuation remains accepted' \
    run_submit "$TEST_ROOT/jsv-prose-noperiod.grami"
assert_grep 'the unpunctuated ordinary success ID is persisted' \
    '^joboption_jobid=73016$' "$TEST_ROOT/jsv-prose-noperiod.grami"

reset_scheduler
write_submit_grami "$TEST_ROOT/mixed-output.grami" 1
printf 'warning on stdout\n73003\n' > "$TEST_ROOT/qsub.output"
assert_failure 'terse output mixed with other stdout is rejected' run_submit "$TEST_ROOT/mixed-output.grami"
assert_not_grep 'an ID from mixed stdout is not persisted' '^joboption_jobid=' "$TEST_ROOT/mixed-output.grami"
assert_grep 'a uniquely recoverable mixed-output job is cancelled' '^qdel 73003$' "$TEST_ROOT/calls"

reset_scheduler
write_submit_grami "$TEST_ROOT/array-output.grami" 1
printf '73011.1-10:1\n' > "$TEST_ROOT/qsub.output"
assert_failure 'unexpected terse array ID is not accepted as an ARC job' run_submit "$TEST_ROOT/array-output.grami"
assert_not_grep 'an array ID is not persisted' '^joboption_jobid=' "$TEST_ROOT/array-output.grami"
assert_grep 'the untrackable array is cancelled by its base ID' '^qdel 73011$' "$TEST_ROOT/calls"

reset_scheduler
write_submit_grami "$TEST_ROOT/prose-array-output.grami" 1
printf 'Your job-array 73012.1-4:1 ("array") has been submitted.\n' > "$TEST_ROOT/qsub.output"
assert_failure 'unexpected ordinary-output array is not accepted as an ARC job' \
    run_submit "$TEST_ROOT/prose-array-output.grami"
assert_not_grep 'an ordinary-output array ID is not persisted' \
    '^joboption_jobid=' "$TEST_ROOT/prose-array-output.grami"
assert_grep 'the ordinary-output array is cancelled by its base ID' \
    '^qdel 73012$' "$TEST_ROOT/calls"

reset_scheduler
write_submit_grami "$TEST_ROOT/nonzero.grami" 1
printf '73004\n' > "$TEST_ROOT/qsub.output"
printf '2\n' > "$TEST_ROOT/qsub.rc"
assert_failure 'nonzero qsub status fails despite numeric output' run_submit "$TEST_ROOT/nonzero.grami"
assert_not_grep 'a failed qsub job ID is not persisted' '^joboption_jobid=' "$TEST_ROOT/nonzero.grami"

reset_scheduler
write_submit_grami "$TEST_ROOT/unpersisted.grami" 1
: > "$TEST_ROOT/qsub.break_grami"
printf '73009\n' > "$TEST_ROOT/qsub.output"
assert_failure 'submission fails when the assigned ID cannot be persisted' \
    run_submit "$TEST_ROOT/unpersisted.grami"
assert_grep 'untracked submitted job is cancelled' '^qdel 73009$' "$TEST_ROOT/calls"

reset_scheduler
write_submit_grami "$TEST_ROOT/native-array.grami" 1
printf "CONFIG_sge_jobopts='-t 1-10'\n" > "$TEST_ROOT/native-array.conf"
assert_failure 'native options cannot turn an ARC job into an array' \
    run_submit_config "$TEST_ROOT/native-array.conf" "$TEST_ROOT/native-array.grami"
assert_not_grep 'rejected structural native options never reach qsub' '^qsub ' "$TEST_ROOT/calls"

reset_scheduler
write_submit_grami "$TEST_ROOT/native-option-file.grami" 1
printf "CONFIG_sge_jobopts='-@ /tmp/qsub-options'\n" > "$TEST_ROOT/native-option-file.conf"
assert_failure 'native option files cannot bypass ARC structural guards' \
    run_submit_config "$TEST_ROOT/native-option-file.conf" "$TEST_ROOT/native-option-file.grami"
assert_not_grep 'rejected native option files never reach qsub' '^qsub ' "$TEST_ROOT/calls"

reset_scheduler
write_submit_grami "$TEST_ROOT/quoted-native-array.grami" 1
printf "CONFIG_sge_jobopts='\"-t\" 1-10'\n" > "$TEST_ROOT/quoted-native-array.conf"
assert_failure 'quoted native options cannot bypass structural guards' \
    run_submit_config "$TEST_ROOT/quoted-native-array.conf" "$TEST_ROOT/quoted-native-array.grami"
assert_not_grep 'rejected quoted structural options never reach qsub' '^qsub ' "$TEST_ROOT/calls"

reset_scheduler
write_submit_grami "$TEST_ROOT/mixed-quoted-native-array.grami" 1
printf '%s\n' "CONFIG_sge_jobopts=\"'-\\\"t' 1-10\"" \
    > "$TEST_ROOT/mixed-quoted-native-array.conf"
assert_failure 'Grid Engine mixed-quote stripping cannot bypass the array guard' \
    run_submit_config "$TEST_ROOT/mixed-quoted-native-array.conf" \
        "$TEST_ROOT/mixed-quoted-native-array.grami"
assert_not_grep 'rejected mixed-quote native option never reaches qsub' \
    '^qsub ' "$TEST_ROOT/calls"

for unmatched_native in '-sync y"' '-t 1-10"'; do
    reset_scheduler
    write_submit_grami "$TEST_ROOT/unmatched-native.grami" 1
    printf "CONFIG_sge_jobopts='%s'\n" "$unmatched_native" \
        > "$TEST_ROOT/unmatched-native.conf"
    assert_failure "unmatched quote in native '$unmatched_native' is rejected" \
        run_submit_config "$TEST_ROOT/unmatched-native.conf" \
            "$TEST_ROOT/unmatched-native.grami"
    assert_not_grep "unmatched-quote native '$unmatched_native' never reaches qsub" \
        '^qsub ' "$TEST_ROOT/calls"
done

reset_scheduler
write_submit_grami "$TEST_ROOT/native-now.grami" 1
printf "CONFIG_sge_jobopts='-now y'\n" > "$TEST_ROOT/native-now.conf"
assert_failure 'native immediate mode cannot change asynchronous submission' \
    run_submit_config "$TEST_ROOT/native-now.conf" "$TEST_ROOT/native-now.grami"
assert_not_grep 'rejected immediate mode never reaches qsub' '^qsub ' "$TEST_ROOT/calls"

reset_scheduler
mkdir -p "$TEST_ROOT/sge/default/common"
printf '%s\n' '-sync y' > "$TEST_ROOT/sge/default/common/sge_request"
write_submit_grami "$TEST_ROOT/default-sync.grami" 1
assert_failure 'inherited synchronous mode is rejected before qsub' \
    run_submit "$TEST_ROOT/default-sync.grami"
assert_not_grep 'unsafe qsub defaults never reach qsub' '^qsub ' "$TEST_ROOT/calls"
rm -f "$TEST_ROOT/sge/default/common/sge_request"

reset_scheduler
printf '%s\n' "'-s\"ync' y" > "$TEST_ROOT/sge/default/common/sge_request"
write_submit_grami "$TEST_ROOT/default-mixed-sync.grami" 1
assert_failure 'mixed-quote inherited synchronous mode is rejected' \
    run_submit "$TEST_ROOT/default-mixed-sync.grami"
assert_not_grep 'mixed-quote synchronous default never reaches qsub' \
    '^qsub ' "$TEST_ROOT/calls"
rm -f "$TEST_ROOT/sge/default/common/sge_request"

reset_scheduler
printf '%s\n' "'-\"@' /tmp/more-options" > "$TEST_ROOT/sge/default/common/sge_request"
write_submit_grami "$TEST_ROOT/default-mixed-option-file.grami" 1
assert_failure 'mixed-quote inherited option file is rejected' \
    run_submit "$TEST_ROOT/default-mixed-option-file.grami"
assert_not_grep 'mixed-quote option-file default never reaches qsub' \
    '^qsub ' "$TEST_ROOT/calls"
rm -f "$TEST_ROOT/sge/default/common/sge_request"

for unsafe_default in '-t 1-10' '-tc 2' '-binding linear:4' '-@ /tmp/more-options'; do
    reset_scheduler
    printf '%s\n' "$unsafe_default" > "$TEST_ROOT/sge/default/common/sge_request"
    write_submit_grami "$TEST_ROOT/default-structural.grami" 1
    assert_failure "pre-clear qsub default '$unsafe_default' is rejected" \
        run_submit "$TEST_ROOT/default-structural.grami"
    assert_not_grep "rejected qsub default '$unsafe_default' never reaches qsub" \
        '^qsub ' "$TEST_ROOT/calls"
done
rm -f "$TEST_ROOT/sge/default/common/sge_request"

reset_scheduler
printf '%s\n' '-shell n' > "$TEST_ROOT/sge/default/common/sge_request"
write_submit_grami "$TEST_ROOT/default-shell.grami" 1
printf '73015\n' > "$TEST_ROOT/qsub.output"
assert_success 'command line restores script interpretation after a default -shell n' \
    run_submit "$TEST_ROOT/default-shell.grami"
rm -f "$TEST_ROOT/sge/default/common/sge_request"

reset_scheduler
printf '%s\n' '-now y' > "$TEST_ROOT/sge/default/common/sge_request"
write_submit_grami "$TEST_ROOT/default-now.grami" 1
printf '73013\n' > "$TEST_ROOT/qsub.output"
assert_success 'pre-clear immediate default is reset by the embedded clear' \
    run_submit "$TEST_ROOT/default-now.grami"
rm -f "$TEST_ROOT/sge/default/common/sge_request"

# More than one slot is never silently submitted without a valid PE.
reset_scheduler
write_submit_grami "$TEST_ROOT/no-pe.grami" 4
printf 'mpi\n' > "$TEST_ROOT/qconf.spl"
assert_failure 'multi-slot submission without a selected PE is rejected' run_submit "$TEST_ROOT/no-pe.grami"
assert_not_grep 'qsub is not reached after PE validation fails' '^qsub ' "$TEST_ROOT/calls"

reset_scheduler
write_submit_grami "$TEST_ROOT/with-pe.grami" 4
printf "CONFIG_sge_pe='mpi'\n" > "$TEST_ROOT/pe.conf"
printf 'mpi\nopenmp\n' > "$TEST_ROOT/qconf.spl"
cat > "$TEST_ROOT/qconf.sp" <<'EOF'
pe_name            mpi
allocation_rule    $pe_slots
control_slaves     TRUE
accounting_summary TRUE
EOF
printf '73005\n' > "$TEST_ROOT/qsub.output"
assert_success 'configured PE permits a multi-slot submission' \
    run_submit_config "$TEST_ROOT/pe.conf" "$TEST_ROOT/with-pe.grami"
assert_grep 'multi-slot request emits the PE and slot directive' '^#\$ -pe mpi 4$' "$TEST_ROOT/submitted.job"

reset_scheduler
write_submit_grami "$TEST_ROOT/loose-pe.grami" 4
printf "CONFIG_sge_pe='mpi'\n" > "$TEST_ROOT/loose-pe.conf"
printf 'mpi\n' > "$TEST_ROOT/qconf.spl"
cat > "$TEST_ROOT/qconf.sp" <<'EOF'
pe_name            mpi
allocation_rule    $round_robin
control_slaves     FALSE
accounting_summary FALSE
EOF
assert_failure 'distributed PE without controlled slaves and summary accounting is rejected' \
    run_submit_config "$TEST_ROOT/loose-pe.conf" "$TEST_ROOT/loose-pe.grami"
assert_not_grep 'unaccountable PE job never reaches qsub' '^qsub ' "$TEST_ROOT/calls"

reset_scheduler
write_submit_grami "$TEST_ROOT/smp-pe.grami" 4
printf "CONFIG_sge_pe='smp'\n" > "$TEST_ROOT/smp-pe.conf"
printf 'smp\n' > "$TEST_ROOT/qconf.spl"
cat > "$TEST_ROOT/qconf.sp" <<'EOF'
pe_name            smp
allocation_rule    $pe_slots
control_slaves     FALSE
accounting_summary FALSE
EOF
printf '73010\n' > "$TEST_ROOT/qsub.output"
assert_success 'single-host loose PE is accepted for threaded workloads' \
    run_submit_config "$TEST_ROOT/smp-pe.conf" "$TEST_ROOT/smp-pe.grami"

reset_scheduler
write_submit_grami "$TEST_ROOT/tight-smp-pe.grami" 4
printf "CONFIG_sge_pe='smp'\n" > "$TEST_ROOT/tight-smp-pe.conf"
printf 'smp\n' > "$TEST_ROOT/qconf.spl"
cat > "$TEST_ROOT/qconf.sp" <<'EOF'
pe_name            smp
allocation_rule    $pe_slots
control_slaves     TRUE
accounting_summary FALSE
EOF
assert_failure 'tightly integrated single-host PE requires summary accounting' \
    run_submit_config "$TEST_ROOT/tight-smp-pe.conf" "$TEST_ROOT/tight-smp-pe.grami"
assert_not_grep 'unaccountable tight PE never reaches qsub' '^qsub ' "$TEST_ROOT/calls"

# ARC memory is per slot.  A JOB consumable or a non-MEMORY complex would
# silently reserve too little (or the wrong kind of) resource.
reset_scheduler
write_submit_grami "$TEST_ROOT/memory.grami" 1
printf "joboption_memory='768'\n" >> "$TEST_ROOT/memory.grami"
printf "CONFIG_sge_memory_resource='mem_per_slot'\n" > "$TEST_ROOT/memory.conf"
printf 'mem_per_slot mps MEMORY <= YES YES 0 0\n' > "$TEST_ROOT/qconf.sc"
printf '73006\n' > "$TEST_ROOT/qsub.output"
assert_success 'per-slot MEMORY complex is accepted' \
    run_submit_config "$TEST_ROOT/memory.conf" "$TEST_ROOT/memory.grami"
assert_grep 'memory request uses the validated complex' '^#\$ -l mem_per_slot=768M$' "$TEST_ROOT/submitted.job"

reset_scheduler
write_submit_grami "$TEST_ROOT/h-vmem.grami" 1
printf "joboption_memory='768'\n" >> "$TEST_ROOT/h-vmem.grami"
printf 'h_vmem h_vmem MEMORY <= YES NO 0 0\n' > "$TEST_ROOT/qconf.sc"
printf '73012\n' > "$TEST_ROOT/qsub.output"
assert_success 'stock non-consumable h_vmem is accepted as the fallback limit' \
    run_submit "$TEST_ROOT/h-vmem.grami"
assert_grep 'h_vmem fallback applies the configured hard-limit ratio' \
    '^#\$ -l h_vmem=1536M$' "$TEST_ROOT/submitted.job"

reset_scheduler
write_submit_grami "$TEST_ROOT/job-memory.grami" 2
printf "joboption_memory='768'\n" >> "$TEST_ROOT/job-memory.grami"
printf "CONFIG_sge_memory_resource='mem_per_job'\nCONFIG_sge_pe='mpi'\n" > "$TEST_ROOT/job-memory.conf"
printf 'mpi\n' > "$TEST_ROOT/qconf.spl"
cat > "$TEST_ROOT/qconf.sp" <<'EOF'
pe_name            mpi
allocation_rule    $pe_slots
control_slaves     TRUE
accounting_summary TRUE
EOF
printf 'mem_per_job mpj MEMORY <= YES JOB 0 0\n' > "$TEST_ROOT/qconf.sc"
assert_failure 'per-job memory consumable is rejected' \
    run_submit_config "$TEST_ROOT/job-memory.conf" "$TEST_ROOT/job-memory.grami"
assert_not_grep 'invalid memory semantics never reach qsub' '^qsub ' "$TEST_ROOT/calls"

reset_scheduler
write_submit_grami "$TEST_ROOT/string-memory.grami" 1
printf "joboption_memory='768'\n" >> "$TEST_ROOT/string-memory.grami"
printf "CONFIG_sge_memory_resource='not_memory'\n" > "$TEST_ROOT/string-memory.conf"
printf 'not_memory nm STRING == YES YES NONE 0\n' > "$TEST_ROOT/qconf.sc"
assert_failure 'non-MEMORY complex is rejected for memory' \
    run_submit_config "$TEST_ROOT/string-memory.conf" "$TEST_ROOT/string-memory.grami"

reset_scheduler
write_submit_grami "$TEST_ROOT/wrong-relop-memory.grami" 1
printf "joboption_memory='768'\n" >> "$TEST_ROOT/wrong-relop-memory.grami"
printf "CONFIG_sge_memory_resource='backwards_mem'\n" > "$TEST_ROOT/wrong-relop-memory.conf"
printf 'backwards_mem bm MEMORY >= YES YES 0 0\n' > "$TEST_ROOT/qconf.sc"
assert_failure 'memory complex with wrong relation is rejected' \
    run_submit_config "$TEST_ROOT/wrong-relop-memory.conf" "$TEST_ROOT/wrong-relop-memory.grami"

# Exclusive execution has precise Grid Engine complex semantics.
reset_scheduler
write_submit_grami "$TEST_ROOT/exclusive.grami" 1
printf "joboption_exclusivenode='true'\n" >> "$TEST_ROOT/exclusive.grami"
printf "CONFIG_sge_exclusive_resource='exclusive'\n" > "$TEST_ROOT/exclusive.conf"
printf 'exclusive excl BOOL EXCL YES YES 0 1000\n' > "$TEST_ROOT/qconf.sc"
printf '73007\n' > "$TEST_ROOT/qsub.output"
assert_success 'proper EXCL Boolean consumable is accepted' \
    run_submit_config "$TEST_ROOT/exclusive.conf" "$TEST_ROOT/exclusive.grami"
assert_grep 'exclusive request uses the validated complex' '^#\$ -l exclusive=true$' "$TEST_ROOT/submitted.job"

reset_scheduler
write_submit_grami "$TEST_ROOT/nonexclusive.grami" 1
printf "joboption_exclusivenode='true'\n" >> "$TEST_ROOT/nonexclusive.grami"
printf "CONFIG_sge_exclusive_resource='exclusive'\n" > "$TEST_ROOT/nonexclusive.conf"
printf 'exclusive excl BOOL == YES YES 0 1000\n' > "$TEST_ROOT/qconf.sc"
assert_failure 'Boolean complex without EXCL relation is rejected' \
    run_submit_config "$TEST_ROOT/nonexclusive.conf" "$TEST_ROOT/nonexclusive.grami"

# qsub only spools the script.  It cannot stage an ARC session to a worker
# which has no view of the frontend filesystem.
reset_scheduler
write_submit_grami "$TEST_ROOT/detached.grami" 1
printf 'RUNTIME_NODE_SEES_FRONTEND=\n' > "$TEST_ROOT/detached.conf"
assert_failure 'detached SGE workers are rejected before submission' \
    run_submit_config "$TEST_ROOT/detached.conf" "$TEST_ROOT/detached.grami"
assert_not_grep 'detached job never reaches qsub' '^qsub ' "$TEST_ROOT/calls"

make_scan_job () {
    control_dir=$1
    grid_id=$2
    local_id=$3
    session_path=$TEST_ROOT/session-$grid_id
    mkdir -p "$control_dir/processing" "$control_dir/jobs" "$session_path"
    printf 'INLRMS\n' > "$control_dir/processing/$grid_id.status"
    printf 'localid=%s\nsessiondir=%s\n' "$local_id" "$session_path" > "$control_dir/jobs/$grid_id.local"
    printf 'exitcode=0\n' > "$session_path.diag"
}

run_scan () {
    control_dir=$1
    "$TEST_ROOT/scan-sge-job" "$control_dir" > "$TEST_ROOT/stdout" 2> "$TEST_ROOT/stderr"
}

# Exact XML IDs matter: active job 123 must not conceal completed job 12.
reset_scheduler
scan_control=$TEST_ROOT/scan-control
make_scan_job "$scan_control" arc12 12
make_scan_job "$scan_control" arc123 123
printf 'exitcode=not-a-number\nnodename=node01.example\nnodename=node02.example\n' > "$TEST_ROOT/session-arc12.diag"
printf 'joboption_sge_submit_time=1705312800\n' > "$scan_control/jobs/arc12.grami"
cat > "$TEST_ROOT/qstat.xml" <<'EOF'
<?xml version='1.0'?>
<job_info><queue_info><Queue-List><name>short.q@node01</name>
<job_list state="running"><JB_job_number>123</JB_job_number><state>r</state></job_list>
</Queue-List></queue_info><job_info/></job_info>
EOF
cat > "$TEST_ROOT/qacct.output" <<'EOF'
==============================================================
qname        short.q
hostname     old.example
jobnumber    12
taskid       undefined
pe_taskid    NONE
slots        1
qsub_time    Fri Dec 15 10:00:00 2023
start_time   Fri Dec 15 10:00:01 2023
end_time     Fri Dec 15 10:00:02 2023
failed       0
exit_status  99
ru_wallclock 1.0
cpu          1.0
==============================================================
qname        short.q
hostname     node02.example
jobnumber    12
taskid       undefined
pe_taskid    NONE
slots        2
qsub_time    Mon Jan 15 09:59:00 2024
start_time   Mon Jan 15 10:00:00 2024
end_time     Mon Jan 15 10:00:05 2024
failed       0
exit_status  0
ru_wallclock 4.5
ru_utime     2.50
ru_stime     1.25
cpu          3.75
maxvmem      1.5G
maxrss       0.000
max_cgroups_memory 256M
==============================================================
qname        short.q
hostname     recycled.example
jobnumber    12
taskid       undefined
pe_taskid    NONE
slots        1
qsub_time    Mon Jan 15 10:04:00 2024
start_time   Mon Jan 15 10:04:01 2024
end_time     Mon Jan 15 10:04:05 2024
failed       0
exit_status  77
ru_wallclock 4.0
cpu          3.0
EOF
assert_success 'qstat XML and terminal qacct record complete a disappeared job' run_scan "$scan_control"
assert_grep 'scanner explicitly requests all Grid Engine job states' \
    '^qstat -xml -u \* -s a -q \*$' "$TEST_ROOT/calls"
assert_grep 'the exact disappeared ID is queried in qacct' '^qacct -j 12$' "$TEST_ROOT/calls"
assert_not_grep 'the exact active ID is not queried in qacct' '^qacct -j 123$' "$TEST_ROOT/calls"
assert_grep 'terminal accounting writes successful completion' '^0$' "$scan_control/jobs/arc12.lrms_done"
assert_grep 'malformed wrapper exit cannot override qacct' '^0$' "$scan_control/jobs/arc12.lrms_done"
assert_grep 'decimal CPU accounting is preserved' '^CPUTime=3\.75s$' "$TEST_ROOT/session-arc12.diag"
assert_grep 'binary Grid Engine memory units become kB' '^AverageTotalMemory=1572864kB$' "$TEST_ROOT/session-arc12.diag"
assert_grep 'Altair cgroup resident memory is recorded' '^AverageResidentMemory=262144kB$' "$TEST_ROOT/session-arc12.diag"
assert_grep 'first PE execution host survives accounting merge' '^nodename=node01\.example$' "$TEST_ROOT/session-arc12.diag"
assert_grep 'second PE execution host survives accounting merge' '^nodename=node02\.example$' "$TEST_ROOT/session-arc12.diag"
assert_not_grep 'qacct master hostname does not replace PE host list' '^nodename=recycled\.example$' "$TEST_ROOT/session-arc12.diag"
assert_not_grep 'a later recycled numeric ID is not selected' '^77 ' "$scan_control/jobs/arc12.lrms_done"
if [ ! -e "$scan_control/jobs/arc123.lrms_done" ]; then
    ok 'active XML job remains unfinished'
else
    not_ok 'active XML job remains unfinished'
fi

# A live job with a recycled numeric ID must not mask the original ARC job.
# The persisted and scheduler submission times distinguish the two even if a
# JSV has rewritten the native job name.
reset_scheduler
reused_control=$TEST_ROOT/reused-control
make_scan_job "$reused_control" reusedid 87
printf 'joboption_sge_submit_time=1705312800\njoboption_sge_job_name=arc_expected_reusedid\n' \
    > "$reused_control/jobs/reusedid.grami"
cat > "$TEST_ROOT/qstat.xml" <<'EOF'
<?xml version='1.0'?>
<job_info><queue_info><Queue-List><name>short.q@node04</name>
<job_list state="running"><JB_job_number>87</JB_job_number><JB_name>unrelated_job</JB_name><JB_submission_time>2024-01-15T10:10:00</JB_submission_time><state>r</state></job_list>
</Queue-List></queue_info><job_info/></job_info>
EOF
cat > "$TEST_ROOT/qacct.output" <<'EOF'
==============================================================
qname        short.q
hostname     node03.example
jobnumber    87
taskid       undefined
pe_taskid    NONE
slots        1
qsub_time    Mon Jan 15 09:59:00 2024
start_time   Mon Jan 15 10:00:00 2024
end_time     Mon Jan 15 10:00:05 2024
failed       0
exit_status  0
ru_wallclock 5.0
cpu          4.0
EOF
assert_success 'recycled active numeric ID does not mask the ARC job' run_scan "$reused_control"
assert_grep 'original accounting completes despite the recycled active ID' \
    '^0$' "$reused_control/jobs/reusedid.lrms_done"

# A JSV can rewrite the submitted name without changing the job identity.
reset_scheduler
jsv_control=$TEST_ROOT/jsv-control
make_scan_job "$jsv_control" jsvname 90
printf 'joboption_sge_submit_time=1705312800\njoboption_sge_job_name=arc_expected_jsvname\n' \
    > "$jsv_control/jobs/jsvname.grami"
cat > "$TEST_ROOT/qstat.xml" <<'EOF'
<?xml version='1.0'?>
<job_info><queue_info><Queue-List><name>short.q@node04</name>
<job_list state="running"><JB_job_number>90</JB_job_number><JB_name>site_rewritten_name</JB_name><JB_submission_time>2024-01-15T10:00:00</JB_submission_time><state>r</state></job_list>
</Queue-List></queue_info><job_info/></job_info>
EOF
assert_success 'JSV-renamed live job remains active when submission time matches' \
    run_scan "$jsv_control"
if [ ! -e "$jsv_control/jobs/jsvname.lrms_done" ]; then
    ok 'JSV-renamed live job remains unfinished'
else
    not_ok 'JSV-renamed live job remains unfinished'
fi
assert_not_grep 'JSV-renamed live job is not queried in qacct' \
    '^qacct -j 90$' "$TEST_ROOT/calls"

# If the old accounting record has already expired, a recycled live ID must
# not reset the bounded missing-accounting counter forever.
reset_scheduler
expired_control=$TEST_ROOT/expired-control
make_scan_job "$expired_control" expiredacct 88
printf 'joboption_sge_submit_time=1705312800\njoboption_sge_job_name=arc_expected_expiredacct\n' \
    > "$expired_control/jobs/expiredacct.grami"
cat > "$TEST_ROOT/qstat.xml" <<'EOF'
<?xml version='1.0'?>
<job_info><queue_info><Queue-List><name>short.q@node04</name>
<job_list state="pending"><JB_job_number>88</JB_job_number><JB_name>job.sh</JB_name><JB_submission_time>2024-01-15T10:10:00Z</JB_submission_time><state>Eqw</state></job_list>
</Queue-List></queue_info><job_info/></job_info>
EOF
printf '0\n' > "$TEST_ROOT/qstat.job.rc"
assert_success 'recycled ID without old accounting starts the bounded retry' run_scan "$expired_control"
assert_success 'recycled ID without old accounting reaches retry exhaustion' run_scan "$expired_control"
assert_grep 'expired original job is eventually completed from wrapper status' \
    '^0$' "$expired_control/jobs/expiredacct.lrms_done"
assert_not_grep 'recycled ID is not mistaken for a reappearing original job' \
    '^qstat -u \* -s a -q \* -j 88$' "$TEST_ROOT/calls"
assert_not_grep 'recycled error-state job is never deleted by ARC' \
    '^qdel 88$' "$TEST_ROOT/calls"

# Newer Grid Engine derivatives can retain finished jobs in qstat -s a.  A
# retained record must not reset the bounded wait for the final qacct record.
reset_scheduler
retained_control=$TEST_ROOT/retained-control
make_scan_job "$retained_control" retainedacct 89
printf 'joboption_sge_job_name=arc_expected_retainedacct\n' \
    > "$retained_control/jobs/retainedacct.grami"
cat > "$TEST_ROOT/qstat.xml" <<'EOF'
<?xml version='1.0'?>
<job_info><queue_info/><job_info>
<job_list state="finished"><JB_job_number>89</JB_job_number><JB_name>arc_expected_retainedacct</JB_name><state>f</state></job_list>
</job_info></job_info>
EOF
printf '0\n' > "$TEST_ROOT/qstat.job.rc"
assert_success 'retained finished job starts the bounded accounting wait' \
    run_scan "$retained_control"
if [ ! -e "$retained_control/jobs/retainedacct.lrms_done" ]; then
    ok 'retained finished job remains pending during accounting lag'
else
    not_ok 'retained finished job remains pending during accounting lag'
fi
assert_success 'retained finished job reaches accounting retry exhaustion' \
    run_scan "$retained_control"
assert_grep 'retained finished job completes from wrapper status' \
    '^0$' "$retained_control/jobs/retainedacct.lrms_done"
assert_not_grep 'retained finished record is not treated as live again' \
    '^qstat -u \* -s a -q \* -j 89$' "$TEST_ROOT/calls"

# Recheck for rescheduling before exhausting the accounting wait.  Otherwise a
# job which reappears after the cluster snapshot could be completed as live.
reset_scheduler
race_control=$TEST_ROOT/race-control
make_scan_job "$race_control" rescheduled 92
printf '1\n' > "$race_control/jobs/rescheduled.lrms_job"
cat > "$TEST_ROOT/qstat.xml" <<'EOF'
<?xml version='1.0'?>
<job_info><queue_info/><job_info/></job_info>
EOF
printf '0\n' > "$TEST_ROOT/qstat.job.rc"
assert_success 'final accounting retry rechecks a rescheduled live job first' \
    run_scan "$race_control"
assert_grep 'rescheduled job receives the exact live recheck' \
    '^qstat -u \* -s a -q \* -j 92$' "$TEST_ROOT/calls"
if [ ! -e "$race_control/jobs/rescheduled.lrms_done" ] \
    && [ ! -e "$race_control/jobs/rescheduled.lrms_job" ]; then
    ok 'reappeared job is neither completed nor left on the retry counter'
else
    not_ok 'reappeared job is neither completed nor left on the retry counter'
fi

# qstat default files are applied even to programmatic calls. ARC overrides
# the three ordinary selectors and fails closed on options it cannot clear.
reset_scheduler
mkdir -p "$TEST_ROOT/sge/default/common"
printf '%s\n' '-s rs -u $user -q short.q # ARC overrides these selectors' \
    > "$TEST_ROOT/sge/default/common/sge_qstat"
defaults_control=$TEST_ROOT/defaults-control
make_scan_job "$defaults_control" defaultsafe 90
cat > "$TEST_ROOT/qstat.xml" <<'EOF'
<?xml version='1.0'?>
<job_info><queue_info/><job_info/></job_info>
EOF
assert_success 'overridden qstat selector defaults are accepted' \
    run_scan "$defaults_control"
assert_grep 'scanner overrides safe qstat selector defaults' \
    '^qstat -xml -u \* -s a -q \*$' "$TEST_ROOT/calls"

reset_scheduler
printf '%s\n' '-ne' > "$TEST_ROOT/sge/default/common/sge_qstat"
unsafe_defaults_control=$TEST_ROOT/unsafe-defaults-control
make_scan_job "$unsafe_defaults_control" defaultunsafe 91
assert_failure 'unneutralizable qstat defaults fail the scanner closed' \
    run_scan "$unsafe_defaults_control"
assert_not_grep 'unsafe qstat defaults prevent a narrowed scheduler query' \
    '^qstat ' "$TEST_ROOT/calls"
rm -f "$TEST_ROOT/sge/default/common/sge_qstat"

# Univa/Altair qacct can emit numeric dates with millisecond precision.  The
# portable getrusage resident-memory field is already expressed in kB.
reset_scheduler
modern_control=$TEST_ROOT/modern-control
make_scan_job "$modern_control" modernacct 84
modern_submit_epoch=`/usr/bin/perl -MTime::Local=timegm -e 'print timegm(0, 0, 10, 15, 0, 2024)'`
printf 'joboption_sge_submit_time=%s\n' "$modern_submit_epoch" > "$modern_control/jobs/modernacct.grami"
cat > "$TEST_ROOT/qstat.xml" <<'EOF'
<?xml version='1.0'?>
<job_info><queue_info/><job_info/></job_info>
EOF
cat > "$TEST_ROOT/qacct.output" <<'EOF'
==============================================================
qname        short.q
hostname     node03.example
jobnumber    84
taskid       undefined
pe_taskid    NONE
slots        1
qsub_time    01/15/2024 09:59:00.706
start_time   01/15/2024 10:00:00.123
end_time     01/15/2024 10:00:05.987
failed       0
exit_status  0
ru_wallclock 5.0
cpu          4.0
ru_maxrss    1856
EOF
assert_success 'millisecond Univa accounting timestamps are accepted' run_scan "$modern_control"
assert_grep 'standard ru_maxrss is recorded as kB' \
    '^AverageResidentMemory=1856kB$' "$TEST_ROOT/session-modernacct.diag"

# A qacct master record is not terminal until its failure status and end time
# have been flushed.  It must follow the normal lag retry path.
reset_scheduler
CONFIG_sge_debug=yes
export CONFIG_sge_debug
partial_control=$TEST_ROOT/partial-control
make_scan_job "$partial_control" partialacct 85
cat > "$TEST_ROOT/qstat.xml" <<'EOF'
<?xml version='1.0'?>
<job_info><queue_info/><job_info/></job_info>
EOF
cat > "$TEST_ROOT/qacct.output" <<'EOF'
==============================================================
qname        short.q
hostname     node03.example
jobnumber    85
taskid       undefined
pe_taskid    NONE
slots        1
qsub_time    Mon Jan 15 09:59:00 2024
start_time   Mon Jan 15 10:00:00 2024
end_time     Mon Jan 15 10:00:05 2024
exit_status  0
ru_wallclock 5.0
cpu          4.0
EOF
assert_success 'incomplete master accounting enters the retry path' run_scan "$partial_control"
assert_grep 'accounting trace explains an incomplete master' \
    'arc_job=partialacct sge_job=85 DEBUG event=accounting reason=incomplete_terminal_record' "$TEST_ROOT/stderr"
assert_grep 'accounting trace gives the retry budget' \
    'event=accounting_wait attempt=1 limit=1' "$TEST_ROOT/stderr"
assert_grep 'query trace reports status and timing' \
    'event=query_result attempt=1 exit_status=0 elapsed_seconds=[0-9]+ command=.*qacct -j 85' "$TEST_ROOT/stderr"
assert_not_grep 'scanner debug output never reaches stdout' 'DEBUG|event=' "$TEST_ROOT/stdout"
if [ ! -e "$partial_control/jobs/partialacct.lrms_done" ]; then
    ok 'incomplete accounting does not complete the ARC job'
else
    not_ok 'incomplete accounting does not complete the ARC job'
fi

# A terminal timestamp alone does not make a missing/corrupt exit status safe.
badacct_index=0
for exit_line in '' 'exit_status garbage' 'exit_status 0
exit_status 1' 'exit_status 0'; do
    reset_scheduler
    badacct_index=$((badacct_index + 1))
    [ "$badacct_index" -ne 4 ] || printf '1\n' > "$TEST_ROOT/qacct.rc"
    badacct_control="$TEST_ROOT/badacct-$badacct_index-control"
    make_scan_job "$badacct_control" badacct 86
    printf '<job_info><queue_info/><job_info/></job_info>\n' > "$TEST_ROOT/qstat.xml"
    cat > "$TEST_ROOT/qacct.output" <<EOF
==============================================================
qname short.q
hostname node1
jobnumber 86
taskid undefined
pe_taskid NONE
end_time Mon Jan 15 10:00:05 2024
failed 0
$exit_line
EOF
    assert_success 'failed/missing/corrupt accounting exit enters retry path' run_scan "$badacct_control"
    assert_grep 'bad accounting produces a retry counter' '^1$' "$badacct_control/jobs/badacct.lrms_job"
    if [ ! -e "$badacct_control/jobs/badacct.lrms_done" ]; then
        ok 'bad accounting cannot immediately complete a job'
    else
        not_ok 'bad accounting cannot immediately complete a job'
    fi
    printf 'exitcode=garbage\n' > "$TEST_ROOT/session-badacct.diag"
    assert_success 'bad wrapper diagnostic follows bounded accounting fallback' run_scan "$badacct_control"
    assert_grep 'bad wrapper exit is reported as unknown, not arbitrary text' \
        '^-1 Job failed with unknown exit code$' "$badacct_control/jobs/badacct.lrms_done"
done

# A PE task record can be flushed before the master summary.  It is not a
# terminal accounting record for the ARC job.
reset_scheduler
slave_control=$TEST_ROOT/slave-control
make_scan_job "$slave_control" onlyslave 80
cat > "$TEST_ROOT/qstat.xml" <<'EOF'
<?xml version='1.0'?>
<job_info><queue_info/><job_info/></job_info>
EOF
cat > "$TEST_ROOT/qacct.output" <<'EOF'
==============================================================
qname        short.q
hostname     node02.example
jobnumber    80
taskid       undefined
pe_taskid    1
slots        1
failed       0
exit_status  0
ru_wallclock 1.0
cpu          1.0
EOF
assert_success 'PE slave-only accounting is treated as not ready' run_scan "$slave_control"
assert_grep 'accounting trace distinguishes a slave-only record' \
    'event=accounting reason=no_master_record matching_records=1' "$TEST_ROOT/stderr"
unset CONFIG_sge_debug
if [ ! -e "$slave_control/jobs/onlyslave.lrms_done" ]; then
    ok 'PE slave record does not complete the ARC job'
else
    not_ok 'PE slave record does not complete the ARC job'
fi

# failed=0 means Grid Engine ran the command normally, even when the
# application deliberately uses an exit status above the shell signal range.
reset_scheduler
exit_control=$TEST_ROOT/exit-control
make_scan_job "$exit_control" exit200 82
cat > "$TEST_ROOT/qstat.xml" <<'EOF'
<?xml version='1.0'?>
<job_info><queue_info/><job_info/></job_info>
EOF
cat > "$TEST_ROOT/qacct.output" <<'EOF'
==============================================================
qname        short.q
hostname     node02.example
jobnumber    82
taskid       undefined
pe_taskid    NONE
slots        1
end_time     Mon Jan 15 10:00:05 2024
failed       0
exit_status  200
ru_wallclock 1.0
cpu          1.0
EOF
assert_success 'high application exit status is handled' run_scan "$exit_control"
assert_grep 'failed zero keeps application exit semantics' \
    '^200 Job failed with exit code 200$' "$exit_control/jobs/exit200.lrms_done"

# A failed cluster-wide status query must never make every ARC job disappear.
reset_scheduler
failed_control=$TEST_ROOT/failed-control
make_scan_job "$failed_control" failclosed 77
printf '1\n' > "$TEST_ROOT/qstat.rc"
assert_failure 'qstat failure makes the scan fail closed' run_scan "$failed_control"
assert_not_grep 'qacct is not queried after qstat failure' '^qacct ' "$TEST_ROOT/calls"
if [ ! -e "$failed_control/jobs/failclosed.lrms_done" ]; then
    ok 'qstat failure does not complete an ARC job'
else
    not_ok 'qstat failure does not complete an ARC job'
fi

reset_scheduler
malformed_control=$TEST_ROOT/malformed-control
make_scan_job "$malformed_control" malformedxml 79
printf '<job_info><queue_info></job_info>\n' > "$TEST_ROOT/qstat.xml"
assert_failure 'malformed qstat XML makes the scan fail closed' run_scan "$malformed_control"
assert_not_grep 'qacct is not queried after malformed XML' '^qacct ' "$TEST_ROOT/calls"
if [ ! -e "$malformed_control/jobs/malformedxml.lrms_done" ]; then
    ok 'malformed qstat XML does not complete an ARC job'
else
    not_ok 'malformed qstat XML does not complete an ARC job'
fi

# Well-formed XML can still be a failed/incomplete scheduler response. None of
# these documents may cause accounting, deletion, counters, or completion.
for bad_snapshot in \
    '<error>qmaster unavailable</error>' \
    '<job_info/>' \
    '<job_info><queue_info/><job_info><error>denied</error></job_info></job_info>' \
    '<job_info><queue_info/></job_info>' \
    '<job_info><queue_info/><job_info><job_list>bad</job_list></job_info></job_info>' \
    '<job_info><queue_info/><job_info><job_list><state>r</state></job_list></job_info></job_info>' \
    '<job_info><queue_info/><job_info><job_list><JB_job_number>79</JB_job_number></job_list></job_info></job_info>' \
    '<job_info><queue_info/><job_info><job_list><JB_job_number>79</JB_job_number><JB_job_number>80</JB_job_number><state>r</state></job_list></job_info></job_info>'
do
    reset_scheduler
    printf '%s\n' "$bad_snapshot" > "$TEST_ROOT/qstat.xml"
    assert_failure 'structurally invalid qstat snapshot fails closed' run_scan "$malformed_control"
    assert_not_grep 'invalid snapshot does not query accounting or delete jobs' '^(qacct|qdel) ' "$TEST_ROOT/calls"
    assert_grep 'invalid snapshot has an actionable diagnostic' 'invalid|missing' "$TEST_ROOT/stderr"
    if [ ! -e "$malformed_control/jobs/malformedxml.lrms_done" ]; then
        ok 'invalid snapshot leaves job unfinished'
    else
        not_ok 'invalid snapshot leaves job unfinished'
    fi
done

# Legacy failed=37 is an ambiguous qmaster limit code; use the measured/requested
# resource comparison instead of always misreporting it as a wall-time failure.
reset_scheduler
limit_control=$TEST_ROOT/limit-control
make_scan_job "$limit_control" memorylimit 78
printf 'joboption_memory=1000\n' > "$limit_control/jobs/memorylimit.grami"
cat > "$TEST_ROOT/qstat.xml" <<'EOF'
<?xml version='1.0'?>
<job_info><queue_info/><job_info/></job_info>
EOF
cat > "$TEST_ROOT/qacct.output" <<'EOF'
==============================================================
qname        short.q
hostname     node02.example
jobnumber    78
taskid       undefined
pe_taskid    NONE
slots        1
end_time     Mon Jan 15 10:00:05 2024
failed       37 : qmaster enforced hard resource limit
exit_status  137
ru_wallclock 2.0
cpu          1.0
maxvmem      990M
maxrss       990M
EOF
assert_success 'ambiguous qmaster limit accounting is handled' run_scan "$limit_control"
assert_grep 'failed 37 is classified from measured memory usage' '^271 job killed: memory$' "$limit_control/jobs/memorylimit.lrms_done"

# IndividualPhysicalMemory is a per-slot ARC request.  Whole-job accounting
# must be compared with the request multiplied by the allocated slot count.
reset_scheduler
parallel_limit_control=$TEST_ROOT/parallel-limit-control
make_scan_job "$parallel_limit_control" parallelmemory 86
printf 'joboption_memory=1000\njoboption_count=4\n' > "$parallel_limit_control/jobs/parallelmemory.grami"
cat > "$TEST_ROOT/qstat.xml" <<'EOF'
<?xml version='1.0'?>
<job_info><queue_info/><job_info/></job_info>
EOF
cat > "$TEST_ROOT/qacct.output" <<'EOF'
==============================================================
qname        short.q
hostname     node02.example
jobnumber    86
taskid       undefined
pe_taskid    NONE
slots        4
end_time     Mon Jan 15 10:00:05 2024
failed       37 : qmaster enforced hard resource limit
exit_status  137
ru_wallclock 2.0
cpu          1.0
maxvmem      1500M
maxrss       1500M
EOF
assert_success 'parallel ambiguous-limit accounting is handled' run_scan "$parallel_limit_control"
assert_not_grep 'per-slot memory is not falsely classified as exceeded' \
    '^271 job killed: memory$' "$parallel_limit_control/jobs/parallelmemory.lrms_done"

run_cancel () {
    grami_path=$1
    "$TEST_ROOT/cancel-sge-job" "$grami_path" > "$TEST_ROOT/stdout" 2> "$TEST_ROOT/stderr"
}

# Job IDs are validated before qdel, and a real qdel error is propagated.
reset_scheduler
printf "joboption_jobid='-9'\n" > "$TEST_ROOT/invalid-cancel.grami"
assert_failure 'cancellation rejects an option-like job ID' run_cancel "$TEST_ROOT/invalid-cancel.grami"
assert_not_grep 'invalid cancellation never invokes qdel' '^qdel ' "$TEST_ROOT/calls"

reset_scheduler
printf "joboption_jobid='88'\n" > "$TEST_ROOT/qdel-failure.grami"
printf '3\n' > "$TEST_ROOT/qdel.rc"
printf '0\n' > "$TEST_ROOT/qstat.job.rc"
printf '1\n' > "$TEST_ROOT/qacct.rc"
assert_failure 'qdel failure for a still-active job is propagated' run_cancel "$TEST_ROOT/qdel-failure.grami"
assert_grep 'qdel receives the validated numeric ID' '^qdel 88$' "$TEST_ROOT/calls"
assert_grep 'failed cancellation logs the exact status and native ID' \
    'sge_job=88 event=cancel_result exit_status=3' "$TEST_ROOT/stderr"

reset_scheduler
printf "joboption_jobid='89'\n" > "$TEST_ROOT/qdel-success.grami"
assert_success 'successful qdel completes cancellation request' run_cancel "$TEST_ROOT/qdel-success.grami"

# Tracing must not alter the submission protocol or expose native -v values.
reset_scheduler
write_submit_grami "$TEST_ROOT/debug.grami" 1
printf '73020\n' > "$TEST_ROOT/qsub.output"
printf '%s\n' 'CONFIG_sge_debug=yes' \
    "CONFIG_sge_jobopts='-v ARC_DEBUG_TEST_SECRET=do-not-log-this-value'" > "$TEST_ROOT/debug.conf"
assert_success 'submission works with debug enabled through configuration' \
    run_submit_config "$TEST_ROOT/debug.conf" "$TEST_ROOT/debug.grami"
assert_grep 'debug submission logs resolved environment' \
    'DEBUG event=environment root=.* cell=default binaries=' "$TEST_ROOT/stderr"
assert_grep 'debug submission logs command status and timing' \
    'event=command_result command=.*qsub exit_status=0 elapsed_seconds=[0-9]+' "$TEST_ROOT/stderr"
assert_grep 'the native secret is present in the submitted script' \
    'ARC_DEBUG_TEST_SECRET=do-not-log-this-value' "$TEST_ROOT/submitted.job"
assert_not_grep 'job script secrets are not dumped into logs' \
    'do-not-log-this-value' "$TEST_ROOT/stderr"
assert_not_grep 'submission debug messages never reach stdout' 'DEBUG|event=' "$TEST_ROOT/stdout"
printf 'CONFIG_sge_debug=maybe\n' > "$TEST_ROOT/debug.conf"
assert_failure 'invalid debug configuration is rejected' \
    run_submit_config "$TEST_ROOT/debug.conf" "$TEST_ROOT/debug.grami"

printf '1..%d\n' "$tests"
if [ "$failures" -ne 0 ]; then
    printf '%d SGE regression assertion(s) failed\n' "$failures" 1>&2
    exit 1
fi
exit 0
