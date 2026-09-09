#!/bin/sh

# Shared control paths must remain compatible with existing jobs, including
# historical short IDs and the trailing slashes in the on-disk layout.
. "${LRMS_COMMON:?}" || exit 1
. "${SCAN_COMMON:?}" || exit 1

failures=0
tests=0
check_equal () {
    tests=$((tests + 1))
    if [ "$2" = "$3" ]; then
        printf 'ok %s - %s\n' "$tests" "$1"
    else
        printf 'not ok %s - %s: expected <%s>, got <%s>\n' "$tests" "$1" "$2" "$3"
        failures=$((failures + 1))
    fi
}

while IFS=' ' read -r test_id test_path; do
    [ "$test_id" != EMPTY ] || test_id=
    # No external executable is available during the lookup.
    actual=$(PATH=/nonexistent control_path '/control space' "$test_id" local)
    check_equal "control path for '$test_id'" "/control space/jobs/$test_path/local" "$actual"
done <<'EOF'
EMPTY /
a a/
ab ab/
abc abc//
abcd abc/d/
abcdef abc/def//
abcdefghi abc/def/ghi//
abcdefghij abc/def/ghi/j/
abcdefghijklmnop abc/def/ghi/jklmnop/
aaaaaaaaaaaaaaaa aaa/aaa/aaa/aaaaaaa/
abc-def_ghi.jkl abc/-de/f_g/hi.jkl/
EOF

diag_test_dir=$(mktemp -d "${TMPDIR:-/tmp}/arc-common-test.XXXXXX") || exit 1
trap 'rm -f "$diag_test_dir/job.diag" "$diag_test_dir/job.grami"; rmdir "$diag_test_dir"' 0
trap 'exit 1' 1 2 15
sessiondir=$diag_test_dir/job
uid=$(id -u)
# Exercise the real parser/filter; account switching itself is tested in ARC's
# file-access tests. The only file used here belongs to this test account.
do_as_uid () { /bin/sh -c "$2"; }
cat > "$sessiondir.diag" <<'EOF'
nodename=node1
nodename=node2
WallTime=1.5s
WallTime=2.5s
UserTime=1.0s
KernelTime=0.2s
AverageTotalMemory=1024kB
AverageResidentMemory=512kB
LRMSStartTime=20240115100000Z
LRMSEndTime=20240115100003Z
exitcode=7
LRMSExitcode=271
LRMSMessage=old value
custom=preserve-this
WallTimeExtra=keep
EOF
job_read_diag
check_equal 'last timing value wins' 2.5 "$WallTime"
check_equal 'all execution hosts survive' "$(printf 'node1\nnode2')" "$nodename"
check_equal 'wrapper exit status survives' 7 "$exitcode"
check_equal 'standard memory is parsed' 512 "$ResidentMemory"
check_equal 'only exact managed keys are removed' \
    "$(printf 'custom=preserve-this\nWallTimeExtra=keep')" "$diagstring"

cat > "$diag_test_dir/job.grami" <<'EOF'
joboption_controldir=/control
joboption_gridid=test
joboption_arg_0='/bin/echo'
joboption_arg_1='two words'
joboption_arg_2='a\b"c'
joboption_arg_3='-n'
joboption_arg_4=''
EOF
parse_grami_file "$diag_test_dir/job.grami"
check_equal 'argument quoting preserves spaces, quotes, backslashes and empty values' \
    ' "/bin/echo" "two words" "a\\b\"c" "-n" ""' "$joboption_args"
printf '1..%s\n' "$tests"
[ "$failures" -eq 0 ]
