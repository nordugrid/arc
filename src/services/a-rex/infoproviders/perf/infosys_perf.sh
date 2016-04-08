#!/bin/bash -x

# TODO: use one of the parsers to get performance info from arc.conf

echo "Usage: $0 <arc performance data path>"
echo "default: $0 /var/log/arc/perfdata/"

PERFDIR=${1:-/var/log/arc/perfdata/}
MERGEDATE=`date +%Y%m%d%H%M%S`

nytprofmerge -o infosys_$MERGEDATE.perflog $PERFDIR/infosys_*.raw
