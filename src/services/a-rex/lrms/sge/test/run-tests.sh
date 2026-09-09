#!/bin/sh
# Run both SGE interfaces from an unconfigured source tree. No live LRMS needed.
set -eu
testdir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$(CDPATH= cd -- "$testdir/../../../../../.." && pwd)
provider="$root/src/services/a-rex/infoproviders"
workdir=$(mktemp -d "${TMPDIR:-/tmp}/arc-sge-contracts.XXXXXX")
trap 'status=$?; if [ "$status" -ne 0 ] || [ "${SGE_TEST_KEEP:-no}" = yes ]; then
  echo "SGE test workspace retained: $workdir" >&2
else
  rm -rf "$workdir"
fi; exit "$status"' 0
cp "$root/src/tests/lrms/command-simulator.sh" "$workdir/command-simulator.sh"
cd "$workdir"
perl -I"$provider/test" -I"$provider" -MTest::Harness \
  -e '$Test::Harness::verbose=1; runtests @ARGV' \
  "$provider/test/sge.t" "$provider/test/sge-failures.t" "$provider/test/sge-reporting.t"
SGE_SUBMIT_SCRIPT="$testdir/../submit-sge-job.in" \
SGE_SCAN_SCRIPT="$testdir/../scan-sge-job.in" \
SGE_CANCEL_SCRIPT="$testdir/../cancel-sge-job.in" \
SGE_CONFIGURE_ENV="$testdir/../configure-sge-env.sh" \
SGE_TEST_SRCDIR="$testdir" sh "$testdir/sge-lrms-regression.sh"
